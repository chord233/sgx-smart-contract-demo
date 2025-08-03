#include "contract_verifier.h"
#include "enclave.h"
#include "enclave_t.h"
#include "crypto_utils.h"

#include <sgx_trts.h>
#include <sgx_tcrypto.h>
#include <string.h>
#include <stdlib.h>

// 简化的Gas成本表（演示版本）
static const uint64_t GAS_COSTS[] = {
    [OP_NOP] = 1,
    [OP_PUSH] = 3,
    [OP_POP] = 2,
    [OP_ADD] = 3,
    [OP_SUB] = 3,
    [OP_MUL] = 5,
    [OP_HALT] = 0
    // 其他操作码已简化
};

/**
 * 初始化合约验证器
 */
sgx_status_t init_contract_verifier(contract_verifier_t* verifier) {
    if (!verifier) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memset(verifier, 0, sizeof(contract_verifier_t));
    
    // 生成主密钥
    sgx_status_t ret = sgx_read_rand((uint8_t*)&verifier->master_key, sizeof(verifier->master_key));
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 初始化执行计数器
    verifier->execution_counter = 0;
    
    // 计算验证器哈希
    const char* verifier_info = "SGX Smart Contract Verifier v1.0";
    ret = sgx_sha256_msg((uint8_t*)verifier_info, strlen(verifier_info), &verifier->verifier_hash);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    verifier->initialized = true;
    
    return SGX_SUCCESS;
}

/**
 * 验证合约字节码
 */
sgx_status_t validate_contract_code(const uint8_t* code, size_t size) {
    if (!code || size == 0 || size > MAX_CONTRACT_SIZE) {
        return SGX_ERROR_CONTRACT_INVALID;
    }
    
    // 检查字节码格式
    for (size_t i = 0; i < size; i++) {
        contract_opcode_t opcode = (contract_opcode_t)code[i];
        
        // 验证操作码有效性
        switch (opcode) {
            case OP_NOP:
            case OP_PUSH:
            case OP_POP:
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
            case OP_MOD:
            case OP_AND:
            case OP_OR:
            case OP_XOR:
            case OP_NOT:
            case OP_EQ:
            case OP_LT:
            case OP_GT:
            case OP_JMP:
            case OP_JMPIF:
            case OP_CALL:
            case OP_RET:
            case OP_LOAD:
            case OP_STORE:
            case OP_HASH:
            case OP_VERIFY:
            case OP_HALT:
                break;
            default:
                // 对于PUSH指令，跳过操作数
                if (opcode == OP_PUSH && i + 8 < size) {
                    i += 8; // 跳过8字节操作数
                } else if (opcode == OP_JMP || opcode == OP_JMPIF) {
                    if (i + 4 < size) {
                        i += 4; // 跳过4字节跳转地址
                    }
                } else {
                    return SGX_ERROR_CONTRACT_INVALID;
                }
                break;
        }
    }
    
    // 检查是否以HALT指令结束
    if (code[size - 1] != OP_HALT) {
        return SGX_ERROR_CONTRACT_INVALID;
    }
    
    return SGX_SUCCESS;
}

/**
 * 执行智能合约
 */
sgx_status_t execute_contract(contract_verifier_t* verifier, contract_execution_context_t* context) {
    if (!verifier || !context || !verifier->initialized) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    sgx_status_t ret = SGX_SUCCESS;
    
    // 验证合约代码
    ret = validate_contract_code(context->contract_code, context->code_size);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 初始化执行上下文
    context->pc = 0;
    context->gas_used = 0;
    context->state = CONTRACT_STATE_RUNNING;
    memset(&context->stack, 0, sizeof(context->stack));
    memset(context->memory, 0, sizeof(context->memory));
    
    // 分配结果缓冲区
    context->result_data = (uint8_t*)malloc(MAX_RESULT_SIZE);
    if (!context->result_data) {
        return SGX_ERROR_OUT_OF_MEMORY;
    }
    context->result_size = 0;
    
    // 执行合约字节码
    while (context->state == CONTRACT_STATE_RUNNING && context->pc < context->code_size) {
        contract_opcode_t opcode = (contract_opcode_t)context->contract_code[context->pc];
        
        // 检查Gas
        uint64_t gas_cost = get_opcode_gas_cost(opcode);
        if (!check_gas(context, gas_cost)) {
            context->state = CONTRACT_STATE_OUT_OF_GAS;
            ret = SGX_ERROR_INSUFFICIENT_GAS;
            break;
        }
        
        // 执行指令
        ret = execute_instruction(context, opcode);
        if (ret != SGX_SUCCESS) {
            context->state = CONTRACT_STATE_ERROR;
            break;
        }
        
        // 消耗Gas
        consume_gas(context, gas_cost);
        
        // 更新程序计数器
        context->pc++;
        
        // 检查执行状态
        if (opcode == OP_HALT) {
            context->state = CONTRACT_STATE_COMPLETED;
            break;
        }
    }
    
    // 计算执行哈希
    if (context->state == CONTRACT_STATE_COMPLETED) {
        ret = compute_execution_hash(context, &context->execution_hash);
        if (ret != SGX_SUCCESS) {
            context->state = CONTRACT_STATE_ERROR;
        }
    }
    
    // 更新验证器状态
    verifier->execution_counter++;
    
    return ret;
}

