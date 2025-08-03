# 快速开始指南

这是SGX智能合约验证演示项目的快速开始指南。

## 前提条件

1. 确保已安装Intel SGX SDK
2. 设置环境变量：`source /opt/intel/sgxsdk/environment`
3. 确保系统支持SGX（或在模拟模式下运行）

## 构建项目

```bash
# 清理并构建
make clean && make
```

## 运行演示

### 方法1：使用脚本
```bash
./scripts/run_demo.sh
```

### 方法2：直接运行
```bash
./app
```

## 演示功能

运行后会看到交互式菜单：

```
=== SGX Smart Contract Demo ===
1. Execute sample contract
2. Load and execute contract from file
3. Generate execution proof
4. Get enclave measurement
5. Create attestation report
0. Exit
```

### 功能说明

1. **执行示例合约**: 运行内置的简单加法合约（10 + 20 = 30）
2. **从文件加载合约**: 加载外部合约文件并执行
3. **生成执行证明**: 创建执行证明（模拟实现）
4. **获取Enclave测量值**: 显示Enclave的测量哈希
5. **创建远程证明报告**: 生成远程证明报告（模拟实现）

## 示例输出

执行示例合约时的典型输出：

```
=== Executing Smart Contract ===
Contract size: 20 bytes
Input data size: 0 bytes

=== Execution Results ===
Execution time: 245 microseconds
Result size: 8 bytes
Result data: 1e 00 00 00 00 00 00 00
Execution hash: a1b2c3d4e5f6...
```

## 注意事项

- 这是演示版本，包含模拟实现
- 证明生成和验证功能为模拟实现
- 适用于学习和理解SGX基本概念
- 不适用于生产环境

## 故障排除

### 编译错误
- 确保SGX SDK正确安装
- 检查环境变量是否设置
- 确保有足够的权限

### 运行时错误
- 检查SGX驱动是否正确安装
- 在不支持SGX的系统上，确保使用模拟模式
- 检查enclave.signed.so文件是否存在

## 下一步

- 查看 `DEMO_VERSION.md` 了解简化内容
- 阅读源代码了解SGX编程模式
- 尝试修改示例合约逻辑
- 学习完整版本的实现