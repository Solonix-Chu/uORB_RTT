# uORB API Reference

- Return values: success `RT_EOK` or positive bytes copied; errors are negative (e.g., `-RT_EINVAL`)
- Handles: `orb_advert_t` (publisher), `orb_subscr_t` (subscriber)
- Metadata: `struct orb_metadata`, retrieved via `ORB_ID(topic)`

## Macros & Metadata

- `ORB_DECLARE(topic)`: declare topic metadata (for generator or external usage)
- `ORB_DEFINE(topic, struct_type, size_no_padding, fields_signature, orb_id)`: define topic metadata
- `ORB_ID(topic)`: get `const struct orb_metadata*`

## Advertisement (initial publication)

- `orb_advert_t orb_advertise(const struct orb_metadata *meta, const void *data);`
- `orb_advert_t orb_advertise_queue(const struct orb_metadata *meta, const void *data, unsigned int queue_size);`
- `orb_advert_t orb_advertise_multi(const struct orb_metadata *meta, const void *data, int *instance);`
- `orb_advert_t orb_advertise_multi_queue(const struct orb_metadata *meta, const void *data, int *instance, unsigned int queue_size);`
- `int orb_unadvertise(orb_advert_t handle);`

## Publish

- `int orb_publish(const struct orb_metadata *meta, orb_advert_t handle, const void *data);`

## Subscribe

- `orb_subscr_t orb_subscribe(const struct orb_metadata *meta);`
- `orb_subscr_t orb_subscribe_multi(const struct orb_metadata *meta, uint8_t instance);`
- `int orb_unsubscribe(orb_subscr_t handle);`
- `int orb_copy(const struct orb_metadata *meta, orb_subscr_t handle, void *buffer);`
- `int orb_check(orb_subscr_t handle, rt_bool_t *updated);`
- `int orb_wait(orb_subscr_t handle, int timeout_ms);`

## Utilities

- `int orb_exists(const struct orb_metadata *meta, int instance);`
- `int orb_group_count(const struct orb_metadata *meta);`
- `int orb_set_interval(orb_subscr_t sub, unsigned interval_ms);`
- `int orb_get_interval(orb_subscr_t sub, unsigned *interval_ms);`
- `const char *orb_get_c_type(unsigned char short_type);`
- `void orb_print_message_internal(const struct orb_metadata *meta, const void *data, bool print_topic_name);`

## Device interface (optional)

- Devices: `/dev/<topic><instance>`
- `int rt_uorb_register_topic(const struct orb_metadata *meta, rt_uint8_t instance);`
- `int rt_uorb_unregister_topic(const struct orb_metadata *meta, rt_uint8_t instance);`
- `void uorb_make_dev_name(const struct orb_metadata *meta, rt_uint8_t instance, char *out, rt_size_t outsz);`
- `rt_device_control` commands:
  - `UORB_DEVICE_CTRL_CHECK` (int*)
  - `UORB_DEVICE_CTRL_SET_INTERVAL` (unsigned*)
  - `UORB_DEVICE_CTRL_GET_STATUS` (struct uorb_device_status*)
  - `UORB_DEVICE_CTRL_EXISTS` (int*)

## Errors

- `RT_EOK`, `-RT_EINVAL`, `-RT_ENOENT`, `-RT_ETIMEOUT`, `-RT_ERROR`

## Snippets

- Publish:
```c
struct orb_test_s t = {0};
orb_advert_t pub = orb_advertise(ORB_ID(orb_test), &t);
t.val = 1; orb_publish(ORB_ID(orb_test), pub, &t);
```
- Subscribe:
```c
orb_subscr_t sub = orb_subscribe(ORB_ID(orb_test));
rt_bool_t updated = RT_FALSE;
if (orb_check(sub, &updated) == RT_EOK && updated) {
    struct orb_test_s rx;
    if (orb_copy(ORB_ID(orb_test), sub, &rx) > 0) { /* use rx */ }
}
```
- Wait:
```c
if (orb_wait(sub, 1000) == RT_EOK) {
    struct orb_test_s rx; orb_copy(ORB_ID(orb_test), sub, &rx);
}
```