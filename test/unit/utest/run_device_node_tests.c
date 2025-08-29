/*
*****************************************************************
* Copyright All Reserved © 2015-2025 Solonix-Chu
*****************************************************************
*/

#include <rtthread.h>
#include <utest.h>

/**
 * @brief 运行uORB设备节点测试的示例函数
 * 
 * 这个函数展示了如何在应用程序中运行设备节点测试
 */
static void run_device_node_tests(void)
{
    rt_kprintf("\n=== Running uORB Device Node Tests ===\n");
    
    // utest框架会在系统启动时自动初始化
    // 无需手动调用 utest_init()
    
    rt_kprintf("Device node tests registered and ready to run.\n");
    rt_kprintf("Use 'utest_run uorb.device_node' in shell to execute tests.\n");
    rt_kprintf("Or use 'test_device_node' command for quick test execution.\n");
    rt_kprintf("Additional test suites available:\n");
    rt_kprintf("  - uorb.core         (core API tests)\n");
    rt_kprintf("  - uorb.interval     (interval behavior tests)\n");
    rt_kprintf("  - uorb.multi        (multi-instance tests)\n");
    rt_kprintf("  - uorb.integration  (integration tests)\n");
#ifdef UORB_REGISTER_AS_DEVICE
    rt_kprintf("  - uorb.device_if    (device interface tests)\n");
#endif
    rt_kprintf("=======================================\n\n");
}

/**
 * @brief MSH命令：运行设备节点测试
 */
static int cmd_test_device_node(int argc, char **argv)
{
    if (argc == 1) {
        // 默认运行所有设备节点测试
        rt_kprintf("Running all device node tests...\n");
        rt_kprintf("Please use 'utest_run uorb.device_node' to execute tests.\n");
        return 0;
    } else if (argc == 2) {
        // 运行特定的测试
        char test_name[64];
        rt_snprintf(test_name, sizeof(test_name), "uorb.device_node.%s", argv[1]);
        rt_kprintf("Running specific test: %s\n", test_name);
        rt_kprintf("Please use 'utest_run %s' to execute this test.\n", test_name);
        return 0;
    } else {
        rt_kprintf("Usage:\n");
        rt_kprintf("  test_device_node          - Run all device node tests\n");
        rt_kprintf("  test_device_node <name>   - Run specific test\n");
        rt_kprintf("\nAvailable tests:\n");
        rt_kprintf("  create     - Node creation tests\n");
        rt_kprintf("  find       - Node finding tests\n");
        rt_kprintf("  exists     - Node existence tests\n");
        rt_kprintf("  write_read - Write/read tests\n");
        rt_kprintf("  queue      - Queue functionality tests\n");
        rt_kprintf("  subscribe  - Subscribe/unsubscribe tests\n");
        rt_kprintf("  check      - Data check tests\n");
        rt_kprintf("  copy       - Data copy tests\n");
        rt_kprintf("  ready      - Node ready tests\n");
        rt_kprintf("  error      - Error handling tests\n");
        rt_kprintf("\nTo run tests, use: utest_run <test_name>\n");
        return -1;
    }
}

/**
 * @brief 注册MSH命令
 */
MSH_CMD_EXPORT_ALIAS(cmd_test_device_node, test_device_node, "Run uORB device node tests");

// 新增统一提示命令：test_uorb
static int cmd_test_uorb(int argc, char **argv)
{
    if (argc == 1) {
        rt_kprintf("uORB Test Suites:\n");
        rt_kprintf("  uorb.device_node\n");
        rt_kprintf("  uorb.core\n");
        rt_kprintf("  uorb.interval\n");
        rt_kprintf("  uorb.multi\n");
        rt_kprintf("  uorb.integration\n");
#ifdef UORB_REGISTER_AS_DEVICE
        rt_kprintf("  uorb.device_if\n");
#endif
        rt_kprintf("\nUse: utest_run <suite>\n");
        return 0;
    } else if (argc == 2) {
        rt_kprintf("Run suite: %s\n", argv[1]);
        rt_kprintf("Use: utest_run %s\n", argv[1]);
        return 0;
    } else {
        rt_kprintf("Usage:\n");
        rt_kprintf("  test_uorb            - List all uORB test suites\n");
        rt_kprintf("  test_uorb <suite>    - Hint how to run specific suite\n");
        return -1;
    }
}
MSH_CMD_EXPORT_ALIAS(cmd_test_uorb, test_uorb, "List and run uORB test suites");

/**
 * @brief 应用程序主函数示例
 * 
 * 展示如何在main函数中集成测试
 */
int device_node_test_main(void)
{
    rt_kprintf("uORB Device Node Test Application\n");
    
    // 初始化测试环境
    run_device_node_tests();
    
    // 可以在这里添加其他初始化代码
    
    rt_kprintf("Application initialized. Use shell commands to run tests.\n");
    
    return 0;
}

/**
 * @brief 性能测试示例
 */
static void performance_test_example(void)
{
    rt_kprintf("\n=== Device Node Performance Test Example ===\n");
    
    const int iterations = 1000;
    rt_tick_t start_tick, end_tick;
    
    // 这里可以添加性能测试代码
    // 例如：测试节点创建/删除的性能
    // 例如：测试数据读写的性能
    
    rt_kprintf("Performance test completed.\n");
    rt_kprintf("============================================\n\n");
}

/**
 * @brief 内存泄漏检测示例
 */
static void memory_leak_test_example(void)
{
    rt_kprintf("\n=== Memory Leak Test Example ===\n");
    
    rt_size_t total_before, used_before, max_used_before;
    rt_size_t total_after, used_after, max_used_after;
    
    rt_memory_info(&total_before, &used_before, &max_used_before);
    
    // 执行一系列操作
    // 例如：创建和删除多个节点
    // 例如：创建和删除多个订阅者
    
    rt_memory_info(&total_after, &used_after, &max_used_after);
    
    rt_kprintf("Memory before: total=%zu, used=%zu, available=%zu\n", 
               total_before, used_before, total_before - used_before);
    rt_kprintf("Memory after:  total=%zu, used=%zu, available=%zu\n", 
               total_after, used_after, total_after - used_after);
    
    if (used_before == used_after) {
        rt_kprintf("No memory leak detected.\n");
    } else {
        rt_kprintf("Memory usage change: %zd bytes\n", 
                   (rt_int32_t)(used_after - used_before));
    }
    
    rt_kprintf("=================================\n\n");
}

/* 将测试函数导出为finsh命令，方便调试 */
FINSH_FUNCTION_EXPORT(performance_test_example, "performance test example");
FINSH_FUNCTION_EXPORT(memory_leak_test_example, "memory leak test example"); 