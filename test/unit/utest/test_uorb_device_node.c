/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include <rtthread.h>
#include <utest.h>
#include "uORB.h"
#include "uorb_device_node.h"
#include "uorb_test_msg.h"

/* Test initialization */
static rt_err_t utest_tc_init(void)
{
    rt_kprintf("uORB Device Node tests initialized\n");
    return RT_EOK;
}

/* Test cleanup */
static rt_err_t utest_tc_cleanup(void)
{
    rt_kprintf("uORB Device Node test cleanup completed\n");
    return RT_EOK;
}

/* Helper function to create test metadata */
static struct orb_metadata test_meta = {
    .o_name = "test_node",
    .o_size = sizeof(struct test_simple_s),
    .o_size_no_padding = sizeof(struct test_simple_s),
    .o_fields = "uint64_t timestamp;int32_t value;float data;bool flag",
    .o_id = 0
};

/* Test 1: Node Creation */
static void test_orb_node_create(void)
{
    rt_kprintf("Testing orb_node_create...\n");
    
    // Test normal creation
    orb_node_t *node = orb_node_create(&test_meta, 0, 1);
    uassert_not_null(node);
    uassert_ptr_equal(node->meta, &test_meta);
    uassert_int_equal(node->instance, 0);
    uassert_int_equal(node->queue_size, 1);
    uassert_int_equal(node->generation, 0);
    uassert_false(node->advertised);
    uassert_int_equal(node->subscriber_count, 0);
    uassert_false(node->data_valid);
    uassert_null(node->data);
    
    // Test with queue size that needs rounding
    orb_node_t *node2 = orb_node_create(&test_meta, 1, 3);
    uassert_not_null(node2);
    uassert_int_equal(node2->queue_size, 4); // Should round up to power of 2
    uassert_int_equal(node2->instance, 1);
    
    // Test with queue size 0 (should become 1)
    orb_node_t *node3 = orb_node_create(&test_meta, 2, 0);
    uassert_not_null(node3);
    uassert_int_equal(node3->queue_size, 1);
    
    // Cleanup
    orb_node_delete(node);
    orb_node_delete(node2);
    orb_node_delete(node3);
    
    rt_kprintf("orb_node_create test passed\n");
}

/* Test 2: Node Finding */
static void test_orb_node_find(void)
{
    rt_kprintf("Testing orb_node_find...\n");
    
    // Create test nodes
    orb_node_t *node1 = orb_node_create(&test_meta, 0, 1);
    orb_node_t *node2 = orb_node_create(&test_meta, 1, 1);
    
    // Test finding existing nodes
    orb_node_t *found1 = orb_node_find(&test_meta, 0);
    uassert_ptr_equal(found1, node1);
    
    orb_node_t *found2 = orb_node_find(&test_meta, 1);
    uassert_ptr_equal(found2, node2);
    
    // Test finding non-existing node
    orb_node_t *found3 = orb_node_find(&test_meta, 99);
    uassert_null(found3);
    
    // Cleanup
    orb_node_delete(node1);
    orb_node_delete(node2);
    
    rt_kprintf("orb_node_find test passed\n");
}

/* Test 3: Node Existence Check */
static void test_orb_node_exists(void)
{
    rt_kprintf("Testing orb_node_exists...\n");
    
    // Test with NULL metadata
    bool exists = orb_node_exists(RT_NULL, 0);
    uassert_false(exists);
    
    // Test with invalid instance numbers
    exists = orb_node_exists(&test_meta, -1);
    uassert_false(exists);
    
    exists = orb_node_exists(&test_meta, ORB_MULTI_MAX_INSTANCES);
    uassert_false(exists);
    
    // Create a node but don't advertise it
    orb_node_t *node = orb_node_create(&test_meta, 0, 1);
    exists = orb_node_exists(&test_meta, 0);
    uassert_false(exists); // Should be false because not advertised
    
    // Advertise the node
    node->advertised = true;
    exists = orb_node_exists(&test_meta, 0);
    uassert_true(exists); // Should be true now
    
    // Cleanup
    orb_node_delete(node);
    
    rt_kprintf("orb_node_exists test passed\n");
}

/* Test 4: Node Write and Read */
static void test_orb_node_write_read(void)
{
    rt_kprintf("Testing orb_node_write and orb_node_read...\n");
    
    orb_node_t *node = orb_node_create(&test_meta, 0, 1);
    
    // Create test data
    struct test_simple_s write_data = {
        .timestamp = 12345,
        .value = TEST_SIMPLE_VALUE_1,
        .data = TEST_SIMPLE_DATA_1,
        .flag = TEST_SIMPLE_FLAG_1
    };
    
    // Test write
    int write_result = orb_node_write(node, &write_data);
    uassert_int_equal(write_result, sizeof(struct test_simple_s));
    uassert_true(node->data_valid);
    uassert_int_equal(node->generation, 1);
    uassert_not_null(node->data);
    
    // Test read
    struct test_simple_s read_data;
    rt_uint32_t generation = 0;
    int read_result = orb_node_read(node, &read_data, &generation);
    uassert_int_equal(read_result, sizeof(struct test_simple_s));
    uassert_int_equal(read_data.timestamp, write_data.timestamp);
    uassert_int_equal(read_data.value, write_data.value);
    uassert_float_equal(read_data.data, write_data.data);
    uassert_int_equal(read_data.flag, write_data.flag);
    uassert_int_equal(generation, 1);
    
    // Test read with NULL generation
    int read_result2 = orb_node_read(node, &read_data, RT_NULL);
    uassert_int_equal(read_result2, sizeof(struct test_simple_s));
    
    // Cleanup
    orb_node_delete(node);
    
    rt_kprintf("orb_node_write_read test passed\n");
}

