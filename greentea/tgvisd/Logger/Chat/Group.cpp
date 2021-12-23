// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 * @license GPL-2.0-only
 * @package tgvisd::Logger::Chat
 *
 * Copyright (C) 2021  Alviro Iskandar Setiawan <alviro.iskandar@gmail.com>
 */

#include <inttypes.h>
#include <tgvisd/mysql_helpers.hpp>
#include <tgvisd/Logger/Chat/Group.hpp>

namespace tgvisd::Logger::Chat {

static uint64_t create_group(mysql::MySQL *db, const td_api::chat &chat)
{
	uint64_t pk_id;
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

	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt)) {
		mysql_handle_prepare_err(db, stmt);
		return 0;
	}

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	stmt->bind(0, MYSQL_TYPE_LONGLONG, (void *)&chat.id_, sizeof(chat.id_));
	stmt->bind(1, MYSQL_TYPE_STRING, (void *)chat.title_.c_str(),
		   chat.title_.size());

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

static uint64_t get_group_pk(mysql::MySQL *db, const td_api::chat &chat)
{
	static const char q[] =
		"SELECT id FROM gt_groups WHERE tg_group_id = %" PRId64;

	uint64_t ret;
	int qlen, tmp;
	MYSQL_ROW row;
	mysql::MySQLRes *res;
	char qbuf[sizeof(q) + 64];

	qlen = snprintf(qbuf, sizeof(qbuf), q, chat.id_);

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
		ret = create_group(db, chat);
		goto out;
	}

	ret = strtoull(row[0], NULL, 10);
out:
	delete res;
	return ret;
}

static uint64_t create_chat_group(mysql::MySQL *db, uint64_t pk_chat_id,
				  uint64_t pk_group_id)
{
	static const char q[] =
		"INSERT INTO `gt_chat_group` (group_id, chat_id) VALUES "
		"(%" PRId64", %" PRId64 ")";

	int qlen, tmp;
	char qbuf[sizeof(q) + 64];

	qlen = snprintf(qbuf, sizeof(qbuf), q, pk_group_id, pk_chat_id);

	tmp = db->realQuery(qbuf, (size_t) qlen);
	if (unlikely(tmp)) {
		pr_err("query(): %s", db->getError());
		return 0;
	}

	return 1;
}

static uint64_t create_chat(mysql::MySQL *db, const td_api::chat &chat)
{
	size_t chat_type_len;
	uint64_t pk_chat_id = 0;
	uint64_t pk_group_id = 0;
	const char *stmtErrFunc = nullptr;
	const char *chat_type = nullptr;
	mysql::MySQLStmt *stmt = nullptr;

	pk_group_id = get_group_pk(db, chat);
	if (unlikely(!pk_group_id))
		return 0;

	stmt = db->prepare(1, "INSERT INTO `gt_chats` (type) VALUES (?)");

	if (MYSQL_IS_ERR_OR_NULL<mysql::MySQLStmt>(stmt)) {
		mysql_handle_prepare_err(db, stmt);
		return 0;
	}

	if (unlikely(stmt->stmtInit())) {
		stmtErrFunc = "stmtInit";
		goto stmt_err;
	}

	switch (chat.type_->get_id()) {
	case td_api::chatTypeBasicGroup::ID:
		chat_type = "chatTypeBasicGroup";
		chat_type_len = sizeof("chatTypeBasicGroup") - 1;
		break;
	case td_api::chatTypeSupergroup::ID:
		chat_type = "chatTypeSupergroup";
		chat_type_len = sizeof("chatTypeSupergroup") - 1;
		break;
	case td_api::chatTypeSecret::ID:
		chat_type = "chatTypeSecret";
		chat_type_len = sizeof("chatTypeSecret") - 1;
		break;
	case td_api::chatTypePrivate::ID:
		chat_type = "chatTypePrivate";
		chat_type_len = sizeof("chatTypePrivate") - 1;
		break;
	default:
		pr_err("Invalid chat type on create_chat()");
		goto out;
	}
	stmt->bind(0, MYSQL_TYPE_STRING, (void *)chat_type, chat_type_len);

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_chat_id = stmt->getInsertId();
	goto out;

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_chat_id = 0;
out:
	delete stmt;

	if (pk_chat_id) {
		if (!create_chat_group(db, pk_chat_id, pk_group_id))
			pk_chat_id = 0;
	}
	return pk_chat_id;
}

static uint64_t create_chat_in_trx(mysql::MySQL *db, const td_api::chat &chat)
{
	int tmp;
	uint64_t pk_chat_id;

	tmp = db->realQuery(ZSTRL("START TRANSACTION"));
	if (unlikely(tmp)) {
		pr_err("realQuery(\"START TRANSACTION\"): %s", db->getError());
		return 0;
	}

	pk_chat_id = create_chat(db, chat);
	if (unlikely(!pk_chat_id))
		goto rollback;

	tmp = db->realQuery(ZSTRL("COMMIT"));
	if (unlikely(tmp)) {
		pr_err("realQuery(\"COMMIT\"): %s", db->getError());
		goto rollback;
	}

	/* Creation successful! */
	return pk_chat_id;

rollback:
	tmp = db->realQuery(ZSTRL("ROLLBACK"));
	if (unlikely(tmp))
		pr_err("realQuery(\"ROLLBACK\"): %s", db->getError());

	return 0;
}

static uint64_t get_chat_pk(mysql::MySQL *db, const td_api::chat &chat)
{
	static const char q[] =
		"SELECT gt_chats.id FROM gt_chats INNER JOIN gt_chat_group "
		"ON gt_chats.id = gt_chat_group.chat_id INNER JOIN gt_groups "
		"ON gt_groups.id = gt_chat_group.group_id WHERE "
		"gt_groups.tg_group_id = %" PRId64;

	int qlen, tmp;
	MYSQL_ROW row;
	uint64_t pk_chat_id;
	mysql::MySQLRes *res;
	char qbuf[sizeof(q) + 64];

	qlen = snprintf(qbuf, sizeof(qbuf), q, chat.id_);

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
		pk_chat_id = create_chat_in_trx(db, chat);
		goto out;
	}

	pk_chat_id = strtoull(row[0], NULL, 10);
out:
	delete res;
	return pk_chat_id;
}

uint64_t Group::getPK(void)
{
	mysql::MySQL *db;

	db = getDbPool();
	if (unlikely(!db))
		return 0;

	return get_chat_pk(db, chat_);
}

} /* namespace tgvisd::Logger::Chat */
