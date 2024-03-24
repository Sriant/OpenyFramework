/*
 * @Author: yaosy fmusrr@foxmail.com
 * @Date: 2024-03-10 09:11:05
 * @LastEditors: yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-20 20:45:16
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#include "k_kernel.h"

#ifdef K_CONFIG_TIMER

static void timer_expiration_handler(struct _timeout *t) {
    struct k_timer *timer = CONTAINER_OF(t, struct k_timer, timeout);
    /*
	 * if the timer is periodic, start it again; don't add _TICK_ALIGN
	 * since we're already aligned to a tick boundary
	 */
    if (!K_TIMEOUT_EQ(timer->period, K_NO_WAIT) && !K_TIMEOUT_EQ(timer->period, K_FOREVER)) {
        k_timeout_t next = timer->period;

        next.ticks = MAX(next.ticks - 1, 0);
        k_timeout_add(&timer->timeout, timer_expiration_handler, next);
    }

    /* invoke timer expiry function */
	if (timer->expiry_fn != NULL) {
		timer->expiry_fn(timer);
	}
}

k_timer_t *k_timer_create(k_timer_expiry_t expiry_fn, void *user_data) {
    k_timer_t *timer = NULL;

    timer = (k_timer_t *)K_MALLOC(sizeof(k_timer_t));
    if (timer) {
        timer->expiry_fn = expiry_fn;
        timer->user_data = user_data;
        sys_dnode_init(&timer->timeout.node);
    }

    return timer;
}

void k_timer_init(k_timer_t *timer, k_timer_expiry_t expiry_fn, void *user_data) {
    timer->expiry_fn = expiry_fn;
    timer->user_data = user_data;
    sys_dnode_init(&timer->timeout.node);
}

void k_timer_start(k_timer_t *timer, k_timeout_t duration, k_timeout_t period) {

    (void)k_timeout_abort(&timer->timeout);
    timer->period = period;
    k_timeout_add(&timer->timeout, timer_expiration_handler, duration);
    
    return;
}

void k_timer_stop(k_timer_t *timer) {
    (void)k_timeout_abort(&timer->timeout);
    return;
}

#endif // K_CONFIG_TIMER
