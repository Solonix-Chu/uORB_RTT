/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include "uorb_device_node.h"
#include "uorb_device_if.h"
#include <rtthread.h>
#include <string.h>
#include <stdlib.h>

#if defined(UORB_USING_MSG_GEN) && defined(UORB_TOPICS_GENERATED)
#include "topics/orb_test.h"
#include "topics/sensor_demo.h"
#else
#include "uorb_demo_topics.h"
#endif

extern rt_list_t _orb_node_list;
extern rt_bool_t _orb_node_list_initialized;

static const struct orb_metadata *find_meta_by_name(const char *name)
{
    if (!_orb_node_list_initialized) return RT_NULL;
    rt_list_t *pos;
    rt_enter_critical();
    rt_list_for_each(pos, &_orb_node_list)
    {
        orb_node_t *node = rt_list_entry(pos, orb_node_t, list);
        if (rt_strcmp(node->meta->o_name, name) == 0)
        {
            const struct orb_metadata *m = node->meta;
            rt_exit_critical();
            return m;
        }
    }
    rt_exit_critical();
    return RT_NULL;
}

static void uorb_test_basic(void)
{
    struct orb_test_s t = {0};
    int inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(orb_test), &t, &inst);
    if (!adv || inst < 0)
    {
        rt_kprintf("uorb test basic: FAIL\n");
        return;
    }
    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst);
    if (!sub)
    {
        rt_kprintf("uorb test basic: FAIL\n");
        orb_unadvertise(adv);
        return;
    }

    int ok = 1;
    for (int i = 1; i <= 3; i++)
    {
        t.timestamp = rt_tick_get();
        t.val = i;
        if (orb_publish(ORB_ID(orb_test), adv, &t) != RT_EOK) { ok = 0; break; }
        rt_bool_t updated = RT_FALSE;
        if (orb_check(sub, &updated) != RT_EOK || !updated) { ok = 0; break; }
        struct orb_test_s rx;
        if (orb_copy(ORB_ID(orb_test), sub, &rx) <= 0 || rx.val != i) { ok = 0; break; }
    }
    rt_kprintf("uorb test basic: %s\n", ok ? "OK" : "FAIL");

    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}

static void uorb_test_interval(void)
{
    struct orb_test_s t = {0};
    int inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(orb_test), &t, &inst);
    if (!adv || inst < 0) { rt_kprintf("uorb test interval: FAIL\n"); return; }
    orb_subscr_t sub = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst);
    if (!sub) { rt_kprintf("uorb test interval: FAIL\n"); orb_unadvertise(adv); return; }
    orb_set_interval(sub, 200);

    int ok = 1;
    t.timestamp = rt_tick_get(); t.val = 1; orb_publish(ORB_ID(orb_test), adv, &t);
    rt_bool_t updated = RT_FALSE;
    if (ok && (orb_check(sub, &updated) != RT_EOK || !updated)) ok = 0;
    struct orb_test_s rx;
    if (ok && orb_copy(ORB_ID(orb_test), sub, &rx) <= 0) ok = 0;

    /* 立即再次发布并检查：应因 interval 未到而 updated=FALSE */
    t.timestamp = rt_tick_get(); t.val = 2; orb_publish(ORB_ID(orb_test), adv, &t);
    updated = RT_TRUE;
    if (ok && (orb_check(sub, &updated) != RT_EOK || updated)) ok = 0;

    rt_thread_mdelay(250);
    updated = RT_FALSE;
    if (ok && (orb_check(sub, &updated) != RT_EOK || !updated)) ok = 0;

    rt_kprintf("uorb test interval: %s\n", ok ? "OK" : "FAIL");
    orb_unsubscribe(sub);
    orb_unadvertise(adv);
}

