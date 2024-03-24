/*
 * @Date: 2024-03-11 11:34:51
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-19 21:32:43
 * @FilePath: \Openy_Framework\port\k_port.c
 */
#include "k_kernel.h"
#include "main.h"

#define ATOMIC_BITS            (sizeof(atomic_t) * 8)
#define ATOMIC_MASK(bit)       BIT((unsigned long)(bit) & (ATOMIC_BITS - 1U))
#define ATOMIC_ELEM(addr, bit) ((addr) + ((bit) / ATOMIC_BITS))

void __attribute__((weak)) k_print(int level, const char *fmt, ...) {
    const char *level_str[] = {"[INFO] ", "[DEBUG] ", "[ERROR] "};

    fprintf(K_LOG_OUTPUT_STREAM, "%s", level_str[level]);
    va_list args;
    va_start(args, fmt);
    vfprintf(K_LOG_OUTPUT_STREAM, fmt, args);
    va_end(args);
    fprintf(K_LOG_OUTPUT_STREAM, "\n");
    return;
}

atomic_t k_interrupt_disable(void) {
    atomic_t key;
    __asm {
		MRS 	key, PRIMASK
		CPSID   I
    }
    return key;
}

void k_interrupt_enable(atomic_t key) {
    __asm {
		MSR 	PRIMASK, key
    }
}

/**
 *
 * @brief Atomic bitwise AND.
 *
 * This routine atomically sets @a target to the bitwise AND of @a target
 * and @a value.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to AND.
 *
 * @return Previous value of @a target.
 */
static inline atomic_t atomic_and(atomic_t *target, atomic_t value) {
#ifdef __CM_CMSIS_VERSION
    atomic_t prev_val;
    do {
        prev_val = __LDREXW((__IO uint32_t *)target);
    } while ((__STREXW(prev_val & (value), (__IO uint32_t *)target)) != 0);
    return prev_val;
#else
    return __atomic_fetch_and(target, value, __ATOMIC_SEQ_CST);
#endif
}

/**
 *
 * @brief Atomic bitwise inclusive OR.
 *
 * This routine atomically sets @a target to the bitwise inclusive OR of
 * @a target and @a value.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable.
 * @param value Value to OR.
 *
 * @return Previous value of @a target.
 */
static inline atomic_t atomic_or(atomic_t *target, atomic_t value) {
#ifdef __CM_CMSIS_VERSION
    atomic_t prev_val;
    do {
        prev_val = __LDREXW((__IO uint32_t *)target);
    } while ((__STREXW(prev_val | (value), (__IO uint32_t *)target)) != 0);
    return prev_val;
#else
    return __atomic_fetch_or(target, value, __ATOMIC_SEQ_CST);
#endif
}

/**
 * @brief Atomically set a bit.
 *
 * Atomically set bit number @a bit of @a target and return its old value.
 * The target may be a single atomic variable or an array of them.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable or array.
 * @param bit Bit number (starting from 0).
 *
 * @return true if the bit was set, false if it wasn't.
 */
bool atomic_test_and_set_bit(atomic_t *target, int bit) {
    atomic_t old;
    atomic_t mask = ATOMIC_MASK(bit);

    old = atomic_or(ATOMIC_ELEM(target, bit), mask);

    return (old & mask) != 0;
}

/**
 * @brief Atomically test and clear a bit.
 *
 * Atomically clear bit number @a bit of @a target and return its old value.
 * The target may be a single atomic variable or an array of them.
 *
 * @note As for all atomic APIs, includes a
 * full/sequentially-consistent memory barrier (where applicable).
 *
 * @param target Address of atomic variable or array.
 * @param bit Bit number (starting from 0).
 *
 * @return true if the bit was set, false if it wasn't.
 */
bool atomic_test_and_clear_bit(atomic_t *target, int bit) {
    atomic_t old;
    atomic_t mask = ATOMIC_MASK(bit);

    old = atomic_and(ATOMIC_ELEM(target, bit), ~mask);

    return (old & mask) != 0;
}

void atomic_clear_bit(atomic_t *target, int bit) {
    atomic_t mask = ATOMIC_MASK(bit);
    
    (void)atomic_and(ATOMIC_ELEM(target, bit), ~mask);
}

#define COUNTER_MAX       0x0000ffff
#define TIMER_STOPPED     0xffff0000

#define CYC_PER_TICK      (K_CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / K_CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_TICKS         (COUNTER_MAX / CYC_PER_TICK - 1)
#define MAX_CYCLES        (MAX_TICKS * CYC_PER_TICK)

/* Minimum nb of clock cycles to have to set autoreload register correctly */
#define LPTIM_GUARD_VALUE 1

static inline bool update_state_get(void) {
    return (LL_TIM_IsActiveFlag_UPDATE(TIM11) && LL_TIM_IsEnabledIT_UPDATE(TIM11));
}

