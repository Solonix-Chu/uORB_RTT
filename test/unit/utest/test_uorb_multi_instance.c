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

static void test_multi_isolation(void)
{
    struct orb_test_s t0 = {0}, t1 = {0};
    int inst0 = -1, inst1 = -1;
    orb_advert_t adv0 = orb_advertise_multi(ORB_ID(orb_test), &t0, &inst0);
    orb_advert_t adv1 = orb_advertise_multi(ORB_ID(orb_test), &t1, &inst1);
    uassert_true(adv0 && adv1);
    uassert_true(inst0 >= 0 && inst1 >= 0 && inst0 != inst1);

    orb_subscr_t sub0 = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst0);
    orb_subscr_t sub1 = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst1);
    uassert_true(sub0 && sub1);

    // clear baselines
    rt_bool_t up = RT_FALSE; struct orb_test_s drop;
    (void)orb_check(sub0, &up); if (up) (void)orb_copy(ORB_ID(orb_test), sub0, &drop);
    up = RT_FALSE; (void)orb_check(sub1, &up); if (up) (void)orb_copy(ORB_ID(orb_test), sub1, &drop);

    // publish to inst0 only
    t0.timestamp = rt_tick_get(); t0.val = 11; (void)orb_publish(ORB_ID(orb_test), adv0, &t0);
    rt_bool_t up0 = RT_FALSE, up1 = RT_FALSE;
    uassert_int_equal(orb_check(sub0, &up0), RT_EOK); uassert_true(up0);
    uassert_int_equal(orb_check(sub1, &up1), RT_EOK); uassert_false(up1);

    // publish to inst1 only
    t1.timestamp = rt_tick_get(); t1.val = 22; (void)orb_publish(ORB_ID(orb_test), adv1, &t1);
    up0 = up1 = RT_FALSE;
    uassert_int_equal(orb_check(sub1, &up1), RT_EOK); uassert_true(up1);
    uassert_int_equal(orb_check(sub0, &up0), RT_EOK); uassert_false(up0);

    orb_unsubscribe(sub0); orb_unsubscribe(sub1);
    orb_unadvertise(adv0); orb_unadvertise(adv1);
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_multi_isolation);
}

UTEST_TC_EXPORT(testcase, "uorb.multi", tc_init, tc_cleanup, 20); 