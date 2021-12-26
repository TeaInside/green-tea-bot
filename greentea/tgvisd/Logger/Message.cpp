// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 */

#include <inttypes.h>
#include <tgvisd/mysql_helpers.hpp>
#include <tgvisd/Logger/Message.hpp>

using SenderUser = tgvisd::Logger::Sender::User;
using SenderChat = tgvisd::Logger::Sender::Chat;
using ChatGroup = tgvisd::Logger::Chat::Group;
using ChatUser = tgvisd::Logger::Chat::User;

namespace tgvisd::Logger {

Message::Message(KWorker *kworker, const td_api::message &message):
	message_(message),
	kworker_(kworker)
{

}

Message::~Message(void)
{
	if (m_sender_) {
		delete m_sender_;
		m_sender_ = nullptr;
	}

	if (m_chat_) {
		delete m_chat_;
		m_chat_ = nullptr;
	}

	if (db_) {
		kworker_->putDbPool(db_);
		db_ = nullptr;
	}
}

bool Message::resolve_chat(void)
{
	if (!chat_) {
		chat_ = kworker_->getChat(message_.chat_id_);
		if (unlikely(!chat_)) {
			pr_err("resolve_chat(): "
			       "Could not get chat from message object %ld",
			       message_.id_);
			return false;
		}
	}

	if (!chat_lock_) {
		chat_lock_ = kworker_->getChatLock(chat_->id_);
		if (unlikely(!chat_lock_)) {
			pr_err("resolve_chat(): "
			       "Could not get chat lock (%ld) [%s]",
			       chat_->id_, chat_->title_.c_str());
			return false;
		}
	}

	switch (chat_->type_->get_id()) {
	case td_api::chatTypeBasicGroup::ID:
	case td_api::chatTypeSupergroup::ID:
		m_chat_ = new ChatGroup(kworker_, *chat_);
		break;
	case td_api::chatTypeSecret::ID:
	case td_api::chatTypePrivate::ID:
		m_chat_ = new ChatUser(kworker_, *chat_);
		break;
	default:
		pr_err("Invalid chat type on resolve_sender()");
		return false;
	}

	return true;
}

bool Message::resolve_sender(void)
{
	int64_t lock_id;
	const auto &s = message_.sender_;

	switch (s->get_id()) {
	case td_api::messageSenderUser::ID:
		m_sender_ = new SenderUser(kworker_, *s);
		if (sender_lock_)
			break;
		lock_id = static_cast<const td_api::messageSenderChat &>(*s).chat_id_;
		sender_lock_ = kworker_->getUserLock(lock_id);
		break;
	case td_api::messageSenderChat::ID:
		m_sender_ = new SenderChat(kworker_, *s);
		if (sender_lock_)
			break;
		lock_id = static_cast<const td_api::messageSenderChat &>(*s).chat_id_;
		sender_lock_ = kworker_->getChatLock(lock_id);
		break;
	default:
		pr_err("Invalid sender type on resolve_sender()");
		return false;
	}

	if (unlikely(!sender_lock_)) {
		pr_err("resolve_sender(): Could not get chat lock (%ld) [%s]",
		       chat_->id_, chat_->title_.c_str());
		return false;
	}

	return true;
}

bool Message::resolve_db_pool(void)
{
	db_ = kworker_->getDbPool();
	if (unlikely(!db_))
		return false;

	assert(m_chat_);
	assert(m_sender_);
	m_chat_->setDbPool(db_);
	m_sender_->setDbPool(db_);
	return true;
}

bool Message::resolve_pk(void)
	__acquires(chat_lock_)
	__releases(chat_lock_)
{
	assert(m_chat_);
	assert(m_sender_);
	assert(chat_lock_);
	assert(sender_lock_);

	chat_lock_->lock();
	pk_chat_id_ = m_chat_->getPK();
	if (unlikely(!pk_chat_id_)) {
		chat_lock_->unlock();
		return false;
	}
	chat_lock_->unlock();

	sender_lock_->lock();
	pk_sender_id_ = m_sender_->getPK();
	if (unlikely(!pk_sender_id_)) {
		sender_lock_->unlock();
		return false;
	}
	sender_lock_->unlock();

	return true;
}

static uint64_t save_message_if_not_exist(mysql::MySQL *db,
					  const td_api::message &message,
					  uint64_t pk_chat_id,
					  uint64_t pk_sender_id);

void Message::save(void)
{
	if (!resolve_sender())
		return;

	if (!resolve_chat())
		return;

	if (!resolve_db_pool())
		return;

	if (!resolve_pk())
		return;

	chat_lock_->lock();
	save_message_if_not_exist(db_, message_, pk_chat_id_, pk_sender_id_);
	chat_lock_->unlock();
}

static uint64_t create_message(mysql::MySQL *db, const td_api::message &message,
			       uint64_t pk_chat_id, uint64_t pk_sender_id)
{
	uint64_t tg_msg_id;
	uint64_t pk_message_id;
	uint64_t reply_to_tg_msg_id;
	mysql::MySQLStmt *stmt = nullptr;
	const char *stmtErrFunc = nullptr;

	stmt = db->prepare(8,
		"INSERT INTO `gt_messages` "
		"("
			"`chat_id`,"
			"`sender_id`,"
			"`tg_msg_id`,"
			"`reply_to_tg_msg_id`,"
			"`msg_type`,"
			"`has_edited_msg`,"
			"`is_forwarded_msg`,"
			"`is_deleted`,"
			"`created_at`,"
			"`updated_at`"
		")"
			" VALUES "
		"("
			"?,"		/* chat_id */
			"?,"		/* sender_id */
			"?,"		/* tg_msg_id */
			"?,"		/* reply_to_tg_msg_id */
			"?,"		/* msg_type */
			"?,"		/* has_edited_msg */
			"?,"		/* is_forwarded_msg */
			"?,"		/* is_deleted */
			"NOW(),"	/* created_at */
			"NULL"
		");"
	);

	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt)) {
		mysql_handle_prepare_err(db, stmt);
		return 0;
	}

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	tg_msg_id = message.id_ >> 20u;
	reply_to_tg_msg_id = message.reply_to_message_id_ >> 20u;

	stmt->bind(0, MYSQL_TYPE_LONGLONG, (void *) &pk_chat_id,
		   sizeof(pk_chat_id));
	stmt->bind(1, MYSQL_TYPE_LONGLONG, (void *) &pk_sender_id,
		   sizeof(pk_sender_id));
	stmt->bind(2, MYSQL_TYPE_LONGLONG, (void *) &tg_msg_id,
		   sizeof(tg_msg_id));

	if (reply_to_tg_msg_id)
		stmt->bind(3, MYSQL_TYPE_LONGLONG, (void *) &reply_to_tg_msg_id,
			   sizeof(reply_to_tg_msg_id));
	else
		stmt->bind(3, MYSQL_TYPE_NULL, NULL, 0);

	stmt->bind(4, MYSQL_TYPE_STRING, (void *) "text", 4);
	stmt->bind(5, MYSQL_TYPE_STRING,
		   (void *) (message.edit_date_ ? "1" : "0"), 1);
	stmt->bind(6, MYSQL_TYPE_STRING,
		   (void *) (message.forward_info_ ? "1" : "0"), 1);
	stmt->bind(7, MYSQL_TYPE_STRING, (void *) "0" , 1);

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_message_id = stmt->getInsertId();
	goto out;

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_message_id = 0;
out:
	delete stmt;
	return pk_message_id;
}

