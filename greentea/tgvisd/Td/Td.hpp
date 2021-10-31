// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0
 * @package tgvisd::Td
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef TGVISD__TD__TD_HPP
#define TGVISD__TD__TD_HPP

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>
#include <td/telegram/td_api.hpp>

#include <atomic>
#include <functional>
#include <unordered_map>

namespace td_api = td::td_api;
using Object = td_api::object_ptr<td_api::Object>;

namespace tgvisd::Td {

using std::atomic;
using std::function;
using std::unique_ptr;
using std::unordered_map;


class Td
{
private:
	uint32_t api_id_ = 0;
	const char *api_hash_ = nullptr;
	const char *data_path_ = nullptr;

	int64_t	client_id_ = 0;
	unique_ptr<td::ClientManager> client_manager_;


	atomic<uint64_t> current_query_id_ = 0;

	inline uint64_t next_query_id(void)
	{
		return atomic_fetch_add(&current_query_id_, 1);
	}

	unordered_map<uint64_t, function<void(Object)>> handlers_;

public:
	Td(uint32_t api_id, const char *api_hash, const char *data_path);

	void send_query(td_api::object_ptr<td_api::Function> f,
			std::function<void(Object)> handler);
};

} /* namespace tgvisd::Td */

#endif /* #ifndef TGVISD__TD__TD_HPP */

