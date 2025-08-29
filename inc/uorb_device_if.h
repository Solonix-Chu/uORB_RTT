/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#ifndef __UORB_DEVICE_IF_H__
#define __UORB_DEVICE_IF_H__

#include "uORB.h"
#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* control 命令码 */
#define UORB_DEVICE_CTRL_CHECK        (0x01)  /* args: int* updated */
#define UORB_DEVICE_CTRL_SET_INTERVAL (0x02)  /* args: unsigned* interval_ms */
#define UORB_DEVICE_CTRL_GET_STATUS   (0x03)  /* args: struct uorb_device_status* */
#define UORB_DEVICE_CTRL_EXISTS       (0x04)  /* args: int* exists */

struct uorb_device_status
{
    const char             *name;
    const struct orb_metadata *meta;
    rt_uint8_t              instance;
    rt_uint8_t              queue_size;
    rt_uint32_t             generation;
    rt_uint8_t              subscriber_count;
    rt_bool_t               advertised;
    rt_bool_t               data_valid;
};

/* 注册/注销设备接口 */
int rt_uorb_register_topic(const struct orb_metadata *meta, rt_uint8_t instance);
int rt_uorb_unregister_topic(const struct orb_metadata *meta, rt_uint8_t instance);

/* 生成设备节点名：确保不超过 RT_NAME_MAX，格式为 前缀截断+实例号 */
void uorb_make_dev_name(const struct orb_metadata *meta, rt_uint8_t instance, char *out, rt_size_t outsz);

#ifdef __cplusplus
}
#endif

#endif /* __UORB_DEVICE_IF_H__ */ 