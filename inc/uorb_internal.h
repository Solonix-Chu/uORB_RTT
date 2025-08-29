/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#ifndef __UORB_INTERNAL_H__
#define __UORB_INTERNAL_H__

#include "uORB.h"
#include <rtthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 轻量锁封装 */
typedef struct uorb_lock_s {
    rt_mutex_t handle;
} uorb_lock_t;

int  uorb_lock_init(uorb_lock_t *lock, const char *name);
void uorb_lock_deinit(uorb_lock_t *lock);
void uorb_lock_acquire(uorb_lock_t *lock);
void uorb_lock_release(uorb_lock_t *lock);

/* 订阅通知：基于事件的更新通知抽象 */
#define UORB_EVENT_UPDATE 0x01

typedef struct uorb_notifier_s {
    rt_event_t event;
} uorb_notifier_t;

int  uorb_notifier_init(uorb_notifier_t *notifier, const char *name);
void uorb_notifier_deinit(uorb_notifier_t *notifier);
int  uorb_notifier_wait(uorb_notifier_t *notifier, rt_int32_t timeout_ms);
void uorb_notifier_notify(uorb_notifier_t *notifier);

/* 全局/列表级别的内部锁（用于节点列表、全局资源） */
uorb_lock_t *uorb_get_global_lock(void);

/* 时间与工具函数 */
rt_tick_t    uorb_tick_now(void);
rt_uint32_t  uorb_tick_from_ms(rt_uint32_t ms);
size_t       uorb_min_size(size_t a, size_t b);

#ifdef __cplusplus
}
#endif

#endif /* __UORB_INTERNAL_H__ */ 