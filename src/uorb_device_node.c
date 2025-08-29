/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#define LOG_TAG "uorb"
#define LOG_LVL LOG_LVL_INFO

#include "uORB.h"
#include <rtdbg.h>
#include <stdbool.h>
#include <rtthread.h>
#include "uorb_device_node.h"
#ifdef UORB_REGISTER_AS_DEVICE
#include "uorb_device_if.h"
#endif
#include <string.h>

rt_list_t _orb_node_list;
rt_bool_t _orb_node_list_initialized = RT_FALSE;

// 初始化节点列表
static void orb_node_list_init(void)
{
    if (!_orb_node_list_initialized)
    {
        rt_list_init(&_orb_node_list);
        _orb_node_list_initialized = RT_TRUE;
    }
}

// Determine the data range
static inline bool is_in_range(unsigned left, unsigned value, unsigned right)
{
    if (right > left)
    {
        return (left <= value) && (value <= right);
    }
    else
    { // Maybe the data overflowed and a wraparound occurred
        return (left <= value) || (value <= right);
    }
}

// round up to nearest power of two
// Such as 0 => 1, 1 => 1, 2 => 2 ,3 => 4, 10 => 16, 60 => 64, 65...255 => 128
// Note: When the input value > 128, the output is always 128
static inline rt_uint8_t round_pow_of_two_8(rt_uint8_t n)
{
    if (n == 0)
    {
        return 1;
    }

    // Avoid is already a power of 2
    rt_uint8_t value = n - 1;

    // Fill 1
    value |= value >> 1U;
    value |= value >> 2U;
    value |= value >> 4U;

    // Unable to round-up, take the value of round-down
    if (value == RT_UINT8_MAX)
    {
        value >>= 1U;
    }

    return value + 1;
}

/* 从 meta->o_fields 中解析形如 "@key=123;" 的整型值，未找到返回 -1 */
static int parse_meta_value(const char *fields, const char *key)
{
    if (!fields || !key) return -1;
    char pattern[32];
    rt_snprintf(pattern, sizeof(pattern), "@%s=", key);
    const char *p = strstr(fields, pattern);
    if (!p) return -1;
    p += rt_strlen(pattern);
    int val = 0;
    int any = 0;
    while (*p && *p >= '0' && *p <= '9')
    {
        any = 1;
        val = val * 10 + (*p - '0');
        p++;
    }
    return any ? val : -1;
}

static int get_default_queue_len(const struct orb_metadata *meta)
{
    if (!meta || !meta->o_fields) return -1;
    int v = parse_meta_value(meta->o_fields, "queue");
    return (v > 0 && v <= 255) ? v : -1;
}

static int get_default_instances(const struct orb_metadata *meta)
{
    if (!meta || !meta->o_fields) return -1;
    int v = parse_meta_value(meta->o_fields, "instances");
    return (v > 0 && v <= ORB_MULTI_MAX_INSTANCES) ? v : -1;
}

orb_node_t *orb_node_create(const struct orb_metadata *meta, const rt_uint8_t instance, rt_uint8_t queue_size)
{
    RT_ASSERT(meta != RT_NULL);

    orb_node_list_init(); // Ensure list is initialized

    /* 当调用方给出 queue_size=0 时，使用主题的默认队列长度（若配置），否则回退为1 */
    if (queue_size == 0)
    {
        int defq = get_default_queue_len(meta);
        queue_size = (defq > 0) ? (rt_uint8_t)defq : 1;
    }

    orb_node_t *node = (orb_node_t *)rt_calloc(sizeof(orb_node_t), 1);
    if (!node)
    {
        return RT_NULL;
    }

    node->meta             = meta;
    node->instance         = instance;
    node->queue_size       = round_pow_of_two_8(queue_size);
    node->generation       = 0;
    node->advertised       = 0;
    node->subscriber_count = 0;
    node->data_valid       = 0;
    node->data             = RT_NULL;
    // Initialize callbacks list
    rt_list_init(&node->callbacks);
    node->dev_min_interval = 0;
    node->last_dev_read    = 0;
    node->pending_delete   = RT_FALSE;

    /* 初始化事件通知器 */
    uorb_notifier_init(&node->notifier, "uorb_evt");

    rt_list_insert_after(_orb_node_list.prev, &node->list);

    // 注册设备
    // char name[RT_NAME_MAX];
    // rt_snprintf(name, RT_NAME_MAX, "%s%d", meta->o_name, instance);
    // rt_uorb_register(node, name, 0, RT_NULL);

    return node;
}