static inline uint32_t clock_lptim_getcounter(void) {
    uint32_t lp_time;
    uint32_t lp_time_prev_read;

    /* It should be noted that to read reliably the content
     * of the LPTIM_CNT register, two successive read accesses
     * must be performed and compared
     */
    lp_time = LL_TIM_GetCounter(TIM11);
    do {
        lp_time_prev_read = lp_time;
        lp_time = LL_TIM_GetCounter(TIM11);
    } while (lp_time != lp_time_prev_read);
    return lp_time;
}

static inline uint32_t sys_clock_lp_time_get(void) {
    uint32_t lp_time;

    do {
        /* In case of counter roll-over, add the autoreload value,
         * because the irq has not yet been handled
         */
        if (update_state_get()) {
            lp_time = LL_TIM_GetAutoReload(TIM11) + 1;
            lp_time += clock_lptim_getcounter();
            break;
        }
        lp_time = clock_lptim_getcounter();

    } while (update_state_get());

    return lp_time;
}

uint32_t sys_clock_elapsed(void) {
#ifdef K_CONFIG_TICKLESS_KERNEL
    uint32_t lp_time = sys_clock_lp_time_get();

    /* gives the value of LPTIM counter (ms)
     * since the previous 'announce'
     */
    uint32_t ret = lp_time / CYC_PER_TICK;

    return (ret);
#else
    return 0;
#endif
}

void sys_clock_set_timeout(int32_t ticks, bool idle) {
#ifdef K_CONFIG_TICKLESS_KERNEL

    if (idle && ticks == K_TICKS_FOREVER) {
		/* clock_control_off */
		LL_TIM_DisableCounter(TIM11);
		return;
	}
    /* if LPTIM clock was previously stopped, it must now be restored */
    if (!LL_TIM_IsEnabledCounter(TIM11)) {
		LL_TIM_EnableCounter(TIM11);
	}

    uint32_t next_arr = 0;
    uint32_t lp_time = 0;
    uint32_t autoreload = 0;

    ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
    ticks = CLAMP(ticks - 1, 1, (int32_t)MAX_TICKS);
    lp_time = clock_lptim_getcounter();
    autoreload = LL_TIM_GetAutoReload(TIM11);

    if (update_state_get() || ((autoreload - lp_time) < LPTIM_GUARD_VALUE)) {
        /* interrupt happens or happens soon.
         * It's impossible to set autoreload value.
         */
        return;
    }
    /* calculate the next arr value (cannot exceed 16bit)
     * adjust the next ARR match value to align on Ticks
     * from the current counter value to first next Tick
     */
    next_arr = ((lp_time / CYC_PER_TICK) + 1) * CYC_PER_TICK;
    next_arr = next_arr + (uint32_t)ticks * CYC_PER_TICK;
    /* if the K_CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <  one ticks/sec, then next_arr must be > 0 */

    /* maximise to TIMEBASE */
    if (next_arr > MAX_TICKS) {
        next_arr = MAX_TICKS;
    }
    /* The new autoreload value must be LPTIM_GUARD_VALUE clock cycles
     * after current lptim to make sure we don't miss
     * an autoreload interrupt
     */
    else if (next_arr < (lp_time + LPTIM_GUARD_VALUE)) {
        next_arr = lp_time + LPTIM_GUARD_VALUE;
    }
    /* with slow lptim_clock_freq, LPTIM_GUARD_VALUE of 1 is enough */
	next_arr = next_arr - 1;
    
    /* The ARR register ready, we could set it directly */
    if ((next_arr > 0) && (next_arr != LL_TIM_GetAutoReload(TIM11))) {
        LL_TIM_SetAutoReload(TIM11, next_arr);
    }
#endif
}

/* Callout out of platform assembly, not hooked via IRQ_CONNECT... */
void sys_clock_isr(void) {
#ifdef K_CONFIG_TICKLESS_KERNEL
    uint32_t autoreload = LL_TIM_GetAutoReload(TIM11);

    if (update_state_get()) {
        /* do not change ARR yet, sys_clock_announce will do */
        LL_TIM_ClearFlag_UPDATE(TIM11);

        /* increase the total nb of autoreload count
         * used in the sys_clock_cycle_get_32() function.
         */
        autoreload++;

        /* announce the elapsed time in ms (count register is 16bit) */
        uint32_t dticks = autoreload / CYC_PER_TICK;

        // sys_clock_announce(IS_ENABLED(K_CONFIG_TICKLESS_KERNEL) ? dticks : (dticks > 0));
        sys_clock_announce(dticks);
    }
#else
    if (update_state_get()) {
        LL_TIM_ClearFlag_UPDATE(TIM11);
        sys_clock_announce(1);
    }
#endif
}
