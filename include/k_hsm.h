/*
 * @Author: Yaosy fmusrr@foxmail.com
 * @Date: 2024-03-30 17:56:57
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-04-05 20:40:29
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#ifndef __K_HSM_H
#define __K_HSM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "k_kernel.h"

typedef uint8_t k_signal;
typedef enum k_state_ret k_state;
typedef struct k_asm k_asm_t;
typedef struct k_mevt k_mevt_t;

typedef k_state (*k_stateHandler_t)(void *const me, k_mevt_t const *const e);

enum k_state_ret {
    // unhandled and need to "bubble up"
    K_RET_SUPER,     
    K_RET_UNHANDLED, 
    // handled and do not need to "bubble up"
    K_RET_HANDLED, 
    K_RET_IGNORED, 
    // entry/exit
    K_RET_ENTRY,
    K_RET_EXIT,  
    // no side effects
    K_RET_NULL,
    // transitions need to execute transition-action table
    K_RET_TRAN, 
};

struct k_mevt {
    k_signal sig;
    uint8_t flag;
};

enum k_reserved_sig { K_EMPTY_SIG, K_ENTRY_SIG, K_EXIT_SIG, K_INIT_SIG, K_USER_SIG };

struct k_asm {
    k_stateHandler_t state;
    k_stateHandler_t temp;

    void (*init)(k_asm_t *const me, void const *const e);
    void (*dispatch)(k_asm_t *const me, k_mevt_t const *const e);
    bool (*isIn)(k_asm_t *const me, k_stateHandler_t const s);
};

typedef struct {
    k_asm_t super;
    k_msgq_t eQueue;
} k_hsm_t;

#define K_MEVT_INITIALIZER(sig_)                                                                   \
    { (k_signal)(sig_), 0U }
#define K_STATE_CAST(handler_) ((k_stateHandler_t)(handler_))
#define K_ASM_UPCAST(ptr_)     ((k_asm_t *)(ptr_))
#define K_SUPER(super_)        ((K_ASM_UPCAST(me))->temp = K_STATE_CAST(super_), (k_state)K_RET_SUPER)
#define K_TRAN(target_)        ((K_ASM_UPCAST(me))->temp = K_STATE_CAST(target_), (k_state)K_RET_TRAN)

k_state k_hsm_top(k_hsm_t const *const me, k_mevt_t const *const e);
void k_hsm_constructor(k_hsm_t *const me, k_stateHandler_t const initial);
bool k_mevt_post(k_hsm_t *const me, k_mevt_t const *const e);
k_mevt_t const *k_mevt_get(k_hsm_t *const me);

#ifdef __cplusplus
}
#endif

#endif
