/*
 * @Date: 2024-03-14 18:00:43
 * @LastEditors: YAOSIYAN fmusrr@foxmail.com
 * @LastEditTime: 2024-03-15 15:55:58
 * @FilePath: \Openy_Framework\include\k_timeout.h
 */
#ifndef __K_TIMEOUT_H
#define __K_TIMEOUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "k_dlist.h"
#include "k_utils.h"
#include "k_config.h"

/* Some more concise declarations to simplify the generator script and
 * save bytes below
 */
#define Z_HZ_ms    1000
#define Z_HZ_us    1000000
#define Z_HZ_ns    1000000000
#define Z_HZ_cyc   K_CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define Z_HZ_ticks K_CONFIG_SYS_CLOCK_TICKS_PER_SEC

#define CONFIG_SYS_CLOCK_MAX_TIMEOUT_DAYS 365

/** @internal
 * Macro determines if fast conversion algorithm can be used. It checks if
 * maximum timeout represented in source frequency domain and multiplied by
 * target frequency fits in 64 bits.
 *
 * @param from_hz Source frequency.
 * @param to_hz Target frequency.
 *
 * @retval true Use faster algorithm.
 * @retval false Use algorithm preventing overflow of intermediate value.
 */
#define Z_TMCVT_USE_FAST_ALGO(from_hz, to_hz)                                                   \
    ((DIV_ROUND_UP(CONFIG_SYS_CLOCK_MAX_TIMEOUT_DAYS * 24ULL * 3600ULL * from_hz, UINT32_MAX) * \
      to_hz) <= UINT32_MAX)

/* Time converter generator gadget.  Selects from one of three
 * conversion algorithms: ones that take advantage when the
 * frequencies are an integer ratio (in either direction), or a full
 * precision conversion.  Clever use of extra arguments causes all the
 * selection logic to be optimized out, and the generated code even
 * reduces to 32 bit only if a ratio conversion is available and the
 * result is 32 bits.
 *
 * This isn't intended to be used directly, instead being wrapped
 * appropriately in a user-facing API.  The boolean arguments are:
 *
 *    const_hz  - The hz arguments are known to be compile-time
 *                constants (because otherwise the modulus test would
 *                have to be done at runtime)
 *    result32  - The result will be truncated to 32 bits on use
 *    round_up  - Return the ceiling of the resulting fraction
 *    round_off - Return the nearest value to the resulting fraction
 *                (pass both round_up/off as false to get "round_down")
 */
static inline uint64_t z_tmcvt(uint64_t t, uint32_t from_hz, uint32_t to_hz, bool const_hz,
                               bool result32, bool round_up, bool round_off) {
    bool mul_ratio = const_hz && (to_hz > from_hz) && ((to_hz % from_hz) == 0U);
    bool div_ratio = const_hz && (from_hz > to_hz) && ((from_hz % to_hz) == 0U);

    if (from_hz == to_hz) {
        return result32 ? ((uint32_t)t) : t;
    }

    uint64_t off = 0;

    if (!mul_ratio) {
        uint32_t rdivisor = div_ratio ? (from_hz / to_hz) : from_hz;

        if (round_up) {
            off = rdivisor - 1U;
        }
        if (round_off) {
            off = rdivisor / 2U;
        }
    }

    /* Select (at build time!) between three different expressions for
     * the same mathematical relationship, each expressed with and
     * without truncation to 32 bits (I couldn't find a way to make
     * the compiler correctly guess at the 32 bit result otherwise).
     */
    if (div_ratio) {
        t += off;
        if (result32 && (t < BIT64(32))) {
            return ((uint32_t)t) / (from_hz / to_hz);
        } else {
            return t / ((uint64_t)from_hz / to_hz);
        }
    } else if (mul_ratio) {
        if (result32) {
            return ((uint32_t)t) * (to_hz / from_hz);
        } else {
            return t * ((uint64_t)to_hz / from_hz);
        }
    } else {
        if (result32) {
            return (uint32_t)((t * to_hz + off) / from_hz);
        } else if (const_hz && Z_TMCVT_USE_FAST_ALGO(from_hz, to_hz)) {
            /* Faster algorithm but source is first multiplied by target frequency
             * and it can overflow even though final result would not overflow.
             * Kconfig option shall prevent use of this algorithm when there is a
             * risk of overflow.
             */
            return ((t * to_hz + off) / from_hz);
        } else {
            /* Slower algorithm but input is first divided before being multiplied
             * which prevents overflow of intermediate value.
             */
            return (t / from_hz) * to_hz + ((t % from_hz) * to_hz + off) / from_hz;
        }
    }
}

static inline uint32_t k_ms_to_ticks_ceil32(uint32_t t) {
    return z_tmcvt(t, Z_HZ_ms, Z_HZ_ticks, true, true, true, false);
}
static inline uint32_t k_us_to_ticks_ceil32(uint32_t t) {
    return z_tmcvt(t, Z_HZ_us, Z_HZ_ticks, true, true, true, false);
}
static inline uint32_t k_ns_to_ticks_ceil32(uint32_t t) {
    return z_tmcvt(t, Z_HZ_ns, Z_HZ_ticks, true, true, true, false);
}

/**
 * @brief Tick precision used in timeout APIs
 */
typedef uint32_t k_ticks_t;

typedef struct {
    k_ticks_t ticks;
} k_timeout_t;

#define K_TIMEOUT_TICKS(t)     ((k_timeout_t){.ticks = (t)})
#define K_TIMEOUT_EQ(a, b)     ((a).ticks == (b).ticks)
#define K_TICKS_FOREVER        ((k_ticks_t)-1)
#define K_TICK_ABS(t)          (K_TICKS_FOREVER - 1 - (t))
#define K_TIMEOUT_ABS_TICKS(t) K_TIMEOUT_TICKS(K_TICK_ABS((k_ticks_t)MAX(t, 0)))

#define K_MSEC(t) K_TIMEOUT_TICKS((k_ticks_t)k_ms_to_ticks_ceil32(MAX(t, 0)))
#define K_USEC(t) K_TIMEOUT_TICKS((k_ticks_t)k_us_to_ticks_ceil32(MAX(t, 0)))
#define K_NSEC(t) K_TIMEOUT_TICKS((k_ticks_t)k_ns_to_ticks_ceil32(MAX(t, 0)))

#define K_NO_WAIT    ((k_timeout_t){0})
#define K_SECONDS(s) K_MSEC((s) * 1000)
#define K_MINUTES(m) K_SECONDS((m) * 60)
#define K_HOURS(h)   K_MINUTES((h) * 60)
#define K_FOREVER    K_TIMEOUT_TICKS(K_TICKS_FOREVER)

struct _timeout;
typedef void (*_timeout_func_t)(struct _timeout *t);

struct _timeout {
    sys_dnode_t node;
    _timeout_func_t fn;
    int32_t dticks;
};

void k_timeout_add(struct _timeout *to, _timeout_func_t fn, k_timeout_t timeout);
int k_timeout_abort(struct _timeout *to);
void sys_clock_announce(int32_t ticks);
k_ticks_t sys_clock_tick_get(void);

#ifdef __cplusplus
}
#endif

#endif  // __K_TIMEOUT_H
