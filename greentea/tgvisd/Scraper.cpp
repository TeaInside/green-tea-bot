// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021  Ammar Faizi <ammarfaizi2@gmail.com>
 */

#if defined(__linux__)
	#include <unistd.h>
	#include <pthread.h>
#endif

#include <stack>
#include <mutex>
#include <queue>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <tgvisd/Td/Td.hpp>
#include <tgvisd/common.hpp>
#include <tgvisd/KWorker.hpp>
#include <tgvisd/Scraper.hpp>


namespace tgvisd {


Scraper::Scraper(Main *main):
	main_(main),
	kworker_(main->getKWorker())
{
}


__hot void Scraper::run(void)
{
	while (!main_->isReady()) {
		if (shouldStop())
			return;
		sleep(1);
	}

	while (!shouldStop()) {
		_run();
		sleep(1);
	}
}


__hot void Scraper::_run(void)
{
	int32_t i;
	int64_t chat_id;

	pr_notice("Getting chat list...");
	auto chats = kworker_->getChats(nullptr, 300);
	if (unlikely(!chats))
		return;

	for (i = 0; i < chats->total_count_; i++) {
		if (shouldStop())
			break;

		chat_id = chats->chat_ids_[i];
		auto chat = kworker_->getChat(chat_id);

		if (unlikely(!chat))
			continue;

		if (chat->type_->get_id() != td_api::chatTypeSupergroup::ID)
			continue;

		pr_notice("Submitting visit to %ld...", chat_id);
		visit_chat(chat);
	}
}


__hot void Scraper::visit_chat(td_api::object_ptr<td_api::chat> &chat)
{
	int ret;
	struct task_work tw;

	tw.func = [this, &chat](struct tw_data *data){
		_visit_chat(data, std::move(chat));
	};

	do {
		if (shouldStop())
			break;

		ret = kworker_->submitTaskWork(&tw);
		if (ret == -EAGAIN)
			sleep(1);

	} while (ret == -EAGAIN);
}


__hot void Scraper::_visit_chat(struct tw_data *data,
				td_api::object_ptr<td_api::chat> &chat)
{
	printf("Scraping %ld...\n", chat->id_);
	sleep(2);
}


} /* namespace tgvisd */
