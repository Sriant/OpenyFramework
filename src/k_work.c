/*
 * @Date: 2024-03-12 08:51:30
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-23 22:41:58
 * @FilePath: \Openy_Framework\Openy_Framework\src\k_work.c
 */
#include "k_kernel.h"

#ifdef K_CONFIG_WORKQ

static struct k_queue sWork_q;

/* Timeout handler for delayable work.
 *
 * Invoked by timeout infrastructure.
 * Takes and releases work lock.
 * Conditionally reschedules.
 */
static void work_timeout(struct _timeout *to) {
    struct k_work_delayable *dwork = CONTAINER_OF(to, struct k_work_delayable, timeout);
    (void)k_work_user_submit(&dwork->work);
}

int k_work_user_submit(k_work_user_t *work) {
    int ret = -EINVAL;

    if (!atomic_test_and_set_bit(&work->flags, 0)) {
        ret = k_queue_alloc_append(&sWork_q, work);

        /* Couldn't insert into the queue. Clear the pending bit
         * so the work item can be submitted again
         */
        if (ret != 0) {
            atomic_clear_bit(&work->flags, 0);
        }
    }

    return ret;
}

int k_work_schedule(k_work_delayable_t *dwork, k_timeout_t delay) {
    if (K_TIMEOUT_EQ(delay, K_NO_WAIT)) {
        return k_work_user_submit(&dwork->work);
    }
    (void)k_timeout_abort(&dwork->timeout);
    k_timeout_add(&dwork->timeout, work_timeout, delay);

    return 0;
}

int k_work_user_wait(void) {
    k_work_user_t *work;
    k_work_user_handler_t handler;

    work = k_queue_get(&sWork_q);
    if (work == NULL) {
        return -EINVAL;
    }

    handler = work->handler;
    __ASSERT(handler != NULL, "handler must be provided");

    /* Reset pending state so it can be resubmitted by handler */
    if (atomic_test_and_clear_bit(&work->flags, 0)) {
        handler(work);
        return 0;
    }
    return -EINVAL;
}


#endif // K_CONFIG_WORKQ
