/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include <rtthread.h>
#if defined(UORB_USING_MSG_GEN) && defined(UORB_TOPICS_GENERATED)
#include "topics/orb_test.h"
#include "topics/sensor_demo.h"
#else
#include "uorb_demo_topics.h"
#endif

static void uorb_demo_pub_entry(void *parameter)
{
    struct orb_test_s     t = {0};
    struct sensor_demo_s  s = {0};

    orb_advert_t pub_test = orb_advertise(ORB_ID(orb_test), &t);
    int          inst     = 0;
    orb_advert_t pub_sens = orb_advertise_multi(ORB_ID(sensor_demo), &s, &inst);

    int32_t cnt = 0;
    while (1)
    {
        t.timestamp = rt_tick_get();
        t.val       = cnt;
        orb_publish(ORB_ID(orb_test), pub_test, &t);

        s.timestamp = rt_tick_get();
        s.x = cnt * 1;
        s.y = cnt * 2;
        s.z = cnt * 3;
        orb_publish(ORB_ID(sensor_demo), pub_sens, &s);

        cnt++;
        rt_thread_mdelay(100);
    }
}

int uorb_demo_pub_init(void)
{
    rt_thread_t tid = rt_thread_create("u_pub", uorb_demo_pub_entry, RT_NULL, 2048, 15, 10);
    if (tid) rt_thread_startup(tid);
    return 0;
}
#ifdef UORB_ENABLE_DEMO
INIT_APP_EXPORT(uorb_demo_pub_init); 
#endif
