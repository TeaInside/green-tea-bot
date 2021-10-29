/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef mysql__MySQL__HPP
#define mysql__MySQL__HPP

#include <memory>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#include <mysql/mysql.h>

#define likely(EXPR)	__builtin_expect((bool)(EXPR), 1)
#define unlikely(EXPR)	__builtin_expect((bool)(EXPR), 0)
#define __hot		__attribute__((__hot__))
#define __cold		__attribute__((__cold__))

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


class MySQLStmt
{
private:
	MYSQL_STMT *stmt_;
	MYSQL_BIND *bind_;
	MySQL *mysql_;
	size_t bindValNum_;
	bool isBound = false;
public:
	MySQLStmt(size_t bindValNum, MYSQL_STMT *stmt, MYSQL_BIND *bind,
		  MySQL *mysql);

	~MySQLStmt(void);

	inline MYSQL_BIND *bind(size_t i, enum enum_field_types type, void *buf,
				size_t buflen, bool *is_null, size_t *len) noexcept
	{
		MYSQL_BIND *b = &bind_[i];

		b->buffer_type   = type;
		b->buffer        = buf;
		b->buffer_length = buflen;
		b->is_null       = is_null;
		b->length        = len;
		return b;
	}


	inline int mergeBind(void) noexcept
	{
		return mysql_stmt_bind_param(stmt_, bind_);
	}


	inline int execute(void) noexcept
	{
		if (!isBound) {
			isBound = true;
			mergeBind();
		}

		return mysql_stmt_execute(stmt_);
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
	MYSQL_STMT *createStmt(size_t bindValNum, const char *q, size_t qlen,
			       MYSQL_BIND **bind_p);

public:
	MySQL(const char *host, const char *user, const char *passwd,
	      const char *dbname);
	bool connect(void);
	MySQLRes *storeResultRaw(void);
	std::unique_ptr<MySQLRes> storeResult(void);
	void err(const char *msg, const char *sql_msg);
	MySQLStmt *prepareRaw(size_t bindValNum, const char *q);
	MySQLStmt *prepareLenRaw(size_t bindValNum, const char *q, size_t qlen);
	std::unique_ptr<MySQLStmt> prepare(size_t bindValNum, const char *q);
	std::unique_ptr<MySQLStmt> prepareLen(size_t bindValNum, const char *q);

	inline void err(const char *msg)
	{
		err(msg, nullptr);
	}


	inline bool getThrowErr(void) noexcept
	{
		return throw_err_;
	}


	inline void setThrowErr(bool b) noexcept
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
		if (likely(conn_))
			mysql_close(conn_);
	}


	inline int ping(void) noexcept
	{
		return mysql_ping(conn_);
	}


	inline ~MySQL(void) noexcept
	{
		this->close();
		if (likely(err_buf_))
			free(err_buf_);
	}
};

} /* namespace mysql */

#endif /* #ifndef mysql__MySQL__HPP */
