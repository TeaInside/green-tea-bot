/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include "MySQL.hpp"

#include <cstring>

namespace mysql {


__cold void MySQL::err(const char *msg, const char *sql_msg)
{
	size_t l;

	l = (size_t)snprintf(err_buf_, err_buf_size, "%s", msg);
	if (sql_msg)
		snprintf(err_buf_ + l, err_buf_size - l, ": %s", sql_msg);

	if (throw_err_)
		throw std::runtime_error(std::string(err_buf_));
}


MySQL::MySQL(const char *host, const char *user, const char *passwd,
	     const char *dbname):
	host_(host),
	user_(user),
	passwd_(passwd),
	dbname_(dbname)
{
	err_buf_ = (char *)calloc(err_buf_size, sizeof(char));
	if (unlikely(!err_buf_)) {
		throw std::bad_alloc();
		return;
	}

	conn_ = mysql_init(NULL);
	if (unlikely(!conn_))
		throw std::runtime_error("Cannot init mysql on mysql_init()");
}


bool MySQL::connect(void)
{
	MYSQL *ret;

	ret = mysql_real_connect(conn_, host_, user_, passwd_, dbname_,
				 (unsigned int)port_, NULL, 0);
	if (unlikely(!ret)) {
		err("Cannot connect on mysql_real_connect()",
		    mysql_error(conn_));
		return false;
	}

	/*
	 * For a successful connection, the return value is the
	 * same as the value of the first parameter.
	 */
	if (unlikely(ret != conn_)) {
		mysql_close(ret);
		err("Bug on MySQL::connect()");
		return false;
	}

	return true;
}


MySQLRes *MySQL::storeResultRaw(void)
{
	MYSQL_RES *res;

	res = mysql_store_result(conn_);
	if (unlikely(!res)) {
		err("Error on mysql_store_result()", mysql_error(conn_));
		return nullptr;
	}

	return new MySQLRes(res);
}


std::unique_ptr<MySQLRes> MySQL::storeResult(void)
{
	MYSQL_RES *res;

	res = mysql_store_result(conn_);
	if (unlikely(!res)) {
		err("Error on mysql_store_result()", mysql_error(conn_));
		return nullptr;
	}

	return std::make_unique<MySQLRes>(res);
}


__hot __attribute__((noinline))
MYSQL_STMT *MySQL::createStmt(size_t bindValNum, const char *q, size_t qlen,
			      MYSQL_BIND **bind_p)
{
	int err_ret;
	MYSQL_STMT *stmt = nullptr;
	MYSQL_BIND *bind = nullptr;
	const char *err_msg, *err_sql;

	stmt = mysql_stmt_init(conn_);
	if (unlikely(!stmt)) {
		err_msg = "Error on mysql_stmt_init()";
		err_sql = "ENOMEM"; 
		goto err;
	}

	err_ret = mysql_stmt_prepare(stmt, q, qlen);
	if (unlikely(err_ret)) { 
		err_msg = "Error on mysql_stmt_prepare()";
		err_sql = mysql_stmt_error(stmt); 
		goto err;
	}

	bind = (MYSQL_BIND *)calloc(bindValNum, sizeof(*bind));
	if (unlikely(!bind)) {
		err_msg = "calloc() ENOMEM";
		err_sql = NULL;
		goto err;
	}

	*bind_p = bind;
	return stmt;

err:
	if (stmt)
		mysql_stmt_close(stmt);

	if (bind)
		free(bind);

	err(err_msg, err_sql);
	return nullptr;
}


__hot __attribute__((noinline))
MySQLStmt *MySQL::prepareLenRaw(size_t bindValNum, const char *q, size_t qlen)
{
	MySQLStmt *ret;
	MYSQL_STMT *stmt;
	MYSQL_BIND *bind;

	stmt = createStmt(bindValNum, q, qlen, &bind);
	if (unlikely(!stmt))
		return nullptr;

	try {
		ret = new MySQLStmt(bindValNum, stmt, bind, this);
	} catch (const std::bad_alloc &e) {
		mysql_stmt_close(stmt);
		free(bind);
		err("new ENOMEM on prepareLenRaw()");
		return nullptr;
	}

	return ret;
}


__hot MySQLStmt *MySQL::prepareRaw(size_t bindValNum, const char *q)
{
	return prepareLenRaw(bindValNum, q, strlen(q));
}


MySQLStmt::MySQLStmt(size_t bindValNum, MYSQL_STMT *stmt, MYSQL_BIND *bind,
		     MySQL *mysql):
	stmt_(stmt),
	bind_(bind),
	mysql_(mysql),
	bindValNum_(bindValNum)
{
}


} /* namespace mysql */
