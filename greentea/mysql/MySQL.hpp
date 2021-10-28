/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef mysql__MySQL__HPP
#define mysql__MySQL__HPP

#include <memory>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <mysql/mysql.h>

namespace mysql {

class MySQL;

class MySQLRes
{
private:
	MYSQL_RES *res_ = nullptr;
	MySQL *mysql_ = nullptr;

public:
	inline MySQLRes(MySQL *mysql, MYSQL_RES *res):
		res_(res),
		mysql_(mysql)
	{
	}


	inline ~MySQLRes(void)
	{
		if (res_)
			mysql_free_result(res_);
	}

	inline int numFields(void) noexcept
	{
		return mysql_num_fields(res_);
	}


	inline MYSQL_ROW fetchRow(void) noexcept
	{
		return mysql_fetch_row(res_);
	}
};


class MySQL
{
private:
	MYSQL *conn_ = nullptr;
	const char *host_ = nullptr;
	const char *user_ = nullptr;
	const char *passwd_ = nullptr;
	const char *dbname_ = nullptr;
	uint16_t port_ = 0;
	bool throw_err_ = true;
	char err_buf_[512];

	template<typename T>
	T err(T ret, const char *err, const char *sql_err);

	template<typename T>
	T err(T ret, const char *err);

public:
	MySQL(const char *host, const char *user, const char *passwd,
	      const char *dbname);
	bool connect(void);
	MySQLRes *storeResultRaw(void);
	std::unique_ptr<MySQLRes> storeResult(void);


	inline void setThrowErr(bool b)
	{
		throw_err_ = b;
	}


	inline MYSQL *getConn(void) noexcept
	{
		return conn_;
	}


	inline void setPort(uint16_t port) noexcept
	{
		port_ = port;
	}


	inline const char *error(void) noexcept
	{
		return mysql_error(conn_);
	}


	inline int query(const char *q) noexcept
	{
		return mysql_query(conn_, q);
	}


	inline int realQuery(const char *q, unsigned long len) noexcept
	{
		return mysql_real_query(conn_, q, len);
	}


	inline void close(void) noexcept
	{
		if (conn_)
			mysql_close(conn_);
	}


	inline ~MySQL(void) noexcept
	{
		this->close();
	}
};

} /* namespace mysql */

#endif /* #ifndef mysql__MySQL__HPP */