/**
 * 执行单个指令
 */
sgx_status_t execute_instruction(contract_execution_context_t* context, contract_opcode_t opcode) {
    sgx_status_t ret = SGX_SUCCESS;
    uint64_t a, b, result;
    
    switch (opcode) {
        case OP_NOP:
            // 无操作
            break;
            
        case OP_PUSH:
            // 压入8字节常量
            if (context->pc + 8 >= context->code_size) {
                return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
            }
            memcpy(&result, &context->contract_code[context->pc + 1], 8);
            ret = stack_push(&context->stack, result);
            context->pc += 8; // 跳过操作数
            break;
            
        case OP_POP:
            ret = stack_pop(&context->stack, &result);
            break;
            
        case OP_ADD:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, a + b);
            break;
            
        case OP_SUB:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, a - b);
            break;
            
        case OP_MUL:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, a * b);
            break;
            
        case OP_DIV:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            if (b == 0) {
                return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
            }
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, a / b);
            break;
            
        case OP_MOD:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            if (b == 0) {
                return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
            }
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, a % b);
            break;
            
        case OP_AND:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, a & b);
            break;
            
        case OP_OR:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, a | b);
            break;
            
        case OP_XOR:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, a ^ b);
            break;
            
        case OP_NOT:
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, ~a);
            break;
            
        case OP_EQ:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, (a == b) ? 1 : 0);
            break;
            
        case OP_LT:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, (a < b) ? 1 : 0);
            break;
            
        case OP_GT:
            ret = stack_pop(&context->stack, &b);
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            ret = stack_push(&context->stack, (a > b) ? 1 : 0);
            break;
            
        case OP_JMP:
            // 无条件跳转
            if (context->pc + 4 >= context->code_size) {
                return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
            }
            memcpy(&result, &context->contract_code[context->pc + 1], 4);
            if (result >= context->code_size) {
                return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
            }
            context->pc = (size_t)result - 1; // -1因为主循环会+1
            break;
            
        case OP_JMPIF:
            // 条件跳转
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            if (a != 0) {
                if (context->pc + 4 >= context->code_size) {
                    return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
                }
                memcpy(&result, &context->contract_code[context->pc + 1], 4);
                if (result >= context->code_size) {
                    return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
                }
                context->pc = (size_t)result - 1;
            } else {
                context->pc += 4; // 跳过跳转地址
            }
            break;
            
        case OP_LOAD:
            // 从存储加载数据
            ret = stack_pop(&context->stack, &a); // 键
            if (ret != SGX_SUCCESS) break;
            
            // 这里应该调用存储接口
            // 简化实现：从内存加载
            if (a >= sizeof(context->memory) - 8) {
                return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
            }
            memcpy(&result, &context->memory[a], 8);
            ret = stack_push(&context->stack, result);
            break;
            
        case OP_STORE:
            // 存储数据
            ret = stack_pop(&context->stack, &b); // 值
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &a); // 键
            if (ret != SGX_SUCCESS) break;
            
            // 简化实现：存储到内存
            if (a >= sizeof(context->memory) - 8) {
                return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
            }
            memcpy(&context->memory[a], &b, 8);
            break;
            
        case OP_HASH:
            // 计算哈希
            ret = stack_pop(&context->stack, &a); // 数据长度
            if (ret != SGX_SUCCESS) break;
            ret = stack_pop(&context->stack, &b); // 数据地址
            if (ret != SGX_SUCCESS) break;
            
            if (b + a > sizeof(context->memory)) {
                return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
            }
            
            sgx_sha256_hash_t hash;
            ret = sgx_sha256_msg(&context->memory[b], (uint32_t)a, &hash);
            if (ret != SGX_SUCCESS) break;
            
            // 将哈希的前8字节压入栈
            memcpy(&result, &hash, 8);
            ret = stack_push(&context->stack, result);
            break;
            
        case OP_VERIFY:
            // 验证签名（简化实现）
            ret = stack_pop(&context->stack, &a);
            if (ret != SGX_SUCCESS) break;
            // 简化：总是返回验证成功
            ret = stack_push(&context->stack, 1);
            break;
            
        case OP_HALT:
            // 停止执行
            context->state = CONTRACT_STATE_COMPLETED;
            break;
            
        default:
            return SGX_ERROR_CONTRACT_EXECUTION_FAILED;
    }
    
    return ret;
}

