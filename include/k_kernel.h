/*
 * @Author: yaosy fmusrr@foxmail.com
 * @Date: 2024-03-07 23:24:49
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-23 22:40:52
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#ifndef __K_QUEUE_H
#define __K_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include "k_assert.h"
#include "k_log.h"
#include "k_malloc.h"

#include "k_utils.h"
#include "k_sflist.h"
#include "k_timeout.h"

#include "k_config.h"

#ifdef K_CONFIG_RINGBUFFER
#include "k_ring_buffer.h"
#endif // K_CONFIG_RINGBUFFER

/* system ticks, user implement */
void sys_clock_isr(void);
void sys_clock_set_timeout(int32_t ticks, bool idle);
uint32_t sys_clock_elapsed(void);

/* atomic operation, user implement */
typedef long atomic_t;
bool atomic_test_and_set_bit(atomic_t *target, int bit);
bool atomic_test_and_clear_bit(atomic_t *target, int bit);
void atomic_clear_bit(atomic_t *target, int bit);
atomic_t k_interrupt_disable(void);
void k_interrupt_enable(atomic_t key);

#ifdef K_CONFIG_QUEUE

#define K_QUEUE_INITIALIZER(obj) \
    { .data_q = SYS_SFLIST_STATIC_INIT(&obj.data_q), .lock = 0 }

typedef struct k_queue k_queue_t;
struct k_queue {
    sys_sflist_t data_q;
    atomic_t lock;
};

void *z_queue_node_peek(sys_sfnode_t *node, bool needs_free);
void k_queue_init(k_queue_t *queue);
void k_queue_insert(k_queue_t *queue, void *prev, void *data);
int32_t k_queue_alloc_insert(k_queue_t *queue, void *prev, void *data);
void k_queue_append(k_queue_t *queue, void *data);
int32_t k_queue_alloc_append(k_queue_t *queue, void *data);
void k_queue_prepend(k_queue_t *queue, void *data);
int32_t k_queue_alloc_prepend(k_queue_t *queue, void *data);
void *k_queue_get(k_queue_t *queue);
bool k_queue_remove(k_queue_t *queue, void *data);
int k_queue_is_empty(k_queue_t *queue);
void *k_queue_peek_head(k_queue_t *queue);
void *k_queue_peek_tail(k_queue_t *queue);

#endif // K_CONFIG_QUEUE

#ifdef K_CONFIG_TIMER

#define K_TIMER_INITIALIZER(obj, expiry, data)  \
    {                                           \
        .timeout =                              \
            {                                   \
                .node = {},                     \
                .fn = NULL,                     \
                .dticks = 0,                    \
            },                                  \
        .expiry_fn = expiry, .user_data = data, \
    }

/**
 * @brief Statically define and initialize a timer.
 *
 * The timer can be accessed outside the module where it is defined using:
 *
 * @code extern struct k_timer <name>; @endcode
 *
 * @param name Name of the timer variable.
 * @param expiry_fn Function to invoke each time the timer expires.
 * @param user_data User data to associate with the timer.
 */
#define K_TIMER_DEFINE(name, expiry_fn, user_data) \
    static k_timer_t name = K_TIMER_INITIALIZER(name, expiry_fn, user_data)

typedef struct k_timer k_timer_t;
typedef void (*k_timer_expiry_t)(k_timer_t *timer);

struct k_timer {
    /*
	 * _timeout structure must be first here if we want to use
	 * dynamic timer allocation. timeout.node is used in the Doubly-linked
	 * list of free timers
	 */
    struct _timeout timeout;
    k_timer_expiry_t expiry_fn;
    k_timeout_t period;
    void *user_data;
};

k_timer_t *k_timer_create(k_timer_expiry_t expiry_fn, void *user_data);
void k_timer_init(k_timer_t *timer, k_timer_expiry_t expiry_fn, void *user_data);
void k_timer_start(k_timer_t *timer, k_timeout_t duration, k_timeout_t period);
void k_timer_stop(k_timer_t *timer);

#endif // K_CONFIG_TIMER

#ifdef K_CONFIG_MSGQ

/**
 * @brief Statically define and initialize a message queue.
 *
 * The message queue's ring buffer contains space for @a q_max_msgs messages,
 * each of which is @a q_msg_size bytes long. The buffer is aligned to a
 * @a q_align -byte boundary, which must be a power of 2. To ensure that each
 * message is similarly aligned to this boundary, @a q_msg_size must also be
 * a multiple of @a q_align.
 *
 * The message queue can be accessed outside the module where it is defined
 * using:
 *
 * @code extern struct k_msgq <name>; @endcode
 *
 * @param q_name Name of the message queue.
 * @param q_msg_size Message size (in bytes).
 * @param q_max_msgs Maximum number of messages that can be queued.
 * @param q_align Alignment of the message queue's ring buffer.
 *
 */
#define K_MSGQ_DEFINE(q_name, q_msg_size, q_max_msgs, q_align) \
    static char __attribute__((__aligned__(q_align)))          \
    _k_fifo_buf_##q_name[(q_max_msgs) * (q_msg_size)];         \
    k_msgq_t q_name = K_MSGQ_INITIALIZER(_k_fifo_buf_##q_name, (q_msg_size), (q_max_msgs))

#define K_MSGQ_INITIALIZER(q_buffer, q_msg_size, q_max_msgs) \
	{ \
	    .msg_size = q_msg_size, \
	    .max_msgs = q_max_msgs, \
	    .buffer_start = q_buffer, \
	    .buffer_end = q_buffer + (q_max_msgs * q_msg_size), \
	    .read_ptr = q_buffer, \
	    .write_ptr = q_buffer, \
	    .used_msgs = 0, \
        .lock = 0, \
	}

typedef struct k_msgq k_msgq_t;

struct k_msgq {
    /** Message size */
    size_t msg_size;
    /** Maximal number of messages */
    uint32_t max_msgs;
    /** Start of message buffer */
    char *buffer_start;
    /** End of message buffer */
    char *buffer_end;
    /** Read pointer */
    char *read_ptr;
    /** Write pointer */
    char *write_ptr;
    /** Number of used messages */
    uint32_t used_msgs;
    atomic_t lock;
};

void k_msgq_init(k_msgq_t *msgq, char *buffer, size_t msg_size, uint32_t max_msgs);
int k_msgq_put(k_msgq_t *msgq, const void *data);
int k_msgq_get(k_msgq_t *msgq, void *data);
void k_msgq_purge(k_msgq_t *msgq);
int k_msgq_peek(struct k_msgq *msgq, void *data);

#endif // K_CONFIG_MSGQ

#ifdef K_CONFIG_WORKQ

#define K_WORK_USER_INITIALIZER(work_handler) \
    { .handler = work_handler, .flags = 0 }

typedef struct k_work_user k_work_user_t;
typedef struct k_work_delayable k_work_delayable_t;

struct k_work_user;
typedef void (*k_work_user_handler_t)(k_work_user_t *work);

struct k_work_user {
    k_work_user_handler_t handler;
    void *context;
    atomic_t flags;
};

struct k_work_delayable {
    struct _timeout timeout;
    k_work_user_t work;
};

int k_work_user_submit(k_work_user_t *work);
int k_work_schedule(k_work_delayable_t *dwork, k_timeout_t delay);
int k_work_user_wait(void);

#endif // K_CONFIG_WORKQ

#ifdef __cplusplus
}
#endif

#endif // __k_QUEUE_H
