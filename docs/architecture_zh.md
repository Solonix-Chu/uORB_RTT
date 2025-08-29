# uORB 组件设计与架构说明

## 一、概述

uORB（Micro Object Request Broker）是运行于 RT-Thread 的轻量级发布/订阅（Pub/Sub）消息中间件，参考 PX4 uORB 语义实现。目标是在嵌入式资源受限环境下提供高效、可观测、可扩展的主题通信能力，支持多实例、可配置队列、回调与设备模型适配。

## 二、设计目标

- 语义对齐：与 PX4 uORB 的 API 与使用方式基本一致
- 小巧高效：单核/中断场景可用，临界区短，无阻塞发布
- 可观测：命令行 status/top、设备化统计与 demo
- 可扩展：.msg 生成、多实例/队列、事件化等待

## 三、核心组成

- 元数据（metadata）
  - 通过 `ORB_DEFINE` 实例化 `struct orb_metadata`，包含主题名、结构大小、字段签名与枚举 ID
  - 字段签名中支持元标签：`@queue=N;@instances=M;`（由生成脚本嵌入）
- 主题节点（orb_node_t）
  - 每个主题实例对应一个节点，包含：环形队列存储、当前代数（generation）、订阅者计数、公告状态（advertised）、事件通知器、回调链表
  - 队列深度按 2 的幂向上取整以利于取模性能
- 订阅者（orb_subscribe_t）
  - 保存订阅的元数据、实例号、最小更新间隔、最近更新时刻、已消费代数、与绑定的节点指针
- API 层（`uorb_core.c` + `uorb_device_node.c`）
  - 提供 `orb_advertise*`、`orb_publish`、`orb_subscribe*`、`orb_copy`、`orb_check`、`orb_wait`、`orb_exists` 等
- 设备化适配（可选，`uorb_device_if.c`）
  - 将主题实例导出为 `/dev/<topic><instance>` 设备；提供 `read/write/control` 统计/检查/节流等
- CLI（`uorb_cli.c`）
  - `uorb status/top/test/dev` 与 `uorb wait`，便于演示与排障
- 生成工具（`tools/msggen.py`）
  - 从 `msg/*.msg` 生成 `inc/topics/*.h` 与 `src/metadata/*_metadata.c`

## 四、数据流与时序

- 发布（Publish）
  - `orb_publish` 将新数据写入环形缓冲（`orb_node_write`），自增 `generation`，标记 data_valid，并触发回调与事件通知
- 订阅（Subscribe/Copy）
  - `orb_subscribe[_multi]` 绑定节点并初始化订阅者的 `generation`
  - `orb_check` 比较订阅者已消费代数与节点当前代数，并结合 `interval` 节流
  - `orb_copy` 读取对应代数的数据，更新订阅者代数（队列>1 时按环形窗口校正）
- 等待（Wait）
  - `orb_wait` 优先通过事件阻塞等待更新；缺省退化为短间隔轮询

## 五、多实例与队列

- 多实例：同一主题可有多个实例（默认上限 `ORB_MULTI_MAX_INSTANCES=4`），`orb_advertise_multi` 自动选择或返回实例号
- 队列深度：队列=1 表示覆盖语义；队列>1 为环形 FIFO，订阅者按 `generation` 顺序消费，过快覆盖将导致最旧数据丢弃

## 六、并发与内存管理

- 并发策略：节点写入路径使用短临界区保护，避免长时间持锁；订阅端读取按 `generation` 无需长锁
- 事件通知：发布时 `uorb_notifier_notify`；订阅端 `orb_wait` 使用事件等待
- 节点生命周期：`orb_unadvertise`/设备注销在无订阅者时释放节点；示例与 CLI 可协助观测泄漏

## 七、错误码约定

- 成功：`RT_EOK` 或正数长度（如 `orb_copy`）
- 参数错误：`-RT_EINVAL`
- 未找到/不存在：`-RT_ENOENT`
- 超时：`-RT_ETIMEOUT`
- 其他错误：`-RT_ERROR`

## 八、构建与开关

- Kconfig：`RT_USING_UORB`、`UORB_USING_MSG_GEN`、`UORB_USING_RTDEVICE`
- SCons：`SConscript` 自动执行 `tools/msggen.py` 生成代码，失败回退到 demo 主题

## 九、可观测性与调试

- CLI：`uorb status`/`uorb top`/`uorb wait`/`uorb dev ...`
- 设备：`rt_device_control` 可查询状态与设置读间隔
- 打印：`orb_print_message_internal` 提供十六进制转储（可逐步增强为按字段打印）
