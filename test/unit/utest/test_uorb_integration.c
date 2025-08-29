/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include <rtthread.h>
#include <utest.h>
#include "uORB.h"
#if defined(UORB_TOPICS_GENERATED)
#include "topics/orb_test.h"
#include "topics/sensor_demo.h"
#else
#include "uorb_demo_topics.h"
#endif
#ifdef UORB_REGISTER_AS_DEVICE
#include "uorb_device_if.h"
#endif

static rt_err_t tc_init(void) { return RT_EOK; }
static rt_err_t tc_cleanup(void) { return RT_EOK; }

/* 1) 发布-订阅顺序与 wait 行为（无多线程） */
static void test_integration_pubsub_sequence(void)
{
    struct orb_test_s t = {0};
    int inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(orb_test), &t, &inst);
    uassert_true(adv != RT_NULL);
    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst);
    uassert_true(sub != RT_NULL);

    for (int i = 1; i <= 5; i++)
    {
        t.timestamp = rt_tick_get(); t.val = i; (void)orb_publish(ORB_ID(orb_test), adv, &t);
        int ret = orb_wait(sub, 200);
        uassert_int_equal(ret, RT_EOK);
        struct orb_test_s rx; (void)orb_copy(ORB_ID(orb_test), sub, &rx);
        uassert_int_equal(rx.val, i);
        rt_thread_mdelay(10);
    }

    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}

/* 2) 小队列下的覆盖观察：发布多条，订阅端仅能获取最后窗口 */
static void test_integration_queue_overflow_window(void)
{
    struct orb_test_s t = {0};
    /* 使用队列长度2 */
    orb_advert_t adv = orb_advertise_queue(ORB_ID(orb_test), &t, 2);
    uassert_true(adv != RT_NULL);
    /* 默认实例0 */
    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(orb_test), 0);
    uassert_true(sub != RT_NULL);

    /* 连续发布5条 */
    for (int i = 1; i <= 5; i++)
    {
        t.timestamp = rt_tick_get(); t.val = i; (void)orb_publish(ORB_ID(orb_test), adv, &t);
        rt_thread_mdelay(5);
    }

    /* 读取直到无更新，统计读取次数与最后值 */
    int copies = 0; struct orb_test_s rx; rt_bool_t updated = RT_FALSE;
    while (1)
    {
        updated = RT_FALSE;
        (void)orb_check(sub, &updated);
        if (!updated) break;
        (void)orb_copy(ORB_ID(orb_test), sub, &rx);
        copies++;
    }
    /* 最后值应为5；读取次数不应超过队列长度2（允许等于2） */
    uassert_int_equal(rx.val, 5);
    uassert_true(copies <= 2);

    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}

#ifdef UORB_REGISTER_AS_DEVICE
/* 3) 设备路径集成：API 发布 -> 设备按间隔读取，设备写 -> API 读取 */
static void test_integration_device_paths(void)
{
    /* 注册并打开设备 */
    rt_uorb_register_topic(ORB_ID(sensor_demo), 0);
    char name[RT_NAME_MAX] = {0};
    uorb_make_dev_name(ORB_ID(sensor_demo), 0, name, sizeof(name));
    rt_device_t dev = rt_device_find(name);
    uassert_true(dev != RT_NULL);
    uassert_int_equal(rt_device_open(dev, RT_DEVICE_OFLAG_RDWR), RT_EOK);

    unsigned iv = 100; rt_device_control(dev, UORB_DEVICE_CTRL_SET_INTERVAL, &iv);

    /* API 订阅/公告 */
    struct sensor_demo_s init = {0}; int inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(sensor_demo), &init, &inst);
    uassert_true(adv != RT_NULL);
    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(sensor_demo), 0);
    uassert_true(sub != RT_NULL);

    /* API -> 设备 */
    struct sensor_demo_s tx = { .timestamp = rt_tick_get(), .x=10, .y=20, .z=30 };
    (void)orb_publish(ORB_ID(sensor_demo), adv, &tx);
    struct sensor_demo_s rd = {0};
    (void)rt_device_read(dev, 0, &rd, sizeof(rd));
    if (rd.x != 10) { rt_thread_mdelay(120); (void)rt_device_read(dev, 0, &rd, sizeof(rd)); }
    uassert_int_equal(rd.x, 10);

    /* 设备 -> API */
    tx.timestamp = rt_tick_get(); tx.x=11; tx.y=22; tx.z=33; (void)rt_device_write(dev, 0, &tx, sizeof(tx));
    rt_bool_t updated = RT_FALSE; (void)orb_check(sub, &updated);
    if (updated) { struct sensor_demo_s rx; (void)orb_copy(ORB_ID(sensor_demo), sub, &rx); uassert_int_equal(rx.x, 11); }

    rt_device_close(dev);
    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}
#endif

static void testcase(void)
{
    UTEST_UNIT_RUN(test_integration_pubsub_sequence);
    UTEST_UNIT_RUN(test_integration_queue_overflow_window);
#ifdef UORB_REGISTER_AS_DEVICE
    UTEST_UNIT_RUN(test_integration_device_paths);
#endif
}

UTEST_TC_EXPORT(testcase, "uorb.integration", tc_init, tc_cleanup, 60); 