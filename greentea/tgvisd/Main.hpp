// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#ifndef TGVISD__MAIN_HPP
#define TGVISD__MAIN_HPP

#include <thread>
#include <tgvisd/Td/Td.hpp>
#include <tgvisd/common.hpp>

namespace tgvisd {

extern volatile bool stopEventLoop;

class Scraper;

class KWorker;


class Main
{
private:
	tgvisd::Td::Td	td_;
	volatile bool	isReady_ = false;
	std::thread	*kworkerThread_ = nullptr;
	std::thread	*scraperThread_ = nullptr;
	KWorker		*kworker_ = nullptr;
	Scraper		*scraper_ = nullptr;

public:
	Main(uint32_t api_id, const char *api_hash, const char *data_path);
	~Main(void);
	int run(void);


	inline KWorker *getKWorker(void)
	{
		return kworker_;
	}


	inline void doStop(void)
	{
		stopEventLoop = true;
	}


	inline bool getStop(void)
	{
		return stopEventLoop;
	}


	inline bool isReady(void)
	{
		return isReady_;
	}


	inline tgvisd::Td::Td *getTd(void)
	{
		return &td_;
	}
};


} /* namespace tgvisd */

#endif /* #ifndef TGVISD__MAIN_HPP */
