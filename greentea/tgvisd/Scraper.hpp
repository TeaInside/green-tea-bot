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

#include <tgvisd/Main.hpp>

namespace tgvisd {

class Scraper {
private:
	Main *main_ = nullptr;

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
