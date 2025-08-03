#ifndef CONTRACT_VERIFIER_H
#define CONTRACT_VERIFIER_H

#include "enclave.h"
#include "crypto_utils.h"

#include <sgx_tcrypto.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化合约验证器
 * @param verifier 验证器实例
 * @return SGX状态码
 */
sgx_status_t init_contract_verifier(contract_verifier_t* verifier);

/**
 * 验证合约字节码
 * @param code 合约字节码
 * @param size 字节码大小
 * @return SGX状态码
 */
sgx_status_t validate_contract_code(const uint8_t* code, size_t size);

/**
 * 执行智能合约
 * @param verifier 验证器实例
 * @param context 执行上下文
 * @return SGX状态码
 */
sgx_status_t execute_contract(contract_verifier_t* verifier, contract_execution_context_t* context);

/**
 * 执行单个指令
 * @param context 执行上下文
 * @param opcode 操作码
 * @return SGX状态码
 */
sgx_status_t execute_instruction(contract_execution_context_t* context, contract_opcode_t opcode);

/**
 * 计算执行哈希
 * @param context 执行上下文
 * @param hash 输出哈希
 * @return SGX状态码
 */
sgx_status_t compute_execution_hash(const contract_execution_context_t* context, sgx_sha256_hash_t* hash);

/**
 * 获取操作码Gas成本
 * @param opcode 操作码
 * @return Gas成本
 */
uint64_t get_opcode_gas_cost(contract_opcode_t opcode);

/**
 * 检查Gas是否足够
 * @param context 执行上下文
 * @param gas_cost Gas成本
 * @return 是否足够
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
 * 栈操作函数
 */
sgx_status_t stack_push(vm_stack_t* stack, uint64_t value);
sgx_status_t stack_pop(vm_stack_t* stack, uint64_t* value);
sgx_status_t stack_peek(const vm_stack_t* stack, uint64_t* value);
bool stack_is_empty(const vm_stack_t* stack);
bool stack_is_full(const vm_stack_t* stack);

/**
 * 验证内存访问
 * @param context 执行上下文
 * @param address 内存地址
 * @param size 访问大小
 * @return 是否有效
 */
bool validate_memory_access(const contract_execution_context_t* context, size_t address, size_t size);

#ifdef __cplusplus
}
#endif

#endif // CONTRACT_VERIFIER_H