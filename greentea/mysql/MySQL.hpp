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

#define likely(EXPR)	__builtin_expect((bool)(EXPR), 1)
#define unlikely(EXPR)	__builtin_expect((bool)(EXPR), 0)

namespace mysql {

class MySQL;

class MySQLRes
{
private:
	MYSQL_RES *res_ = nullptr;

public:
	inline MySQLRes(MYSQL_RES *res) noexcept:
		res_(res)
	{
	}


	inline ~MySQLRes(void) noexcept
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
	char *err_buf_ = nullptr;
	bool throw_err_ = true;

	static constexpr size_t err_buf_size = 8192;

	inline void err(const char *msg)
	{
		err(msg, nullptr);
	}
public:
	MySQL(const char *host, const char *user, const char *passwd,
	      const char *dbname);
	bool connect(void);
	MySQLRes *storeResultRaw(void);
	std::unique_ptr<MySQLRes> storeResult(void);
	void err(const char *msg, const char *sql_msg);


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


	inline int ping(void)
	{
		return mysql_ping(conn_);
	}


	inline ~MySQL(void) noexcept
	{
		this->close();
		if (err_buf_)
			delete[] err_buf_;
	}
};

} /* namespace mysql */

#endif /* #ifndef mysql__MySQL__HPP */
