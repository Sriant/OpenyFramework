/*
 * @Author: yaosy fmusrr@foxmail.com
 * @Date: 2024-03-14 20:53:56
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-15 21:46:12
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef __K_CONFIG_H
#define __K_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#define K_CONFIG_TICKLESS_KERNEL

#define K_CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC    2000
#define K_CONFIG_SYS_CLOCK_TICKS_PER_SEC        2000

#define K_CONFIG_QUEUE
#define K_CONFIG_RINGBUFFER
#define K_CONFIG_TIMER
#define K_CONFIG_MSGQ
#define K_CONFIG_WORKQ

#ifdef __cplusplus
}
#endif

#endif  // __K_CONFIG_H