/* 多实例用例：演示同一主题的不同实例彼此隔离 */
static void uorb_test_multi_instance(void)
{
    int ok = 1;

    /* 两个发布者，各自获得不同的实例号 */
    struct orb_test_s t0 = {0};
    struct orb_test_s t1 = {0};
    int inst0 = -1;
    int inst1 = -1;
    orb_advert_t adv0 = orb_advertise_multi(ORB_ID(orb_test), &t0, &inst0);
    orb_advert_t adv1 = orb_advertise_multi(ORB_ID(orb_test), &t1, &inst1);
    if (!adv0 || !adv1 || inst0 < 0 || inst1 < 0 || inst0 == inst1)
    {
        rt_kprintf("uorb test multi: FAIL (advertise_multi)\n");
        return;
    }

    /* 订阅各自实例 */
    orb_subscr_t sub0 = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst0);
    orb_subscr_t sub1 = orb_subscribe_multi(ORB_ID(orb_test), (rt_uint8_t)inst1);
    if (!sub0 || !sub1)
    {
        rt_kprintf("uorb test multi: FAIL (subscribe_multi)\n");
        if (sub0) orb_unsubscribe(sub0);
        if (sub1) orb_unsubscribe(sub1);
        return;
    }

    /* 清空各自实例的初始公告更新，建立基线 */
    rt_bool_t up = RT_FALSE;
    struct orb_test_s dummy = {0};
    if (orb_check(sub0, &up) == RT_EOK && up)
    {
        (void)orb_copy(ORB_ID(orb_test), sub0, &dummy);
    }
    up = RT_FALSE;
    if (orb_check(sub1, &up) == RT_EOK && up)
    {
        (void)orb_copy(ORB_ID(orb_test), sub1, &dummy);
    }

    /* 发布到实例0，只应唤醒 sub0 */
    t0.timestamp = rt_tick_get();
    t0.val = 11;
    if (orb_publish(ORB_ID(orb_test), adv0, &t0) != RT_EOK) ok = 0;
    rt_bool_t up0 = RT_FALSE, up1 = RT_FALSE;
    if (ok && (orb_check(sub0, &up0) != RT_EOK || !up0)) ok = 0;
    struct orb_test_s rx = {0};
    if (ok && (orb_copy(ORB_ID(orb_test), sub0, &rx) <= 0 || rx.val != 11)) ok = 0;
    up1 = RT_FALSE;
    if (ok && (orb_check(sub1, &up1) != RT_EOK || up1)) ok = 0;

    /* 发布到实例1，只应唤醒 sub1 */
    t1.timestamp = rt_tick_get();
    t1.val = 22;
    if (orb_publish(ORB_ID(orb_test), adv1, &t1) != RT_EOK) ok = 0;
    up1 = RT_FALSE; up0 = RT_FALSE;
    if (ok && (orb_check(sub1, &up1) != RT_EOK || !up1)) ok = 0;
    if (ok && (orb_copy(ORB_ID(orb_test), sub1, &rx) <= 0 || rx.val != 22)) ok = 0;
    up0 = RT_FALSE;
    if (ok && (orb_check(sub0, &up0) != RT_EOK)) ok = 0;
    if (ok && up0) ok = 0;

    rt_kprintf("uorb test multi: %s (inst0=%d, inst1=%d)\n", ok ? "OK" : "FAIL", inst0, inst1);

    orb_unsubscribe(sub0);
    orb_unsubscribe(sub1);
    if (adv0) orb_unadvertise(adv0);
    if (adv1) orb_unadvertise(adv1);
}

#ifdef UORB_REGISTER_AS_DEVICE
static void uorb_test_device(void)
{
    /* 注册设备并比较设备读与 API 读一致性，验证设备interval */
    rt_uorb_register_topic(ORB_ID(sensor_demo), 0);
    char devname[RT_NAME_MAX] = {0};
    uorb_make_dev_name(ORB_ID(sensor_demo), 0, devname, sizeof(devname));
    rt_device_t dev = rt_device_find(devname);
    if (!dev || rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("uorb test device: OPEN FAIL\n");
        return;
    }

    /* 设置设备读间隔 500ms */
    unsigned iv = 500;
    rt_device_control(dev, UORB_DEVICE_CTRL_SET_INTERVAL, &iv);

    /* 启动发布 */
    struct sensor_demo_s s = {0};
    orb_advert_t pub = orb_advertise(ORB_ID(sensor_demo), &s);

    int ok = 1;
    s.timestamp = rt_tick_get(); s.x=1; s.y=2; s.z=3; orb_publish(ORB_ID(sensor_demo), pub, &s);
    struct sensor_demo_s d = {0};
    rt_size_t n = rt_device_read(dev, 0, &d, sizeof(d));
    if (n == 0) ok = 0;

    /* 立即再次读，应返回0（间隔未到） */
    memset(&d, 0, sizeof(d));
    n = rt_device_read(dev, 0, &d, sizeof(d));
    if (n != 0) ok = 0;

    rt_thread_mdelay(600);
    s.timestamp = rt_tick_get(); s.x=4; s.y=5; s.z=6; orb_publish(ORB_ID(sensor_demo), pub, &s);
    memset(&d, 0, sizeof(d));
    n = rt_device_read(dev, 0, &d, sizeof(d));
    if (n == 0) ok = 0;

    rt_device_close(dev);
    rt_kprintf("uorb test device: %s\n", ok ? "OK" : "FAIL");
}
#endif

