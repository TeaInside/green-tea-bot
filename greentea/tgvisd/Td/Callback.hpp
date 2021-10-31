// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd::Td
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef TGVISD__TD__CALLBACK_HPP
#define TGVISD__TD__CALLBACK_HPP

namespace tgvisd::Td {

namespace td_api = td::td_api;
using Object = td_api::object_ptr<td_api::Object>;

class Callback
{
public:
	std::function<void(td_api::updateAuthorizationState &update)>
		updateAuthorizationState = nullptr;
	inline void execute(td_api::updateAuthorizationState &update)
	{
		if (updateAuthorizationState)
			updateAuthorizationState(update);
	}

	std::function<void(td_api::updateNewChat &update)>
		updateNewChat = nullptr;
	inline void execute(td_api::updateNewChat &update)
	{
		if (updateNewChat)
		   updateNewChat(update);
	}

	std::function<void(td_api::updateChatTitle &update)>
		updateChatTitle = nullptr;
	inline void execute(td_api::updateChatTitle &update)
	{
		if (updateChatTitle)
			updateChatTitle(update);
	}

	std::function<void(td_api::updateUser &update)>
		updateUser = nullptr;
	inline void execute(td_api::updateUser &update)
	{
		if (updateUser)
			updateUser(update);
	}

	std::function<void(td_api::updateNewMessage &update)>
		updateNewMessage = nullptr;
	inline void execute(td_api::updateNewMessage &update)
	{
		if (updateNewMessage)
			updateNewMessage(update);
	}
};

} /* namespace tgvisd::Td */

#endif
