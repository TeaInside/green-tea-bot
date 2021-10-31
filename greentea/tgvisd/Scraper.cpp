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

#include <tgvisd/Scraper.hpp>

namespace tgvisd {


Scraper::Scraper(Main *main, std::thread *threadPtr):
	main_(main)
{
}


Scraper::~Scraper(void)
{
}


void Scraper::run(void)
{
}


} /* namespace tgvisd */
