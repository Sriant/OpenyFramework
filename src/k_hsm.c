/*
 * @Author: Yaosy fmusrr@foxmail.com
 * @Date: 2024-03-30 17:57:07
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-04-06 15:30:19
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#include "k_hsm.h"

// maximum depth of state nesting in a k_hsm (including the top level),
// must be >= 3
#define K_HSM_MAX_NEST_DEPTH 6

k_mevt_t const sEventReserved[4] = {K_MEVT_INITIALIZER(K_EMPTY_SIG),
                                    K_MEVT_INITIALIZER(K_ENTRY_SIG), 
                                    K_MEVT_INITIALIZER(K_EXIT_SIG),
                                    K_MEVT_INITIALIZER(K_INIT_SIG)};

#define K_HSM_RESERVED_EVT(state_, sig_) ((*(state_))(me, &sEventReserved[(sig_)]))

static int_fast8_t hsm_transfer(k_asm_t *const me, k_stateHandler_t *const path) {
    int_fast8_t ip = -1; // tran. entry path index
    k_stateHandler_t t = path[0];
    k_stateHandler_t const s = path[2];

    // (a) check source==target (tran. to self)...
    if (s == t) {
        // exit source s
        (void)K_HSM_RESERVED_EVT(s, K_EXIT_SIG);
        ip = 0; // enter the target
    } else {
        // find superstate of target
        (void)K_HSM_RESERVED_EVT(t, K_EMPTY_SIG);

        t = me->temp;

        // (b) check source==target->super...
        if (s == t) {
            ip = 0; // enter the target
        } else {
            // find superstate of src
            (void)K_HSM_RESERVED_EVT(s, K_EMPTY_SIG);

            // (c) check source->super==target->super...
            if (me->temp == t) {
                // exit source s
                (void)K_HSM_RESERVED_EVT(s, K_EXIT_SIG);
                ip = 0; // enter the target
            } else {
                // (d) check source->super==target...
                if (me->temp == path[0]) {
                    // exit source s
                    (void)K_HSM_RESERVED_EVT(s, K_EXIT_SIG);
                } else {
                    // (e) check rest of source==target->super->super..
                    // and store the entry path along the way
                    int_fast8_t iq = 0; // indicate that LCA was found
                    ip = 1;             // enter target and its superstate
                    path[1] = t;        // save the superstate of target
                    t = me->temp;       // save source->super

                    // find target->super->super...
                    k_state r = K_HSM_RESERVED_EVT(path[1], K_EMPTY_SIG);
                    while ((r == K_RET_SUPER) && (ip < (K_HSM_MAX_NEST_DEPTH - 1))) {
                        ++ip;
                        path[ip] = me->temp;   // store the entry path
                        if (me->temp == s) {   // is it the source?
                            iq = 1;            // indicate that the LCA found
                            --ip;              // do not enter the source
                            r = K_RET_HANDLED; // terminate the loop
                        } else {               // it is not the source, keep going up
                            r = K_HSM_RESERVED_EVT(me->temp, K_EMPTY_SIG);
                        }
                    }

                    // the LCA not found yet?
                    if (iq == 0) {
                        // exit source s
                        (void)K_HSM_RESERVED_EVT(s, K_EXIT_SIG);

                        // (f) check the rest of source->super
                        //                  == target->super->super...
                        iq = ip;
                        r = K_RET_IGNORED; // indicate that the LCA NOT found
                        do {
                            if (t == path[iq]) {   // is this the LCA?
                                r = K_RET_HANDLED; // indicate the LCA found
                                ip = iq - 1;       // do not enter the LCA
                                iq = -1;           // cause termination of the loop
                            } else {
                                --iq; // try lower superstate of target
                            }
                        } while (iq >= 0);

                        // the LCA not found yet?
                        if (r != K_RET_HANDLED) {
                            // (g) check each source->super->...
                            // for each target->super...
                            r = K_RET_IGNORED; // keep looping
                            int_fast8_t limit = K_HSM_MAX_NEST_DEPTH;
                            do {
                                // exit from t
                                if (K_HSM_RESERVED_EVT(t, K_EXIT_SIG) == K_RET_HANDLED) {
                                    // find superstate of t
                                    (void)K_HSM_RESERVED_EVT(t, K_EMPTY_SIG);
                                }
                                t = me->temp; // set to super of t
                                iq = ip;
                                do {
                                    // is this the LCA?
                                    if (t == path[iq]) {
                                        ip = iq - 1;       // do not enter the LCA
                                        iq = -1;           // break out of inner loop
                                        r = K_RET_HANDLED; // break outer loop
                                    } else {
                                        --iq;
                                    }
                                } while (iq >= 0);

                                --limit;
                            } while ((r != K_RET_HANDLED) && (limit > 0));
                        }
                    }
                }
            }
        }
    }

    return ip;
}

static void hsm_init(k_asm_t *const me, void const *const e) {
    k_stateHandler_t t = me->state;

    // execute the top-most initial tran.
    k_state r = (*me->temp)(me, e);

    // drill down into the state hierarchy with initial transitions...
    int_fast8_t limit = K_HSM_MAX_NEST_DEPTH; // loop hard limit
    do {
        k_stateHandler_t path[K_HSM_MAX_NEST_DEPTH]; // tran entry path array
        int_fast8_t ip = 0;                          // tran entry path index

        path[0] = me->temp;
        (void)K_HSM_RESERVED_EVT(me->temp, K_EMPTY_SIG);
        while ((me->temp != t) && (ip < (K_HSM_MAX_NEST_DEPTH - 1))) {
            ++ip;
            path[ip] = me->temp;
            (void)K_HSM_RESERVED_EVT(me->temp, K_EMPTY_SIG);
        }

        me->temp = path[0];

        // retrace the entry path in reverse (desired) order...
        do {
            // enter path[ip]
            (void)K_HSM_RESERVED_EVT(path[ip], K_ENTRY_SIG);
            --ip;
        } while (ip >= 0);

        t = path[0]; // current state becomes the new source

        r = K_HSM_RESERVED_EVT(t, K_INIT_SIG); // execute initial tran.

        --limit;
    } while ((r == K_RET_TRAN) && (limit > 0));

    me->state = t; // change the current active state
}

static void hsm_dispatch(k_asm_t *const me, k_mevt_t const *const e) {
    k_stateHandler_t s = me->state;
    k_stateHandler_t t = s;

    // process the event hierarchically...
    k_state r;
    me->temp = s;
    int_fast8_t limit = K_HSM_MAX_NEST_DEPTH; // loop hard limit
    do {
        s = me->temp;
        r = (*s)(me, e); // invoke state handler s

        if (r == K_RET_UNHANDLED) { // unhandled due to a guard?
            r = K_HSM_RESERVED_EVT(s, K_EMPTY_SIG); // superstate of s
        }

        --limit;
    } while ((r == K_RET_SUPER) && (limit > 0));

    if (r >= K_RET_TRAN) { // regular tran. taken?
        k_stateHandler_t path[K_HSM_MAX_NEST_DEPTH];

        path[0] = me->temp; // tran. target
        path[1] = t;        // current state
        path[2] = s;        // tran. source

        // exit current state to tran. source s...
        limit = K_HSM_MAX_NEST_DEPTH; // loop hard limit
        for (; (t != s) && (limit > 0); t = me->temp) {
            // exit from t
            if (K_HSM_RESERVED_EVT(t, K_EXIT_SIG) == K_RET_HANDLED) {
                // find superstate of t
                (void)K_HSM_RESERVED_EVT(t, K_EMPTY_SIG);
            }
            --limit;
        }

        int_fast8_t ip = hsm_transfer(me, path); // take the tran.

        // execute state entry actions in the desired order...
        for (; ip >= 0; --ip) {
            // enter path[ip]
            (void)K_HSM_RESERVED_EVT(path[ip], K_ENTRY_SIG);
        }
        t = path[0];  // stick the target into register
        me->temp = t; // update the next state

        // drill into the target hierarchy...
        while (K_HSM_RESERVED_EVT(t, K_INIT_SIG) == K_RET_TRAN) {
            ip = 0;
            path[0] = me->temp;

            // find superstate
            (void)K_HSM_RESERVED_EVT(me->temp, K_EMPTY_SIG);

            while ((me->temp != t) && (ip < (K_HSM_MAX_NEST_DEPTH - 1))) {
                ++ip;
                path[ip] = me->temp;
                // find superstate
                (void)K_HSM_RESERVED_EVT(me->temp, K_EMPTY_SIG);
            }

            me->temp = path[0];

            // retrace the entry path in reverse (correct) order...
            do {
                // enter path[ip]
                (void)K_HSM_RESERVED_EVT(path[ip], K_ENTRY_SIG);
                --ip;
            } while (ip >= 0);

            t = path[0]; // current state becomes the new source
        }
    }

    me->state = t; // change the current active state
}

static bool hsm_isIn(k_asm_t *const me, k_stateHandler_t const state) {

    bool inState = false; // assume that this HSM is not in 'state'

    // scan the state hierarchy bottom-up
    k_stateHandler_t s = me->state;
    int_fast8_t limit = K_HSM_MAX_NEST_DEPTH + 1; // loop hard limit
    k_state r = K_RET_SUPER;
    for (; (r != K_RET_IGNORED) && (limit > 0); --limit) {
        if (s == state) {   // do the states match?
            inState = true; // 'true' means that match found
            break;          // break out of the for-loop
        } else {
            r = K_HSM_RESERVED_EVT(s, K_EMPTY_SIG);
            s = me->temp;
        }
    }

    return inState; // return the status
}

k_state k_hsm_top(k_hsm_t const *const me, k_mevt_t const *const e) {
    ARG_UNUSED(me);
    ARG_UNUSED(e);
    return K_RET_IGNORED; // the top state ignores all events
}

void k_hsm_constructor(k_hsm_t *const me, k_stateHandler_t const initial) {
    me->super.init = hsm_init;
    me->super.dispatch = hsm_dispatch;
    me->super.isIn = hsm_isIn;
    me->super.state = K_STATE_CAST(&k_hsm_top);
    me->super.temp = initial;
}

bool k_mevt_post(k_hsm_t *const me, k_mevt_t const *const e) {

    uint_fast16_t nFree = me->eQueue.max_msgs - me->eQueue.used_msgs;

    bool status;

    if (nFree > 0) { // can post the event?
        int err = k_msgq_put(&me->eQueue, (void const *)&e);
        status = err == 0 ? true : false;
    }

    return status;
}

k_mevt_t const *k_mevt_get(k_hsm_t *const me) {
    // wait for an event (forever)
    k_mevt_t const *e;
    int err = k_msgq_get(&me->eQueue, (void *)&e);
    if (err) {
        return NULL;
    }
    return e;
}
