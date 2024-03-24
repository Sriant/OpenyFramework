/*
 * @Author: yaosy fmusrr@foxmail.com
 * @Date: 2024-03-06 22:55:37
 * @LastEditors: yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-20 21:49:48
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include "k_kernel.h"

#ifdef K_CONFIG_QUEUE

struct alloc_node {
    sys_sfnode_t node;
    void *data;
};

void *z_queue_node_peek(sys_sfnode_t *node, bool needs_free) {
    void *ret;

    if ((node != NULL) && (sys_sfnode_flags_get(node) != (uint8_t)0)) {
        /* If the flag is set, then the enqueue operation for this item
         * did a behind-the scenes memory allocation of an alloc_node
         * struct, which is what got put in the queue. Free it and pass
         * back the data pointer.
         */
        struct alloc_node *anode;

        anode = CONTAINER_OF(node, struct alloc_node, node);
        ret = anode->data;
        if (needs_free) {
            K_FREE(anode);
        }
    } else {
        /* Data was directly placed in the queue, the first word
         * reserved for the linked list. User mode isn't allowed to
         * do this, although it can get data sent this way.
         */
        ret = (void *)node;
    }

    return ret;
}

/**
 * @brief Insert a node into a Zephyr kernel queue
 *
 * @param queue A pointer to the Zephyr kernel queue
 * @param prev A pointer to the previous node
 * @param data A pointer to the data to insert
 * @param alloc Whether to allocate memory for the new node
 * @param is_append Whether to append the node to the end of the queue
 *
 * @return 0 on success, negative error code on failure
 */
static int32_t queue_insert(k_queue_t *queue, void *prev, void *data, bool alloc, bool is_append) {
    
    queue->lock = k_interrupt_disable();
    
    if (is_append) {
        prev = sys_sflist_peek_tail(&queue->data_q);
    }

    /* Only need to actually allocate if no threads are pending */
    if (alloc) {
        struct alloc_node *anode;

        anode = K_MALLOC(sizeof(*anode));
        if (anode == NULL) {
            k_interrupt_enable(queue->lock);
            return -ENOMEM;
        }
        anode->data = data;
        sys_sfnode_init(&anode->node, 0x1);
        data = anode;
    } else {
        sys_sfnode_init(data, 0x0);
    }

    sys_sflist_insert(&queue->data_q, prev, data);
    
    k_interrupt_enable(queue->lock);

    return 0;
}

/**
 * @brief Cancel waiting on a queue.
 *
 * This routine causes first thread pending on @a queue, if any, to
 * return from k_queue_get() call with NULL value (as if timeout expired).
 * If the queue is being waited on by k_poll(), it will return with
 * -EINTR and K_POLL_STATE_CANCELLED state (and per above, subsequent
 * k_queue_get() will return NULL).
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 */
void k_queue_init(k_queue_t *queue) { 
    queue->lock = 0;
    sys_sflist_init(&queue->data_q); 
}

/**
 * @brief Inserts an element to a queue.
 *
 * This routine inserts a data item to @a queue after previous item. A queue
 * data item must be aligned on a word boundary, and the first word of
 * the item is reserved for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param prev Address of the previous data item.
 * @param data Address of the data item.
 */
void k_queue_insert(k_queue_t *queue, void *prev, void *data) {
    (void)queue_insert(queue, prev, data, false, false);
}

/**
 * @brief Inserts an element to a queue.
 *
 * This routine appends a data item to @a queue. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param prev Address of the previous data item.
 * @param data Address of the data item.
 * 
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
int32_t k_queue_alloc_insert(k_queue_t *queue, void *prev, void *data) {
    int32_t ret = queue_insert(queue, prev, data, true, false);
    return ret;
}

/**
 * @brief Append an element to the end of a queue.
 *
 * This routine appends a data item to @a queue. A queue data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 */
void k_queue_append(k_queue_t *queue, void *data) {
    (void)queue_insert(queue, NULL, data, false, true);
}