rt_err_t orb_node_delete(orb_node_t *node)
{
    if (!node)
    {
        return -RT_ERROR;
    }

    node->advertised = false;

    // 从链表中移除
    rt_list_remove(&node->list);
    
    // 释放data内存
    if (node->data)
    {
        rt_free(node->data);
        node->data = RT_NULL;
    }

    // 释放事件通知器
    uorb_notifier_deinit(&node->notifier);

    if (node->subscriber_count == 0)
    {
        rt_free(node);
    }
    else
    {
        /* 仍有订阅者：设置延迟删除标记 */
        node->pending_delete = RT_TRUE;
    }

    return RT_EOK;
}


orb_node_t *orb_node_find(const struct orb_metadata *meta, int instance)
{
    orb_node_list_init(); // Ensure list is initialized
    
    // 遍历_node_list
    rt_list_t  *pos;
    orb_node_t *node;

    rt_enter_critical();
    rt_list_for_each(pos, &_orb_node_list)
    {
        node = rt_list_entry(pos, orb_node_t, list);
        if (node->meta == meta && node->instance == instance)
        {
            rt_exit_critical();
            return node;
        }
    }
    rt_exit_critical();


    return RT_NULL;
}

bool orb_node_exists(const struct orb_metadata *meta, int instance)
{
    if (!meta)
    {
        return false;
    }

    if (instance < 0 || instance > (ORB_MULTI_MAX_INSTANCES - 1))
    {
        return false;
    }

    orb_node_t *node = orb_node_find(meta, instance);

    if (node && node->advertised)
    {
        return true;
    }

    return false;
}

int orb_node_read(orb_node_t *node, void *data, rt_uint32_t *generation)
{
    RT_ASSERT(node != RT_NULL);
    
    // 添加对data参数的NULL检查，返回错误而非断言失败
    if (data == RT_NULL)
    {
        return -RT_EINVAL;
    }

    if (!node->data)
    {
        return 0;
    }

    // 当前节点的数据代数（generation），每次写入数据时自增
    const unsigned current_generation = node->generation;
    unsigned       updated_generation = generation ? (*generation) : current_generation;


    if (node->queue_size == 1)
    {
        rt_memcpy(data, node->data, node->meta->o_size);
        updated_generation = current_generation;
    }
    else
    {
        // 多队列场景：
        // 如果订阅者的generation等于当前generation，说明没有新数据，回退一代，防止重复读取
        if (current_generation == updated_generation)
        {
            updated_generation--;
        }

        // 检查updated_generation是否在合法范围内（即数据是否还在队列中，未被覆盖）
        // 如果不在范围内，说明数据已被覆盖，只能读取最早可用的数据
        if (!is_in_range(current_generation - node->queue_size, updated_generation, current_generation - 1))
        {
            updated_generation = current_generation - node->queue_size;
        }

        rt_memcpy(data, node->data + (node->meta->o_size * (updated_generation % node->queue_size)), node->meta->o_size);

        // 读取后，generation自增，表示已消费一条数据
        updated_generation++;
    }

    if (generation)
    {
        *generation = updated_generation;
    }

    return node->meta->o_size;
}

int orb_node_write(orb_node_t *node, const void *data)
{
    RT_ASSERT(node != RT_NULL);
    
    // 添加对data参数的NULL检查，返回错误而非断言失败  
    if (data == RT_NULL)
    {
        return -RT_EINVAL;
    }

    // create buffer
    if (!node->data)
    {
        const size_t size = node->meta->o_size * node->queue_size;
        node->data        = rt_calloc(size, 1);
    }

    // buffer invalid
    if (!node->data)
    {
        return -RT_ERROR;
    }

    rt_enter_critical();

    // copy data to buffer
    rt_memcpy(node->data + (node->meta->o_size * (node->generation % node->queue_size)), data, node->meta->o_size);

    // invoke callbacks
    rt_list_t      *pos;
    orb_callback_t *item;
    rt_list_for_each(pos, &node->callbacks)
    {
        item = rt_list_entry(pos, orb_callback_t, list);
        if (item->call)
        {
            item->call();
        }
    }

    // 通知订阅者（事件）
    uorb_notifier_notify(&node->notifier);

    // mark data valid
    node->data_valid = true;

    // update generation
    node->generation++;

    rt_exit_critical();

    return node->meta->o_size;
}

