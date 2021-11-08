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
	Main *main_ = nullptr;
	std::thread *threadPtr_ = nullptr;
	volatile bool stopScraper_ = false;

	void scraperEventLoop(void);

public:
	Scraper(Main *main, std::thread *threadPtr);
	~Scraper(void);
	void run(void);


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
};

} /* namespace tgvisd */

#endif /* #ifndef TGVISD__SCRAPER_HPP */
