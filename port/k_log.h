/*
 * @Author: yaosy fmusrr@foxmail.com
 * @Date: 2024-03-06 21:55:58
 * @LastEditors: yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-08 00:55:13
 * @Description:
 *
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved.
 */
#ifndef __K_LOG_H
#define __K_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>

#define K_LOG_OUTPUT_STREAM stdout

#define K_LOG_LEVEL_INFO    0
#define K_LOG_LEVEL_DEBUG   1
#define K_LOG_LEVEL_ERROR   2

void k_print(int level, const char *fmt, ...);

#define K_LOG_INFO(format, ...)  k_print(K_LOG_LEVEL_INFO, (format), ##__VA_ARGS__)
#define K_LOG_DEBUG(format, ...) k_print(K_LOG_LEVEL_DEBUG, (format), ##__VA_ARGS__)
#define K_LOG_ERROR(format, ...) k_print(K_LOG_LEVEL_ERROR, (format), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // __K_LOG_H
