// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <tgvisd/Main.hpp>
#include <tgvisd/Scraper.hpp>

#if defined(__linux__)
	#include <signal.h>
	#include <unistd.h>
#endif

namespace tgvisd {

volatile bool stopEventLoop = false;
static void set_interrupt_handler(void);


__cold Main::Main(uint32_t api_id, const char *api_hash, const char *data_path):
	td_(api_id, api_hash, data_path)
{
	set_interrupt_handler();

	pr_notice("Spawning scraper thread...");
	scraperThread_ = new std::thread([this]{
		Scraper *st;

		/*
		 * The assignment to scraperThread_ outside this
		 * lambda might've not happened yet we the thread's
		 * PC enters this lambda. Thus `this->scraperThread_`
		 * may be NULL. Let's make sure the assignment has
		 * been done before we run the scraper work.
		 */
		while (unlikely(!this->scraperThread_))
			cpu_relax();

		st = new Scraper(this, this->scraperThread_);
		st->run();
		delete st;
	});
}


__cold Main::~Main(void)
{
	if (scraperThread_) {
		pr_notice("Waiting for scraper thread to exit...");
		scraperThread_->join();
		delete scraperThread_;
	}

	td_.close();

#if defined(__linux__)
	pr_notice("Syncing...");
	sync();
#endif
}


__hot int Main::run(void)
{
	constexpr int timeout = 1;

	td_.loop(timeout);
	isReady_ = true;

	while (likely(!stopEventLoop))
		td_.loop(timeout);

	return 0;
}



#if defined(__linux__)
/* 
 * Interrupt handler for Linux only.
 */
static std::mutex sig_mutex;
static volatile bool is_sighandler_set = false;

__cold static void main_sighandler(int sig)
{
	if (!stopEventLoop) {
		printf("\nGot an interrupt signal %d\n", sig);
		stopEventLoop = true;
	}
}

__cold static void set_interrupt_handler(void)
{
	/*
	 * Use signal interrupt handler to kill the
	 * process gracefully on Linux.
	 *
	 * Mutex note:
	 * We may have several instances calling
	 * Main::Main simultaneously, so we need a
	 * mutex here.
	 *
	 * Signal handler is only required to be
	 * set once.
	 */
	sig_mutex.lock();
	if (unlikely(!is_sighandler_set)) {
		signal(SIGINT, main_sighandler);
		signal(SIGHUP, main_sighandler);
		signal(SIGTERM, main_sighandler);
		is_sighandler_set = true;
	}
	sig_mutex.unlock();
}

#else /* #if defined(__linux__) */

static void set_interrupt_handler(void)
{
}

#endif


} /* namespace tgvisd */
