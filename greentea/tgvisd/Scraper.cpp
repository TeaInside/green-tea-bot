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


__cold Scraper::Scraper(Main *main):
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

	while (1) {
		uint32_t i;

		for (i = 0; i < 30; i++) {
			if (shouldStop())
				return;
			_run();
			sleep(1);
		}


		for (i = 0; i < 60; i++) {
			if (shouldStop())
				return;
			if (i % 10 == 0)
				_run();
			sleep(1);
		}
	}
}


__hot void Scraper::_run(void)
{
	int32_t i;
	int64_t chat_id;

	pr_notice("Getting chat list...");
	auto chats = kworker_->getChats(nullptr, 500);
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


struct scraper_payload {
	td_api::object_ptr<td_api::chat>	chat;
};


static void scraper_payload_deleter(void *p)
{
	delete (struct scraper_payload *)p;
}


__hot void Scraper::visit_chat(td_api::object_ptr<td_api::chat> &chat)
{
	int ret;
	struct task_work tw;
	struct scraper_payload *payload;

	payload = new struct scraper_payload;
	payload->chat = std::move(chat);

	tw.func = [this](struct tw_data *data){
		struct scraper_payload *payload;

		payload = (struct scraper_payload *)data->tw->payload;
		this->_visit_chat(data, payload->chat);
	};
	tw.payload = (void *)payload;
	tw.deleter = scraper_payload_deleter;

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
	uint64_t pk_gid, pk_uid, pk_mid;
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


	if (unlikely(!msg->sender_)) {
		pr_notice("save_message(): Ignoring message, as it does not "
			  "have a sender (%ld) [%s]", (*chat)->id_,
			  (*chat)->title_.c_str());
		return;
	}


	pk_gid = touch_group_chat(*chat, chat_lock);
	if (unlikely(pk_gid == 0)) {
		pr_err("save_message(): Ignoring message, could not get pk_gid "
		       "(%ld) [%s]", (*chat)->id_, (*chat)->title_.c_str());
		return;
	}


	if (unlikely(msg->sender_->get_id() != td_api::messageSenderUser::ID)) {
		pr_notice("save_message(): Ignoring message, as it is not sent "
			  "by messageSenderUser object (%ld) [%s]",
			  (*chat)->id_, (*chat)->title_.c_str());
		return;
	}

	auto sender = td::move_tl_object_as<td_api::messageSenderUser>(msg->sender_);
	pk_uid = touch_user_with_uid(sender->user_id_);
	if (unlikely(pk_uid == 0)) {
		pr_err("save_message(): Ignoring message, could not get pk_uid "
		       "(%ld) [%s]", (*chat)->id_, (*chat)->title_.c_str());
		return;
	}

	pr_notice("pk_uid: %lu; pk_gid: %lu", pk_uid, pk_gid);

	_save_msg(msg, pk_gid, pk_uid);

	// auto &text = static_cast<td_api::messageText &>(*content);
	// pr_notice("text = %s", text.text_->text_.c_str());
	// chat_lock->unlock();
	// sleep(3);
}


#define ZSTRL(STR) STR, sizeof(STR) - 1


__hot static uint64_t __save_msg(mysql::MySQL *db,
				 td_api::object_ptr<td_api::message> &msg,
				 uint64_t pk_gid, uint64_t pk_uid)
{
	int errret;
	uint64_t pk_mid;
	const char *errstr = nullptr;
	const char *stmtErrFunc = nullptr;
	mysql::MySQLStmt *stmt = nullptr;
	int64_t tg_msg_id, reply_to_tg_msg_id;


	stmt = db->prepare(8,
		"INSERT INTO `gt_group_messages` "
		"("
			"`group_id`,"
			"`user_id`,"
			"`tg_msg_id`,"
			"`reply_to_tg_msg_id`,"
			"`msg_type`,"
			"`has_edited_msg`,"
			"`is_forwarded_msg`,"
			"`is_deleted`,"
			"`desc`,"
			"`created_at`,"
			"`updated_at`"
		") VALUES (?, ?, ?, ?, ?, ?, ?, ?, NULL, NOW(), NULL);"
	);

	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt))
		goto prepare_err;

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	tg_msg_id = msg->id_ >> 20u;
	reply_to_tg_msg_id = msg->reply_to_message_id_ >> 20u;

	stmt->bind(0, MYSQL_TYPE_LONGLONG, &pk_gid, sizeof(int64_t));
	stmt->bind(1, MYSQL_TYPE_LONGLONG, &pk_uid, sizeof(int64_t));
	stmt->bind(2, MYSQL_TYPE_LONGLONG, &tg_msg_id, sizeof(int64_t));

	if (reply_to_tg_msg_id)
		stmt->bind(3, MYSQL_TYPE_LONGLONG, &reply_to_tg_msg_id,
			   sizeof(int64_t));
	else
		stmt->bind(3, MYSQL_TYPE_NULL, nullptr, 0);

	stmt->bind(4, MYSQL_TYPE_STRING, (void *)"text", 4);
	stmt->bind(5, MYSQL_TYPE_STRING, (void *)"0", 1);
	stmt->bind(6, MYSQL_TYPE_STRING, (void *)"0", 1);
	stmt->bind(7, MYSQL_TYPE_STRING, (void *)"0", 1);

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_mid = stmt->getInsertId();
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
	pk_mid = -1ULL;
