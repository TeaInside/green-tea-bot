// SPDX-License-Identifier: GPL-2.0-only
/*
 * @author Ammar Faizi <ammarfaizi2@gmail.com> https://www.facebook.com/ammarfaizi2
 * @license GPL-2.0-only
 * @package tgvisd
 *
 * Copyright (C) 2021 Ammar Faizi <ammarfaizi2@gmail.com>
 */


#ifndef TGVISD__COMMON_HPP
#define TGVISD__COMMON_HPP

#include <cstdio>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdbool>

#ifndef __must_hold
	#define __must_hold(MUTEX)
#endif

#ifndef __releases
	#define __releases(MUTEX)
#endif

#ifndef __acquires
	#define __acquires(MUTEX)
#endif

#ifndef likely
	#define likely(EXPR)	__builtin_expect((bool)(EXPR), 1)
#endif

#ifndef unlikely
	#define unlikely(EXPR)	__builtin_expect((bool)(EXPR), 0)
#endif

#ifndef __hot
	#define __hot		__attribute__((__hot__))
#endif

#ifndef __cold
	#define __cold		__attribute__((__cold__))
#endif

#include <tgvisd/print.h>

#endif /* #ifndef TGVISD__COMMON_HPP */
