#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <sgx_error.h>
#include <sgx_tcrypto.h>
#include <sgx_key_exchange.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 常量定义
#define MAX_CONTRACT_SIZE       (1024 * 1024)  // 1MB
#define MAX_INPUT_SIZE          (64 * 1024)    // 64KB
#define MAX_RESULT_SIZE         (64 * 1024)    // 64KB
#define MAX_STATE_KEY_SIZE      256
#define MAX_STATE_VALUE_SIZE    (4 * 1024)     // 4KB
#define HASH_SIZE               32
#define NONCE_SIZE              16
#define DEFAULT_GAS_LIMIT       1000000

// 错误码定义
#define SGX_ERROR_CONTRACT_INVALID          (SGX_ERROR_UNEXPECTED + 0x1001)
#define SGX_ERROR_CONTRACT_EXECUTION_FAILED (SGX_ERROR_UNEXPECTED + 0x1002)
#define SGX_ERROR_INSUFFICIENT_GAS          (SGX_ERROR_UNEXPECTED + 0x1003)
#define SGX_ERROR_STATE_ACCESS_DENIED       (SGX_ERROR_UNEXPECTED + 0x1004)
#define SGX_ERROR_PROOF_GENERATION_FAILED   (SGX_ERROR_UNEXPECTED + 0x1005)

// 合约操作码
typedef enum {
    OP_NOP = 0x00,
    OP_PUSH = 0x01,
    OP_POP = 0x02,
    OP_ADD = 0x03,
    OP_SUB = 0x04,
    OP_MUL = 0x05,
    OP_DIV = 0x06,
    OP_MOD = 0x07,
    OP_AND = 0x08,
    OP_OR = 0x09,
    OP_XOR = 0x0A,
    OP_NOT = 0x0B,
    OP_EQ = 0x0C,
    OP_LT = 0x0D,
    OP_GT = 0x0E,
    OP_JMP = 0x0F,
    OP_JMPIF = 0x10,
    OP_CALL = 0x11,
    OP_RET = 0x12,
    OP_LOAD = 0x13,
    OP_STORE = 0x14,
    OP_HASH = 0x15,
    OP_VERIFY = 0x16,
    OP_HALT = 0xFF
} contract_opcode_t;

// 合约执行状态
typedef enum {
    CONTRACT_STATE_INIT = 0,
    CONTRACT_STATE_RUNNING,
    CONTRACT_STATE_COMPLETED,
    CONTRACT_STATE_ERROR,
    CONTRACT_STATE_OUT_OF_GAS
} contract_execution_state_t;

// 虚拟机栈
typedef struct {
    uint64_t data[256];  // 栈数据
    size_t top;          // 栈顶指针
} vm_stack_t;

// 合约执行上下文
typedef struct {
    const uint8_t* contract_code;           // 合约字节码
    size_t code_size;                       // 字节码大小
    const uint8_t* input_data;              // 输入数据
    size_t input_size;                      // 输入数据大小
    uint8_t* result_data;                   // 执行结果
    size_t result_size;                     // 结果大小
    uint64_t gas_limit;                     // Gas限制
    uint64_t gas_used;                      // 已使用Gas
    size_t pc;                              // 程序计数器
    contract_execution_state_t state;       // 执行状态
    vm_stack_t stack;                       // 虚拟机栈
    uint8_t memory[4096];                   // 临时内存
    sgx_sha256_hash_t execution_hash;       // 执行哈希
} contract_execution_context_t;

// 合约验证器
typedef struct {
    bool initialized;                       // 是否已初始化
    sgx_aes_gcm_128bit_key_t master_key;   // 主密钥
    uint64_t execution_counter;             // 执行计数器
    sgx_sha256_hash_t verifier_hash;        // 验证器哈希
} contract_verifier_t;

// 执行证明结构
typedef struct {
    uint8_t execution_hash[HASH_SIZE];      // 执行哈希
    uint64_t timestamp;                     // 时间戳
    uint8_t nonce[NONCE_SIZE];              // 随机数
    sgx_ec256_public_t public_key;          // 公钥
    sgx_ec256_signature_t signature;        // 签名
} execution_proof_t;

// 状态存储项
typedef struct {
    uint8_t key[MAX_STATE_KEY_SIZE];        // 状态键
    size_t key_size;                        // 键大小
    uint8_t value[MAX_STATE_VALUE_SIZE];    // 状态值
    size_t value_size;                      // 值大小
    uint64_t version;                       // 版本号
    sgx_sha256_hash_t hash;                 // 哈希值
} state_item_t;