out:
	delete stmt;
	return pk_mid;
}


__hot void Scraper::_save_msg(td_api::object_ptr<td_api::message> &msg,
			      uint64_t pk_gid, uint64_t pk_uid)
{
	int tmp;
	uint64_t pk_mid;
	mysql::MySQL *db;

	auto &content = msg->content_;

	if (unlikely(!content))
		return;

	db = kworker_->getDbPool();
	if (unlikely(!db)) {
		pr_err("_save_msg(): Could not get DB pool");
		return;
	}

	tmp = db->realQuery(ZSTRL("START TRANSACTION"));
	if (unlikely(tmp)) {
		pr_err("realQuery(\"START TRANSACTION\"): %s", db->getError());
		goto out_put;
	}

	pk_mid = __save_msg(db, msg, pk_gid, pk_uid);
	if (unlikely(pk_mid == 0 || pk_mid == -1ULL))
		goto rollback;

	switch (content->get_id()) {
	case td_api::messageText::ID:
		break;
	}

	tmp = db->realQuery(ZSTRL("COMMIT"));
	if (unlikely(tmp)) {
		pr_err("realQuery(\"COMMIT\"): %s", db->getError());
		goto rollback;
	}

	goto out_put;

rollback:
	tmp = db->realQuery(ZSTRL("ROLLBACK"));
	if (unlikely(tmp))
		pr_err("realQuery(\"ROLLBACK\"): %s", db->getError());

out_put:
	kworker_->putDbPool(db);
}


__hot static uint64_t tgc_get_pk_id(mysql::MySQL *db, int64_t tg_chat_id)
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
		pk_id = 0;
		goto out;
	}

	pk_id = strtoull(row[0], NULL, 10);
out:
	delete res;
	return pk_id;
}


__hot static uint64_t tgc_save_chat(mysql::MySQL *db,
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


__hot uint64_t Scraper::touch_group_chat(td_api::object_ptr<td_api::chat> &chat,
					 std::mutex *chat_lock)
	__acquires(chat_lock)
	__releases(chat_lock)
{
	uint64_t ret = 0;
	mysql::MySQL *db;

	if (unlikely(!chat))
		return 0;

	if (unlikely(chat->type_->get_id() != td_api::chatTypeSupergroup::ID))
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
	ret = tgc_get_pk_id(db, chat->id_);
	if (ret == 0)
		ret = tgc_save_chat(db, chat);
	chat_lock->unlock();
	kworker_->putDbPool(db);
	return unlikely(ret == -1ULL) ? 0 : ret;
}


uint64_t Scraper::touch_user_with_uid(int64_t tg_user_id, std::mutex *user_lock)
{
	auto user = kworker_->getUser(tg_user_id);
	if (unlikely(!user))
		return 0;

	return touch_user(user, user_lock);
}


__hot static uint64_t tu_get_pk_id(mysql::MySQL *db, int64_t tg_user_id)
{
	uint64_t pk_id;
	int qlen, tmp;
	MYSQL_ROW row;
	char qbuf[128];
	mysql::MySQLRes *res;

	qlen = snprintf(qbuf, sizeof(qbuf),
			"SELECT id FROM gt_users WHERE tg_user_id = %" PRId64,
			tg_user_id);

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
		pk_id = 0;
		goto out;
	}

	pk_id = strtoull(row[0], NULL, 10);
out:
	delete res;
	return pk_id;
}


