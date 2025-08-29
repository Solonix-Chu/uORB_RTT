/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include "uorb_internal.h"
#include <rtthread.h>
#include <string.h>

int uorb_lock_init(uorb_lock_t *lock, const char *name)
{
    if (!lock) return -RT_ERROR;
    lock->handle = rt_mutex_create(name ? name : "uorb_lock", RT_IPC_FLAG_PRIO);
    return lock->handle ? RT_EOK : -RT_ERROR;
}

void uorb_lock_deinit(uorb_lock_t *lock)
{
    if (!lock || !lock->handle) return;
    rt_mutex_delete(lock->handle);
    lock->handle = RT_NULL;
}

void uorb_lock_acquire(uorb_lock_t *lock)
{
    if (!lock || !lock->handle) return;
    rt_mutex_take(lock->handle, RT_WAITING_FOREVER);
}

void uorb_lock_release(uorb_lock_t *lock)
{
    if (!lock || !lock->handle) return;
    rt_mutex_release(lock->handle);
}

static uorb_lock_t g_uorb_global_lock;
static int g_uorb_global_lock_inited = 0;

uorb_lock_t *uorb_get_global_lock(void)
{
    if (!g_uorb_global_lock_inited)
    {
        if (uorb_lock_init(&g_uorb_global_lock, "uorb_g") == RT_EOK)
        {
            g_uorb_global_lock_inited = 1;
        }
    }
    return g_uorb_global_lock_inited ? &g_uorb_global_lock : RT_NULL;
}

int uorb_notifier_init(uorb_notifier_t *notifier, const char *name)
{
    if (!notifier) return -RT_ERROR;
    notifier->event = rt_event_create(name ? name : "uorb_evt", RT_IPC_FLAG_PRIO);
    return notifier->event ? RT_EOK : -RT_ERROR;
}

void uorb_notifier_deinit(uorb_notifier_t *notifier)
{
    if (!notifier || !notifier->event) return;
    rt_event_delete(notifier->event);
    notifier->event = RT_NULL;
}

int uorb_notifier_wait(uorb_notifier_t *notifier, rt_int32_t timeout_ms)
{
    if (!notifier || !notifier->event) return -RT_ERROR;
    rt_uint32_t recved;
    rt_tick_t   ticks = (timeout_ms < 0) ? RT_WAITING_FOREVER : rt_tick_from_millisecond(timeout_ms);
    return rt_event_recv(notifier->event, UORB_EVENT_UPDATE, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, ticks, &recved);
}

void uorb_notifier_notify(uorb_notifier_t *notifier)
{
    if (!notifier || !notifier->event) return;
    rt_event_send(notifier->event, UORB_EVENT_UPDATE);
}

rt_tick_t uorb_tick_now(void)
{
    return rt_tick_get();
}

rt_uint32_t uorb_tick_from_ms(rt_uint32_t ms)
{
    return rt_tick_from_millisecond(ms);
}

size_t uorb_min_size(size_t a, size_t b)
{
    return a < b ? a : b;
} 