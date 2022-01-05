// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @author Ammar Faizi <ammarfaizi2@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger::Sender
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <inttypes.h>
#include <tgvisd/mysql_helpers.hpp>
#include <tgvisd/Logger/Sender/User.hpp>

namespace tgvisd::Logger::Sender {

struct user_data {
	const td_api::MessageSender			&sender_;
	td_api::object_ptr<td_api::user>		user_;
	td_api::object_ptr<td_api::userFullInfo>	userFull_;

	inline ~user_data(void) = default;

	inline user_data(const td_api::MessageSender &sender):
		sender_(sender)
	{
	}
};

static inline void *bn_str(bool p)
{
	return (void *) (p ? "1" : "0");
}

static uint64_t create_user_history(mysql::MySQL *db, struct user_data *ud,
				    uint64_t pk_user_id)
{
	uint64_t pk_id;
	size_t user_type_len;
	const char *user_type;
	const char *stmtErrFunc = nullptr;
	mysql::MySQLStmt *stmt = nullptr;
	const td_api::user &user = *ud->user_;
	const td_api::userFullInfo &userFull = *ud->userFull_;

	stmt = db->prepare(10,
		"INSERT INTO `gt_users_history` "
		"("
			"`user_id`,"
			"`username`,"
			"`first_name`,"
			"`last_name`,"
			"`phone`,"
			"`is_verified`,"
			"`is_support`,"
			"`is_scam`,"
			"`bio`,"
			"`type`,"
			"`created_at`"
		")"
			" VALUES "
		"("
			"?,"		/* user_id */
			"?,"		/* username */
			"?,"		/* first_name */
			"?,"		/* last_name */
			"?,"		/* phone */
			"?,"		/* is_verified */
			"?,"		/* is_support */
			"?,"		/* is_scam */
			"?,"		/* bio */
			"?,"		/* type */
			"NOW()"		/* created_at */
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

	stmt->bind(0, MYSQL_TYPE_LONGLONG, (void *) &pk_user_id,
		   sizeof(pk_user_id));

	if (!user.username_.size())
		stmt->bind(1, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(1, MYSQL_TYPE_STRING,
			   (void *) user.username_.c_str(),
			   user.username_.size());

	if (!user.first_name_.size())
		stmt->bind(2, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(2, MYSQL_TYPE_STRING,
			   (void *) user.first_name_.c_str(),
			   user.first_name_.size());

	if (!user.last_name_.size())
		stmt->bind(3, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(3, MYSQL_TYPE_STRING,
			   (void *) user.last_name_.c_str(),
			   user.last_name_.size());

	if (!user.phone_number_.size())
		stmt->bind(4, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(4, MYSQL_TYPE_STRING,
			   (void *) user.phone_number_.c_str(),
			   user.phone_number_.size());

	stmt->bind(5, MYSQL_TYPE_STRING, bn_str(user.is_verified_), 1);
	stmt->bind(6, MYSQL_TYPE_STRING, bn_str(user.is_support_), 1);
	stmt->bind(7, MYSQL_TYPE_STRING, bn_str(user.is_scam_), 1);

	if (!userFull.bio_.size())
		stmt->bind(8, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(8, MYSQL_TYPE_STRING,
			   (void *) userFull.bio_.c_str(),
			   userFull.bio_.size());

	switch (ud->user_->type_->get_id()) {
	case td_api::userTypeBot::ID:
		user_type = "bot";
		user_type_len = sizeof("bot") - 1;
		break;
	case td_api::userTypeDeleted::ID:
		user_type = "deleted";
		user_type_len = sizeof("deleted") - 1;
		break;
	case td_api::userTypeRegular::ID:
		user_type = "user";
		user_type_len = sizeof("user") - 1;
		break;
	case td_api::userTypeUnknown::ID:
	default:
		user_type = "unknown";
		user_type_len = sizeof("unknown") - 1;
		break;
	}
	stmt->bind(9, MYSQL_TYPE_STRING, (void *) user_type, user_type_len);

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

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_id = 0;
out:
	delete stmt;
	return pk_id;
}

static uint64_t create_user(mysql::MySQL *db, struct user_data *ud)
{
	uint64_t pk_user_id;
	size_t user_type_len;
	const char *user_type;
	const char *stmtErrFunc = nullptr;
	mysql::MySQLStmt *stmt = nullptr;
	const td_api::user &user = *ud->user_;
	const td_api::userFullInfo &userFull = *ud->userFull_;

	stmt = db->prepare(10,
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
			"`bio`,"
			"`type`,"
			"`created_at`,"
			"`updated_at`"
		")"
			" VALUES "
		"("
			"?,"		/* tg_user_id */
			"?,"		/* username */
			"?,"		/* first_name */
			"?,"		/* last_name */
			"?,"		/* phone */
			"?,"		/* is_verified */
			"?,"		/* is_support */
			"?,"		/* is_scam */
			"?,"		/* bio */
			"?,"		/* type */
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

	stmt->bind(0, MYSQL_TYPE_LONGLONG, (void *) &user.id_, sizeof(user.id_));

	if (!user.username_.size())
		stmt->bind(1, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(1, MYSQL_TYPE_STRING,
			   (void *) user.username_.c_str(),
			   user.username_.size());

	if (!user.first_name_.size())
		stmt->bind(2, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(2, MYSQL_TYPE_STRING,
			   (void *) user.first_name_.c_str(),
			   user.first_name_.size());

	if (!user.last_name_.size())
		stmt->bind(3, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(3, MYSQL_TYPE_STRING,
			   (void *) user.last_name_.c_str(),
			   user.last_name_.size());

	if (!user.phone_number_.size())
		stmt->bind(4, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(4, MYSQL_TYPE_STRING,
			   (void *) user.phone_number_.c_str(),
			   user.phone_number_.size());

	stmt->bind(5, MYSQL_TYPE_STRING, bn_str(user.is_verified_), 1);
	stmt->bind(6, MYSQL_TYPE_STRING, bn_str(user.is_support_), 1);
	stmt->bind(7, MYSQL_TYPE_STRING, bn_str(user.is_scam_), 1);

	if (!userFull.bio_.size())
		stmt->bind(8, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(8, MYSQL_TYPE_STRING,
			   (void *) userFull.bio_.c_str(),
			   userFull.bio_.size());

	switch (ud->user_->type_->get_id()) {
	case td_api::userTypeBot::ID:
		user_type = "bot";
		user_type_len = sizeof("bot") - 1;
		break;
	case td_api::userTypeDeleted::ID:
		user_type = "deleted";
		user_type_len = sizeof("deleted") - 1;
		break;
	case td_api::userTypeRegular::ID:
		user_type = "user";
		user_type_len = sizeof("user") - 1;
		break;
	case td_api::userTypeUnknown::ID:
	default:
		user_type = "unknown";
		user_type_len = sizeof("unknown") - 1;
		break;
	}
	stmt->bind(9, MYSQL_TYPE_STRING, (void *) user_type, user_type_len);

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_user_id = stmt->getInsertId();
	if (!create_user_history(db, ud, pk_user_id))
		pk_user_id = 0;

	goto out;

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_user_id = 0;
out:
	delete stmt;
	return pk_user_id;
}

static uint64_t get_user_pk(mysql::MySQL *db, struct user_data *ud)
{
	static const char q[] =
		"SELECT id FROM gt_users WHERE tg_user_id = %" PRId64;

	uint64_t ret;
	int qlen, tmp;
	MYSQL_ROW row;
	mysql::MySQLRes *res;
	char qbuf[sizeof(q) + 64];

	qlen = snprintf(qbuf, sizeof(qbuf), q, ud->user_->id_);

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
	if (unlikely(!row)) {
		ret = create_user(db, ud);
		goto out;
	}

	ret = strtoull(row[0], NULL, 10);
out:
	delete res;
	return ret;

}

static uint64_t create_sender_user(mysql::MySQL *db, uint64_t pk_sender_id,
				   uint64_t pk_user_id)
{
	static const char q[] =
		"INSERT INTO `gt_sender_user` (user_id, sender_id) VALUES "
		"(%" PRId64", %" PRId64 ")";

	int qlen, tmp;
	char qbuf[sizeof(q) + 64];

	qlen = snprintf(qbuf, sizeof(qbuf), q, pk_user_id, pk_sender_id);

	tmp = db->realQuery(qbuf, (size_t) qlen);
	if (unlikely(tmp)) {
		pr_err("query(): %s", db->getError());
		return 0;
	}

	return 1;
}

static uint64_t create_sender(mysql::MySQL *db, struct user_data *ud)
{
	size_t sender_type_len;
	uint64_t pk_sender_id = 0;
	uint64_t pk_user_id = 0;
	const char *stmtErrFunc = nullptr;
	const char *sender_type = nullptr;
	mysql::MySQLStmt *stmt = nullptr;

	pk_user_id = get_user_pk(db, ud);
	if (unlikely(!pk_user_id))
		return 0;

	stmt = db->prepare(1, "INSERT INTO `gt_senders` (type) VALUES (?)");

	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt)) {
		mysql_handle_prepare_err(db, stmt);
		return 0;
	}

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	switch (ud->sender_.get_id()) {
	case td_api::messageSenderChat::ID:
		sender_type = "messageSenderChat";
		sender_type_len = sizeof("messageSenderChat") - 1;
		break;
	case td_api::messageSenderUser::ID:
		sender_type = "messageSenderUser";
		sender_type_len = sizeof("messageSenderUser") - 1;
		break;
	default:
		pr_err("Invalid chat type on create_chat()");
		goto out;
	}
	stmt->bind(0, MYSQL_TYPE_STRING, (void *) sender_type, sender_type_len);

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_sender_id = stmt->getInsertId();
	goto out;

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_sender_id = 0;
out:
	delete stmt;

	if (pk_sender_id) {
		if (!create_sender_user(db, pk_sender_id, pk_user_id))
			pk_sender_id = 0;
	}
	return pk_sender_id;
}

static uint64_t create_sender_in_trx(mysql::MySQL *db, struct user_data *cd)
{
	int tmp;
	uint64_t pk_chat_id;

	tmp = db->beginTransaction();
	if (unlikely(tmp)) {
		pr_err("beginTransaction(): %s", db->getError());
		return 0;
	}

	pk_chat_id = create_sender(db, cd);
	if (unlikely(!pk_chat_id))
		goto rollback;

	tmp = db->commit();
	if (unlikely(tmp)) {
		pr_err("commit(): %s", db->getError());
		goto rollback;
	}

	/* Creation successful! */
	return pk_chat_id;

rollback:
	tmp = db->rollback();
	if (unlikely(tmp))
		pr_err("rollback(): %s", db->getError());

	return 0;
}

static uint64_t get_sender_pk(mysql::MySQL *db, struct user_data *ud)
{
	static const char q[] =
		"SELECT gt_senders.id FROM gt_senders INNER JOIN gt_sender_user "
		"ON gt_senders.id = gt_sender_user.sender_id INNER JOIN gt_users "
		"ON gt_users.id = gt_sender_user.user_id WHERE "
		"gt_users.tg_user_id = %" PRId64;

	int qlen, tmp;
	MYSQL_ROW row;
	uint64_t pk_chat_id;
	mysql::MySQLRes *res;
	char qbuf[sizeof(q) + 64];
	const td_api::user &user = *ud->user_;

	qlen = snprintf(qbuf, sizeof(qbuf), q, user.id_);

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
	if (unlikely(!row)) {
		pk_chat_id = create_sender_in_trx(db, ud);
		goto out;
	}

	pk_chat_id = strtoull(row[0], NULL, 10);
out:
	delete res;
	return pk_chat_id;
}

static bool get_user_data(KWorker *kworker, const td_api::MessageSender &sender,
			  struct user_data *ud)
{
	tgvisd::Td::Td *td;
	const uint32_t timeout = 60;
	const auto &tmp = static_cast<const td_api::messageSenderUser &>(sender);
	int64_t user_id = tmp.user_id_;

	td = kworker->getTd();
	assert(td);

	ud->user_ = td->send_query_sync<td_api::getUser, td_api::user>(
		td_api::make_object<td_api::getUser>(user_id),
		timeout
	);
	if (unlikely(!ud->user_))
		return false;

	ud->userFull_ = td->send_query_sync<td_api::getUserFullInfo, td_api::userFullInfo>(
		td_api::make_object<td_api::getUserFullInfo>(user_id),
		timeout
	);
	if (unlikely(!ud->userFull_))
		return false;

	return true;
}

uint64_t User::getPK(void)
{
	mysql::MySQL *db;
	struct user_data ud(sender_);

	if (unlikely(!get_user_data(kworker_, sender_, &ud))) {
		pr_err("Cannot get chat data on getPK");
		return 0;
	}

	db = getDbPool();
	if (unlikely(!db))
		return 0;

	return get_sender_pk(db, &ud);
}

} /* namespace tgvisd::Logger::Sender */
