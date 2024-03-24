/*
 * @Author: yaosy fmusrr@foxmail.com
 * @Date: 2024-03-07 23:24:49
 * @LastEditors: YAOSIYAN fmusrr@foxmail.com
 * @LastEditTime: 2024-03-14 18:44:52
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#ifndef __K_UTILS_H
#define __K_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stddef.h>
#include <stdint.h>

#ifndef __CHAR_BIT__
#define __CHAR_BIT__ 8
#endif
#ifndef __SIZEOF_LONG__
#define __SIZEOF_LONG__ 4
#endif
#ifndef __SIZEOF_LONG_LONG__
#define __SIZEOF_LONG_LONG__ 4
#endif

#if !(defined(__CHAR_BIT__) && defined(__SIZEOF_LONG__) && defined(__SIZEOF_LONG_LONG__))
#	error Missing required predefined macros for BITS_PER_LONG calculation
#endif

/** Number of bits in a long int. */
#define BITS_PER_LONG	(__CHAR_BIT__ * __SIZEOF_LONG__)

/** Number of bits in a long long int. */
#define BITS_PER_LONG_LONG	(__CHAR_BIT__ * __SIZEOF_LONG_LONG__)

/**
 * @brief Create a contiguous bitmask starting at bit position @p l
 *        and ending at position @p h.
 */
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

/**
 * @brief Create a contiguous 64-bit bitmask starting at bit position @p l
 *        and ending at position @p h.
 */
