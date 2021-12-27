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

struct chat_data {
	const td_api::chat				&chat_;
	td_api::object_ptr<td_api::supergroup>		sgroup_;
	td_api::object_ptr<td_api::supergroupFullInfo>	sgroup_full_;

	inline ~chat_data(void) = default;

	inline chat_data(const td_api::chat &chat):
		chat_(chat)
	{
	}
};

static inline void *bn_str(bool p)
{
	return (void *) (p ? "1" : "0");
}

static uint64_t create_group_history(mysql::MySQL *db, struct chat_data *cd,
				     uint64_t pk_group_id)
{
	uint64_t pk_id;
	const char *stmtErrFunc = nullptr;
	mysql::MySQLStmt *stmt = nullptr;
	const td_api::chat &chat = cd->chat_;
	const td_api::supergroup &sgroup = *cd->sgroup_;
	const td_api::supergroupFullInfo &sgroup_full = *cd->sgroup_full_;

	stmt = db->prepare(9,
		"INSERT INTO `gt_groups_history` "
		"("
			"`group_id`,"
			"`username`,"
			"`link`,"
			"`name`,"
			"`description`,"
			"`has_linked_chat`,"
			"`is_slow_mode_enabled`,"
			"`is_channel`,"
			"`is_verified`,"
			"`created_at`"
		")"
			" VALUES "
		"("
			"?,"		/* group_id */
			"?,"		/* username */
			"?,"		/* link */
			"?,"		/* name */
			"?,"		/* description */
			"?,"		/* has_linked_chat */
			"?,"		/* is_slow_mode_enabled */
			"?,"		/* is_channel */
			"?,"		/* is_verified */
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

	stmt->bind(0, MYSQL_TYPE_LONGLONG, (void *) &pk_group_id,
		   sizeof(pk_group_id));

	if (!sgroup.username_.size())
		stmt->bind(1, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(1, MYSQL_TYPE_STRING,
			   (void *) sgroup.username_.c_str(),
			   sgroup.username_.size());

	if (!sgroup_full.invite_link_ ||
	    !sgroup_full.invite_link_->invite_link_.size())
		stmt->bind(2, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(2, MYSQL_TYPE_STRING,
			   (void *) sgroup_full.invite_link_->invite_link_.c_str(),
			   sgroup_full.invite_link_->invite_link_.size());

	stmt->bind(3, MYSQL_TYPE_STRING, (void *) chat.title_.c_str(),
		   chat.title_.size());

	if (!sgroup_full.description_.size())
		stmt->bind(4, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(4, MYSQL_TYPE_STRING,
			   (void *) sgroup_full.description_.c_str(),
			   sgroup_full.description_.size());

	stmt->bind(5, MYSQL_TYPE_STRING, bn_str(sgroup.has_linked_chat_), 1);
	stmt->bind(6, MYSQL_TYPE_STRING, bn_str(sgroup.is_slow_mode_enabled_), 1);
	stmt->bind(7, MYSQL_TYPE_STRING, bn_str(sgroup.is_channel_), 1);
	stmt->bind(8, MYSQL_TYPE_STRING, bn_str(sgroup.is_verified_), 1);

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

static uint64_t create_group(mysql::MySQL *db, struct chat_data *cd)
{
	uint64_t pk_group_id;
	const char *stmtErrFunc = nullptr;
	mysql::MySQLStmt *stmt = nullptr;
	const td_api::chat &chat = cd->chat_;
	const td_api::supergroup &sgroup = *cd->sgroup_;
	const td_api::supergroupFullInfo &sgroup_full = *cd->sgroup_full_;

	stmt = db->prepare(9,
		"INSERT INTO `gt_groups` "
		"("
			"`tg_group_id`,"
			"`username`,"
			"`link`,"
			"`name`,"
			"`description`,"
			"`has_linked_chat`,"
			"`is_slow_mode_enabled`,"
			"`is_channel`,"
			"`is_verified`,"
			"`created_at`,"
			"`updated_at`"
		")"
			" VALUES "
		"("
			"?,"		/* tg_group_id */
			"?,"		/* username */
			"?,"		/* link */
			"?,"		/* name */
			"?,"		/* description */
			"?,"		/* has_linked_chat */
			"?,"		/* is_slow_mode_enabled */
			"?,"		/* is_channel */
			"?,"		/* is_verified */
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

	stmt->bind(0, MYSQL_TYPE_LONGLONG, (void *) &chat.id_, sizeof(chat.id_));

	if (!sgroup.username_.size())
		stmt->bind(1, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(1, MYSQL_TYPE_STRING,
			   (void *) sgroup.username_.c_str(),
			   sgroup.username_.size());

	if (!sgroup_full.invite_link_ ||
	    !sgroup_full.invite_link_->invite_link_.size())
		stmt->bind(2, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(2, MYSQL_TYPE_STRING,
			   (void *) sgroup_full.invite_link_->invite_link_.c_str(),
			   sgroup_full.invite_link_->invite_link_.size());

	stmt->bind(3, MYSQL_TYPE_STRING, (void *) chat.title_.c_str(),
		   chat.title_.size());

	if (!sgroup_full.description_.size())
		stmt->bind(4, MYSQL_TYPE_NULL, NULL, 0);
	else
		stmt->bind(4, MYSQL_TYPE_STRING,
			   (void *) sgroup_full.description_.c_str(),
			   sgroup_full.description_.size());

	stmt->bind(5, MYSQL_TYPE_STRING, bn_str(sgroup.has_linked_chat_), 1);
	stmt->bind(6, MYSQL_TYPE_STRING, bn_str(sgroup.is_slow_mode_enabled_), 1);
	stmt->bind(7, MYSQL_TYPE_STRING, bn_str(sgroup.is_channel_), 1);
	stmt->bind(8, MYSQL_TYPE_STRING, bn_str(sgroup.is_verified_), 1);

	if (unlikely(stmt->bindStmt())) {
		stmtErrFunc = "bindStmt";
		goto stmt_err;
	}

	if (unlikely(stmt->execute())) {
		stmtErrFunc = "execute";
		goto stmt_err;
	}

	pk_group_id = stmt->getInsertId();
	if (!create_group_history(db, cd, pk_group_id))
		pk_group_id = 0;

	goto out;

stmt_err:
	mysql_handle_stmt_err(stmtErrFunc, stmt);
	pk_group_id = 0;
out:
	delete stmt;
	return pk_group_id;
}

static uint64_t get_group_pk(mysql::MySQL *db, struct chat_data *cd)
{
	static const char q[] =
		"SELECT id FROM gt_groups WHERE tg_group_id = %" PRId64;

	uint64_t ret;
	int qlen, tmp;
	MYSQL_ROW row;
	mysql::MySQLRes *res;
	char qbuf[sizeof(q) + 64];

	qlen = snprintf(qbuf, sizeof(qbuf), q, cd->chat_.id_);

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
		ret = create_group(db, cd);
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

static uint64_t create_chat(mysql::MySQL *db, struct chat_data *cd)
{
	size_t chat_type_len;
	uint64_t pk_chat_id = 0;
	uint64_t pk_group_id = 0;
	const char *stmtErrFunc = nullptr;
	const char *chat_type = nullptr;
	mysql::MySQLStmt *stmt = nullptr;

	pk_group_id = get_group_pk(db, cd);
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

	switch (cd->chat_.type_->get_id()) {
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

static uint64_t create_chat_in_trx(mysql::MySQL *db, struct chat_data *cd)
{
	int tmp;
	uint64_t pk_chat_id;

	tmp = db->beginTransaction();
	if (unlikely(tmp)) {
		pr_err("beginTransaction(): %s", db->getError());
		return 0;
	}

	pk_chat_id = create_chat(db, cd);
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

static uint64_t get_chat_pk(mysql::MySQL *db, struct chat_data *cd)
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
	const td_api::chat &chat = cd->chat_;

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
		pk_chat_id = create_chat_in_trx(db, cd);
		goto out;
	}

	pk_chat_id = strtoull(row[0], NULL, 10);
out:
	delete res;
	return pk_chat_id;
}

static bool get_chat_data(KWorker *kworker, const td_api::chat &chat,
			  struct chat_data *cd)
{
	tgvisd::Td::Td *td;
	const uint32_t timeout = 60;

	const auto &tmp = static_cast<const td_api::chatTypeSupergroup &>(*chat.type_);
	int32_t supergroup_id = tmp.supergroup_id_;

	td = kworker->getTd();
	assert(td);

	cd->sgroup_ = td->send_query_sync<td_api::getSupergroup, td_api::supergroup>(
		td_api::make_object<td_api::getSupergroup>(supergroup_id),
		timeout
	);
	if (unlikely(!cd->sgroup_))
		return false;

	cd->sgroup_full_ = td->send_query_sync<td_api::getSupergroupFullInfo, td_api::supergroupFullInfo>(
		td_api::make_object<td_api::getSupergroupFullInfo>(supergroup_id),
		timeout
	);
	if (unlikely(!cd->sgroup_full_))
		return false;

	return true;
}

uint64_t Group::getPK(void)
{
	mysql::MySQL *db;
	struct chat_data cd(chat_);

	if (unlikely(!get_chat_data(kworker_, chat_, &cd))) {
		pr_err("Cannot get chat data on getPK");
		return 0;
	}

	db = getDbPool();
	if (unlikely(!db))
		return 0;

	return get_chat_pk(db, &cd);
}

} /* namespace tgvisd::Logger::Chat */