// 审计日志级别
typedef enum {
    AUDIT_LEVEL_DEBUG = 0,
    AUDIT_LEVEL_INFO = 1,
    AUDIT_LEVEL_WARN = 2,
    AUDIT_LEVEL_ERROR = 3,
    AUDIT_LEVEL_CRITICAL = 4
} audit_level_t;

// 网络请求类型
typedef enum {
    HTTP_GET = 0,
    HTTP_POST = 1,
    HTTP_PUT = 2,
    HTTP_DELETE = 3
} http_method_t;

// 函数声明

/**
 * 初始化合约验证器
 * @param verifier 验证器实例
 * @return SGX状态码
 */
sgx_status_t init_contract_verifier(contract_verifier_t* verifier);

/**
 * 执行智能合约
 * @param verifier 验证器实例
 * @param context 执行上下文
 * @return SGX状态码
 */
sgx_status_t execute_contract(contract_verifier_t* verifier, contract_execution_context_t* context);

/**
 * 验证合约字节码
 * @param code 字节码
 * @param size 字节码大小
 * @return SGX状态码
 */
sgx_status_t validate_contract_code(const uint8_t* code, size_t size);

/**
 * 计算执行哈希
 * @param context 执行上下文
 * @param hash 输出哈希
 * @return SGX状态码
 */
sgx_status_t compute_execution_hash(const contract_execution_context_t* context, sgx_sha256_hash_t* hash);

/**
 * 生成ECC256密钥对
 * @param private_key 私钥输出
 * @param public_key 公钥输出
 * @return SGX状态码
 */
sgx_status_t generate_ec256_key_pair(sgx_ec256_private_t* private_key, sgx_ec256_public_t* public_key);

/**
 * 安全内存清零
 * @param ptr 内存指针
 * @param size 内存大小
 */
void secure_memzero(void* ptr, size_t size);

/**
 * 安全内存比较
 * @param a 内存块A
 * @param b 内存块B
 * @param size 比较大小
 * @return 比较结果
 */
int secure_memcmp(const void* a, const void* b, size_t size);

/**
 * 检查Gas消耗
 * @param context 执行上下文
 * @param gas_cost Gas成本
 * @return 是否有足够Gas
 */
bool check_gas(contract_execution_context_t* context, uint64_t gas_cost);

/**
 * 消耗Gas
 * @param context 执行上下文
 * @param gas_cost Gas成本
 * @return SGX状态码
 */
sgx_status_t consume_gas(contract_execution_context_t* context, uint64_t gas_cost);

/**
 * 栈操作 - 压入
 * @param stack 栈指针
 * @param value 值
 * @return SGX状态码
 */
sgx_status_t stack_push(vm_stack_t* stack, uint64_t value);

/**
 * 栈操作 - 弹出
 * @param stack 栈指针
 * @param value 值输出
 * @return SGX状态码
 */
sgx_status_t stack_pop(vm_stack_t* stack, uint64_t* value);

/**
 * 栈操作 - 查看栈顶
 * @param stack 栈指针
 * @param value 值输出
 * @return SGX状态码
 */
sgx_status_t stack_peek(const vm_stack_t* stack, uint64_t* value);

/**
 * 栈操作 - 检查是否为空
 * @param stack 栈指针
 * @return 是否为空
 */
bool stack_is_empty(const vm_stack_t* stack);

/**
 * 栈操作 - 检查是否已满
 * @param stack 栈指针
 * @return 是否已满
 */
bool stack_is_full(const vm_stack_t* stack);

/**
 * 获取操作码Gas成本
 * @param opcode 操作码
 * @return Gas成本
 */
uint64_t get_opcode_gas_cost(contract_opcode_t opcode);

/**
 * 验证内存访问
 * @param context 执行上下文
 * @param address 内存地址
 * @param size 访问大小
 * @return 是否有效
 */
bool validate_memory_access(const contract_execution_context_t* context, size_t address, size_t size);

/**
 * 执行单个指令
 * @param context 执行上下文
 * @param opcode 操作码
 * @return SGX状态码
 */
sgx_status_t execute_instruction(contract_execution_context_t* context, contract_opcode_t opcode);

#ifdef __cplusplus
}
#endif

#endif /* _ENCLAVE_H_ */