/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#ifndef _UORB_DEVICE_NODE_H_
#define _UORB_DEVICE_NODE_H_

#include "uORB.h"
#include <rtthread.h>
#include "uorb_internal.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct orb_callback_s
{
    rt_list_t list;
    void (*call)();
} orb_callback_t;


typedef struct orb_node_s
{
    rt_list_t                    list;
    const struct orb_metadata   *meta;
    rt_uint8_t                   instance;         // 实例序号
    rt_uint8_t                   queue_size;       // 栈的长度
    rt_uint32_t                  generation;       // 更新代数
    rt_list_t                    callbacks;        // 回调函数链表
    rt_bool_t                    advertised;       // 是否公告
    rt_uint8_t                   subscriber_count; // 订阅者个数
    rt_bool_t                    data_valid;       // data是否有效
    rt_uint8_t                  *data;
    rt_uint32_t                  dev_min_interval;
    rt_tick_t                    last_dev_read;
    uorb_notifier_t              notifier;         // 事件通知器（用于阻塞等待）
    rt_bool_t                    pending_delete;   // 延迟删除标记
} orb_node_t;


typedef struct orb_subscribe_s
{
    const struct orb_metadata   *meta;
    rt_uint8_t                   instance;
    rt_tick_t                    interval;
    orb_node_t *node;
    rt_uint32_t generation;
    rt_tick_t   last_update;
    rt_bool_t   callback_registered;
} orb_subscribe_t;

/* Function declarations */
orb_node_t* orb_node_create(const struct orb_metadata* meta, const rt_uint8_t instance, rt_uint8_t queue_size);
rt_err_t orb_node_delete(orb_node_t* node);
orb_node_t* orb_node_find(const struct orb_metadata* meta, int instance);
bool orb_node_exists(const struct orb_metadata* meta, int instance);
int orb_node_read(orb_node_t* node, void* data, rt_uint32_t* generation);
int orb_node_write(orb_node_t* node, const void* data);
bool orb_node_ready(orb_subscribe_t* handle);

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // _UORB_DEVICE_NODE_H_