/**
 * 计算执行哈希
 */
sgx_status_t compute_execution_hash(const contract_execution_context_t* context, sgx_sha256_hash_t* hash) {
    if (!context || !hash) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 创建哈希输入
    sgx_sha_state_handle_t sha_handle;
    sgx_status_t ret = sgx_sha256_init(&sha_handle);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 添加合约代码哈希
    sgx_sha256_hash_t code_hash;
    ret = sgx_sha256_msg(context->contract_code, context->code_size, &code_hash);
    if (ret != SGX_SUCCESS) {
        sgx_sha256_close(sha_handle);
        return ret;
    }
    
    ret = sgx_sha256_update((uint8_t*)&code_hash, sizeof(code_hash), sha_handle);
    if (ret != SGX_SUCCESS) {
        sgx_sha256_close(sha_handle);
        return ret;
    }
    
    // 添加输入数据哈希
    if (context->input_data && context->input_size > 0) {
        sgx_sha256_hash_t input_hash;
        ret = sgx_sha256_msg(context->input_data, context->input_size, &input_hash);
        if (ret != SGX_SUCCESS) {
            sgx_sha256_close(sha_handle);
            return ret;
        }
        
        ret = sgx_sha256_update((uint8_t*)&input_hash, sizeof(input_hash), sha_handle);
        if (ret != SGX_SUCCESS) {
            sgx_sha256_close(sha_handle);
            return ret;
        }
    }
    
    // 添加执行结果
    if (context->result_data && context->result_size > 0) {
        ret = sgx_sha256_update(context->result_data, context->result_size, sha_handle);
        if (ret != SGX_SUCCESS) {
            sgx_sha256_close(sha_handle);
            return ret;
        }
    }
    
    // 添加Gas使用量
    ret = sgx_sha256_update((uint8_t*)&context->gas_used, sizeof(context->gas_used), sha_handle);
    if (ret != SGX_SUCCESS) {
        sgx_sha256_close(sha_handle);
        return ret;
    }
    
    // 完成哈希计算
    ret = sgx_sha256_get_hash(sha_handle, hash);
    sgx_sha256_close(sha_handle);
    
    return ret;
}

/**
 * 获取操作码Gas成本
 */
uint64_t get_opcode_gas_cost(contract_opcode_t opcode) {
    if (opcode < sizeof(GAS_COSTS) / sizeof(GAS_COSTS[0])) {
        return GAS_COSTS[opcode];
    }
    return 1; // 默认成本
}

/**
 * 检查Gas
 */
bool check_gas(contract_execution_context_t* context, uint64_t gas_cost) {
    return (context->gas_used + gas_cost) <= context->gas_limit;
}

/**
 * 消耗Gas
 */
sgx_status_t consume_gas(contract_execution_context_t* context, uint64_t gas_cost) {
    if (!check_gas(context, gas_cost)) {
        return SGX_ERROR_INSUFFICIENT_GAS;
    }
    
    context->gas_used += gas_cost;
    return SGX_SUCCESS;
}

/**
 * 栈操作实现
 */
sgx_status_t stack_push(vm_stack_t* stack, uint64_t value) {
    if (!stack || stack_is_full(stack)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    stack->data[stack->top++] = value;
    return SGX_SUCCESS;
}

sgx_status_t stack_pop(vm_stack_t* stack, uint64_t* value) {
    if (!stack || !value || stack_is_empty(stack)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    *value = stack->data[--stack->top];
    return SGX_SUCCESS;
}

sgx_status_t stack_peek(const vm_stack_t* stack, uint64_t* value) {
    if (!stack || !value || stack_is_empty(stack)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    *value = stack->data[stack->top - 1];
    return SGX_SUCCESS;
}

bool stack_is_empty(const vm_stack_t* stack) {
    return stack ? (stack->top == 0) : true;
}

bool stack_is_full(const vm_stack_t* stack) {
    return stack ? (stack->top >= sizeof(stack->data) / sizeof(stack->data[0])) : true;
}

/**
 * 验证内存访问
 */
bool validate_memory_access(const contract_execution_context_t* context, size_t address, size_t size) {
    return context && (address + size <= sizeof(context->memory));
}