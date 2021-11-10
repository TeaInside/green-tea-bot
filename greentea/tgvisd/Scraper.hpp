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
	Main		*main_ = nullptr;
	KWorker		*kworker_ = nullptr;
	volatile bool	stopScraper_ = false;

	void scraperEventLoop(void);
	void _run(void);
	void visit_chat(td_api::object_ptr<td_api::chat> &chat);
	void _visit_chat(struct tw_data *data,
			 td_api::object_ptr<td_api::chat> &chat);

	void save_message(td_api::object_ptr<td_api::message> &msg,
			  td_api::object_ptr<td_api::chat> *chat = nullptr,
			  std::mutex *chat_lock = nullptr);

public:
	Scraper(Main *main);
	~Scraper(void);
	void run(void);

	uint64_t touch_user_with_uid(int64_t tg_user_id,
				     std::mutex *user_lock = nullptr);

	uint64_t touch_user(td_api::object_ptr<td_api::user> &user,
			    std::mutex *user_lock = nullptr);

	uint64_t touch_group_chat(td_api::object_ptr<td_api::chat> &chat,
				  std::mutex *chat_lock = nullptr);


	inline bool shouldStop(void)
	{
		return unlikely(main_->getStop() || stopScraper_);
	}


	inline bool getStop(void)
	{
		return stopScraper_;
	}


	inline void doStop(void)
	{
		stopScraper_ = true;
	}


	inline Main *getMain(void)
	{
		return main_;
	}


	inline static void setThreadName(std::thread *task)
	{
#if defined(__linux__)
		pthread_t pt = task->native_handle();
		pthread_setname_np(pt, "tgv-scraper");
#endif
	}
};

} /* namespace tgvisd */

#endif /* #ifndef TGVISD__SCRAPER_HPP */
