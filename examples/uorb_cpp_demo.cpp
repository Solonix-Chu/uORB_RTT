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

#include "cxx/uORB.hpp"

static void uorb_cpp_demo_entry(void *parameter)
{
	uORB::Publication<orb_test_s> pub_test{ORB_ID(orb_test)};
	uORB::PublicationMulti<sensor_demo_s> pub_sens{ORB_ID(sensor_demo)};

	// 先进行一次公告，确保实例号可得
	orb_test_s t0{};
	t0.timestamp = rt_tick_get();
	t0.val = 0;
	(void)pub_test.publish(t0);

	sensor_demo_s s0{};
	s0.timestamp = t0.timestamp;
	s0.x = 0; s0.y = 0; s0.z = 0;
	(void)pub_sens.publish(s0);
	int sens_inst = pub_sens.instance();

	uORB::Subscription sub_test{ORB_ID(orb_test)};
	uORB::SubscriptionInterval sub_sens{ORB_ID(sensor_demo), 200, (uint8_t)(sens_inst < 0 ? 0 : sens_inst)};

	int32_t cnt = 1;
	while (1)
	{
		orb_test_s t{};
		t.timestamp = rt_tick_get();
		t.val = cnt;
		(void)pub_test.publish(t);

		sensor_demo_s s{};
		s.timestamp = t.timestamp;
		s.x = cnt * 1;
		s.y = cnt * 2;
		s.z = cnt * 3;
		(void)pub_sens.publish(s);

		orb_test_s t_in{};
		if (sub_test.update(&t_in) || sub_test.copy(&t_in) > 0)
		{
			rt_kprintf("cpp orb_test: ts=%u val=%d\n", (unsigned)t_in.timestamp, t_in.val);
		}

		sensor_demo_s s_in{};
		if (sub_sens.update(&s_in) || sub_sens.copy(&s_in) > 0)
		{
			rt_kprintf("cpp sensor_demo: ts=%u xyz=(%d,%d,%d)\n", (unsigned)s_in.timestamp, s_in.x, s_in.y, s_in.z);
		}

		cnt++;
		/* 等待最多200ms以接收事件触发 */
		( void )sub_test.wait(200);
		( void )sub_sens.wait(200);
		rt_thread_mdelay(100);
	}
}

int uorb_cpp_demo_init(void)
{
	rt_thread_t tid = rt_thread_create("u_cpp", uorb_cpp_demo_entry, RT_NULL, 2048, 15, 10);
	if (tid) rt_thread_startup(tid);
	return 0;
}

#if defined(UORB_USING_CXX) && defined(UORB_ENABLE_CPP_DEMO)
INIT_APP_EXPORT(uorb_cpp_demo_init);
#endif 