// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd::Td
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef TGVISD__TD__TD_HPP
#define TGVISD__TD__TD_HPP

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

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#if defined(__linux__)
	#include <unistd.h>
	#include <sys/types.h>
#else
	#define gettid() -1
#endif

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <mutex>
#include <chrono>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <condition_variable>

#include <tgvisd/print.h>

#include "Callback.hpp"

namespace td_api = td::td_api;
using Object = td_api::object_ptr<td_api::Object>;

namespace tgvisd::Td {

using std::mutex;
using std::atomic;
using std::string;
using std::function;
using std::unique_ptr;
using std::unique_lock;
using std::unordered_map;
using std::condition_variable;
using namespace std::chrono_literals;


extern volatile bool cancel_delayed_work;


class Td
{
private:
	uint32_t api_id_ = 0;
	const char *api_hash_ = nullptr;
	const char *data_path_ = nullptr;

	int64_t user_id_ = 0;
	int64_t client_id_ = 0;
	Callback callback;
	unique_ptr<td::ClientManager> client_manager_;
	td_api::object_ptr<td_api::AuthorizationState> authorization_state_;

	unordered_map<int64_t, string> chat_title_;
	unordered_map<uint64_t, function<void(Object)>> handlers_;
	unordered_map<int32_t, td_api::object_ptr<td_api::user>> users_;

	atomic<uint64_t> current_query_id_ = 0;
	inline uint64_t next_query_id(void)
	{
		return atomic_fetch_add(&current_query_id_, 1);
	}

	atomic<uint64_t> authentication_query_id_ = 0;

	mutex on_auth_update_mutex;
	mutex handlersMutex_;

	bool closed_ = false;
	bool need_restart_ = false;
	bool is_authorized_ = false;

	void restart(void);
	void on_authorization_state_update(void);
	void check_authentication_error(Object object);
	void process_response(td::ClientManager::Response response);
	void process_update(td_api::object_ptr<td_api::Object> update);
	function<void(Object object)> create_authentication_query_handler(void);

public:
	Td(uint32_t api_id, const char *api_hash, const char *data_path);

	uint64_t send_query(td_api::object_ptr<td_api::Function> f,
			    function<void(Object)> handler);

	void loop(int timeout);
	void close(void);

	template <typename T, typename U>
	td_api::object_ptr<U> send_query_sync(td_api::object_ptr<T> method,
					      uint32_t timeout);

	template <typename T, typename U>
	td_api::object_ptr<U> send_query_sync(td_api::object_ptr<T> method,
					      uint32_t timeout,
					      td_api::object_ptr<td_api::error> *err);


	inline void setCancelDelayedWork(bool cancel)
	{
		cancel_delayed_work = true;
	}


	inline bool getCancelDelayedWork(void)
	{
		return cancel_delayed_work;
	}
};


#define QSD_CLEAN_FROM_SQS	(0u)
#define QSD_CLEAN_FROM_CALLBACK	(1u)
template <typename U>
struct query_sync_data {
	condition_variable			cond;
	std::mutex				mutex;
	std::unique_lock<std::mutex>		lock;
	td_api::object_ptr<U>			ret;
	td_api::object_ptr<td_api::error>	*err;
	volatile bool				finished = false;
	volatile uint8_t			cleaner  = QSD_CLEAN_FROM_SQS;

	query_sync_data(void):
		lock(this->mutex, std::defer_lock)
	{
	}
};


template <typename U>
static inline auto query_sync_callback(query_sync_data<U> *data)
{
	return [=](td_api::object_ptr<td_api::Object> obj) {

		data->mutex.lock();
		if (unlikely(data->cleaner == QSD_CLEAN_FROM_CALLBACK)) {
			data->mutex.unlock();
			delete data;
			return;
		}

		if (obj->get_id() == td_api::error::ID) {
			if (data->err) {
				*data->err = td::move_tl_object_as<td_api::error>(obj);
				pr_err("Got error on query_sync_callback");
			}
			goto out;
		}

		if (obj->get_id() != U::ID) {
			pr_error("Invalid object returned on send_query_sync");
			goto out;
		}

	out:
		data->ret = td::move_tl_object_as<U>(obj);
		data->finished = true;
		data->cond.notify_one();
		data->mutex.unlock();
	};
}


template <typename T, typename U>
td_api::object_ptr<U> Td::send_query_sync(td_api::object_ptr<T> method,
					  uint32_t timeout)
{
	return send_query_sync<T, U>(std::move(method), timeout, nullptr);
}


/*
 * T for the method name.
 * U for the return value.
 */
template <typename T, typename U>
td_api::object_ptr<U> Td::send_query_sync(td_api::object_ptr<T> method,
					  uint32_t timeout,
					  td_api::object_ptr<td_api::error> *err)
{
	td_api::object_ptr<U> ret;
	query_sync_data<U> *data = new query_sync_data<U>();
	data->err = err;

	uint32_t secs = 0;
	const uint32_t warnOnSecs = 120;

	send_query(std::move(method), query_sync_callback<U>(data));

	data->lock.lock();
	while (!data->finished) {

		if (unlikely(getCancelDelayedWork())) {
			data->cleaner = QSD_CLEAN_FROM_CALLBACK;
			break;
		}

		data->cond.wait_for(data->lock, 1000ms);

		if (likely(data->finished))
			break;

		if (unlikely(++secs >= warnOnSecs))
			pr_notice("[tid=%ld] Warning: send_query_sync() blocked "
				  "for more than %u seconds", (long)gettid(),
				  secs);

		if (unlikely(timeout > 0 && secs >= timeout)) {
			pr_notice("[tid=%ld] Warning: send_query_sync() reached "
				  "timeout after %u seconds", (long)gettid(),
				  secs);
			break;
		}
	}
	ret = std::move(data->ret);
	data->lock.unlock();

	if (likely(data->cleaner == QSD_CLEAN_FROM_SQS))
		delete data;

	return ret;
}


} /* namespace tgvisd::Td */


namespace detail {

template <class... Fs>
struct overload;


template <class F>
struct overload<F>: public F
{
	explicit overload(F f):
		F(f)
	{
	}
};


template <class F, class... Fs>
struct overload<F, Fs...>: public overload<F>, overload<Fs...>
{
	overload(F f, Fs... fs):
		overload<F>(f),
		overload<Fs...>(fs...)
	{
	}

	using overload<F>::operator();
	using overload<Fs...>::operator();
};

}  /* namespace detail */


template <class... F>
auto overloaded(F... f)
{
	return detail::overload<F...>(f...);
}


#endif /* #ifndef TGVISD__TD__TD_HPP */

