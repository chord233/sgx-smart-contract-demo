#include "enclave_t.h"
#include "enclave.h"
#include "contract_verifier.h"
#include "crypto_utils.h"

#include <sgx_trts.h>
#include <sgx_tcrypto.h>
#include <sgx_tseal.h>
#include <sgx_utils.h>
#include <sgx_tkey_exchange.h>

#include <string.h>
#include <stdlib.h>

// 全局状态
static bool g_verifier_initialized = false;
static contract_verifier_t g_verifier;
static sgx_aes_gcm_128bit_key_t g_master_key;

/**
 * 初始化智能合约验证器
 */
sgx_status_t ecall_init_contract_verifier(void) {
    sgx_status_t ret = SGX_SUCCESS;
    
    if (g_verifier_initialized) {
        ocall_print_string("Contract verifier already initialized");
        return SGX_SUCCESS;
    }
    
    // 生成主密钥
    ret = sgx_read_rand((uint8_t*)&g_master_key, sizeof(g_master_key));
    if (ret != SGX_SUCCESS) {
        ocall_print_string("Failed to generate master key");
        return ret;
    }
    
    // 初始化合约验证器
    ret = init_contract_verifier(&g_verifier);
    if (ret != SGX_SUCCESS) {
        ocall_print_string("Failed to initialize contract verifier");
        return ret;
    }
    
    g_verifier_initialized = true;
    ocall_print_string("Contract verifier initialized successfully");
    
    return SGX_SUCCESS;
}

/**
 * 验证智能合约执行
 */