#define GENMASK64(h, l) \
	(((~0ULL) - (1ULL << (l)) + 1) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

/** @brief Extract the Least Significant Bit from @p value. */
#define LSB_GET(value) ((value) & -(value))

/**
 * @brief Extract a bitfield element from @p value corresponding to
 *	  the field mask @p mask.
 */
#define FIELD_GET(mask, value)  (((value) & (mask)) / LSB_GET(mask))

/** @brief 0 if @p cond is true-ish; causes a compile error otherwise. */
#define ZERO_OR_COMPILE_ERROR(cond) ((int)sizeof(char[1 - 2 * !(cond)]) - 1)

#define BIT(n)                      (1UL << (n))
#define BIT64(_n)                   (1ULL << (_n))

#if defined(__cplusplus)

/* The built-in function used below for type checking in C is not
 * supported by GNU C++.
 */
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#else /* __cplusplus */

/**
 * @brief Zero if @p array has an array type, a compile error otherwise
 *
 * This macro is available only from C, not C++.
 */
#define IS_ARRAY(array)                                                                            \
    ZERO_OR_COMPILE_ERROR(!__builtin_types_compatible_p(__typeof__(array), __typeof__(&(array)[0])))

/**
 * @brief Number of elements in the given @p array
 *
 * In C++, due to language limitations, this will accept as @p array
 * any type that implements <tt>operator[]</tt>. The results may not be
 * particularly meaningful in this case.
 *
 * In C, passing a pointer as @p array causes a compile error.
 */
#define ARRAY_SIZE(array) ((size_t)(IS_ARRAY(array) + (sizeof(array) / sizeof((array)[0]))))

#endif /* __cplusplus */

/**
 * @brief Get a pointer to a structure containing the element
 *
 * Example:
 *
 *	struct foo {
 *		int bar;
 *	};
 *
 *	struct foo my_foo;
 *	int *ptr = &my_foo.bar;
 *
 *	struct foo *container = CONTAINER_OF(ptr, struct foo, bar);
 *
 * Above, @p container points at @p my_foo.
 *
 * @param ptr pointer to a structure element
 * @param type name of the type that @p ptr is an element of
 * @param field the name of the field within the struct @p ptr points to
 * @return a pointer to the structure that contains @p ptr
 */
#define CONTAINER_OF(ptr, type, field) \
	((type *)(((char *)(ptr)) - offsetof(type, field)))

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Clamp a value to a given range.
 *
 * @note Arguments are evaluated multiple times. Use Z_CLAMP for a GCC-only,
 * single evaluation version.
 *
 * @param val Value to be clamped.
 * @param low Lowest allowed value (inclusive).
 * @param high Highest allowed value (inclusive).
 *
 * @returns Clamped value.
 */
#define CLAMP(val, low, high) (((val) <= (low)) ? (low) : MIN(val, high))

/**
 * @brief Checks if a value is within range.
 *
 * @note @p val is evaluated twice.
 *
 * @param val Value to be checked.
 * @param min Lower bound (inclusive).
 * @param max Upper bound (inclusive).
 *
 * @retval true If value is within range
 * @retval false If the value is not within range
 */
#define IN_RANGE(val, min, max) ((val) >= (min) && (val) <= (max))

/** @brief Number of bytes in @p x kibibytes */
#ifdef _LINKER
/* This is used in linker scripts so need to avoid type casting there */
#define KB(x) ((x) << 10)
#else
#define KB(x) (((size_t)x) << 10)
#endif
/** @brief Number of bytes in @p x mebibytes */
#define MB(x) (KB(x) << 10)
/** @brief Number of bytes in @p x gibibytes */
#define GB(x) (MB(x) << 10)

/** @brief Number of Hz in @p x kHz */
#define KHZ(x) ((x) * 1000)
/** @brief Number of Hz in @p x MHz */
#define MHZ(x) (KHZ(x) * 1000)

/**
 * @brief Divide and round up.
 *
 * Example:
 * @code{.c}
 * DIV_ROUND_UP(1, 2); // 1
 * DIV_ROUND_UP(3, 2); // 2
 * @endcode
 *
 * @param n Numerator.
 * @param d Denominator.
 *
 * @return The result of @p n / @p d, rounded up.
 */
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

/**
 * @brief Divide and round to the nearest integer.
 *
 * Example:
 * @code{.c}
 * DIV_ROUND_CLOSEST(5, 2); // 3
 * DIV_ROUND_CLOSEST(5, -2); // -3
 * DIV_ROUND_CLOSEST(5, 3); // 2
 * @endcode
 *
 * @param n Numerator.
 * @param d Denominator.
 *
 * @return The result of @p n / @p d, rounded to the nearest integer.
 */
#define DIV_ROUND_CLOSEST(n, d) \
    ((((n) < 0) ^ ((d) < 0)) ? ((n) - ((d) / 2)) / (d) : ((n) + ((d) / 2)) / (d))

/**
 * @brief For the POSIX architecture add a minimal delay in a busy wait loop.
 * For other architectures this is a no-op.
 *
 * In the POSIX ARCH, code takes zero simulated time to execute,
 * so busy wait loops become infinite loops, unless we
 * force the loop to take a bit of time.
 * Include this macro in all busy wait/spin loops
 * so they will also work when building for the POSIX architecture.
 *
 * @param t Time in microseconds we will busy wait
 */
#if defined(CONFIG_ARCH_POSIX)
#define Z_SPIN_DELAY(t) k_busy_wait(t)
#else
#define Z_SPIN_DELAY(t)
#endif

/**
 * @brief Wait for an expression to return true with a timeout
 *
 * Spin on an expression with a timeout and optional delay between iterations
 *
 * Commonly needed when waiting on hardware to complete an asynchronous
 * request to read/write/initialize/reset, but useful for any expression.
 *
 * @param expr Truth expression upon which to poll, e.g.: XYZREG & XYZREG_EN
 * @param timeout Timeout to wait for in microseconds, e.g.: 1000 (1ms)
 * @param delay_stmt Delay statement to perform each poll iteration
 *                   e.g.: NULL, k_yield(), k_msleep(1) or k_busy_wait(1)
 *
 * @retval expr As a boolean return, if false then it has timed out.
 */
#define WAIT_FOR(expr, timeout, delay_stmt)                                                        \
    ({                                                                                             \
        uint32_t _wf_cycle_count = k_us_to_cyc_ceil32(timeout);                                    \
        uint32_t _wf_start = k_cycle_get_32();                                                     \
        while (!(expr) && (_wf_cycle_count > (k_cycle_get_32() - _wf_start))) {                    \
            delay_stmt;                                                                            \
            Z_SPIN_DELAY(10);                                                                      \
        }                                                                                          \
        (expr);                                                                                    \
    })

#define likely(x)     (__builtin_expect((bool)!!(x), true) != 0L)
#define unlikely(x)   (__builtin_expect((bool)!!(x), false) != 0L)

#define ARG_UNUSED(x) (void)(x)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // __k_UTILS_H
