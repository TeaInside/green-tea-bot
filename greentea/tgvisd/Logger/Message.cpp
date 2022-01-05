// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * Copyright (C) 2022  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <time.h>
#include <inttypes.h>
#include <tgvisd/mysql_helpers.hpp>
#include <tgvisd/Logger/Message.hpp>

using SenderUser = tgvisd::Logger::Sender::User;
using SenderChat = tgvisd::Logger::Sender::Chat;
using ChatGroup = tgvisd::Logger::Chat::Group;
using ChatUser = tgvisd::Logger::Chat::User;

namespace tgvisd::Logger {

static inline void *bn_str(bool p)
{
	return (void *) (p ? "1" : "0");
}

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
	const auto &s = message_.sender_id_;

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
	const uint32_t max_try = 32;
	uint32_t try_num = 0;

	assert(m_chat_);
	assert(m_sender_);

	db_ = kworker_->getDbPool();
	while (unlikely(!db_)) {
		if (unlikely(++try_num >= max_try))
			return false;

		sleep(1);
		db_ = kworker_->getDbPool();
	}

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

static uint64_t save_message_if_not_exist(KWorker *kwrk, mysql::MySQL *db,
					  const td_api::message &message,
					  uint64_t pk_chat_id,
					  uint64_t pk_sender_id);

void Message::save(void)
{
	if (unlikely(!message_.content_))
		return;

	/*
	 * Currently, we only save text message.
	 * TODO: Handle other types of message, like photo, sticker, etc.
	 */
	if (message_.content_->get_id() != td_api::messageText::ID)
		return;

	if (!resolve_sender())
		return;

	if (!resolve_chat())
		return;

	if (!resolve_db_pool())
		return;

	if (!resolve_pk())
		return;

	chat_lock_->lock();
	save_message_if_not_exist(kworker_, db_, message_, pk_chat_id_,
				  pk_sender_id_);
	chat_lock_->unlock();
}

static size_t convert_epoch_to_db_format(char *buf, size_t buf_size,
					 time_t time)
{
	struct tm tm;
	memset(&tm, 0, sizeof(struct tm));
	gmtime_r(&time, &tm);
	return strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", &tm);
}

static uint64_t create_message_content(mysql::MySQL *db,
				       const td_api::message &message,
				       uint64_t pk_message_id)
{
	char tg_date[64];
	size_t tg_date_size;
	time_t tg_date_epoch;
	std::string entities_txt;
	uint64_t pk_message_content_id;
	mysql::MySQLStmt *stmt = nullptr;
	const char *stmtErrFunc = nullptr;

	const auto &content = static_cast<const td_api::messageText &>(*message.content_);
	if (unlikely(!content.text_))
		return 0;

	const auto &formattedText = *content.text_;
	const auto &text = formattedText.text_;
	const auto &entities = formattedText.entities_;

	stmt = db->prepare(5,
		"INSERT INTO `gt_message_content` "
		"("
			"`message_id`,"
			"`text`,"
			"`text_entities`,"
			"`is_edited_msg`,"
			"`tg_date`,"
			"`created_at`"
		")"
			" VALUES "
		"("
			"?,"	/* messsage_id */
			"?,"	/* text */
			"?,"	/* text_entities */
			"?,"	/* is_edited_msg */
			"?,"	/* tg_date */
			"NOW()"	/* created_at */
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

	stmt->bind(0, MYSQL_TYPE_LONGLONG, (void *) &pk_message_id,
		   sizeof(pk_message_id));
	stmt->bind(1, MYSQL_TYPE_STRING, (void *) text.c_str(), text.size());

	if (!entities.size()) {
		stmt->bind(2, MYSQL_TYPE_NULL, NULL, 0);
	} else {
		entities_txt = to_string(entities);
		void *p = (void *) entities_txt.c_str();
		stmt->bind(2, MYSQL_TYPE_STRING, p, entities_txt.size());
	}

	stmt->bind(3, MYSQL_TYPE_STRING, bn_str(!!message.edit_date_), 1);

	if (message.edit_date_)
		tg_date_epoch = message.edit_date_;
	else
		tg_date_epoch = message.date_;

	tg_date_size = convert_epoch_to_db_format(tg_date, sizeof(tg_date),
						  tg_date_epoch);

	if (tg_date_size)
		stmt->bind(4, MYSQL_TYPE_STRING, tg_date, tg_date_size);
	else
		stmt->bind(4, MYSQL_TYPE_NULL, NULL, 0);

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_message_content_id = stmt->getInsertId();
	goto out;

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_message_content_id = 0;
out:
	delete stmt;
	return pk_message_content_id;
}

static uint64_t save_msg_fwd_info(KWorker *kwrk, mysql::MySQL *db,
				  const td_api::messageForwardInfo &mfi,
				  uint64_t pk_chat_id)
{
	char tg_date[64];
	size_t tg_date_size;
	uint64_t pk_msg_fwd_info_id = 0;
	mysql::MySQLStmt *stmt = nullptr;
	const char *stmtErrFunc = nullptr;
	union {
		uint64_t sender_id;
	} p;

	enum {
		f_message_id = 0,
		f_sender_id = 1,
		f_tg_date = 2,
		f_public_service_announcement_type = 3,
		f_from_tg_chat_id = 4,
		f_from_tg_msg_id = 5,
		f_sender_name = 6,
		f_author_signature = 7,
		f_extra = 8
	};

	std::string extra = "";
	const auto &origin = *mfi.origin_;
	const auto obj_id = origin.get_id();
	const std::string &psat = mfi.public_service_announcement_type_;

	stmt = db->prepare(9,
		"INSERT INTO `gt_msg_fwd_info`"
		"("
			"`message_id`,"
			"`sender_id`,"
			"`tg_date`,"
			"`public_service_announcement_type`,"
			"`from_tg_chat_id`,"
			"`from_tg_msg_id`,"
			"`sender_name`,"
			"`author_signature`,"
			"`extra`"
		")"
			" VALUES "
		"("
			"?,"
			"?,"
			"?,"
			"?,"
			"?,"
			"?,"
			"?,"
			"?,"
			"?"
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

	stmt->bind(f_message_id, MYSQL_TYPE_LONGLONG, (void *) &pk_chat_id,
		   sizeof(pk_chat_id));

	tg_date_size = convert_epoch_to_db_format(tg_date, sizeof(tg_date),
						  (time_t) mfi.date_);

	if (tg_date_size)
		stmt->bind(f_tg_date, MYSQL_TYPE_STRING, tg_date, tg_date_size);
	else
		stmt->bind(f_tg_date, MYSQL_TYPE_NULL, NULL, 0);

	if (psat.size())
		stmt->bind(f_public_service_announcement_type,
			   MYSQL_TYPE_STRING, (void *) psat.c_str(),
			   psat.size());
	else
		stmt->bind(f_public_service_announcement_type, MYSQL_TYPE_NULL,
			   NULL, 0);

	if (mfi.from_chat_id_)
		stmt->bind(f_from_tg_chat_id,
			   MYSQL_TYPE_LONGLONG, (void *) &mfi.from_chat_id_,
			   sizeof(mfi.from_chat_id_));
	else
		stmt->bind(f_from_tg_chat_id, MYSQL_TYPE_NULL, NULL, 0);

	if (mfi.from_message_id_)
		stmt->bind(f_from_tg_msg_id,
			   MYSQL_TYPE_LONGLONG, (void *) &mfi.from_message_id_,
			   sizeof(mfi.from_message_id_));
	else
		stmt->bind(f_from_tg_msg_id, MYSQL_TYPE_NULL, NULL, 0);


	if (obj_id == td_api::messageForwardOriginUser::ID) {
		auto &tmp1 = static_cast<const td_api::messageForwardOriginUser &>(origin);
		auto tmp2  = td_api::messageSenderUser(tmp1.sender_user_id_);
		auto tmp3  = SenderUser(kwrk, tmp2);
		tmp3.setDbPool(db);
		p.sender_id = tmp3.getPK();
		if (unlikely(!p.sender_id)) {
			pr_err("Cannot get sender_id in save_msg_fwd_info");
			stmt->bind(f_sender_id, MYSQL_TYPE_NULL, NULL, 0);
		} else {
			stmt->bind(f_sender_id, MYSQL_TYPE_LONGLONG,
				   (void *) &p.sender_id, sizeof(p.sender_id));
		}
		stmt->bind(f_sender_name, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_author_signature, MYSQL_TYPE_NULL, NULL, 0);

	} else if (obj_id == td_api::messageForwardOriginChannel::ID) {
		auto &tmp1 = static_cast<const td_api::messageForwardOriginChannel &>(origin);
		const std::string &tmp2 = tmp1.author_signature_;
		extra = to_string(tmp1);

		/* TODO: Handle chat sender. */
		stmt->bind(f_sender_id, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_sender_name, MYSQL_TYPE_NULL, NULL, 0);
		if (tmp2.size())
			stmt->bind(f_author_signature, MYSQL_TYPE_STRING,
				   (void *) tmp2.c_str(), tmp2.size());
		else
			stmt->bind(f_author_signature, MYSQL_TYPE_NULL, NULL, 0);

	} else if (obj_id == td_api::messageForwardOriginChat::ID) {
		auto &tmp1 = static_cast<const td_api::messageForwardOriginChat &>(origin);
		extra = to_string(tmp1);

		/* TODO: Handle chat sender. */
		stmt->bind(f_sender_id, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_sender_name, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_author_signature, MYSQL_TYPE_NULL, NULL, 0);

	} else if (obj_id == td_api::messageForwardOriginHiddenUser::ID) {
		auto &tmp1 = static_cast<const td_api::messageForwardOriginHiddenUser &>(origin);

		stmt->bind(f_sender_id, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_sender_name, MYSQL_TYPE_STRING,
			   (void *)tmp1.sender_name_.c_str(),
			   tmp1.sender_name_.size());
		stmt->bind(f_author_signature, MYSQL_TYPE_NULL, NULL, 0);

	} else if (obj_id == td_api::messageForwardOriginMessageImport::ID) {
		auto &tmp1 = static_cast<const td_api::messageForwardOriginMessageImport &>(origin);
		extra = to_string(tmp1);

		/* TODO: Handle chat sender. */
		stmt->bind(f_sender_id, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_sender_name, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_author_signature, MYSQL_TYPE_NULL, NULL, 0);
	} else {
		extra = "unknown_type";
		/* TODO: Handle chat sender. */
		stmt->bind(f_sender_id, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_sender_name, MYSQL_TYPE_NULL, NULL, 0);
		stmt->bind(f_author_signature, MYSQL_TYPE_NULL, NULL, 0);
	}

	if (extra.size())
		stmt->bind(f_extra, MYSQL_TYPE_STRING, (void *) extra.c_str(),
			   extra.size());
	else
		stmt->bind(f_extra, MYSQL_TYPE_NULL, NULL, 0);

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_msg_fwd_info_id = stmt->getInsertId();
	goto out;

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_msg_fwd_info_id = 0;
out:
	delete stmt;
	return pk_msg_fwd_info_id;
}

static uint64_t create_message(KWorker *kwrk, mysql::MySQL *db,
			       const td_api::message &message,
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
			"NULL"		/* updated_at */
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

	if (message.forward_info_) {
		if (unlikely(!save_msg_fwd_info(kwrk, db,
						*message.forward_info_,
						pk_message_id))) {
			pk_message_id = 0;
			goto out;
		}
	}

	if (unlikely(!create_message_content(db, message, pk_message_id)))
		pk_message_id = 0;

	goto out;

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_message_id = 0;
out:
	delete stmt;
	return pk_message_id;
}

static uint64_t get_message_pk(KWorker *kwrk, mysql::MySQL *db,
			       const td_api::message &message,
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
		pk_message_id = create_message(kwrk, db, message, pk_chat_id,
					       pk_sender_id);
		goto out;
	}

	pk_message_id = strtoull(row[0], NULL, 10);
out:
	delete res;
	return pk_message_id;
}

static uint64_t save_message_if_not_exist(KWorker *kwrk, mysql::MySQL *db,
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

	pk_message_id = get_message_pk(kwrk, db, message, pk_chat_id,
				       pk_sender_id);
	if (unlikely(!pk_message_id))
		goto rollback;

	tmp = db->commit();
	if (unlikely(tmp)) {
		pr_err("commit(): %s", db->getError());
		goto rollback;
	}

	/* Success! */
	// pr_notice("pk_message_id = %" PRIu64, pk_message_id);
	return pk_message_id;

rollback:
	tmp = db->rollback();
	if (unlikely(tmp))
		pr_err("rollback(): %s", db->getError());
	return 0;
}

/* static */
int64_t Message::getMinMaxMsgIdByTgGroupId(KWorker *kwrk, int64_t tg_group_id,
					   bool minMax)
{
	static const char q[] =
		"SELECT gt_messages.tg_msg_id FROM gt_messages "
		"WHERE gt_messages.chat_id = ( "
			"SELECT gt_chats.id FROM gt_chats "
			"INNER JOIN gt_chat_group ON gt_chats.id = gt_chat_group.chat_id "
			"INNER JOIN gt_groups ON gt_groups.id = gt_chat_group.group_id "
			"WHERE gt_groups.tg_group_id = %" PRId64 " LIMIT 1"
		") "
		"ORDER BY gt_messages.tg_msg_id %s LIMIT 1;";

	int64_t ret;
	int qlen, tmp;
	MYSQL_ROW row;
	mysql::MySQL *db;
	char qbuf[sizeof(q) + 128];
	mysql::MySQLRes *res = nullptr;
	const char *order_type = minMax ? "ASC" : "DESC";


	db = kwrk->getDbPool();
	if (unlikely(!db))
		return -1;

	ret  = -1;
	qlen = snprintf(qbuf, sizeof(qbuf), q, tg_group_id, order_type);
	tmp  = db->realQuery(qbuf, (size_t) qlen);
	if (unlikely(tmp)) {
		pr_err("query(): %s", db->getError());
		goto out;
	}

	res = db->storeResult();
	if (MYSQL_IS_ERR_OR_NULL(res)) {
		pr_err("storeResult(): %s", db->getError());
		goto out;
	}

	row = res->fetchRow();
	if (!row)
		/*
		 * The query didn't fail, we just don't yet have any
		 * record for this tg_group_id. Set the @ret to 0.
		 */
		ret = 0;
	else
		ret = strtoll(row[0], NULL, 10);

	delete res;
out:
	kwrk->putDbPool(db);
	return ret;
}

} /* namespace tgvisd::Logger */
