/*
 * @Author: Yaosy fmusrr@foxmail.com
 * @Date: 2024-04-02 21:21:27
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-04-05 21:07:51
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include "k_hsm.h"
#include "k_kernel.h"

enum BlinkySignals {
    DUMMY_SIG = K_USER_SIG,
    MAX_PUB_SIG, // the last published signal

    TODO_SIG,
    TIMEOUT_SIG,

    MAX_SIG // the last signal
};

typedef struct {
    k_hsm_t active;
    k_mevt_t event;
    k_timer_t timer;
} Blinky;

Blinky sBlinky_inst;

static k_state Blinky_off(Blinky *const me, k_mevt_t const *const e);
static k_state Blinky_on(Blinky *const me, k_mevt_t const *const e);

static void blinky_timer_callback(k_timer_t *timer) {
    Blinky *me = (Blinky *)timer->user_data;

    me->event.sig = TIMEOUT_SIG;
    k_mevt_post(&me->active, &me->event);
}

static k_state Blinky_initial(Blinky *const me, void const *const par) {
    ARG_UNUSED(me);
    ARG_UNUSED(par);
    /* TODO */
    k_timer_init(&me->timer, blinky_timer_callback, me);
    k_timer_start(&me->timer, K_MSEC(500), K_MSEC(500));
    return K_TRAN(&Blinky_off);
}

static k_state Blinky_off(Blinky *const me, k_mevt_t const *const e) {
    k_state status;

    switch (e->sig) {
    case K_ENTRY_SIG:
        me->event.sig = TODO_SIG;
        k_mevt_post(&me->active, &me->event);
        K_LOG_INFO("Blinky_off K_ENTRY_SIG");
        status = K_RET_HANDLED;
        break;

    case K_EXIT_SIG:
        K_LOG_INFO("Blinky_off K_EXIT_SIG");
        status = K_RET_HANDLED;
        break;

    case TODO_SIG:
        K_LOG_INFO("Blinky_off TODO_SIG");
        status = K_RET_HANDLED;
        break;

    case TIMEOUT_SIG:
        K_LOG_INFO("Blinky_off TIMEOUT_SIG");
        status = K_TRAN(&Blinky_on);
        break;

    default: status = K_SUPER(&k_hsm_top); break;
    }
    return status;
}

static k_state Blinky_on(Blinky *const me, k_mevt_t const *const e) {
    k_state status;

    switch (e->sig) {
    case K_ENTRY_SIG:
        me->event.sig = TODO_SIG;
        k_mevt_post(&me->active, &me->event);
        K_LOG_INFO(" Blinky_on K_ENTRY_SIG");
        status = K_RET_HANDLED;
        break;

    case K_EXIT_SIG:
        K_LOG_INFO(" Blinky_on K_EXIT_SIG");
        status = K_RET_HANDLED;
        break;

    case TODO_SIG:
        K_LOG_INFO(" Blinky_on TODO_SIG");
        status = K_RET_HANDLED;
        break;

    case TIMEOUT_SIG:
        K_LOG_INFO(" Blinky_on TIMEOUT_SIG");
        status = K_TRAN(&Blinky_off);
        break;

    default: status = K_SUPER(&k_hsm_top); break;
    }
    return status;
}

void Blinky_test(void) {
    K_LOG_INFO("Blinky hsm test!!!");

    Blinky *const me = &sBlinky_inst;
    static k_mevt_t const *blinkyEvtQueue[10];

    k_hsm_constructor(&me->active, K_STATE_CAST(&Blinky_initial));
    k_msgq_init(&me->active.eQueue, (char *)blinkyEvtQueue, sizeof(k_mevt_t *), 10);
    me->active.super.init(&me->active.super, NULL);

    while (true) {
        k_mevt_t const *event = k_mevt_get(&me->active);
        if (event) {
            me->active.super.dispatch(&me->active.super, event);
        }
    }
}
