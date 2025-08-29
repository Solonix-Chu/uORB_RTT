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
#include "uorb_device_if.h"

#if defined(UORB_REGISTER_AS_DEVICE)
static rt_err_t tc_init(void) { return RT_EOK; }
static rt_err_t tc_cleanup(void) { return RT_EOK; }

static void test_device_if_pub_device_read_api(void)
{
    // register device
    rt_uorb_register_topic(ORB_ID(sensor_demo), 0);
    char name[RT_NAME_MAX] = {0};
    uorb_make_dev_name(ORB_ID(sensor_demo), 0, name, sizeof(name));

    rt_device_t dev = rt_device_find(name);
    uassert_true(dev != RT_NULL);
    uassert_int_equal(rt_device_open(dev, RT_DEVICE_OFLAG_RDWR), RT_EOK);

    unsigned iv = 100; rt_device_control(dev, UORB_DEVICE_CTRL_SET_INTERVAL, &iv);

    // advertise API sub
    struct sensor_demo_s init = {0}; int inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(sensor_demo), &init, &inst);
    uassert_true(adv != RT_NULL);
    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(sensor_demo), 0);
    uassert_true(sub != RT_NULL);

    // device write -> API read
    struct sensor_demo_s tx = { .timestamp = rt_tick_get(), .x=1, .y=2, .z=3 };
    (void)rt_device_write(dev, 0, &tx, sizeof(tx));
    rt_bool_t updated = RT_FALSE; (void)orb_check(sub, &updated);
    if (updated) { struct sensor_demo_s rx; (void)orb_copy(ORB_ID(sensor_demo), sub, &rx); uassert_int_equal(rx.x, 1); }

    // API write -> device read
    tx.timestamp = rt_tick_get(); tx.x=7; tx.y=8; tx.z=9;
    (void)orb_publish(ORB_ID(sensor_demo), adv, &tx);
    struct sensor_demo_s rd = {0}; (void)rt_device_read(dev, 0, &rd, sizeof(rd));
    // may be 0 if interval not elapsed; wait and read again
    if (rd.x != 7) { rt_thread_mdelay(120); (void)rt_device_read(dev, 0, &rd, sizeof(rd)); }
    uassert_int_equal(rd.x, 7);

    rt_device_close(dev);
    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_device_if_pub_device_read_api);
}

UTEST_TC_EXPORT(testcase, "uorb.device_if", tc_init, tc_cleanup, 30);
#endif 