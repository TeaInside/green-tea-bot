/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include "MySQL.hpp"
#include <cstring>
#include <stdexcept>


namespace mysql {


MySQL::MySQL(const char *host, const char *user, const char *passwd,
	     const char *dbname):
	host_(host),
	user_(user),
	passwd_(passwd),
	dbname_(dbname)
{
	conn_ = mysql_init(NULL);
	if (unlikely(!conn_))
		throw std::runtime_error("Cannot init mysql on mysql_init()");
}


bool MySQL::connect(void) noexcept
{
	MYSQL *ret;

	ret = mysql_real_connect(conn_, host_, user_, passwd_, dbname_,
			 	 (unsigned int) port_, NULL, 0);
	if (unlikely(!ret))
		return false;

	/*
	 * For a successful connection, the return value is the
	 * same as the value of the first parameter.
	 */
	if (unlikely(ret != conn_)) {
		puts("Bug on mysql_real_connect()!");
		abort();
		__builtin_unreachable();
	}

	return true;
}


__hot MySQLRes *MySQL::storeResult(void) noexcept
{
	MySQLRes *ret;
	MYSQL_RES *result;

	result = mysql_store_result(conn_);
	if (unlikely(!result))
		return nullptr;

	try {
		ret = new MySQLRes(result);
	} catch (const std::bad_alloc &) {
		mysql_free_result(result);
		ret = MYSQL_ERR_PTR<MySQLRes>(-ENOMEM);
	}

	return ret;
}


__hot MySQLStmt *MySQL::prepare(size_t bind_num, const char *q) noexcept
{
	return prepareLen(bind_num, q, strlen(q));
}


__attribute__((noinline))
__hot MySQLStmt *MySQL::prepareLen(size_t bind_num, const char *q, size_t qlen) noexcept
{
	MySQLStmt *ret;
	MYSQL_STMT *stmt = nullptr;
	MYSQL_BIND *bind = nullptr;

	stmt = mysql_stmt_init(conn_);
	if (unlikely(!stmt))
		return nullptr;

	bind = (MYSQL_BIND *)calloc(bind_num, sizeof(*bind));
	if (unlikely(!bind)) {
		ret = MYSQL_ERR_PTR<MySQLStmt>(-ENOMEM);
		goto err;
	}

	try {
		ret = new MySQLStmt(stmt, bind, q, qlen);
	} catch (const std::bad_alloc &e) {
		ret = MYSQL_ERR_PTR<MySQLStmt>(-ENOMEM);
		goto err;
	}

	return ret;
err:
	if (bind)
		free(bind);

	mysql_stmt_close(stmt);
	return ret;
}


MySQLStmt::~MySQLStmt(void) noexcept
{
	if (bind_) {
		free(bind_);
		bind_ = nullptr;
	}

	if (stmt_) {
		mysql_stmt_close(stmt_);
		stmt_ = nullptr;
	}
}


__hot MySQLStmtRes *MySQLStmt::storeResult(size_t bind_res_num) noexcept
{
	MySQLStmtRes *ret;
	MYSQL_RES *result;
	MYSQL_BIND *bind = nullptr;

	result = mysql_stmt_result_metadata(stmt_);
	if (unlikely(!result))
		return nullptr;

	if (((size_t)mysql_num_fields(result)) != bind_res_num) {
		ret = MYSQL_ERR_PTR<MySQLStmtRes>(-EINVAL);
		goto err;
	}

	bind = (MYSQL_BIND *)calloc(bind_res_num, sizeof(*bind));
	if (unlikely(!bind)) {
		ret = MYSQL_ERR_PTR<MySQLStmtRes>(-ENOMEM);
		goto err;
	}

	try {
		ret = new MySQLStmtRes(result, stmt_, bind);
	} catch (const std::bad_alloc &) {
		ret = MYSQL_ERR_PTR<MySQLStmtRes>(-ENOMEM);
		goto err;
	}

	return ret;
err:
	if (bind)
		free(bind);

	mysql_free_result(result);
	return ret;
}


MySQLStmtRes::~MySQLStmtRes(void) noexcept
{
	if (bind_) {
		free(bind_);
		bind_ = nullptr;
	}

	if (res_) {
		mysql_free_result(res_);
		res_ = nullptr;
	}
}


} /* namespace mysql */
