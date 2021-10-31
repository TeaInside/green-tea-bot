// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */

#include <cstdio>
#include <cstdlib>
#include <tgvisd/Main.hpp>


int main(void)
{
	const char *api_id, *api_hash, *data_path;

	api_id = getenv("TGVISD_API_ID");
	if (!api_id) {
		puts("Missing TGVISD_API_ID");
		return 1;
	}

	api_hash = getenv("TGVISD_API_HASH");
	if (!api_hash) {
		puts("Missing TGVISD_API_HASH");
		return 1;
	}

	data_path = getenv("TGVISD_DATA_PATH");
	if (!data_path) {
		puts("Missing TGVISD_DATA_PATH");
		return 1;
	}

	tgvisd::Main mm((uint32_t)atoi(api_id), api_hash, data_path);
	return mm.run();
}
