/*
 * @Date: 2024-03-14 18:23:19
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-30 21:16:25
 * @FilePath: \Openy_Framework\src\k_timeout.c
 */
#include "k_kernel.h"

#ifndef INT_MAX
#define INT_MAX 0x7FFFFFFF
#endif

static sys_dlist_t sTimeoutList = SYS_DLIST_STATIC_INIT(&sTimeoutList);
static atomic_t sTimeoutLock = 0;
static int sAnnounceRemaining = 0;
static k_ticks_t sCurrTick = 0;

static inline int32_t elapsed(void) {
    /* While sys_clock_announce() is executing, new relative timeouts will be
     * scheduled relatively to the currently firing timeout's original tick
     * value (=curr_tick) rather than relative to the current
     * sys_clock_elapsed().
     *
     * This means that timeouts being scheduled from within timeout callbacks
     * will be scheduled at well-defined offsets from the currently firing
     * timeout.
     *
     * As a side effect, the same will happen if an ISR with higher priority
     * preempts a timeout callback and schedules a timeout.
     *
     * The distinction is implemented by looking at announce_remaining which
     * will be non-zero while sys_clock_announce() is executing and zero
     * otherwise.
     */
    return sAnnounceRemaining == 0 ? sys_clock_elapsed() : 0U;
}

static inline struct _timeout *first(void) {
    sys_dnode_t *t = sys_dlist_peek_head(&sTimeoutList);
    return t == NULL ? NULL : CONTAINER_OF(t, struct _timeout, node);
}

static inline struct _timeout *next(struct _timeout *t) {
    sys_dnode_t *n = sys_dlist_peek_next(&sTimeoutList, &t->node);
    return n == NULL ? NULL : CONTAINER_OF(n, struct _timeout, node);
}

static inline void remove_timeout(struct _timeout *t) {
    if (next(t) != NULL) {
        next(t)->dticks += t->dticks;
    }
    sys_dlist_remove(&t->node);
}

static inline int32_t next_timeout(void) {
    struct _timeout *to = first();
    int32_t ticks_elapsed = elapsed();
    int32_t ret;

    if ((to == NULL) || ((int32_t)(to->dticks - ticks_elapsed) > (int32_t)INT_MAX)) {
        ret = (int32_t)INT_MAX;
    } else {
        ret = MAX(0, to->dticks - ticks_elapsed);
    }
    return ret;
}

int k_timeout_abort(struct _timeout *to) {
    int ret = -EINVAL;
    sTimeoutLock = k_interrupt_disable();
    if (sys_dnode_is_linked(&to->node)) {
        remove_timeout(to);
        ret = 0;
    }
    k_interrupt_enable(sTimeoutLock);
    return ret;
}

void k_timeout_add(struct _timeout *to, _timeout_func_t fn, k_timeout_t timeout) {

    if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
        return;
    }

    sTimeoutLock = k_interrupt_disable();

    to->fn = fn;
    to->dticks = timeout.ticks + 1 + elapsed();

    struct _timeout *t = NULL;
    for (t = first(); t != NULL; t = next(t)) {
        if (t->dticks > to->dticks) {
            t->dticks -= to->dticks;
            sys_dlist_insert(&t->node, &to->node);
            break;
        }
        to->dticks -= t->dticks;
    }

    if (t == NULL) {
        sys_dlist_append(&sTimeoutList, &to->node);
    }

    if (to == first()) {
        sys_clock_set_timeout(next_timeout(), false);
    }
    k_interrupt_enable(sTimeoutLock);
}

k_ticks_t sys_clock_tick_get(void) {
	return sCurrTick;
}

/**
 * @description: It can only run in a timer ISR
 * 
 * Informs the kernel that the specified number of ticks have elapsed
 * since the last call to sys_clock_announce() (or system startup for
 * the first call).  The timer driver is expected to delivery these
 * announcements as close as practical (subject to hardware and
 * latency limitations) to tick boundaries.
 * 
 * @param {int32_t} ticks Elapsed time, in ticks
 * @return {*}
 */
void sys_clock_announce(int32_t ticks) {
    struct _timeout *t;
    sTimeoutLock = k_interrupt_disable();
    sAnnounceRemaining = ticks;
    for (t = first(); t && t->dticks <= sAnnounceRemaining; t = first()) {
        int dt = t->dticks;
        
        sCurrTick += dt;
        t->dticks = 0;
        remove_timeout(t);
        k_interrupt_enable(sTimeoutLock);
        t->fn(t);
        sTimeoutLock = k_interrupt_disable();
        sAnnounceRemaining -= dt;
    }

    if (t != NULL) {
		t->dticks -= sAnnounceRemaining;
	}

    sCurrTick += sAnnounceRemaining;
	sAnnounceRemaining = 0;

    sys_clock_set_timeout(next_timeout(), false);

    k_interrupt_enable(sTimeoutLock);
}

