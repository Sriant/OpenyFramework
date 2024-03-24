/*
 * @Date: 2024-03-12 17:14:30
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-18 23:53:59
 * @FilePath: \Openy_Framework\src\k_msgq.c
 */
#include "k_kernel.h"

#ifdef K_CONFIG_MSGQ

void k_msgq_init(k_msgq_t *msgq, char *buffer, size_t msg_size, uint32_t max_msgs) {
    msgq->msg_size = msg_size;
    msgq->max_msgs = max_msgs;
    msgq->buffer_start = buffer;
    msgq->buffer_end = buffer + (max_msgs * msg_size);
    msgq->read_ptr = buffer;
    msgq->write_ptr = buffer;
    msgq->used_msgs = 0;
    msgq->lock = 0;
}

int k_msgq_put(k_msgq_t *msgq, const void *data) {
    int result;

    /* lock */
    msgq->lock = k_interrupt_disable();

    if (msgq->used_msgs < msgq->max_msgs) {
        /* message queue isn't full */
        /* put message in queue */
        __ASSERT_NO_MSG(msgq->write_ptr >= msgq->buffer_start &&
                        msgq->write_ptr < msgq->buffer_end);
        (void)K_MEMCPY(msgq->write_ptr, data, msgq->msg_size);
        msgq->write_ptr += msgq->msg_size;
        if (msgq->write_ptr == msgq->buffer_end) {
            msgq->write_ptr = msgq->buffer_start;
        }
        msgq->used_msgs++;
        result = 0;
    } else {
        result = -ENOMEM;
    }

    /* unlock */
    k_interrupt_enable(msgq->lock);

    return result;
}

int k_msgq_get(k_msgq_t *msgq, void *data) {
    int result;

    /* lock */
    msgq->lock = k_interrupt_disable();
    if (msgq->used_msgs > 0U) {
        /* take first available message from queue */
        (void)K_MEMCPY(data, msgq->read_ptr, msgq->msg_size);
        msgq->read_ptr += msgq->msg_size;
        if (msgq->read_ptr == msgq->buffer_end) {
            msgq->read_ptr = msgq->buffer_start;
        }
        msgq->used_msgs--;

        result = 0;
    } else {
        result = -EINVAL;
    }
    /* unlock */
    k_interrupt_enable(msgq->lock);

    return result;
}

void k_msgq_purge(k_msgq_t *msgq) {
    /* lock */
    msgq->lock = k_interrupt_disable();
    msgq->used_msgs = 0;
    msgq->read_ptr = msgq->write_ptr;
    /* unlock */
    k_interrupt_enable(msgq->lock);
}

int k_msgq_peek(struct k_msgq *msgq, void *data) {
    int result;
    /* lock */
    msgq->lock = k_interrupt_disable();
    if (msgq->used_msgs > 0U) {
        /* take first available message from queue */
        (void)K_MEMCPY(data, msgq->read_ptr, msgq->msg_size);
        result = 0;
    } else {
        /* don't wait for a message to become available */
        result = -EINVAL;
    }
    /* unlock */
    k_interrupt_enable(msgq->lock);
    return result;
}

#endif
