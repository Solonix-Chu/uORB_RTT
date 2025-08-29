/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include <rtthread.h>
#include <stdio.h>
#if defined(UORB_USING_MSG_GEN) && defined(UORB_TOPICS_GENERATED)
#include "topics/orb_test.h"
#include "topics/sensor_demo.h"
#else
#include "uorb_demo_topics.h"
#endif

static void uorb_demo_sub_entry(void *parameter)
{
    orb_subscr_t sub_test = orb_subscribe(ORB_ID(orb_test));
    orb_subscr_t sub_sens = orb_subscribe_multi(ORB_ID(sensor_demo), 0);
    orb_set_interval(sub_test, 200);
    orb_set_interval(sub_sens, 500);

    while (1)
    {
        rt_bool_t updated = RT_FALSE;
        if (orb_check(sub_test, &updated) == RT_EOK && updated)
        {
            struct orb_test_s t;
            if (orb_copy(ORB_ID(orb_test), sub_test, &t) > 0)
            {
                rt_kprintf("orb_test: ts=%u val=%d\n", (unsigned)t.timestamp, t.val);
            }
        }
        if (orb_check(sub_sens, &updated) == RT_EOK && updated)
        {
            struct sensor_demo_s s;
            if (orb_copy(ORB_ID(sensor_demo), sub_sens, &s) > 0)
            {
                rt_kprintf("sensor_demo: ts=%u xyz=(%d,%d,%d)\n", (unsigned)s.timestamp, s.x, s.y, s.z);
            }
        }
        rt_thread_mdelay(50);
    }
}

int uorb_demo_sub_init(void)
{
    rt_thread_t tid = rt_thread_create("u_sub", uorb_demo_sub_entry, RT_NULL, 2048, 15, 10);
    if (tid) rt_thread_startup(tid);
    return 0;
}
#if defined(UORB_ENABLE_DEMO)
INIT_APP_EXPORT(uorb_demo_sub_init);
#endif 