static void uorb_cmd_status(const char *filter)
{
    if (!_orb_node_list_initialized)
    {
        rt_kprintf("uORB: no topics (list not initialized)\n");
        return;
    }

    int count = 0;
    rt_list_t *pos;

    rt_enter_critical();
    rt_list_for_each(pos, &_orb_node_list)
    {
        orb_node_t *node = rt_list_entry(pos, orb_node_t, list);
        if (filter && filter[0])
        {
            if (rt_strncmp(node->meta->o_name, filter, RT_NAME_MAX) != 0)
            {
                continue;
            }
        }
#ifdef UORB_REGISTER_AS_DEVICE
        rt_kprintf("topic=%s inst=%d dev=/dev/%s%d q=%d gen=%u subs=%d adv=%d valid=%d size=%d\n",
                   node->meta->o_name,
                   node->instance,
                   node->meta->o_name,
                   node->instance,
                   node->queue_size,
                   (unsigned)node->generation,
                   node->subscriber_count,
                   node->advertised,
                   node->data_valid,
                   node->meta->o_size);
#else
        rt_kprintf("topic=%s inst=%d q=%d gen=%u subs=%d adv=%d valid=%d size=%d\n",
                   node->meta->o_name,
                   node->instance,
                   node->queue_size,
                   (unsigned)node->generation,
                   node->subscriber_count,
                   node->advertised,
                   node->data_valid,
                   node->meta->o_size);
#endif
        count++;
    }
    rt_exit_critical();

    rt_kprintf("total=%d\n", count);
}

static void uorb_cmd_top(const char *filter, int loops, int interval_ms, int max_items)
{
    if (loops <= 0) loops = 3;
    if (interval_ms <= 0) interval_ms = 500;

    unsigned last_gen_sum = 0;

    /* 跟踪每个条目的上一代数，用于估算每项频率 */
    #define TOP_MAX_TRACK 64
    struct top_track { const struct orb_metadata *meta; rt_uint8_t inst; unsigned prev_gen; int used; };
    struct top_track tracks[TOP_MAX_TRACK] = {0};

    /* 初始化基线，避免首轮 delta 为 0 */
    if (_orb_node_list_initialized)
    {
        rt_list_t *pos0;
        rt_enter_critical();
        rt_list_for_each(pos0, &_orb_node_list)
        {
            orb_node_t *node0 = rt_list_entry(pos0, orb_node_t, list);
            int idx0 = -1;
            for (int i = 0; i < TOP_MAX_TRACK; i++)
            {
                if (tracks[i].used && tracks[i].meta == node0->meta && tracks[i].inst == node0->instance)
                {
                    idx0 = i; break;
                }
            }
            if (idx0 < 0)
            {
                for (int i = 0; i < TOP_MAX_TRACK; i++)
                {
                    if (!tracks[i].used)
                    {
                        tracks[i].used = 1;
                        tracks[i].meta = node0->meta;
                        tracks[i].inst = node0->instance;
                        tracks[i].prev_gen = (unsigned)node0->generation;
                        idx0 = i; break;
                    }
                }
            }
        }
        rt_exit_critical();
        rt_thread_mdelay(interval_ms);
    }

    while (loops-- > 0)
    {
        if (!_orb_node_list_initialized)
        {
            rt_kprintf("uORB: no topics (list not initialized)\n");
            return;
        }

        int count = 0;
        int printed = 0;
        rt_list_t *pos;
        unsigned gen_sum = 0;

        static rt_tick_t last_tick = 0;
        rt_tick_t now_tick = rt_tick_get();
        int dt_ms = 0;
        if (last_tick == 0) dt_ms = interval_ms; else dt_ms = (int)((now_tick - last_tick) * 1000 / RT_TICK_PER_SECOND);
        last_tick = now_tick;

        rt_enter_critical();
        /* 预统计用于表头 */
        int count_total = 0;
        rt_list_for_each(pos, &_orb_node_list)
        {
            orb_node_t *n = rt_list_entry(pos, orb_node_t, list);
            if (!filter || !filter[0] || rt_strncmp(n->meta->o_name, filter, RT_NAME_MAX) == 0)
            {
                count_total++;
            }
        }
        rt_kprintf("update: %ds, num topics: %d\n", (dt_ms + 500) / 1000, count_total);
        rt_kprintf("TOPIC NAME                    INST #SUB #MSG #LOST #QSIZE\n");
        /* 打印条目 */
        rt_list_for_each(pos, &_orb_node_list)
        {
            orb_node_t *node = rt_list_entry(pos, orb_node_t, list);
            if (filter && filter[0])
            {
                if (rt_strncmp(node->meta->o_name, filter, RT_NAME_MAX) != 0)
                {
                    continue;
                }
            }
            gen_sum += (unsigned)node->generation;

            /* 查找或占位跟踪项 */
            int idx = -1;
            for (int i = 0; i < TOP_MAX_TRACK; i++)
            {
                if (tracks[i].used && tracks[i].meta == node->meta && tracks[i].inst == node->instance)
                {
                    idx = i; break;
                }
            }
            if (idx < 0)
            {
                for (int i = 0; i < TOP_MAX_TRACK; i++)
                {
                    if (!tracks[i].used)
                    {
                        tracks[i].used = 1;
                        tracks[i].meta = node->meta;
                        tracks[i].inst = node->instance;
                        tracks[i].prev_gen = (unsigned)node->generation;
                        idx = i; break;
                    }
                }
            }

            unsigned delta = 0;
            unsigned hz = 0;
            if (idx >= 0)
            {
                unsigned prev = tracks[idx].prev_gen;
                unsigned cur  = (unsigned)node->generation;
                delta = (cur >= prev) ? (cur - prev) : 0;
                if (interval_ms > 0)
                {
                    rt_uint64_t num = (rt_uint64_t)delta * 1000u;
                    hz = (unsigned)(num / (rt_uint64_t)interval_ms);
                }
                tracks[idx].prev_gen = cur;
            }

            if (max_items <= 0 || printed < max_items)
            {
                unsigned lost = 0;
                if (node->subscriber_count > 0)
                {
                    if (node->queue_size > 1)
                    {
                        lost = (delta > node->queue_size) ? (delta - node->queue_size) : 0;
                    }
                    else
                    {
                        lost = (delta > 1) ? (delta - 1) : 0;
                    }
                }
                rt_kprintf("%-28s %4d %4d %4u %5u %6d\n",
                           node->meta->o_name,
                           node->instance,
                           node->subscriber_count,
                           delta,
                           lost,
                           node->queue_size);
                printed++;
            }

            count++;
        }
        rt_exit_critical();

        if (last_gen_sum != 0)
        {
            unsigned delta_all = (gen_sum >= last_gen_sum) ? (gen_sum - last_gen_sum) : 0;
            unsigned hz_all = 0;
            if (count > 0 && interval_ms > 0)
            {
                rt_uint64_t num = (rt_uint64_t)delta_all * 1000u;
                rt_uint64_t den = (rt_uint64_t)interval_ms * (rt_uint64_t)count;
                hz_all = (unsigned)(num / den);
            }
            // rt_kprintf("items=%d, shown=%d, est_rate=%uHz\n", count, (max_items>0? (printed<max_items? printed:max_items):printed), hz_all);
        }
        else
        {
            // rt_kprintf("items=%d, shown=%d\n", count, (max_items>0? (printed<max_items? printed:max_items):printed));
        }
        last_gen_sum = gen_sum;
        rt_thread_mdelay(interval_ms);
    }
}

