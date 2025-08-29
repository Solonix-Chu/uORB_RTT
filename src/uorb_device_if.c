/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include "uorb_device_if.h"
#include "uorb_device_node.h"
#include <rtdevice.h>
#include <string.h>
#include <rtthread.h>

void uorb_make_dev_name(const struct orb_metadata *meta, rt_uint8_t instance, char *out, rt_size_t outsz)
{
    if (!out || outsz == 0) return;
    /* 预留末尾 1 字符给 '\0'，另预留 3 字符给实例号最坏情况 */
    rt_size_t max_prefix = (outsz > 4) ? (outsz - 4) : 1;
    rt_size_t n = rt_strlen(meta->o_name);
    if (n > max_prefix) n = max_prefix;
    rt_memcpy(out, meta->o_name, n);
    out[n] = '\0';
    char buf[8];
    rt_snprintf(buf, sizeof(buf), "%d", (int)instance);
    strncat(out, buf, outsz - 1 - n);
}

/* 统一的设备回调实现 */
static rt_err_t uorb_dev_init(rt_device_t dev)
{
    RT_UNUSED(dev);
    return RT_EOK;
}

static rt_err_t uorb_dev_open(rt_device_t dev, rt_uint16_t oflag)
{
    RT_UNUSED(oflag);
    RT_UNUSED(dev);
    return RT_EOK;
}

static rt_err_t uorb_dev_close(rt_device_t dev)
{
    RT_UNUSED(dev);
    return RT_EOK;
}

static rt_ssize_t uorb_dev_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    RT_UNUSED(pos);
    if (!dev || !buffer) return 0;
    orb_node_t *node = (orb_node_t *)dev->user_data;
    if (!node) return 0;

    /* 节流：若设置了设备侧最小读取间隔，则在间隔未到达时直接返回0 */
    if (node->dev_min_interval > 0)
    {
        rt_tick_t now = rt_tick_get();
        rt_tick_t need = rt_tick_from_millisecond(node->dev_min_interval);
        if (node->last_dev_read != 0 && (now - node->last_dev_read) < need)
        {
            return 0;
        }
    }

    rt_uint32_t gen = node->generation;
    if (orb_node_read(node, buffer, &gen) > 0)
    {
        node->last_dev_read = rt_tick_get();
        return (rt_ssize_t)node->meta->o_size;
    }
    return 0;
}

static rt_ssize_t uorb_dev_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    RT_UNUSED(pos);
    if (!dev || !buffer) return 0;
    orb_node_t *node = (orb_node_t *)dev->user_data;
    if (!node) return 0;

    if (orb_node_write(node, buffer) > 0)
    {
        return (rt_ssize_t)node->meta->o_size;
    }
    return 0;
}

static rt_err_t uorb_dev_control(rt_device_t dev, int cmd, void *args)
{
    if (!dev) return -RT_ERROR;
    orb_node_t *node = (orb_node_t *)dev->user_data;
    if (!node) return -RT_ERROR;

    switch (cmd)
    {
    case UORB_DEVICE_CTRL_CHECK:
        if (!args) return -RT_EINVAL;
        {
            rt_uint32_t last = *(rt_uint32_t *)args;
            *(rt_uint32_t *)args = (last != node->generation) ? 1 : 0;
        }
        return RT_EOK;
    case UORB_DEVICE_CTRL_SET_INTERVAL:
        if (!args) return -RT_EINVAL;
        node->dev_min_interval = *(unsigned *)args;
        return RT_EOK;
    case UORB_DEVICE_CTRL_GET_STATUS:
        if (!args) return -RT_EINVAL;
        {
            struct uorb_device_status *st = (struct uorb_device_status *)args;
            st->name = node->meta->o_name;
            st->meta = node->meta;
            st->instance = node->instance;
            st->queue_size = node->queue_size;
            st->generation = node->generation;
            st->subscriber_count = node->subscriber_count;
            st->advertised = node->advertised;
            st->data_valid = node->data_valid;
        }
        return RT_EOK;
    case UORB_DEVICE_CTRL_EXISTS:
        if (!args) return -RT_EINVAL;
        {
            *(int *)args = node->advertised ? 1 : 0;
        }
        return RT_EOK;
    default:
        return -RT_EINVAL;
    }
}

#ifdef RT_USING_DEVICE_OPS
static struct rt_device_ops uorb_dev_ops = {
    uorb_dev_init,
    uorb_dev_open,
    uorb_dev_close,
    uorb_dev_read,
    uorb_dev_write,
    uorb_dev_control,
};
#endif

int rt_uorb_register_topic(const struct orb_metadata *meta, rt_uint8_t instance)
{
    if (!meta) return -RT_ERROR;

    /* 禁止在中断中注册设备，避免堆栈与注册流程的复杂性 */
    if (rt_interrupt_get_nest())
    {
        return -RT_EINTR;
    }

    orb_node_t *node = orb_node_find(meta, instance);
    if (!node)
    {
        node = orb_node_create(meta, instance, 1);
        if (!node) return -RT_ERROR;
    }

    struct rt_device *dev = (struct rt_device *)rt_calloc(1, sizeof(struct rt_device));
    if (!dev) return -RT_ERROR;

    dev->type = RT_Device_Class_Miscellaneous;
    dev->user_data = node;
#ifdef RT_USING_DEVICE_OPS
    dev->ops = &uorb_dev_ops;
#else
    dev->init = uorb_dev_init;
    dev->open = uorb_dev_open;
    dev->close = uorb_dev_close;
    dev->read = uorb_dev_read;
    dev->write = uorb_dev_write;
    dev->control = uorb_dev_control;
#endif

    char name[RT_NAME_MAX] = {0};
    uorb_make_dev_name(meta, instance, name, sizeof(name));

    return rt_device_register(dev, name, RT_DEVICE_FLAG_RDWR);
}
RTM_EXPORT(rt_uorb_register_topic);

int rt_uorb_unregister_topic(const struct orb_metadata *meta, rt_uint8_t instance)
{
    if (!meta) return -RT_ERROR;

    char devname[RT_NAME_MAX] = {0};
    uorb_make_dev_name(meta, instance, devname, sizeof(devname));

    rt_device_t dev = rt_device_find(devname);
    if (!dev) return -RT_ERROR;

    /* 注销设备，成功后按条件回收节点 */
    int ret = rt_device_unregister(dev);
    if (ret == RT_EOK)
    {
        orb_node_t *node = (orb_node_t *)dev->user_data;
        if (node)
        {
            /* 若已无人订阅且未公告，则安全删除节点 */
            if (node->subscriber_count == 0 && !node->advertised)
            {
                orb_node_delete(node);
            }
        }
        rt_free(dev);
    }
    return ret;
}
RTM_EXPORT(rt_uorb_unregister_topic); 