/* Test 5: Queue Functionality */
static void test_orb_node_queue(void)
{
    rt_kprintf("Testing orb_node queue functionality...\n");
    
    orb_node_t *node = orb_node_create(&test_meta, 0, 4); // Queue size 4
    
    // Write multiple messages
    struct test_simple_s data1 = {.timestamp = 1, .value = 1, .data = 1.0f, .flag = true};
    struct test_simple_s data2 = {.timestamp = 2, .value = 2, .data = 2.0f, .flag = false};
    struct test_simple_s data3 = {.timestamp = 3, .value = 3, .data = 3.0f, .flag = true};
    
    orb_node_write(node, &data1);
    uassert_int_equal(node->generation, 1);
    
    orb_node_write(node, &data2);
    uassert_int_equal(node->generation, 2);
    
    orb_node_write(node, &data3);
    uassert_int_equal(node->generation, 3);
    
    // Read messages in sequence
    struct test_simple_s read_data;
    rt_uint32_t generation = 0;
    
    // Read first message
    orb_node_read(node, &read_data, &generation);
    uassert_int_equal(read_data.value, 1);
    uassert_int_equal(generation, 1);
    
    // Read second message
    orb_node_read(node, &read_data, &generation);
    uassert_int_equal(read_data.value, 2);
    uassert_int_equal(generation, 2);
    
    // Read third message
    orb_node_read(node, &read_data, &generation);
    uassert_int_equal(read_data.value, 3);
    uassert_int_equal(generation, 3);
    
    // Cleanup
    orb_node_delete(node);
    
    rt_kprintf("orb_node_queue test passed\n");
}

/* Test 6: Subscribe/Unsubscribe */
static void test_orb_subscribe_unsubscribe(void)
{
    rt_kprintf("Testing orb_subscribe_multi and orb_unsubscribe...\n");
    
    // Create a node first
    orb_node_t *node = orb_node_create(&test_meta, 0, 1);
    node->advertised = true;
    
    // Test subscribe
    orb_subscribe_t *sub = orb_subscribe_multi(&test_meta, 0);
    uassert_not_null(sub);
    uassert_ptr_equal(sub->meta, &test_meta);
    uassert_int_equal(sub->instance, 0);
    uassert_int_equal(sub->interval, 0);
    uassert_int_equal(sub->generation, 0);
    
    // Test node ready check
    bool ready = orb_node_ready(sub);
    uassert_true(ready);
    uassert_ptr_equal(sub->node, node);
    uassert_int_equal(node->subscriber_count, 1);
    
    // Test unsubscribe
    int unsub_result = orb_unsubscribe(sub);
    uassert_int_equal(unsub_result, RT_EOK);
    uassert_int_equal(node->subscriber_count, 0);
    
    // Test unsubscribe with NULL handle
    int unsub_result2 = orb_unsubscribe(RT_NULL);
    uassert_int_equal(unsub_result2, -RT_ERROR);
    
    // Cleanup
    orb_node_delete(node);
    
    rt_kprintf("orb_subscribe_unsubscribe test passed\n");
}

/* Test 7: orb_check functionality */
static void test_orb_check(void)
{
    rt_kprintf("Testing orb_check...\n");
    
    // Create node and subscriber
    orb_node_t *node = orb_node_create(&test_meta, 0, 1);
    node->advertised = true;
    
    orb_subscribe_t *sub = orb_subscribe_multi(&test_meta, 0);
    orb_node_ready(sub); // Initialize subscription
    
    // Test with NULL parameters
    rt_bool_t updated;
    int result = orb_check(RT_NULL, &updated);
    uassert_int_equal(result, -RT_ERROR);
    
    result = orb_check(sub, RT_NULL);
    uassert_int_equal(result, -RT_ERROR);
    
    // Test with no new data
    result = orb_check(sub, &updated);
    uassert_int_equal(result, RT_EOK);
    uassert_false(updated);
    
    // Write data and check again
    struct test_simple_s data = {.timestamp = 100, .value = 42, .data = 3.14f, .flag = true};
    orb_node_write(node, &data);
    
    result = orb_check(sub, &updated);
    uassert_int_equal(result, RT_EOK);
    uassert_true(updated);
    
    // Cleanup
    orb_unsubscribe(sub);
    orb_node_delete(node);
    
    rt_kprintf("orb_check test passed\n");
}

