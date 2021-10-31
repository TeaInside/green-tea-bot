/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef mysql__MySQL__HPP
#define mysql__MySQL__HPP

#include <errno.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <mysql/mysql.h>

#ifndef likely
#define likely(EXPR)	__builtin_expect((bool)(EXPR), 1)
#endif

#ifndef unlikely
#define unlikely(EXPR)	__builtin_expect((bool)(EXPR), 0)
#endif

#ifndef __hot
#define __hot		__attribute__((__hot__))
#endif

#ifndef __cold
#define __cold		__attribute__((__cold__))
#endif


template<typename T>
static inline intptr_t MYSQL_PTR_ERR(const T *ptr)
{
	return (intptr_t) ptr;
}


template<typename T>
static inline T *MYSQL_ERR_PTR(intptr_t err)
{
	return (T *) err;
}


template<typename T>
static inline bool MYSQL_IS_ERR(const T *ptr)
{
	return unlikely((uintptr_t) ptr >= (uintptr_t) -4095UL);
}


template<typename T>
static inline bool MYSQL_IS_ERR_OR_NULL(const T *ptr)
{
	return MYSQL_IS_ERR(ptr) || unlikely(!ptr);
}


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
		if (res_) {
			mysql_free_result(res_);
			res_ = nullptr;
		}
	}


	inline int numFields(void) noexcept
	{
		return mysql_num_fields(res_);
	}


	inline MYSQL_ROW fetchRow(void) noexcept
	{
		return mysql_fetch_row(res_);
	}


	inline MYSQL_RES *getRes(void) noexcept
	{
		return res_;
	}
};


class MySQLStmtRes
{
private:
	MYSQL_RES *res_ = nullptr;
	MYSQL_STMT *stmt_ = nullptr;
	MYSQL_BIND *bind_ = nullptr;

public:
	~MySQLStmtRes(void) noexcept;


	inline MySQLStmtRes(MYSQL_RES *res, MYSQL_STMT *stmt,
			    MYSQL_BIND *bind) noexcept:
		res_(res),
		stmt_(stmt),
		bind_(bind)
	{
	}


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


	inline int storeResult(void) noexcept
	{
		return mysql_stmt_store_result(stmt_);
	}


	inline int bindResult(void) noexcept
	{
		return mysql_stmt_bind_result(stmt_, bind_);
	}


	inline int numFields(void) noexcept
	{
		return mysql_num_fields(res_);
	}


	inline int fetchRow(void) noexcept
	{
		return mysql_stmt_fetch(stmt_);
	}


	inline MYSQL_RES *getRes(void) noexcept
	{
		return res_;
	}
};


class MySQLStmt
{
private:
	MYSQL_STMT *stmt_ = nullptr;
	MYSQL_BIND *bind_ = nullptr;
	const char *q_ = nullptr;
	size_t qlen_ = 0;

public:
	~MySQLStmt(void) noexcept;
	MySQLStmtRes *storeResult(size_t bind_res_num) noexcept;

	inline MySQLStmt(MYSQL_STMT *stmt, MYSQL_BIND *bind, const char *q,
			 size_t qlen) noexcept:
		stmt_(stmt),
		bind_(bind),
		q_(q),
		qlen_(qlen)
	{
	}


	inline const char *getError(void) noexcept
	{
		return mysql_stmt_error(stmt_);
	}


	inline int getErrno(void) noexcept
	{
		return mysql_stmt_errno(stmt_);
	}


	inline int stmtInit(void) noexcept
	{
		return mysql_stmt_prepare(stmt_, q_, qlen_);
	}


	inline MYSQL_BIND *bind(size_t i, enum enum_field_types type, void *buf,
				size_t buflen) noexcept
	{
		MYSQL_BIND *b = &bind_[i];

		b->buffer_type   = type;
		b->buffer        = buf;
		b->buffer_length = buflen;
		return b;
	}


	inline int bindStmt(void) noexcept
	{
		return mysql_stmt_bind_param(stmt_, bind_);
	}


	inline int execute(void) noexcept
	{
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

public:

	MySQL(const char *host, const char *user, const char *passwd,
	      const char *dbname);

	bool connect(void) noexcept;
	MySQLRes *storeResult(void) noexcept;

	MySQLStmt *prepare(size_t bind_num, const char *q) noexcept;
	MySQLStmt *prepareLen(size_t bind_num, const char *q, size_t qlen) noexcept;


	__hot inline int query(const char *q) noexcept
	{
		return mysql_query(conn_, q);
	}


	__hot inline int realQuery(const char *q, unsigned long len) noexcept
	{
		return mysql_real_query(conn_, q, len);
	}


	inline void close(void) noexcept
	{
		if (likely(conn_)) {
			mysql_close(conn_);
			conn_ = nullptr;
		}
	}


	inline int ping(void) noexcept
	{
		return mysql_ping(conn_);
	}


	inline ~MySQL(void) noexcept
	{
		this->close();
	}


	inline void setPort(uint16_t port) noexcept
	{
		port_ = port;
	}


	inline MYSQL *getConn(void) noexcept
	{
		return conn_;
	}


	__cold inline const char *getError(void) noexcept
	{
		const char *ret;

		ret = mysql_error(conn_);
		if (ret && ret[0] == '\0')
			ret = nullptr;

		return ret;
	}


	__cold inline unsigned int getErrno(void) noexcept
	{
		return mysql_errno(conn_);
	}
};


} /* namespace mysql */

#endif /* #ifndef mysql__MySQL__HPP */
