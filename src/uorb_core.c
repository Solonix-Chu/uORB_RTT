/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include "uORB.h"
#include "uorb_device_node.h"
#include "uorb_internal.h"
#include <rtthread.h>

/*
 * 核心说明：
 * 保持与 PX4 uORB 用法一致：
 *  - orb_advertise / orb_advertise_queue / orb_advertise_multi
 *  - orb_subscribe（基于 orb_subscribe_multi(…, 0)）
 *  - orb_exists / orb_group_count
 *  - orb_set_interval / orb_get_interval
 */

/* -------------------------------------- */
/* Advertise wrappers                      */
/* -------------------------------------- */

orb_advert_t orb_advertise(const struct orb_metadata *meta, const void *data)
{
    return orb_advertise_multi_queue(meta, data, RT_NULL, 1);
}

orb_advert_t orb_advertise_queue(const struct orb_metadata *meta, const void *data, unsigned int queue_size)
{
    return orb_advertise_multi_queue(meta, data, RT_NULL, queue_size);
}

orb_advert_t orb_advertise_multi(const struct orb_metadata *meta, const void *data, int *instance)
{
    return orb_advertise_multi_queue(meta, data, instance, 1);
}

/* -------------------------------------- */
/* Subscribe wrappers                      */
/* -------------------------------------- */

orb_subscr_t orb_subscribe(const struct orb_metadata *meta)
{
    return orb_subscribe_multi(meta, 0);
}

/* -------------------------------------- */
/* Topic utility                           */
/* -------------------------------------- */

int orb_exists(const struct orb_metadata *meta, int instance)
{
    return orb_node_exists(meta, instance) ? RT_EOK : -RT_ERROR;
}

int orb_group_count(const struct orb_metadata *meta)
{
    int count = 0;
    for (int i = 0; i < ORB_MULTI_MAX_INSTANCES; i++)
    {
        if (orb_node_exists(meta, i))
        {
            count++;
        }
    }
    return count;
}

/* -------------------------------------- */
/* Interval control                        */
/* -------------------------------------- */

int orb_set_interval(orb_subscr_t sub, unsigned interval)
{
    if (!sub)
    {
        return -RT_EINVAL;
    }
    sub->interval = interval;
    return RT_EOK;
}

int orb_get_interval(orb_subscr_t sub, unsigned *interval)
{
    if (!sub || !interval)
    {
        return -RT_EINVAL;
    }
    *interval = sub->interval;
    return RT_EOK;
}

int orb_wait(orb_subscr_t handle, int timeout_ms)
{
    /* 事件化实现：优先事件等待，其次轮询退避 */
    if (!handle)
    {
        return -RT_EINVAL;
    }
    /* 若尚未就绪则尝试绑定节点 */
    if (!orb_node_ready(handle))
    {
        return -RT_ENOENT;
    }
    /* 事件前先快速检查一次，避免订阅后因旧事件误唤醒 */
    rt_bool_t updated = RT_FALSE;
    if (orb_check(handle, &updated) == RT_EOK && updated)
    {
        return RT_EOK;
    }

    if (handle->node && handle->node->notifier.event)
    {
        int waited = 0;
        int step = 10;
        while (1)
        {
            int slice = step;
            if (timeout_ms >= 0)
            {
                int remain = timeout_ms - waited;
                if (remain <= 0) return -RT_ETIMEOUT;
                if (remain < step) slice = remain;
            }
            (void)uorb_notifier_wait(&handle->node->notifier, slice);
            (void)orb_check(handle, &updated);
            if (updated)
            {
                return RT_EOK;
            }
            if (timeout_ms >= 0)
            {
                waited += slice;
            }
        }
    }
    /* 退化为轮询 */
    updated = RT_FALSE;
    int waited = 0;
    int step = 5;
    while (1)
    {
        if (orb_check(handle, &updated) != RT_EOK)
        {
            return -RT_ERROR;
        }
        if (updated)
        {
            return RT_EOK;
        }
        if (timeout_ms >= 0 && waited >= timeout_ms)
        {
            return -RT_ETIMEOUT;
        }
        rt_thread_mdelay(step);
        waited += step;
    }
}