/* Test 8: orb_copy functionality */
static void test_orb_copy(void)
{
    rt_kprintf("Testing orb_copy...\n");
    
    // Create node and subscriber
    orb_node_t *node = orb_node_create(&test_meta, 0, 1);
    node->advertised = true;
    
    orb_subscribe_t *sub = orb_subscribe_multi(&test_meta, 0);
    
    // Write test data
    struct test_simple_s write_data = {
        .timestamp = 67890,
        .value = TEST_SIMPLE_VALUE_2,
        .data = TEST_SIMPLE_DATA_2,
        .flag = TEST_SIMPLE_FLAG_2
    };
    orb_node_write(node, &write_data);
    
    // Test copy with NULL parameters
    struct test_simple_s read_data;
    int result = orb_copy(RT_NULL, sub, &read_data);
    uassert_int_equal(result, -RT_ERROR);
    
    result = orb_copy(&test_meta, RT_NULL, &read_data);
    uassert_int_equal(result, -RT_ERROR);
    
    result = orb_copy(&test_meta, sub, RT_NULL);
    uassert_int_equal(result, -RT_ERROR);
    
    // Test successful copy
    result = orb_copy(&test_meta, sub, &read_data);
    uassert_int_equal(result, sizeof(struct test_simple_s));
    uassert_int_equal(read_data.timestamp, write_data.timestamp);
    uassert_int_equal(read_data.value, write_data.value);
    uassert_float_equal(read_data.data, write_data.data);
    uassert_int_equal(read_data.flag, write_data.flag);
    
    // Cleanup
    orb_unsubscribe(sub);
    orb_node_delete(node);
    
    rt_kprintf("orb_copy test passed\n");
}

/* Test 9: Node Ready functionality */
static void test_orb_node_ready(void)
{
    rt_kprintf("Testing orb_node_ready...\n");
    
    // Test with NULL handle
    bool ready = orb_node_ready(RT_NULL);
    uassert_false(ready);
    
    // Create subscriber without corresponding node
    orb_subscribe_t *sub = orb_subscribe_multi(&test_meta, 0);
    ready = orb_node_ready(sub);
    uassert_false(ready); // No node exists yet
    
    // Create node but don't advertise
    orb_node_t *node = orb_node_create(&test_meta, 0, 1);
    ready = orb_node_ready(sub);
    uassert_false(ready); // Node exists but not advertised
    
    // Advertise the node
    node->advertised = true;
    ready = orb_node_ready(sub);
    uassert_true(ready); // Should be ready now
    
    // Check that subscriber is properly linked
    uassert_ptr_equal(sub->node, node);
    uassert_int_equal(node->subscriber_count, 1);
    
    // Cleanup
    orb_unsubscribe(sub);
    orb_node_delete(node);
    
    rt_kprintf("orb_node_ready test passed\n");
}

/* Test 10: Error handling and edge cases */
static void test_orb_error_handling(void)
{
    rt_kprintf("Testing error handling and edge cases...\n");
    
    // Test node_read with NULL data
    orb_node_t *node = orb_node_create(&test_meta, 0, 1);
    int result = orb_node_read(node, RT_NULL, RT_NULL);
    // Should assert fail in debug build, but we can't test that easily
    
    // Test node_read with no data written yet
    struct test_simple_s read_data;
    result = orb_node_read(node, &read_data, RT_NULL);
    uassert_int_equal(result, 0); // No data available
    
    // Test node_write with NULL data
    result = orb_node_write(node, RT_NULL);
    // Should assert fail in debug build
    
    // Test node_delete with NULL
    rt_err_t delete_result = orb_node_delete(RT_NULL);
    uassert_int_equal(delete_result, -RT_ERROR);
    
    // Test normal deletion
    delete_result = orb_node_delete(node);
    uassert_int_equal(delete_result, RT_EOK);
    
    rt_kprintf("orb_error_handling test passed\n");
}

/* Main test suite */
static void testcase(void)
{
    UTEST_UNIT_RUN(test_orb_node_create);
    UTEST_UNIT_RUN(test_orb_node_find);
    UTEST_UNIT_RUN(test_orb_node_exists);
    UTEST_UNIT_RUN(test_orb_node_write_read);
    UTEST_UNIT_RUN(test_orb_node_queue);
    UTEST_UNIT_RUN(test_orb_subscribe_unsubscribe);
    UTEST_UNIT_RUN(test_orb_check);
    UTEST_UNIT_RUN(test_orb_copy);
    UTEST_UNIT_RUN(test_orb_node_ready);
    UTEST_UNIT_RUN(test_orb_error_handling);
}

/* Export test case */
UTEST_TC_EXPORT(testcase, "uorb.device_node", utest_tc_init, utest_tc_cleanup, 30); 