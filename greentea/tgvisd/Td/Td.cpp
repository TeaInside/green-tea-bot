// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0
 * @package tgvisd::Td
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include "Td.hpp"


namespace tgvisd::Td {

using std::unique_ptr;

namespace td_api = td::td_api;
using Object = td_api::object_ptr<td_api::Object>;


Td::Td(uint32_t api_id, const char *api_hash, const char *data_path):
	api_id_(api_id),
	api_hash_(api_hash),
	data_path_(data_path)
{
	auto p = td_api::make_object<td_api::setLogVerbosityLevel>(1);
	td::ClientManager::execute(std::move(p));

	client_manager_ = std::make_unique<td::ClientManager>();
	client_id_ = client_manager_->create_client_id();
	send_query(td_api::make_object<td_api::getOption>("version"), {});
}


void Td::send_query(td_api::object_ptr<td_api::Function> f,
		    std::function<void(Object)> handler)
{
	uint64_t query_id;

	query_id = next_query_id();
	if (handler)
		handlers_.emplace(query_id, std::move(handler));

	client_manager_->send(client_id_, query_id, std::move(f));
}


} /* namespace tgvisd::Td */