bool orb_node_ready(orb_subscribe_t *handle)
{
    if (!handle)
    {
        return false;
    }

    if (handle->node)
    {
        return handle->node->advertised;
    }

    handle->node = orb_node_find(handle->meta, handle->instance);

    if (handle->node)
    {
        handle->node->subscriber_count++;
        handle->generation = handle->node->generation;

        return handle->node->advertised;
    }

    return false;
}

orb_subscribe_t *orb_subscribe_multi(const struct orb_metadata *meta, uint8_t instance)
{
    orb_subscribe_t *sub = rt_calloc(sizeof(orb_subscribe_t), 1);

    sub->meta       = meta;
    sub->instance   = instance;
    sub->interval   = 0;
    sub->generation = 0;
    sub->node       = orb_node_find(meta, instance);
    
    // 如果找到了节点，增加订阅者计数并初始化generation
    if (sub->node)
    {
        sub->node->subscriber_count++;
        sub->generation = sub->node->generation;
    }

    return sub;
}

int orb_unsubscribe(orb_subscribe_t *handle)
{
    if (!handle)
    {
        return -RT_EINVAL;
    }

    if (handle->node)
    {
        handle->node->subscriber_count--;
        /* 若已标记延迟删除且无订阅者且未公告，则回收节点 */
        if (handle->node->subscriber_count == 0 && (handle->node->pending_delete || !handle->node->advertised))
        {
            orb_node_delete(handle->node);
        }
    }

    /* 清理订阅者状态，避免悬挂（即便随后释放） */
    handle->callback_registered = RT_FALSE;
    handle->last_update = 0;
    handle->node       = RT_NULL;
    handle->generation = 0;

    rt_free(handle);
    return RT_EOK;
}

//  检查订阅者是否有新数据可读，并支持定时检查
int orb_check(orb_subscribe_t *handle, rt_bool_t *updated)
{
    if (!handle || !updated)
    {
        return -RT_EINVAL;
    }

    if (!orb_node_ready(handle))
    {
        return -RT_ERROR;
    }

    // 简化逻辑：检查generation是否不同来确定是否有更新
    if (handle->interval == 0 || (rt_tick_get() - handle->last_update) * 1000 / RT_TICK_PER_SECOND >= handle->interval)
    {
        *updated = handle->generation != handle->node->generation;
        if (*updated)
        {
            /* 推进 generation，使得一次检查消费一次更新信号 */
            handle->generation = handle->node->generation;
        }
        return RT_EOK;
    }
    else
    {
        *updated = RT_FALSE;
        return RT_EOK;
    }
}

int orb_copy(const struct orb_metadata *meta, orb_subscribe_t *handle, void *buffer)
{
    if (!meta || !handle || !buffer)
        return -RT_EINVAL;

    // 检查节点是否ready
    if (!orb_node_ready(handle))
        return -RT_ERROR;

    // 读取数据并更新订阅者generation
    int ret = orb_node_read(handle->node, buffer, &handle->generation);
    if (ret > 0)
    {
        // 更新时间戳
        handle->last_update = rt_tick_get();
        return ret;
    }
    return ret; // 返回实际的错误码或0
}