__hot static uint64_t tgc_save_user(mysql::MySQL *db,
				    td_api::object_ptr<td_api::user> &u)
{
	int errret;
	MYSQL_BIND *b;
	uint64_t pk_id;
	const char *errstr = nullptr;
	size_t userTypeLen = 0;
	const char *userType = nullptr;
	const char *stmtErrFunc = nullptr;
	mysql::MySQLStmt *stmt = nullptr;
	bool null_v = true;

	stmt = db->prepare(9,
		"INSERT INTO `gt_users` "
		"("
			"`tg_user_id`,"
			"`username`,"
			"`first_name`,"
			"`last_name`,"
			"`phone`,"
			"`is_verified`,"
			"`is_support`,"
			"`is_scam`,"
			"`type`,"
			"`created_at`"
		")"
			" VALUES "
		"(?, ?, ?, ?, ?, ?, ?, ?, ?, NOW());"
	);

	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt))
		goto prepare_err;

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	stmt->bind(0, MYSQL_TYPE_LONGLONG, &u->id_, sizeof(u->id_));

	b = stmt->bind(1, MYSQL_TYPE_STRING, (void *)u->username_.c_str(),
		       u->username_.size());
	if (!u->username_.size())
		b->is_null = &null_v;

	stmt->bind(2, MYSQL_TYPE_STRING, (void *)u->first_name_.c_str(),
		   u->first_name_.size());
	if (!u->first_name_.size())
		b->is_null = &null_v;

	b = stmt->bind(3, MYSQL_TYPE_STRING, (void *)u->last_name_.c_str(),
		       u->last_name_.size());
	if (!u->last_name_.size())
		b->is_null = &null_v;

	b = stmt->bind(4, MYSQL_TYPE_STRING, (void *)u->phone_number_.c_str(),
		       u->phone_number_.size());
	if (!u->phone_number_.size())
		b->is_null = &null_v;

	stmt->bind(5, MYSQL_TYPE_STRING, (void *)(u->is_verified_ ? "1" : "0"), 1);
	stmt->bind(6, MYSQL_TYPE_STRING, (void *)(u->is_support_ ? "1" : "0"), 1);
	stmt->bind(7, MYSQL_TYPE_STRING, (void *)(u->is_scam_ ? "1" : "0"), 1);

	switch (u->type_->get_id()) {
	case td_api::userTypeBot::ID:
		userType = "bot";
		userTypeLen = 3;
		break;
	case td_api::userTypeDeleted::ID:
		userType = "deleted";
		userTypeLen = 7;
		break;
	case td_api::userTypeRegular::ID:
		userType = "user";
		userTypeLen = 4;
		break;
	case td_api::userTypeUnknown::ID:
	default:
		userType = "unknown";
		userTypeLen = 7;
		break;
	}
	stmt->bind(8, MYSQL_TYPE_STRING, (void *)userType, userTypeLen);

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


__hot uint64_t Scraper::touch_user(td_api::object_ptr<td_api::user> &user,
				   std::mutex *user_lock)
{
	uint64_t ret = 0;
	mysql::MySQL *db;

	if (unlikely(!user))
		return 0;

	if (!user_lock) {
		user_lock = kworker_->getUserLock(user->id_);
		if (unlikely(!user_lock)) {
			pr_err("touch_user(): Could not get user lock %ld",
			       user->id_);
			return 0;
		}
	}

	db = kworker_->getDbPool();
	if (unlikely(!db)) {
		pr_err("touch_user(): Could not get DB pool");
		return 0;
	}

	pr_debug("Touching user id %ld...", user->id_);
	user_lock->lock();
	ret = tu_get_pk_id(db, user->id_);
	if (ret == 0)
		ret = tgc_save_user(db, user);
	user_lock->unlock();
	kworker_->putDbPool(db);
	return unlikely(ret == -1ULL) ? 0 : ret;
}


} /* namespace tgvisd */
