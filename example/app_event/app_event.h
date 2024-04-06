/*
 * @Author: Yaosy fmusrr@foxmail.com
 * @Date: 2024-04-02 23:04:30
 * @LastEditors: Yaosy fmusrr@foxmail.com
 * @LastEditTime: 2024-04-02 23:07:12
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef __APP_EVENT_H
#define __APP_EVENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "k_kernel.h"

typedef struct AppEvent AppEvent_t;
typedef void (*AppEventHandler_t) (const AppEvent_t* event);

enum AppEventType {
	NONE = 0,
	TIMER,
};

struct AppEvent {
	union {
		struct {
			void *context;
		} TimerEvent;
	};
	
	enum AppEventType type;
	AppEventHandler_t handler;
};

void app_event_test(void);

#ifdef __cplusplus
}
#endif

#endif // __APP_EVENT_H
