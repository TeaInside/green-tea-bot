// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#if defined(__linux__)
	#include <unistd.h>
	#include <pthread.h>
#endif

#include <stack>
#include <mutex>
#include <queue>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <cinttypes>
#include <tgvisd/Td/Td.hpp>
#include <tgvisd/common.hpp>
#include <tgvisd/KWorker.hpp>
#include <tgvisd/Scraper.hpp>


namespace tgvisd {


Scraper::Scraper(Main *main):
	main_(main),
	kworker_(main->getKWorker())
{
}


__hot void Scraper::run(void)
{
	while (!main_->isReady()) {
		if (shouldStop())
			return;
		sleep(1);
	}

	while (!shouldStop()) {
		_run();
		sleep(1);
	}
}


__hot void Scraper::_run(void)
{
	int32_t i;
	int64_t chat_id;

	pr_notice("Getting chat list...");
	auto chats = kworker_->getChats(nullptr, 300);
	if (unlikely(!chats))
		return;

	for (i = 0; i < chats->total_count_; i++) {
		if (shouldStop())
			break;

		chat_id = chats->chat_ids_[i];
		auto chat = kworker_->getChat(chat_id);

		if (unlikely(!chat))
			continue;

		if (chat->type_->get_id() != td_api::chatTypeSupergroup::ID)
			continue;

		pr_notice("Submitting visit to %ld...", chat_id);
		visit_chat(chat);
	}
}


__hot void Scraper::visit_chat(td_api::object_ptr<td_api::chat> &chat)
{
	int ret;
	struct task_work tw;

	tw.func = [this](struct tw_data *data){
		this->_visit_chat(data, data->tw->data);
	};
	tw.data = std::move(chat);

	do {
		if (shouldStop())
			break;

		ret = kworker_->submitTaskWork(&tw);
		if (ret == -EAGAIN)
			sleep(1);

	} while (ret == -EAGAIN);
}


__hot void Scraper::_visit_chat(struct tw_data *data,
				td_api::object_ptr<td_api::chat> &chat)
{
	int32_t count, i;
	struct thpool *current = data->current;
	std::mutex *chat_lock;

	pr_notice("Scraping messages from (%ld) [%s]...", chat->id_,
		  chat->title_.c_str());

	chat_lock = kworker_->getChatLock(chat->id_);
	if (unlikely(!chat_lock)) {
		pr_notice("Could not get chat lock (%ld) [%s]", chat->id_,
			  chat->title_.c_str());
		return;
	}

	auto messages = kworker_->getChatHistory(chat->id_, 0, 0, 300);
	if (unlikely(!messages)) {
		pr_notice("Could not get message history from (%ld) [%s]",
			  chat->id_, chat->title_.c_str());
		return;
	}

	count = messages->total_count_;
	current->setInterruptible();
	for (i = 0; i < count; i++) {
		if (shouldStop())
			break;

		auto &msg = messages->messages_[i];
		if (unlikely(!msg))
			continue;

		current->setUninterruptible();
		save_message(msg, &chat, chat_lock);
		current->setInterruptible();
	}
}


__hot void Scraper::save_message(td_api::object_ptr<td_api::message> &msg,
				 td_api::object_ptr<td_api::chat> *chat,
				 std::mutex *chat_lock)
{
	td_api::object_ptr<td_api::chat> chat2 = nullptr;

	if (!chat) {
		chat2 = kworker_->getChat(msg->chat_id_);
		if (unlikely(!chat2)) {
			pr_err("save_message(): "
			       "Could not get chat from message object %ld",
			       msg->id_);
			return;
		}
		chat = &chat2;

		chat_lock = kworker_->getChatLock(chat2->id_);
		if (unlikely(!chat_lock)) {
			pr_err("save_message(): "
			       "Could not get chat lock (%ld) [%s]",
			       chat2->id_, chat2->title_.c_str());
			return;
		}
	}


	touch_chat(*chat, chat_lock);

	// auto &content = msg->content_;

	// if (unlikely(!content))
	// 	return;

	// if (content->get_id() != td_api::messageText::ID)
	// 	return;

	// auto &text = static_cast<td_api::messageText &>(*content);
	// pr_notice("text = %s", text.text_->text_.c_str());
	// chat_lock->unlock();
	// sleep(3);
}


static uint64_t touch_chat_get_pk_id(mysql::MySQL *db, int64_t tg_chat_id)
{
	uint64_t pk_id;
	int qlen, tmp;
	MYSQL_ROW row;
	char qbuf[128];
	mysql::MySQLRes *res;

	qlen = snprintf(qbuf, sizeof(qbuf),
			"SELECT id FROM gt_groups WHERE tg_group_id = %" PRId64,
			tg_chat_id);

	tmp = db->realQuery(qbuf, (size_t)qlen);
	if (unlikely(tmp)) {
		pr_err("query(): %s", db->getError());
		return -1ULL;
	}

	res = db->storeResult();
	if (MYSQL_IS_ERR_OR_NULL(res)) {
		pr_err("storeResult(): %s", db->getError());
		return -1ULL;
	}

	row = res->fetchRow();
	if (unlikely(!row)) {
		delete res;
		return 0;
	}

	pk_id = strtoull(row[0], NULL, 10);
	delete res;
	return pk_id;
}


static uint64_t touch_chat_save_chat(mysql::MySQL *db,
				     td_api::object_ptr<td_api::chat> &chat)
{
	int errret;
	uint64_t pk_id;
	const char *errstr = nullptr;
	const char *stmtErrFunc = nullptr;
	mysql::MySQLStmt *stmt = nullptr;


	stmt = db->prepare(2,
		"INSERT INTO `gt_groups` "
		"("
			"`tg_group_id`,"
			"`username`,"
			"`link`,"
			"`name`,"
			"`created_at`,"
			"`updated_at`"
		")"
			" VALUES "
		"(?, NULL, NULL, ?, NOW(), NULL);"
	);

	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt))
		goto prepare_err;

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	stmt->bind(0, MYSQL_TYPE_LONGLONG, &chat->id_, sizeof(chat->id_));
	stmt->bind(1, MYSQL_TYPE_STRING, (void *)chat->title_.c_str(),
		   chat->title_.size());

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_id = stmt->getInsertId();
	goto out;

