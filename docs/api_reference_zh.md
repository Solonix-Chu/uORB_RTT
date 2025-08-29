# uORB API 参考手册

- 返回值：成功 `RT_EOK` 或正数长度；错误为负值（如 `-RT_EINVAL`）
- 句柄：`orb_advert_t`（发布者）、`orb_subscr_t`（订阅者）
- 元数据：`struct orb_metadata`，通过 `ORB_ID(topic)` 获取

## 宏与元数据

- `ORB_DECLARE(topic)`：声明主题元数据（供生成或外部引用）
- `ORB_DEFINE(topic, struct_type, size_no_padding, fields_signature, orb_id)`：定义主题元数据
- `ORB_ID(topic)`：取得 `const struct orb_metadata*`

## 主题广告（Publish 初始化）

- `orb_advert_t orb_advertise(const struct orb_metadata *meta, const void *data);`
  - 创建/获取实例0；可选发布初始数据
- `orb_advert_t orb_advertise_queue(const struct orb_metadata *meta, const void *data, unsigned int queue_size);`
  - 同上，指定队列深度
- `orb_advert_t orb_advertise_multi(const struct orb_metadata *meta, const void *data, int *instance);`
  - 多实例（返回 `*instance`）
- `orb_advert_t orb_advertise_multi_queue(const struct orb_metadata *meta, const void *data, int *instance, unsigned int queue_size);`
  - 多实例 + 队列深度
- `int orb_unadvertise(orb_advert_t handle);`
  - 取消公告；无人订阅时释放节点

## 发布

- `int orb_publish(const struct orb_metadata *meta, orb_advert_t handle, const void *data);`
  - 向主题写入数据；返回 `RT_EOK` 或错误

## 订阅与读取

- `orb_subscr_t orb_subscribe(const struct orb_metadata *meta);`
  - 订阅实例0
- `orb_subscr_t orb_subscribe_multi(const struct orb_metadata *meta, uint8_t instance);`
  - 订阅指定实例
- `int orb_unsubscribe(orb_subscr_t handle);`
  - 取消订阅
- `int orb_copy(const struct orb_metadata *meta, orb_subscr_t handle, void *buffer);`
  - 复制最新可用数据到 `buffer`，返回拷贝字节数
- `int orb_check(orb_subscr_t handle, rt_bool_t *updated);`
  - 检查是否有更新（结合 interval 节流）
- `int orb_wait(orb_subscr_t handle, int timeout_ms);`
  - 阻塞等待更新，`timeout_ms<0` 表示等待永远

## 工具

- `int orb_exists(const struct orb_metadata *meta, int instance);`
  - 实例是否已经公告
- `int orb_group_count(const struct orb_metadata *meta);`
  - 已公告实例数量
- `int orb_set_interval(orb_subscr_t sub, unsigned interval_ms);`
- `int orb_get_interval(orb_subscr_t sub, unsigned *interval_ms);`
- `const char *orb_get_c_type(unsigned char short_type);`
- `void orb_print_message_internal(const struct orb_metadata *meta, const void *data, bool print_topic_name);`

## 设备化（可选）

- 设备名：`/dev/<topic><instance>`
- `int rt_uorb_register_topic(const struct orb_metadata *meta, rt_uint8_t instance);`
- `int rt_uorb_unregister_topic(const struct orb_metadata *meta, rt_uint8_t instance);`
- `void uorb_make_dev_name(const struct orb_metadata *meta, rt_uint8_t instance, char *out, rt_size_t outsz);`
- `rt_device_control` 命令：
  - `UORB_DEVICE_CTRL_CHECK`（int*）
  - `UORB_DEVICE_CTRL_SET_INTERVAL`（unsigned*）
  - `UORB_DEVICE_CTRL_GET_STATUS`（struct uorb_device_status*）
  - `UORB_DEVICE_CTRL_EXISTS`（int*）

## 错误码

- `RT_EOK`、`-RT_EINVAL`、`-RT_ENOENT`、`-RT_ETIMEOUT`、`-RT_ERROR`

## 示例片段

- 发布：
```c
struct orb_test_s t = {0};
orb_advert_t pub = orb_advertise(ORB_ID(orb_test), &t);
t.val = 1; orb_publish(ORB_ID(orb_test), pub, &t);
```
- 订阅：
```c
orb_subscr_t sub = orb_subscribe(ORB_ID(orb_test));
rt_bool_t updated = RT_FALSE;
if (orb_check(sub, &updated) == RT_EOK && updated) {
    struct orb_test_s rx;
    if (orb_copy(ORB_ID(orb_test), sub, &rx) > 0) { /* use rx */ }
}
```
- 等待：
```c
if (orb_wait(sub, 1000) == RT_EOK) {
    struct orb_test_s rx; orb_copy(ORB_ID(orb_test), sub, &rx);
}
```