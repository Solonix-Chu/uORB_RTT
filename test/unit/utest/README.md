# uORB Device Node RT-Thread utest

这个目录包含了使用RT-Thread utest框架的uORB设备节点单元测试。

## 文件说明

- `test_uorb_device_node.c` - 主测试文件，包含所有测试用例
- `run_device_node_tests.c` - 测试运行器示例，展示如何集成到应用中
- `SConscript` - SCons构建配置文件
- `uorb_test_msg.h` - 测试消息结构定义
- `README_device_node_tests.md` - 详细的测试说明文档

## 环境要求

### RT-Thread配置
在`menuconfig`中启用以下组件：
```
RT-Thread Components → 
    Utilities → 
        [*] Enable utest framework
```

### 依赖组件
- RT-Thread内核
- utest框架
- uORB库

## 集成到项目

### 方法1：使用SConscript（推荐）
将此目录添加到您的RT-Thread项目中：

```python
# 在您的SConscript中添加
if GetDepend(['PKG_USING_UTEST']):
    SConscript('path/to/utest/SConscript')
```

### 方法2：手动添加
在您的应用程序中包含测试文件：

```c
#include "test_uorb_device_node.c"
```

## 运行测试

### 在RT-Thread shell中运行

1. **运行所有uORB设备节点测试**:
```bash
utest_run uorb.device_node
```

2. **运行特定测试**:
```bash
utest_run uorb.device_node.create
utest_run uorb.device_node.find
utest_run uorb.device_node.write_read
```

3. **查看测试列表**:
```bash
utest_list
```

### 在应用程序中运行

```c
#include "run_device_node_tests.c"

int main(void)
{
    // 初始化RT-Thread
    
    // 运行测试
    run_device_node_tests();
    
    return 0;
}
```

## 测试用例

### 节点管理测试
- `test_orb_node_create` - 节点创建功能
- `test_orb_node_find` - 节点查找功能  
- `test_orb_node_exists` - 节点存在性检查

### 数据操作测试
- `test_orb_node_write_read` - 基本读写功能
- `test_orb_node_queue` - 队列功能测试

### 订阅管理测试
- `test_orb_subscribe_unsubscribe` - 订阅/取消订阅
- `test_orb_node_ready` - 节点准备状态
- `test_orb_copy` - 数据拷贝功能

### 发布机制测试
- `test_orb_advertise_publish` - 广告和发布功能
- `test_orb_multi_instance` - 多实例支持

## 调试和故障排除

### 启用调试输出
在测试文件中设置：
```c
#define DBG_ENABLE
#define DBG_SECTION_NAME "uorb.test"
#define DBG_LEVEL DBG_LOG
```

### 常见问题

1. **测试未注册**: 确保在编译时包含了测试文件
2. **内存不足**: 增加heap大小或减少测试数据量
3. **断言失败**: 检查uORB库是否正确初始化

## 性能考虑

- 测试在RT-Thread环境中运行，会消耗系统资源
- 建议在开发阶段运行，生产环境中可以禁用
- 可以通过条件编译控制测试的包含

## 扩展测试

### 添加新测试用例
```c
static void test_new_feature(void)
{
    // 测试实现
    uassert_true(condition);
}

// 注册测试
UTEST_TC_EXPORT(test_new_feature, "uorb.device_node.new_feature", utest_tc_init, utest_tc_cleanup, 10);
```

### 自定义测试消息
在`uorb_test_msg.h`中添加新的消息结构：
```c
struct new_test_msg_s {
    uint64_t timestamp;
    // 其他字段
};
```

## 与Google Test的对比

| 特性 | RT-Thread utest | Google Test |
|------|----------------|-------------|
| 运行环境 | 目标硬件 | PC开发环境 |
| 真实性 | 真实RT-Thread环境 | 模拟环境 |
| 调试 | 硬件调试器 | GDB等PC调试器 |
| 速度 | 较慢（硬件限制） | 快速 |
| 资源消耗 | 受硬件限制 | 充足 |

## 许可证

本测试套件遵循Apache-2.0许可证。 