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
	if (unlikely(!result)) {
		printf("err: %s\n", mysql_error(conn_));
		return nullptr;
	}

	try {
		ret = new MySQLRes(result);
	} catch (const std::bad_alloc &) {
		return MYSQL_ERR_PTR<MySQLRes>(-ENOMEM);
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
	int tmp;
	MySQLStmt *ret;
	MYSQL_STMT *stmt = nullptr;
	MYSQL_BIND *bind = nullptr;

	stmt = mysql_stmt_init(conn_);
	if (unlikely(!stmt))
		return MYSQL_ERR_PTR<MySQLStmt>(-ENOMEM);


	tmp = mysql_stmt_prepare(stmt, q, qlen);
	if (unlikely(tmp)) {
		ret = nullptr;
		goto err;
	}


	bind = (MYSQL_BIND *)calloc(bind_num, sizeof(*bind));
	if (unlikely(!bind)) {
		ret = MYSQL_ERR_PTR<MySQLStmt>(-ENOMEM);
		goto err;
	}


	try {
		ret = new MySQLStmt(stmt, bind, bind_num);
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


} /* namespace mysql */
