// SPDX-License-Identifier: GPL-2.0
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0
 * @package tgvisd
 *
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#if defined(__linux__)
	#include <unistd.h>
	#include <pthread.h>
#endif

#include <cassert>
#include <tgvisd/Scraper.hpp>

namespace tgvisd {


Scraper::Scraper(Main *main, std::thread *threadPtr):
	main_(main)
{
	assert(threadPtr);

#if defined(__linux__)
	pthread_t pt = threadPtr->native_handle();
	pthread_setname_np(pt, "scraper");
#endif

	td_ = main->getTd();
	assert(td_);
}


Scraper::~Scraper(void)
{
	pr_notice("Scraper task work is exiting...");
}


void Scraper::run(void)
{
	while (unlikely(!main_->isReady() && !main_->getStop())) {
		pr_notice("In scraper (waiting for main thread to be ready)...");
		sleep(1);
	}

	try {

		while (likely(!main_->getStop() && !stopScraper))
			scraperEventLoop();

	} catch (const std::runtime_error &e) {
		pr_err("In scraper: std::runtime_error: %s", e.what());
		main_->doStop();
	}
}


td_api::object_ptr<td_api::chats> Scraper::getChats(
		td_api::object_ptr<td_api::ChatList> &&chatList, int32_t limit)
{
	const uint32_t timeout = 150;
	return td_->send_query_sync<td_api::getChats, td_api::chats>(
		td_api::make_object<td_api::getChats>(std::move(chatList), limit),
		timeout
	);
}


td_api::object_ptr<td_api::chat> Scraper::getChat(int64_t chat_id)
{
	const uint32_t timeout = 150;
	return td_->send_query_sync<td_api::getChat, td_api::chat>(
		td_api::make_object<td_api::getChat>(chat_id),
		timeout
	);
}


void Scraper::scraperEventLoop(void)
{
	int32_t i, total;

	pr_debug("ChatScraper: Getting chatList...");
	/*
	 * @chats only contain list of chat_id(s) (array of integer here).
	 */
	auto chats = getChats(nullptr, 300);
	total = chats->total_count_;
	pr_debug("ChatScraper: Got %d chat ID(s)", total);
	pr_debug("Enumerating chat_ids...");

	for (i = 0; i < total; i++)
		scrapeChat(chats->chat_ids_[i]);

	sleep(3);
}


void Scraper::scrapeChat(int64_t chat_id)
{
	auto chat = getChat(chat_id);

	if (unlikely(!chat))
		return;

	if (chat->type_->get_id() != td_api::chatTypeSupergroup::ID)
		return;

	pr_notice("Visiting %ld (%s)...", chat_id, chat->title_.c_str());
}


} /* namespace tgvisd */
