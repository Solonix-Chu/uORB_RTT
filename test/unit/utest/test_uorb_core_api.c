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

static rt_err_t tc_init(void) { return RT_EOK; }
static rt_err_t tc_cleanup(void) { return RT_EOK; }

static void test_core_basic_pubsub(void)
{
    struct orb_test_s t = {0};
    int inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(orb_test), &t, &inst);
    uassert_true(adv != RT_NULL);
    uassert_true(inst >= 0);

    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst);
    uassert_true(sub != RT_NULL);

    t.timestamp = rt_tick_get();
    t.val = 42;
    int pr = orb_publish(ORB_ID(orb_test), adv, &t);
    uassert_int_equal(pr, RT_EOK);

    rt_bool_t updated = RT_FALSE;
    int cr = orb_check(sub, &updated);
    uassert_int_equal(cr, RT_EOK);
    uassert_true(updated);

    struct orb_test_s rx;
    int cp = orb_copy(ORB_ID(orb_test), sub, &rx);
    uassert_true(cp > 0);
    uassert_int_equal(rx.val, 42);

    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}

static void test_core_wait_ok_timeout(void)
{
    struct orb_test_s t = {0};
    int inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(orb_test), &t, &inst);
    uassert_true(adv != RT_NULL);
    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst);
    uassert_true(sub != RT_NULL);

    // wait timeout first
    int ret = orb_wait(sub, 50);
    uassert_int_equal(ret, -RT_ETIMEOUT);

    // then publish and wait OK
    t.timestamp = rt_tick_get(); t.val = 7; (void)orb_publish(ORB_ID(orb_test), adv, &t);
    ret = orb_wait(sub, 200);
    uassert_int_equal(ret, RT_EOK);

    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_core_basic_pubsub);
    UTEST_UNIT_RUN(test_core_wait_ok_timeout);
}

UTEST_TC_EXPORT(testcase, "uorb.core", tc_init, tc_cleanup, 20); 