prepare_err:
	if (MYSQL_IS_ERR<mysql::MySQLStmt>(stmt)) {
		errret = MYSQL_PTR_ERR<mysql::MySQLStmt>(stmt);
		errstr = strerror(errret);
	} else {
		errstr = db->getError();
		errret = db->getErrno();
	}

	stmt = nullptr;
	pr_err("prepare(): (%d) %s", errret, errstr);
	return -1ULL;


stmt_err:
	errstr = stmt->getError();
	errret = stmt->getErrno();
	pr_err("%s(): (%d) %s", stmtErrFunc, errret, errstr);
	pk_id = -1ULL;
out:
	delete stmt;
	return pk_id;
}


__hot uint64_t Scraper::touch_chat(td_api::object_ptr<td_api::chat> &chat,
				   std::mutex *chat_lock)
	__acquires(chat_lock)
	__releases(chat_lock)
{
	uint64_t ret = 0;
	mysql::MySQL *db;

	if (unlikely(!chat))
		return 0;

	if (unlikely(!chat_lock)) {
		chat_lock = kworker_->getChatLock(chat->id_);
		if (unlikely(!chat_lock)) {
			pr_err("touch_chat(): "
			       "Could not get chat lock (%ld) [%s]",
			       chat->id_, chat->title_.c_str());
			return 0;
		}
	}

	db = kworker_->getDbPool();
	if (unlikely(!db)) {
		pr_err("visit_chat(): Could not get DB pool");
		return 0;
	}

	pr_debug("Touching chat id %ld...", chat->id_);
	chat_lock->lock();
	ret = touch_chat_get_pk_id(db, chat->id_);
	if (ret == 0)
		ret = touch_chat_save_chat(db, chat);
	chat_lock->unlock();
	kworker_->putDbPool(db);
	return unlikely(ret == -1ULL) ? 0 : ret;
}


} /* namespace tgvisd */