static uint64_t get_message_pk(mysql::MySQL *db, const td_api::message &message,
			       uint64_t pk_chat_id, uint64_t pk_sender_id)
{
	static const char q[] =
		"SELECT id FROM gt_messages WHERE "
		"chat_id = %" PRIu64 " AND tg_msg_id = %" PRIu64;

	int qlen, tmp;
	MYSQL_ROW row;
	mysql::MySQLRes *res;
	uint64_t tg_msg_id;
	uint64_t pk_message_id;
	char qbuf[sizeof(q) + 128];

	tg_msg_id = message.id_ >> 20u;
	qlen = snprintf(qbuf, sizeof(qbuf), q, pk_chat_id, tg_msg_id);

	tmp = db->realQuery(qbuf, (size_t) qlen);
	if (unlikely(tmp)) {
		pr_err("query(): %s", db->getError());
		return 0;
	}

	res = db->storeResult();
	if (MYSQL_IS_ERR_OR_NULL(res)) {
		pr_err("storeResult(): %s", db->getError());
		return 0;
	}

	row = res->fetchRow();
	if (!row) {
		pk_message_id = create_message(db, message, pk_chat_id,
					       pk_sender_id);
		goto out;
	}

	pk_message_id = strtoull(row[0], NULL, 10);
out:
	delete res;
	return pk_message_id;
}

static uint64_t save_message_if_not_exist(mysql::MySQL *db,
					  const td_api::message &message,
					  uint64_t pk_chat_id,
					  uint64_t pk_sender_id)
{
	int tmp;
	uint64_t pk_message_id;

	tmp = db->beginTransaction();
	if (unlikely(tmp)) {
		pr_err("beginTransaction(): %s", db->getError());
		return 0;
	}

	pk_message_id = get_message_pk(db, message, pk_chat_id, pk_sender_id);
	if (unlikely(!pk_message_id))
		goto rollback;

	tmp = db->commit();
	if (unlikely(tmp)) {
		pr_err("commit(): %s", db->getError());
		goto rollback;
	}

	/* Success! */
	pr_notice("pk_message_id = %" PRIu64, pk_message_id);
	return pk_message_id;

rollback:
	tmp = db->rollback();
	if (unlikely(tmp))
		pr_err("rollback(): %s", db->getError());
	return 0;
}

} /* namespace tgvisd::Logger */