sgx_status_t ecall_verify_contract_execution(
    const uint8_t* contract_code,
    size_t code_size,
    const uint8_t* input_data,
    size_t input_size,
    uint8_t* execution_result,
    size_t* result_size,
    uint64_t* gas_used
) {
    sgx_status_t ret = SGX_SUCCESS;
    
    if (!g_verifier_initialized) {
        ocall_print_string("Contract verifier not initialized");
        return SGX_ERROR_INVALID_STATE;
    }
    
    if (!contract_code || !input_data || !execution_result || !result_size || !gas_used) {
        ocall_print_string("Invalid parameters");
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 验证合约代码完整性
    sgx_sha256_hash_t code_hash;
    ret = sgx_sha256_msg(contract_code, code_size, &code_hash);
    if (ret != SGX_SUCCESS) {
        ocall_print_string("Failed to compute contract code hash");
        return ret;
    }
    
    // 记录审计日志
    ocall_audit_log(1, "Contract execution started", (uint8_t*)&code_hash, sizeof(code_hash));
    
    // 执行合约验证
    contract_execution_context_t context;
    memset(&context, 0, sizeof(context));
    
    context.contract_code = contract_code;
    context.code_size = code_size;
    context.input_data = input_data;
    context.input_size = input_size;
    context.gas_limit = 1000000; // 默认gas限制
    
    ret = execute_contract(&g_verifier, &context);
    if (ret != SGX_SUCCESS) {
        ocall_print_string("Contract execution failed");
        return ret;
    }
    
    // 复制执行结果
    if (*result_size < context.result_size) {
        *result_size = context.result_size;
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memcpy(execution_result, context.result_data, context.result_size);
    *result_size = context.result_size;
    *gas_used = context.gas_used;
    
    // 记录执行完成
    ocall_audit_log(1, "Contract execution completed", execution_result, *result_size);
    
    return SGX_SUCCESS;
}

/**
 * 生成执行证明
 */
sgx_status_t ecall_generate_execution_proof(
    const uint8_t* execution_hash,
    uint8_t* proof_buffer,
    size_t* proof_size
) {
    sgx_status_t ret = SGX_SUCCESS;
    
    if (!execution_hash || !proof_buffer || !proof_size) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 创建执行证明结构
    execution_proof_t proof;
    memset(&proof, 0, sizeof(proof));
    
    // 复制执行哈希
    memcpy(proof.execution_hash, execution_hash, 32);
    
    // 获取当前时间戳
    ocall_get_timestamp(&proof.timestamp);
    
    // 生成随机nonce
    ret = sgx_read_rand(proof.nonce, sizeof(proof.nonce));
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 计算证明签名
    sgx_ecc_state_handle_t ecc_handle;
    ret = sgx_ecc256_open_context(&ecc_handle);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 使用enclave私钥签名
    uint8_t proof_data[sizeof(execution_proof_t) - sizeof(sgx_ec256_signature_t)];
    memcpy(proof_data, &proof, sizeof(proof_data));
    
    sgx_ec256_private_t private_key;
    ret = generate_key_pair(&private_key, &proof.public_key);
    if (ret != SGX_SUCCESS) {
        sgx_ecc256_close_context(ecc_handle);
        return ret;
    }
    
    ret = sgx_ecdsa_sign(proof_data, sizeof(proof_data), &private_key, &proof.signature, ecc_handle);
    sgx_ecc256_close_context(ecc_handle);
    
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 复制证明到输出缓冲区
    if (*proof_size < sizeof(proof)) {
        *proof_size = sizeof(proof);
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memcpy(proof_buffer, &proof, sizeof(proof));
    *proof_size = sizeof(proof);
    
    return SGX_SUCCESS;
}

/**
 * 验证执行证明
 */
sgx_status_t ecall_verify_execution_proof(
    const uint8_t* proof,
    size_t proof_size,
    const uint8_t* execution_hash,
    int* is_valid
) {
    sgx_status_t ret = SGX_SUCCESS;
    
    if (!proof || !execution_hash || !is_valid) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    *is_valid = 0;
    
    if (proof_size != sizeof(execution_proof_t)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    execution_proof_t* exec_proof = (execution_proof_t*)proof;
    
    // 验证执行哈希
    if (memcmp(exec_proof->execution_hash, execution_hash, 32) != 0) {
        return SGX_SUCCESS; // 哈希不匹配，但不是错误
    }
    
    // 验证签名
    sgx_ecc_state_handle_t ecc_handle;
    ret = sgx_ecc256_open_context(&ecc_handle);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    uint8_t proof_data[sizeof(execution_proof_t) - sizeof(sgx_ec256_signature_t)];
    memcpy(proof_data, exec_proof, sizeof(proof_data));
    
    uint8_t verify_result;
    ret = sgx_ecdsa_verify(proof_data, sizeof(proof_data), &exec_proof->public_key, 
                          &exec_proof->signature, &verify_result, ecc_handle);
    
    sgx_ecc256_close_context(ecc_handle);
    
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    *is_valid = (verify_result == SGX_EC_VALID) ? 1 : 0;
    
    return SGX_SUCCESS;
}

/**
 * 获取enclave测量值
 */
sgx_status_t ecall_get_enclave_measurement(uint8_t* mr_enclave) {
    if (!mr_enclave) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    sgx_report_t report;
    sgx_target_info_t target_info;
    sgx_report_data_t report_data;
    
    memset(&target_info, 0, sizeof(target_info));
    memset(&report_data, 0, sizeof(report_data));
    
    sgx_status_t ret = sgx_create_report(&target_info, &report_data, &report);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    memcpy(mr_enclave, &report.body.mr_enclave, sizeof(sgx_measurement_t));
    
    return SGX_SUCCESS;
}

/**
 * 密封数据
 */
sgx_status_t ecall_seal_data(
    const uint8_t* plaintext,
    size_t plaintext_size,
    uint8_t* sealed_data,
    size_t* sealed_size
) {
    if (!plaintext || !sealed_data || !sealed_size) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    uint32_t required_size = sgx_calc_sealed_data_size(0, plaintext_size);
    if (*sealed_size < required_size) {
        *sealed_size = required_size;
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    sgx_status_t ret = sgx_seal_data(0, NULL, plaintext_size, plaintext, 
                                    required_size, (sgx_sealed_data_t*)sealed_data);
    
    if (ret == SGX_SUCCESS) {
        *sealed_size = required_size;
    }
    
    return ret;
}

/**
 * 解封数据
 */
sgx_status_t ecall_unseal_data(
    const uint8_t* sealed_data,
    size_t sealed_size,
    uint8_t* plaintext,
    size_t* plaintext_size
) {
    if (!sealed_data || !plaintext || !plaintext_size) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    uint32_t required_size = sgx_get_encrypt_txt_len((sgx_sealed_data_t*)sealed_data);
    if (*plaintext_size < required_size) {
        *plaintext_size = required_size;
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    uint32_t mac_text_len = sgx_get_add_mac_txt_len((sgx_sealed_data_t*)sealed_data);
    uint8_t* mac_text = (uint8_t*)malloc(mac_text_len);
    if (!mac_text) {
        return SGX_ERROR_OUT_OF_MEMORY;
    }
    
    sgx_status_t ret = sgx_unseal_data((sgx_sealed_data_t*)sealed_data, 
                                      mac_text, &mac_text_len,
                                      plaintext, (uint32_t*)plaintext_size);
    
    free(mac_text);
    
    return ret;
}

/**
 * 创建远程证明报告
 */
sgx_status_t ecall_create_report(
    const sgx_target_info_t* target_info,
    const sgx_report_data_t* report_data,
    sgx_report_t* report
) {
    if (!target_info || !report_data || !report) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    return sgx_create_report(target_info, report_data, report);
}

/**
 * 处理智能合约状态更新
 */
sgx_status_t ecall_handle_state_update(
    const uint8_t* state_key,
    size_t key_size,
    uint8_t* state_value,
    size_t* value_size,
    int operation,
    int* result
) {
    if (!state_key || !state_value || !value_size || !result) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    *result = 0;
    
    switch (operation) {
        case 0: // 读取
            ocall_storage_read(state_key, key_size, state_value, value_size, result);
            break;
            
        case 1: // 写入
            ocall_storage_write(state_key, key_size, state_value, *value_size, result);
            break;
            
        case 2: // 删除
            ocall_storage_delete(state_key, key_size, result);
            break;
            
        default:
            return SGX_ERROR_INVALID_PARAMETER;
    }
    
    return SGX_SUCCESS;
}