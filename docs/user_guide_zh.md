# uORB 用户使用教程

## 一、前置条件

- RT-Thread 工程已可用，支持 FinSH
- SCons 构建环境

## 二、启用组件

在 Kconfig 中启用：
- `RT_USING_UORB=y`
- 可选：`UORB_USING_MSG_GEN=y`（开启 .msg 生成）
- 可选：`UORB_USING_RTDEVICE=y`（导出为设备）
- 可选：`UORB_ENABLE_DEMO=y`（启用 uORB 示例发布/订阅线程）
- 可选：`UORB_ENABLE_DEVTEST=y`（启用 uORB 设备化示例，需同时启用 `UORB_REGISTER_AS_DEVICE`）
- 注意：`UORB_ENABLE_DEMO` 与 `UORB_ENABLE_DEVTEST` 互斥，不能同时启用。

## 三、定义主题

- 方式A：使用 `.msg` 定义（推荐）
  1. 在 `libraries/uORB/msg/` 新增 `your_topic.msg`，包含：
     - 必须字段：`uint64 timestamp`
     - 其他字段：`int32 x` 等
     - 可选元标签：`%queue 4`、`%instances 2`
  2. 构建时 `tools/msggen.py` 自动生成：
     - `inc/topics/your_topic.h`（含 `struct your_topic_s` 与 `ORB_DECLARE`）
     - `src/metadata/your_topic_metadata.c`（`ORB_DEFINE`）
- 方式B：手工定义（demo）
  - 在 `src/uorb_demo_topics.c` 增加 `ORB_DEFINE`，并在 app 中 `#include "uorb_demo_topics.h"`

## 四、发布/订阅基础用法

- 发布端：
```c
#include "topics/your_topic.h"
struct your_topic_s t = {0};
orb_advert_t pub = orb_advertise(ORB_ID(your_topic), &t);
// 周期发布
orb_publish(ORB_ID(your_topic), pub, &t);
```
- 订阅端：
```c
#include "topics/your_topic.h"
orb_subscr_t sub = orb_subscribe(ORB_ID(your_topic));
rt_bool_t updated = RT_FALSE;
if (orb_check(sub, &updated) == RT_EOK && updated) {
    struct your_topic_s rx;
    if (orb_copy(ORB_ID(your_topic), sub, &rx) > 0) {
        // 使用 rx
    }
}
```
- 阻塞等待：
```c
if (orb_wait(sub, 1000) == RT_EOK) {
    struct your_topic_s rx; orb_copy(ORB_ID(your_topic), sub, &rx);
}
```

## 五、多实例与队列

- 多实例发布：
```c
int inst = 0;
orb_advert_t pub = orb_advertise_multi(ORB_ID(your_topic), &t, &inst);
```
- 队列深度：
  - `.msg` 中 `%queue N`，或 `orb_advertise_queue(..., queue_size)`

## 六、命令行（FinSH）调试

- `uorb status [topic]`：查看主题状态
- `uorb top [topic] [loops] [interval_ms] [max_items]`：监控刷新与频率估算
- `uorb wait <topic> [instance] [timeout_ms]`：阻塞等待主题更新
- `uorb test basic|interval|multi|device`：运行内置测试（如 basic/interval/多实例/设备化）
- 设备化启用：
  - `uorb dev register <topic> <instance>`
  - `uorb dev status <topic> <instance>`

## 七、设备接口（可选）

- 打开设备：`rt_device_open("/dev/your_topic0", RT_DEVICE_OFLAG_RDWR)`
- 读取：`rt_device_read(dev, 0, &rx, sizeof(rx))`
- 控制：`UORB_DEVICE_CTRL_SET_INTERVAL`、`UORB_DEVICE_CTRL_GET_STATUS`

## 八、错误排查

- 构建失败：检查 `.msg` 中 `timestamp` 是否存在且类型为 `uint64`
- 订阅无更新：确认已广告且同实例；检查 `interval` 是否过大
- 覆盖风险：`uorb top` 出现 `risk=OVERFLOW`，增大队列或降低发布频率

## 九、示例

- 源码位置：`libraries/uORB/examples/`
  - `uorb_demo_pub.c`：周期发布 `orb_test` 与 `sensor_demo`
  - `uorb_demo_sub.c`：订阅并打印上述主题
- 启用方式（需在 Kconfig 开启）：
  - `UORB_ENABLE_DEMO=y`
- 构建后运行：系统启动时通过 `INIT_APP_EXPORT` 自动创建线程（`u_pub`、`u_sub`），在 FinSH 中可直接观察打印；亦可配合 `uorb status/top` 辅助诊断。
- 与测试隔离：内置 `uorb test basic` 已采用多实例并在结束时释放资源，可与 demo 同时存在。

## 十、设备化示例

- 目的：演示将主题实例注册为 RT-Thread 设备节点并与 uORB API 数据对齐。
- 前置：在 Kconfig 启用
  - `UORB_USING_RTDEVICE=y`
  - `UORB_ENABLE_DEVTEST=y`
  - 同时满足 `UORB_REGISTER_AS_DEVICE=y`
- 示例位置：`libraries/uORB/examples/uorb_devtest.c`
- 运行机制：手动执行 `uorb_devtest` 命令后，自包含执行：注册 `/dev/sensor_demo0`，通过设备端发布（rt_device_write），并分别以设备与 uORB API 读取进行对比打印；不依赖 demo 示例，且与 `UORB_ENABLE_DEMO` 互斥。
- 常用命令：
  - `uorb dev register sensor_demo 0`：手动注册设备
  - `uorb dev status sensor_demo 0`：查看设备节点状态
  - 通过 `rt_device_control(dev, UORB_DEVICE_CTRL_SET_INTERVAL, &ms)` 设置设备读取间隔

## 十一、单元测试（uTest）

- 启用方式：
  - 在 Kconfig 开启 `RT_USING_UTEST`（或 `PKG_USING_UTEST`）。
  - 构建后，uORB 的单测会自动注册为 FinSH 命令。
- 常用命令：
  - 列表：`utest_list`
  - 执行：`utest_run <testcase>`，例如：
    - `utest_run uorb.core`
    - `utest_run uorb.interval`
    - `utest_run uorb.multi`
    - `utest_run uorb.integration`
    - `utest_run uorb.device_if`（需启用 `UORB_REGISTER_AS_DEVICE`）
- 说明：
  - 所有用例默认使用独立实例、多轮后释放资源，彼此隔离。
  - 建议关闭 demo（`UORB_ENABLE_DEMO=n`、`UORB_ENABLE_DEVTEST=n`）以降低并发与日志对栈的占用。
- 栈与运行环境建议（避免 `tidle0 stack overflow`）：
  - `IDLE_THREAD_STACK_SIZE`：≥ 2048（推荐 2048 或更高）