/*
 * @Author: Yaosy fmusrr@foxmail.com
 * @Date: 2024-04-02 23:04:25
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-04-02 23:13:29
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include "app_event.h"

struct context_data {
    uint8_t id;
    char *name;
    uint32_t data;
    k_work_user_t work;
};

K_MSGQ_DEFINE(sEventMsgq, sizeof(AppEvent_t), 10, 4);

static void event_handler_test(const AppEvent_t *event) {
    if (event->type == TIMER) {
        struct context_data *ctx = (struct context_data *)event->TimerEvent.context;
        K_LOG_INFO("%s event hanlder", ctx->name);
    }
}

static void k_work_user_test_handler(struct k_work_user *work) {
    struct context_data *ctx = (struct context_data *)work->context;
    K_LOG_INFO("%s workhandler", ctx->name);
}

static void k_timer_test_callback(k_timer_t *timer) {
    struct context_data *ctx = (struct context_data *)timer->user_data;
    int err;

    /* work test, submit in ISR */
    ctx->data++;
    ctx->work.context = ctx;
    ctx->work.handler = k_work_user_test_handler;
    err = k_work_user_submit(&ctx->work);
    if (0 != err) {
        K_LOG_ERROR("k_work_user_submit failed %d! %s", err, ctx->name);
    }

    /* event messages test, put in ISR */
    AppEvent_t event;
    event.type = TIMER;
    event.handler = event_handler_test;
    event.TimerEvent.context = timer->user_data;
    err = k_msgq_put(&sEventMsgq, &event);
    if (0 != err) {
        K_LOG_ERROR("k_msgq_put failed %d! %s", err, ctx->name);
    }
}

void app_event_test(void) {
    static struct context_data sTestData[2] = {0};
    k_timer_t *timer[2];
    char *str[] = {"timer0", "timer1"};

    for (size_t i = 0; i < 2; i++) {
        sTestData[i].name = str[i];
        sTestData[i].id = i;
        timer[i] = k_timer_create(k_timer_test_callback, &sTestData[i]);
        if (NULL == timer[i]) {
            K_LOG_ERROR("timer1 create failed");
            return;
        }
    }

    k_timer_start(timer[0], K_MSEC(1000), K_MSEC(1000));
    k_timer_start(timer[1], K_MSEC(1000), K_MSEC(1000));

    K_LOG_INFO("timer start !!!");

    while (1) {
        /* main task */
        AppEvent_t event;
        if (0 == k_msgq_get(&sEventMsgq, &event)) {
            if (event.handler)
                event.handler(&event);
        }

        if (0 == k_work_user_wait()) {
            continue;
        } else {
            /* PM */
        }
    }
}
