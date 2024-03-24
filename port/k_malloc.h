/*
 * @Author: yaosy fmusrr@foxmail.com
 * @Date: 2024-03-07 22:39:29
 * @LastEditors: yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-03-07 23:13:40
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef __K_MALLOC_H__
#define __K_MALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <string.h>

#define _K_MALLOC
#ifdef _K_MALLOC
#define K_MALLOC(size_)            malloc(size_)
#define K_FREE(ptr_)               free(ptr_)
#define K_MEMCPY(des_, src_, len_) memcpy(des_, src_, len_)
#endif /* _K_MALLOC */

#ifdef __cplusplus
}
#endif

#endif /* __K_MALLOC_H__ */