orb_advert_t orb_advertise_multi_queue(const struct orb_metadata *meta, const void *data, int *instance,
                                          unsigned int queue_size)
{
    if (!meta)
    {
        return RT_NULL;
    }

    orb_node_t *node = RT_NULL;

    // 允许的最大instance个数

    int max_inst = ORB_MULTI_MAX_INSTANCES;
    int inst     = 0;

    int def_inst = get_default_instances(meta);

    if (!instance)
    {
        /* 未给出instance输出指针：按默认实例上限（若未配置则为1）搜索可用实例 */
        max_inst = (def_inst > 0) ? def_inst : 1;
    }
    else
    {
        /* 允许在提供instance指针时遍历全部可用实例上限 */
        max_inst = ORB_MULTI_MAX_INSTANCES;
    }

    int selected_inst = -1;

    // 搜索实例是否存在，不存在则创建，存在则判断是否公告
    for (inst = 0; inst < max_inst; inst++)
    {
        node = orb_node_find(meta, inst);

        if (node)
        {
            if (!node->advertised)
            {
                selected_inst = inst;
                break;
            }
            else
            {
                // 已经公告，占用该实例，继续查找
                node = RT_NULL;
                continue;
            }
        }
        else
        {
            /* 当调用方未指定队列长度(传入0)时，orb_node_create 将使用默认队列长度（若有） */
            node = orb_node_create(meta, inst, (rt_uint8_t)queue_size);
            if (!node)
            {
                // 创建失败，继续尝试下一个实例
                continue;
            }
            selected_inst = inst;
            break;
        }
    }

    // 未找到可用实例
    if (selected_inst < 0 || !node)
    {
        return RT_NULL;
    }

    // 标记为已经公告，只有公告过的主题才能copy和publish数据
    node->advertised = true;
    if (data)
    {
        orb_node_write(node, data);
    }
    // 返回inst
    if (instance)
    {
        *instance = selected_inst;
    }
#ifdef UORB_REGISTER_AS_DEVICE
    rt_uorb_register_topic(meta, (rt_uint8_t)selected_inst);
#endif

    return node;
}

int orb_unadvertise(orb_node_t *node)
{
    if (!node)
    {
        return -RT_ERROR;
    }

    node->advertised = false;

    /* 若无人订阅则释放节点（与设备化策略配合：设备注销也会尝试回收） */
    if (node->subscriber_count == 0)
    {
        return orb_node_delete(node);
    }

    return RT_EOK;
}

int orb_publish(const struct orb_metadata *meta, orb_node_t *node, const void *data)
{
    if (!data)
    {
        return -RT_EINVAL;
    }

    if (!meta && !node)
    {
        return -RT_EINVAL;
    }
    else if (meta && !node)
    {
        /* 优先选择已公告的最小实例，若不存在则回退实例0 */
        for (int i = 0; i < ORB_MULTI_MAX_INSTANCES; i++)
        {
            orb_node_t *candidate = orb_node_find(meta, i);
            if (candidate && candidate->advertised)
            {
                node = candidate;
                break;
            }
        }
        if (!node)
        {
            node = orb_node_find(meta, 0);
        }
        if (!node)
        {
            return -RT_ENOENT;
        }
    }
    else if (meta && node)
    {
        if (node->meta != meta)
        {
            return -RT_EINVAL;
        }
    }
    // else (!meta && node) 直接用node

    // 新增：检查节点是否已公告
    if (!node->advertised)
    {
        return -RT_EINVAL;
    }

    if (orb_node_write(node, data) == node->meta->o_size)
    {
        return RT_EOK;
    }

    return -RT_ERROR;
}

int orb_register_callback(const struct orb_metadata *meta, uint8_t instance, void (*fn)(void))
{
    if (!meta || !fn)
    {
        return -RT_ERROR;
    }
    orb_node_t *node = orb_node_find(meta, instance);
    if (!node)
    {
        return -RT_ERROR;
    }
    orb_callback_t *item = (orb_callback_t *)rt_calloc(1, sizeof(orb_callback_t));
    if (!item)
    {
        return -RT_ERROR;
    }
    item->call = fn;
    rt_list_insert_after(&node->callbacks, &item->list);
    return RT_EOK;
}

int orb_unregister_callback(const struct orb_metadata *meta, uint8_t instance, void (*fn)(void))
{
    if (!meta || !fn)
    {
        return -RT_ERROR;
    }
    orb_node_t *node = orb_node_find(meta, instance);
    if (!node)
    {
        return -RT_ERROR;
    }
    rt_list_t *pos, *tmp;
    rt_list_for_each_safe(pos, tmp, &node->callbacks)
    {
        orb_callback_t *item = rt_list_entry(pos, orb_callback_t, list);
        if (item->call == fn)
        {
            rt_list_remove(&item->list);
            rt_free(item);
            return RT_EOK;
        }
    }
    return -RT_ERROR;
}
