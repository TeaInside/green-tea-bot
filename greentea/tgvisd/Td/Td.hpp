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

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <mutex>
#include <atomic>
#include <functional>
#include <unordered_map>

#include "Callback.hpp"

namespace td_api = td::td_api;
using Object = td_api::object_ptr<td_api::Object>;

namespace tgvisd::Td {

using std::mutex;
using std::atomic;
using std::string;
using std::function;
using std::unique_ptr;
using std::unordered_map;


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

	void send_query(td_api::object_ptr<td_api::Function> f,
			function<void(Object)> handler);

	void loop(int timeout);
	void close(void);
};

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