static int uorb_main(int argc, char **argv)
{
    if (argc <= 1)
    {
        rt_kprintf("usage: uorb status [topic]\n");
        rt_kprintf("       uorb top [topic] [loops] [interval_ms] [max_items]\n");
        rt_kprintf("       uorb test basic|interval|device|multi\n");
        rt_kprintf("       uorb wait <topic> [instance] [timeout_ms]\n");
#ifdef UORB_REGISTER_AS_DEVICE
        rt_kprintf("       uorb dev register <topic> <instance>\n");
        rt_kprintf("       uorb dev status <topic> <instance>\n");
#endif
        return 0;
    }

    if (rt_strcmp(argv[1], "status") == 0)
    {
        const char *filter = (argc >= 3) ? argv[2] : RT_NULL;
        uorb_cmd_status(filter);
        return 0;
    }

    if (rt_strcmp(argv[1], "top") == 0)
    {
        const char *filter = (argc >= 3) ? argv[2] : RT_NULL;
        int loops = (argc >= 4) ? atoi(argv[3]) : 3;
        int interval_ms = (argc >= 5) ? atoi(argv[4]) : 500;
        int max_items = (argc >= 6) ? atoi(argv[5]) : 0;
        uorb_cmd_top(filter, loops, interval_ms, max_items);
        return 0;
    }

    if (rt_strcmp(argv[1], "test") == 0)
    {
        if (argc >= 3)
        {
            if (rt_strcmp(argv[2], "basic") == 0) { uorb_test_basic(); return 0; }
            if (rt_strcmp(argv[2], "interval") == 0) { uorb_test_interval(); return 0; }
#ifdef UORB_REGISTER_AS_DEVICE
            if (rt_strcmp(argv[2], "device") == 0) { uorb_test_device(); return 0; }
#endif
            if (rt_strcmp(argv[2], "multi") == 0) { uorb_test_multi_instance(); return 0; }
        }
        rt_kprintf("usage: uorb test basic|interval|device|multi\n");
        return -1;
    }

    if (rt_strcmp(argv[1], "wait") == 0 && argc >= 3)
    {
        const char *topic = argv[2];
        int inst = (argc >= 4) ? atoi(argv[3]) : 0;
        int timeout_ms = (argc >= 5) ? atoi(argv[4]) : -1;
        const struct orb_metadata *meta = find_meta_by_name(topic);
        if (!meta)
        {
            rt_kprintf("unknown topic: %s\n", topic);
            return -1;
        }
        orb_subscr_t sub = orb_subscribe_multi(meta, (rt_uint8_t)inst);
        if (!sub)
        {
            rt_kprintf("subscribe failed\n");
            return -1;
        }
        int ret = orb_wait(sub, timeout_ms);
        if (ret == RT_EOK)
        {
            rt_kprintf("wait OK: %s[%d] updated\n", topic, inst);
        }
        else if (ret == -RT_ETIMEOUT)
        {
            rt_kprintf("wait TIMEOUT: %s[%d]\n", topic, inst);
        }
        else
        {
            rt_kprintf("wait FAIL: %s[%d] ret=%d\n", topic, inst, ret);
        }
        orb_unsubscribe(sub);
        return 0;
    }

#ifdef UORB_REGISTER_AS_DEVICE
    if (rt_strcmp(argv[1], "dev") == 0 && argc >= 5)
    {
        if (rt_strcmp(argv[2], "register") == 0)
        {
            const char *topic = argv[3];
            int inst = atoi(argv[4]);
            const struct orb_metadata *meta = find_meta_by_name(topic);
            if (!meta)
            {
                rt_kprintf("unknown topic: %s\n", topic);
                return -1;
            }
            int ret = rt_uorb_register_topic(meta, (rt_uint8_t)inst);
            rt_kprintf("register %s[%d]: %s\n", topic, inst, (ret == RT_EOK) ? "OK" : "FAIL");
            return 0;
        }
        if (rt_strcmp(argv[2], "status") == 0)
        {
            const char *topic = argv[3];
            int inst = atoi(argv[4]);
            const struct orb_metadata *meta = find_meta_by_name(topic);
            if (!meta)
            {
                rt_kprintf("unknown topic: %s\n", topic);
                return -1;
            }
            char devname[RT_NAME_MAX] = {0};
            uorb_make_dev_name(meta, (rt_uint8_t)inst, devname, sizeof(devname));
            rt_device_t dev = rt_device_find(devname);
            if (dev)
            {
                struct uorb_device_status st;
                if (rt_device_control(dev, UORB_DEVICE_CTRL_GET_STATUS, &st) == RT_EOK)
                {
                    rt_kprintf("dev=/dev/%s inst=%d q=%d gen=%u subs=%d adv=%d valid=%d size=%d\n",
                               devname,
                               st.instance,
                               st.queue_size,
                               (unsigned)st.generation,
                               st.subscriber_count,
                               st.advertised,
                               st.data_valid,
                               st.meta ? st.meta->o_size : -1);
                }
                else
                {
                    rt_kprintf("device control failed: %s\n", devname);
                    return -1;
                }
            }
            else
            {
                /* 设备不存在，回退打印节点信息 */
                if (!_orb_node_list_initialized)
                {
                    rt_kprintf("no node list\n");
                    return -1;
                }
                rt_list_t *pos;
                int found = 0;
                rt_enter_critical();
                rt_list_for_each(pos, &_orb_node_list)
                {
                    orb_node_t *node = rt_list_entry(pos, orb_node_t, list);
                    if (node->meta == meta && node->instance == inst)
                    {
                        rt_kprintf("node=%s[%d] q=%d gen=%u subs=%d adv=%d valid=%d size=%d [no device]\n",
                                   node->meta->o_name,
                                   node->instance,
                                   node->queue_size,
                                   (unsigned)node->generation,
                                   node->subscriber_count,
                                   node->advertised,
                                   node->data_valid,
                                   node->meta->o_size);
                        found = 1;
                        break;
                    }
                }
                rt_exit_critical();
                if (!found)
                {
                    rt_kprintf("node not found: %s[%d]\n", topic, inst);
                    return -1;
                }
            }
            return 0;
        }
    }
#endif

    rt_kprintf("unknown subcommand: %s\n", argv[1]);
    return -1;
}

MSH_CMD_EXPORT_ALIAS(uorb_main, uorb, uorb); 