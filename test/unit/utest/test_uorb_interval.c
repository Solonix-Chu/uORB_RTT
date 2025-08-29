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

static void test_interval_basic(void)
{
    struct orb_test_s t = {0};
    int inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(orb_test), &t, &inst);
    uassert_true(adv != RT_NULL);
    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst);
    uassert_true(sub != RT_NULL);

    uassert_int_equal(orb_set_interval(sub, 200), RT_EOK);

    t.timestamp = rt_tick_get(); t.val = 1; (void)orb_publish(ORB_ID(orb_test), adv, &t);
    rt_bool_t updated = RT_FALSE;
    uassert_int_equal(orb_check(sub, &updated), RT_EOK);
    uassert_true(updated);
    struct orb_test_s rx; (void)orb_copy(ORB_ID(orb_test), sub, &rx);

    // immediate publish should not update
    t.timestamp = rt_tick_get(); t.val = 2; (void)orb_publish(ORB_ID(orb_test), adv, &t);
    updated = RT_TRUE;
    uassert_int_equal(orb_check(sub, &updated), RT_EOK);
    uassert_false(updated);

    // after interval, should update
    rt_thread_mdelay(220);
    updated = RT_FALSE;
    uassert_int_equal(orb_check(sub, &updated), RT_EOK);
    uassert_true(updated);

    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_interval_basic);
}

UTEST_TC_EXPORT(testcase, "uorb.interval", tc_init, tc_cleanup, 20); 