/**
 * @brief Inserts an element to a queue.
 *
 * This routine inserts a data item to @a queue after previous item. A queue
 * data item must be aligned on a word boundary, and the first word of
 * the item is reserved for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param prev Address of the previous data item.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
int32_t k_queue_alloc_append(k_queue_t *queue, void *data) {
    int32_t ret = queue_insert(queue, NULL, data, true, true);
    return ret;
}

/**
 * @brief Prepend an element to a queue.
 *
 * This routine prepends a data item to @a queue. A queue data item must be
 * aligned on a word boundary, and the first word of the item is reserved
 * for the kernel's use.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 */
void k_queue_prepend(k_queue_t *queue, void *data) {
    (void)queue_insert(queue, NULL, data, false, false);
}

/**
 * @brief Prepend an element to a queue.
 *
 * This routine prepends a data item to @a queue. There is an implicit memory
 * allocation to create an additional temporary bookkeeping data structure from
 * the calling thread's resource pool, which is automatically freed when the
 * item is removed. The data itself is not copied.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @retval 0 on success
 * @retval -ENOMEM if there isn't sufficient RAM in the caller's resource pool
 */
int32_t k_queue_alloc_prepend(k_queue_t *queue, void *data) {
    int32_t ret = queue_insert(queue, NULL, data, true, false);
    return ret;
}

/**
 * @brief Get an element from a queue.
 *
 * This routine removes first data item from @a queue. The first word of the
 * data item is reserved for the kernel's use.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param timeout Non-negative waiting period to obtain a data item
 *                or one of the special values K_NO_WAIT and
 *                K_FOREVER.
 *
 * @return Address of the data item if successful; NULL if returned
 * without waiting, or waiting period timed out.
 */
void *k_queue_get(k_queue_t *queue) {
    void *data = NULL;

    queue->lock = k_interrupt_disable();
    if (!sys_sflist_is_empty(&queue->data_q)) {
        sys_sfnode_t *node;

        node = sys_sflist_get_not_empty(&queue->data_q);
        data = z_queue_node_peek(node, true);
    }
    k_interrupt_enable(queue->lock);
    return data;
}

/**
 * @brief Remove an element from a queue.
 *
 * This routine removes data item from @a queue. The first word of the
 * data item is reserved for the kernel's use. Removing elements from k_queue
 * rely on sys_slist_find_and_remove which is not a constant time operation.
 *
 * @note @a timeout must be set to K_NO_WAIT if called from ISR.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 * @param data Address of the data item.
 *
 * @return true if data item was removed
 */
bool k_queue_remove(k_queue_t *queue, void *data) {
    bool ret = sys_sflist_find_and_remove(&queue->data_q, (sys_sfnode_t *)data);
    return ret;
}

/**
 * @brief Query a queue to see if it has data available.
 *
 * Note that the data might be already gone by the time this function returns
 * if other threads are also trying to read from the queue.
 *
 * @funcprops \isr_ok
 *
 * @param queue Address of the queue.
 *
 * @return Non-zero if the queue is empty.
 * @return 0 if data is available.
 */
int k_queue_is_empty(k_queue_t *queue) { return (int)sys_sflist_is_empty(&queue->data_q); }

/**
 * @brief Peek element at the head of queue.
 *
 * Return element from the head of queue without removing it.
 *
 * @param queue Address of the queue.
 *
 * @return Head element, or NULL if queue is empty.
 */
void *k_queue_peek_head(k_queue_t *queue) {
    void *ret = z_queue_node_peek(sys_sflist_peek_head(&queue->data_q), false);
    return ret;
}

/**
 * @brief Peek element at the tail of queue.
 *
 * Return element from the tail of queue without removing it.
 *
 * @param queue Address of the queue.
 *
 * @return Tail element, or NULL if queue is empty.
 */
void *k_queue_peek_tail(k_queue_t *queue) {
    void *ret = z_queue_node_peek(sys_sflist_peek_tail(&queue->data_q), false);
    return ret;
}

#endif // K_CONFIG_QUEUE
