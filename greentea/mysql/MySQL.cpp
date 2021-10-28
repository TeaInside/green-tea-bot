/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include "MySQL.hpp"

#define ERR_BUF 512

namespace mysql {

template<typename T>
T MySQL::err(T ret, const char *msg, const char *sql_msg)
{
	size_t l;

	l = (size_t)snprintf(err_buf_, err_buf_size, "%s", msg);

	if (sql_msg)
		snprintf(err_buf_ + l, err_buf_size - l, ": %s", sql_msg);

	if (throw_err_)
		throw std::runtime_error(std::string(err_buf_));

	return ret;
}


template<typename T>
T MySQL::err(T ret, const char *msg)
{
	return err(ret, msg, nullptr);
}


MySQL::MySQL(const char *host, const char *user, const char *passwd,
	     const char *dbname):
	host_(host),
	user_(user),
	passwd_(passwd),
	dbname_(dbname)
{

	err_buf_ = new char[err_buf_size];
	if (!err_buf_)
		return; /* We can't call err! */

	conn_ = mysql_init(NULL);
	if (!conn_)
		err(0, "Cannot init mysql on mysql_init()");
}


bool MySQL::connect(void)
{
	MYSQL *ret;

	ret = mysql_real_connect(conn_, host_, user_, passwd_, dbname_,
				 (unsigned int)port_, NULL, 0);
	if (!ret)
		return err(false, "Cannot connect on mysql_real_connect()",
			   mysql_error(conn_));

	/*
	 * For a successful connection, the return value is the
	 * same as the value of the first parameter.
	 */
	if (ret != conn_) {
		mysql_close(ret);
		return err(false, "Bug on MySQL::connect()");
	}

	return true;
}


MySQLRes *MySQL::storeResultRaw(void)
{
	MYSQL_RES *res;

	res = mysql_store_result(conn_);
	if (!res)
		return err(nullptr, "Error on mysql_store_result()",
			   mysql_error(conn_));

	return new MySQLRes(this, res);
}


std::unique_ptr<MySQLRes> MySQL::storeResult(void)
{
	MYSQL_RES *res;

	res = mysql_store_result(conn_);
	if (!res)
		return err(nullptr, "Error on mysql_store_result()",
			   mysql_error(conn_));

	return std::make_unique<MySQLRes>(this, res);
}


} /* namespace mysql */
