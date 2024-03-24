/*
 * Copyright (c) 2011-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS___ASSERT_H_
#define ZEPHYR_INCLUDE_SYS___ASSERT_H_

#include <stdbool.h>

#undef BUILD_ASSERT /* clear out common version */
/* C++11 has static_assert built in */
#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define BUILD_ASSERT(EXPR, MSG...) static_assert(EXPR, "" MSG)

/*
 * GCC 4.6 and higher have the C11 _Static_assert built in and its
 * output is easier to understand than the common BUILD_ASSERT macros.
 * Don't use this in C++98 mode though (which we can hit, as
 * static_assert() is not available)
 */
#elif !defined(__cplusplus) &&                                                                     \
    ((__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) || (__STDC_VERSION__) >= 201100)
#define BUILD_ASSERT(EXPR, MSG...)
#else
#define BUILD_ASSERT(EXPR, MSG...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Wrapper around printk to avoid including printk.h in assert.h */
void assert_print(const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#define __ASSERT(test, fmt, ...) { }
#define __ASSERT_EVAL(expr1, expr2, test, fmt, ...) expr1
#define __ASSERT_NO_MSG(test) { }
#define __ASSERT_POST_ACTION() { }

#endif /* ZEPHYR_INCLUDE_SYS___ASSERT_H_ */
