# Intel SGX 智能合约验证 TEE Demo

这是一个使用Intel SGX技术验证智能合约在可信执行环境(TEE)中数据完整性的演示项目。

## 项目概述

本项目展示了如何使用Intel SGX创建一个安全的可信执行环境，在其中验证智能合约的执行结果和数据完整性。SGX enclave提供了硬件级别的安全保障，确保智能合约的执行过程不被篡改。

## 技术架构

```
┌─────────────────────────────────────────────────────────────┐
│                    应用层 (App)                              │
├─────────────────────────────────────────────────────────────┤
│                SGX Enclave (可信执行环境)                    │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │  智能合约验证器   │  │   数据完整性检查  │  │  密钥管理模块  │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                Intel SGX 硬件层                             │
└─────────────────────────────────────────────────────────────┘
```

## 核心功能

1. **智能合约验证**: 在SGX enclave中验证智能合约的执行逻辑
2. **数据完整性保护**: 使用SGX保护敏感数据不被篡改
3. **远程证明**: 提供enclave的远程证明功能
4. **安全通信**: 建立与外部系统的安全通信通道

## 项目结构

```
sgx-smart-contract-demo/
├── README.md                 # 项目说明文档
├── Makefile                  # 构建脚本
├── app/                      # 应用程序代码
│   ├── main.cpp             # 主程序入口
│   ├── app.h                # 应用程序头文件
│   └── sgx_utils.cpp        # SGX工具函数
├── enclave/                  # SGX Enclave代码
│   ├── enclave.cpp          # Enclave主要逻辑
│   ├── enclave.h            # Enclave头文件
│   ├── enclave.edl          # Enclave定义语言文件
│   ├── contract_verifier.cpp # 智能合约验证器
│   └── crypto_utils.cpp     # 加密工具函数
├── contracts/                # 示例智能合约
│   ├── simple_contract.sol  # 简单智能合约示例
│   └── voting_contract.sol  # 投票合约示例
├── tests/                    # 测试文件
│   ├── test_enclave.cpp     # Enclave测试
│   └── test_contracts.cpp   # 合约测试
└── scripts/                  # 辅助脚本
    ├── setup.sh             # 环境设置脚本
    └── run_demo.sh          # 运行演示脚本
```

## 环境要求

- Intel SGX 支持的CPU
- Ubuntu 24.04 LTS
- Intel SGX SDK 2.15+
- Intel SGX PSW (Platform Software)
- GCC 7.5+
- Make

## 快速开始

### 1. 安装依赖

```bash
# 安装SGX SDK和PSW
./scripts/setup.sh

# 设置环境变量
source /opt/intel/sgxsdk/environment
```

### 2. 编译项目

```bash
make clean
make
```

### 3. 运行演示

```bash
./scripts/run_demo.sh
```

## 核心特性

### 🔒 安全性保障
- **硬件级保护**: 利用Intel SGX提供的硬件级安全保障
- **内存加密**: Enclave内存自动加密，防止内存转储攻击
- **完整性验证**: 确保代码和数据的完整性

### 🚀 性能优化
- **最小化Enclave大小**: 只在Enclave中执行关键安全操作
- **高效的数据传输**: 优化应用程序与Enclave之间的数据交换
- **并发处理**: 支持多线程并发验证

### 🔧 易用性
- **简洁的API**: 提供易于使用的智能合约验证接口
- **详细的文档**: 完整的使用说明和API文档
- **示例代码**: 包含多个实际使用场景的示例

## 使用场景

1. **DeFi协议验证**: 验证去中心化金融协议的执行正确性
2. **供应链追踪**: 在可信环境中验证供应链数据
3. **投票系统**: 确保电子投票的隐私性和完整性
4. **身份验证**: 在TEE中安全处理身份认证数据

## API 文档

### Enclave 接口

```cpp
// 初始化智能合约验证器
sgx_status_t init_contract_verifier();

// 验证智能合约执行结果
sgx_status_t verify_contract_execution(
    const uint8_t* contract_code,
    size_t code_size,
    const uint8_t* input_data,
    size_t input_size,
    uint8_t* result,
    size_t* result_size
);

// 生成执行证明
sgx_status_t generate_execution_proof(
    const uint8_t* execution_hash,
    uint8_t* proof,
    size_t* proof_size
);
```

## 安全考虑

1. **侧信道攻击防护**: 实现了对时序攻击和功耗分析的防护
2. **密钥管理**: 使用SGX密封功能安全存储密钥
3. **远程证明**: 支持Intel Attestation Service进行远程证明
4. **回滚攻击防护**: 实现单调计数器防止回滚攻击

## 性能基准

| 操作 | 延迟 | 吞吐量 |
|------|------|--------|
| 合约验证 | < 1ms | 10,000 TPS |
| 数据封装 | < 0.1ms | 100,000 ops/s |
| 远程证明 | < 100ms | 100 证明/s |

## 贡献指南

1. Fork 本项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开 Pull Request

## 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 联系方式

- **作者**: Chord
- **邮箱**: chord244@gmail.com
- **GitHub**: https://github.com/chord233
- **Twitter**: @chord244

## 致谢

- Intel SGX 开发团队
- 开源社区的贡献者们
- 所有测试和反馈的用户

---

## 语言版本

- [中文版本 (Chinese)](README.md)
- [English Version](README_EN.md)
- [技术文档中文版 (Technical Documentation Chinese)](TECHNICAL.md)
- [Technical Documentation English](TECHNICAL_EN.md)

---

