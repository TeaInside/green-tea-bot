// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef TGVISD__SCRAPER_HPP
#define TGVISD__SCRAPER_HPP

#include <tgvisd/Td/Td.hpp>
#include <tgvisd/common.hpp>

#include <thread>
#include <tgvisd/Main.hpp>

namespace tgvisd {

class Scraper {
private:
	tgvisd::Td::Td *td_ = nullptr;
	Main *main_ = nullptr;
	std::thread *threadPtr_ = nullptr;
	volatile bool stopScraper = false;

	void scraperEventLoop(void);


	td_api::object_ptr<td_api::chats> getChats(
		td_api::object_ptr<td_api::ChatList> &&chatList, int32_t limit);

	void scrapeChat(int64_t chat_id);
	td_api::object_ptr<td_api::chat> getChat(int64_t chat_id);

public:
	Scraper(Main *main, std::thread *threadPtr);
	~Scraper(void);
	void run(void);


	inline Main *getMain(void)
	{
		return main_;
	}
};

} /* namespace tgvisd */

#endif /* #ifndef TGVISD__SCRAPER_HPP */
