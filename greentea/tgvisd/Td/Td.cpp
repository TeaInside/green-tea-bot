// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd::Td
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include "Td.hpp"
#include <iostream>

namespace tgvisd::Td {

using std::function;
using std::unique_ptr;

namespace td_api = td::td_api;
using Object = td_api::object_ptr<td_api::Object>;


__cold Td::Td(uint32_t api_id, const char *api_hash, const char *data_path):
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


__hot void Td::send_query(td_api::object_ptr<td_api::Function> f,
			  function<void(Object)> handler)
{
	uint64_t query_id;

	query_id = next_query_id();
	if (handler)
		handlers_.emplace(query_id, std::move(handler));

	client_manager_->send(client_id_, query_id, std::move(f));
}


__hot void Td::loop(int timeout)
{
	if (unlikely(need_restart_)) {
		restart();
		return;
	}

	process_response(client_manager_->receive(timeout));
}


__hot void Td::process_response(td::ClientManager::Response res)
{
	if (!res.object)
		return;

	if (0) {
		std::cout
			<< res.request_id
			<< " "
			<< to_string(res.object)
			<< std::endl;
	}

	if (res.request_id == 0) {
		process_update(std::move(res.object));
		return;
	}

	auto it = handlers_.find(res.request_id);
	if (it != handlers_.end()) {
		it->second(std::move(res.object));
		handlers_.erase(it);
	}
}


__hot void Td::process_update(td_api::object_ptr<td_api::Object> update)
{
	auto u_auth = [this](td_api::updateAuthorizationState &update) {
		authorization_state_ = std::move(update.authorization_state_);
		on_authorization_state_update();
		callback.execute(update);
	};

	auto u_new_chat = [this](td_api::updateNewChat &update) {
		chat_title_[update.chat_->id_] = update.chat_->title_;
		callback.execute(update);
	};

	auto u_chat_title = [this](td_api::updateChatTitle &update) {
		chat_title_[update.chat_id_] = update.title_;
		callback.execute(update);
	};

	auto u_user = [this](td_api::updateUser &update) {
		users_[update.user_->id_] = std::move(update.user_);
		callback.execute(update);
	};

	auto u_new_msg = [this](td_api::updateNewMessage &update) {
		callback.execute(update);
	};

	td_api::downcast_call(
		*update,
		overloaded(
			u_auth,
			u_new_chat,
			u_chat_title,
			u_user,
			u_new_msg,
			[](auto &update) {}
		)
	);
}


__cold void Td::restart(void)
{
	client_manager_.reset();
	closed_ = false;
	need_restart_ = false;
	is_authorized_ = false;
	current_query_id_ = 0;
	authentication_query_id_ = 0;
}


__cold void Td::check_authentication_error(Object object)
{
	if (object->get_id() == td_api::error::ID) {
		auto error = td::move_tl_object_as<td_api::error>(object);
		std::cout << "Error: " << to_string(error) << std::flush;
		on_authorization_state_update();
	}
}


__cold function<void(Object object)> Td::create_authentication_query_handler(void)
{
	return [this, id = authentication_query_id_.load()](Object object) {
		if (id == authentication_query_id_.load())
			check_authentication_error(std::move(object));
	};
}


__cold void Td::on_authorization_state_update(void)
{
	/*
	 * `as_*`
	 * The prefix "as" means `authorization_state`
	 */

	on_auth_update_mutex.lock();

	auto as_ready = [this](td_api::authorizationStateReady &) {
		is_authorized_ = true;
		std::cout << "Got authorizationStateReady" << std::endl;

		auto callback = [this](Object obj){
			if (obj->get_id() != td_api::user::ID)
				return;
			auto user = td::move_tl_object_as<td_api::user>(obj);
			user_id_ = user->id_;
		};

		/*
		 * Get main account user_id.
		 */
		send_query(td_api::make_object<td_api::getMe>(), callback);
	};

	auto as_logging_out = [this](td_api::authorizationStateLoggingOut &) {
		is_authorized_ = false;
		std::cout << "Logging out" << std::endl;
	};

	auto as_closing = [](td_api::authorizationStateClosing &) {
		std::cout << "Closing TdLib..." << std::endl;
	};

	auto as_closed = [this](td_api::authorizationStateClosed &) {
		closed_	= true;
		need_restart_ = true;
		is_authorized_ = false;
		std::cout << "Terminated" << std::endl;
	};

	auto as_wait_code = [this](td_api::authorizationStateWaitCode &) {
		std::string code;

		std::cout << "Enter authentication code: " << std::flush;
		std::getline(std::cin, code);

		auto f = td_api::make_object<td_api::checkAuthenticationCode>(code);
		send_query(std::move(f), create_authentication_query_handler());
	};

	auto as_wait_regis = [this](td_api::authorizationStateWaitRegistration &) {
		std::string fn, ln;

		std::cout << "Enter your first name: " << std::flush;
		std::getline(std::cin, fn);

		std::cout << "Enter your last name: " << std::flush;
		std::getline(std::cin, ln);

		auto f = td_api::make_object<td_api::registerUser>(fn, ln);
		send_query(std::move(f), create_authentication_query_handler());
	};

	auto as_wait_pass = [this](td_api::authorizationStateWaitPassword &) {
		std::string pass;

		std::cout << "Enter authentication password: " << std::flush;
		std::getline(std::cin, pass);

		auto f = td_api::make_object<td_api::checkAuthenticationPassword>(
			pass
		);
		send_query(std::move(f), create_authentication_query_handler());
	};

	auto as_wait_odc = [](
		td_api::authorizationStateWaitOtherDeviceConfirmation &state) {

		std::cout
			<< "Confirm this login link on another device: "
			<< state.link_
			<< std::endl;
	};

	auto as_wait_pn = [this](td_api::authorizationStateWaitPhoneNumber &) {
		std::string pn;

		std::cout << "Enter phone number: " << std::flush;
		std::getline(std::cin, pn);

		auto f = td_api::make_object<td_api::setAuthenticationPhoneNumber>(
			pn,
			nullptr
		);
		send_query(std::move(f), create_authentication_query_handler());
	};

	auto as_wait_ek = [this](td_api::authorizationStateWaitEncryptionKey &) {
		std::string key = "";

		// std::cout << "Enter encryption key or DESTROY: " << std::flush;
		// std::getline(std::cin, key);

#if 0
		if (key == "DESTROY") {
			send_query(td_api::make_object<td_api::destroy>(),
					   create_authentication_query_handler());
			return;
		}
#endif

		auto f = td_api::make_object<td_api::checkDatabaseEncryptionKey>(
			std::move(key)
		);
		send_query(std::move(f), create_authentication_query_handler());
	};


	auto as_wait_tp = [this](td_api::authorizationStateWaitTdlibParameters &) {
		auto par = td_api::make_object<td_api::tdlibParameters>();

		par->use_message_database_ = true;
		par->use_secret_chats_ = false;
		par->api_id_ = this->api_id_;
		par->api_hash_ = this->api_hash_;
		par->database_directory_ = this->data_path_;
		par->system_language_code_ = "en";
		par->device_model_ = "Desktop";
		par->application_version_ = "1.0";
		par->enable_storage_optimizer_ = true;

		auto f = td_api::make_object<td_api::setTdlibParameters>(std::move(par));
		send_query(std::move(f), create_authentication_query_handler());
	};

	td_api::downcast_call(
		*authorization_state_,
		overloaded(
			as_ready,
			as_logging_out,
			as_closing,
			as_closed,
			as_wait_code,
			as_wait_regis,
			as_wait_pass,
			as_wait_odc,
			as_wait_pn,
			as_wait_ek,
			as_wait_tp,
			[](auto &){}
		)
	);
	on_auth_update_mutex.unlock();
}


__cold void Td::close(void)
{
	send_query(td_api::make_object<td_api::close>(), {});
	std::cout << "Waiting for authorizationStateClosed..." << std::endl;
	loop(5);
}


} /* namespace tgvisd::Td */
