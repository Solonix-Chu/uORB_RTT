/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include <rtthread.h>
#include <rtdevice.h>
#if defined(UORB_USING_MSG_GEN) && defined(UORB_TOPICS_GENERATED)
#include "topics/sensor_demo.h"
#else
#include "uorb_demo_topics.h"
#endif
#include "uorb_device_if.h"

static void uorb_devtest_entry(void *parameter)
{
#if defined(UORB_ENABLE_DEVTEST) && defined(UORB_REGISTER_AS_DEVICE)
    /* 注册并打开设备 */
    rt_uorb_register_topic(ORB_ID(sensor_demo), 0);

    static char devname[RT_NAME_MAX] = {0};
    uorb_make_dev_name(ORB_ID(sensor_demo), 0, devname, sizeof(devname));

    rt_device_t dev = rt_device_find(devname);
    if (!dev || rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("uorb_devtest: OPEN FAIL %s\n", devname);
        return;
    }

    /* 显式对实例0完成公告，使订阅端可见 */
    struct sensor_demo_s init = {0};
    int adv_inst = -1;
    orb_advert_t adv = orb_advertise_multi(ORB_ID(sensor_demo), &init, &adv_inst);
    rt_kprintf("uorb_devtest: advertised inst=%d (expect 0)\n", adv_inst);

    /* 设备读间隔设置，便于观察 */
    unsigned iv = 200;
    rt_device_control(dev, UORB_DEVICE_CTRL_SET_INTERVAL, &iv);
    rt_kprintf("uorb_devtest: register /dev/%s inst=0 interval=%ums\n", devname, iv);

    /* API 侧订阅实例0，并清空基线 */
    orb_subscr_t sub0 = orb_subscribe_multi(ORB_ID(sensor_demo), 0);
    rt_bool_t updated = RT_FALSE;
    struct sensor_demo_s drop;
    if (orb_check(sub0, &updated) == RT_EOK && updated) (void)orb_copy(ORB_ID(sensor_demo), sub0, &drop);
    rt_kprintf("uorb_devtest: baseline cleared for API sub[0]\n");

    /* 设备端发布一次（通过 rt_device_write） */
    struct sensor_demo_s tx = {0};
    tx.timestamp = rt_tick_get(); tx.x = 10; tx.y = 20; tx.z = 30;
    (void)rt_device_write(dev, 0, &tx, sizeof(tx));
    rt_kprintf("uorb_devtest: WRITE#1 via device -> x=%d y=%d z=%d\n", tx.x, tx.y, tx.z);

    /* 读取对比：设备读与 API 读 */
    struct sensor_demo_s rx_dev = {0};
    struct sensor_demo_s rx_api = {0};

    (void)rt_device_read(dev, 0, &rx_dev, sizeof(rx_dev));
    updated = RT_FALSE;
    if (orb_check(sub0, &updated) == RT_EOK && updated)
    {
        (void)orb_copy(ORB_ID(sensor_demo), sub0, &rx_api);
    }
    rt_kprintf("uorb_devtest: READ#1 dev=(%d,%d,%d), api_updated=%d, api=(%d,%d,%d)\n",
               rx_dev.x, rx_dev.y, rx_dev.z, updated, rx_api.x, rx_api.y, rx_api.z);
    int match1 = (rx_dev.x==rx_api.x && rx_dev.y==rx_api.y && rx_dev.z==rx_api.z);
    rt_kprintf("uorb_devtest: MATCH#1=%s\n", match1?"YES":"NO");

    /* 再次发布不同数据 */
    tx.timestamp = rt_tick_get(); tx.x = 11; tx.y = 22; tx.z = 33;
    (void)rt_device_write(dev, 0, &tx, sizeof(tx));
    rt_kprintf("uorb_devtest: WRITE#2 via device -> x=%d y=%d z=%d\n", tx.x, tx.y, tx.z);

    /* 等待一段时间以满足设备读间隔 */
    rt_thread_mdelay(250);

    (void)rt_device_read(dev, 0, &rx_dev, sizeof(rx_dev));
    updated = RT_FALSE;
    if (orb_check(sub0, &updated) == RT_EOK && updated)
    {
        (void)orb_copy(ORB_ID(sensor_demo), sub0, &rx_api);
    }
    rt_kprintf("uorb_devtest: READ#2 dev=(%d,%d,%d), api_updated=%d, api=(%d,%d,%d)\n",
               rx_dev.x, rx_dev.y, rx_dev.z, updated, rx_api.x, rx_api.y, rx_api.z);
    int match2 = (rx_dev.x==rx_api.x && rx_dev.y==rx_api.y && rx_dev.z==rx_api.z);
    rt_kprintf("uorb_devtest: MATCH#2=%s\n", match2?"YES":"NO");

    rt_device_close(dev);
#endif
    (void)parameter;
}

#if defined(UORB_ENABLE_DEVTEST) && defined(UORB_REGISTER_AS_DEVICE)
static int uorb_devtest_cmd(int argc, char **argv)
{
    RT_UNUSED(argc); RT_UNUSED(argv);
    uorb_devtest_entry(RT_NULL);
    return 0;
}
MSH_CMD_EXPORT_ALIAS(uorb_devtest_cmd, uorb_devtest, uORB device demo run);
#endif 