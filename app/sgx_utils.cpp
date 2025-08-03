#include "app.h"
#include "app_utils.h"
#include <fstream>
#include <iomanip>
#include <chrono>
#include <cstring>

// SGXSmartContractApp类实现
SGXSmartContractApp::SGXSmartContractApp() : enclave_id(0), enclave_initialized(false) {
}

SGXSmartContractApp::~SGXSmartContractApp() {
    destroy_enclave();
}

app_status_t SGXSmartContractApp::initialize_enclave() {
    if (enclave_initialized) {
        return APP_SUCCESS;
    }
    
    sgx_status_t ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &enclave_id, NULL);
    if (ret != SGX_SUCCESS) {
        print_error("创建Enclave失败: 0x" + std::to_string(ret));
        return APP_ERROR_ENCLAVE_INIT;
    }
    
    // 初始化验证器
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    ret = ecall_init_verifier(enclave_id, &enclave_ret);
    if (ret != SGX_SUCCESS || enclave_ret != SGX_SUCCESS) {
        print_error("初始化验证器失败");
        sgx_destroy_enclave(enclave_id);
        return APP_ERROR_ENCLAVE_INIT;
    }
    
    enclave_initialized = true;
    print_success("Enclave初始化成功，EID: " + std::to_string(enclave_id));
    return APP_SUCCESS;
}

void SGXSmartContractApp::destroy_enclave() {
    if (enclave_initialized && enclave_id != 0) {
        sgx_destroy_enclave(enclave_id);
        enclave_id = 0;
        enclave_initialized = false;
        print_success("Enclave已销毁");
    }
}

app_status_t SGXSmartContractApp::load_contract_from_file(const std::string& filename, SmartContract& contract) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        print_error("无法打开合约文件: " + filename);
        return APP_ERROR_FILE_IO;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (size > MAX_CONTRACT_SIZE) {
        print_error("合约文件过大: " + std::to_string(size) + " bytes");
        return APP_ERROR_INVALID_PARAM;
    }
    
    contract.bytecode.resize(size);
    file.read(reinterpret_cast<char*>(contract.bytecode.data()), size);
    contract.name = filename;
    
    print_success("加载合约成功: " + std::to_string(size) + " bytes");
    return APP_SUCCESS;
}

app_status_t SGXSmartContractApp::execute_contract(const SmartContract& contract,
                                                  const std::vector<uint8_t>& input_data,
                                                  ExecutionResult& result) {
    if (!enclave_initialized) {
        print_error("Enclave未初始化");
        return APP_ERROR_ENCLAVE_INIT;
    }
    
    if (contract.bytecode.empty()) {
        print_error("合约字节码为空");
        return APP_ERROR_INVALID_PARAM;
    }
    
    // 准备执行参数
    const uint8_t* contract_code = contract.bytecode.data();
    size_t code_size = contract.bytecode.size();
    const uint8_t* input = input_data.empty() ? nullptr : input_data.data();
    size_t input_size = input_data.size();
    
    // 结果缓冲区
    uint8_t result_buffer[MAX_OUTPUT_SIZE];
    size_t result_size = sizeof(result_buffer);
    uint8_t execution_hash[32];
    
    // 记录开始时间
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 调用Enclave执行合约
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    
    ret = ecall_execute_contract(
        enclave_id,
        &enclave_ret,
        contract_code,
        code_size,
        input,
        input_size,
        contract.gas_limit,
        result_buffer,
        &result_size,
        execution_hash
    );
    
    // 记录结束时间
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    if (ret != SGX_SUCCESS) {
        result.success = false;
        result.error_message = "SGX调用失败: 0x" + std::to_string(ret);
        return APP_ERROR_ENCLAVE_CALL;
    }
    
    if (enclave_ret != SGX_SUCCESS) {
        result.success = false;
        result.error_message = "合约执行失败: 0x" + std::to_string(enclave_ret);
        return APP_ERROR_ENCLAVE_CALL;
    }
    
    // 填充执行结果
    result.success = true;
    result.output.assign(result_buffer, result_buffer + result_size);
    result.execution_hash.assign(execution_hash, execution_hash + 32);
    result.gas_used = duration.count(); // 使用执行时间作为gas消耗的近似值
    
    return APP_SUCCESS;
}

app_status_t SGXSmartContractApp::generate_execution_proof(const SmartContract& contract,
                                                          const std::vector<uint8_t>& input_data,
                                                          ExecutionProof& proof) {
    if (!enclave_initialized) {
        return APP_ERROR_ENCLAVE_INIT;
    }
    
    uint8_t proof_data[1024];
    size_t proof_size = sizeof(proof_data);
    
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    
    ret = ecall_generate_proof(
        enclave_id,
        &enclave_ret,
        contract.bytecode.data(),
        contract.bytecode.size(),
        input_data.empty() ? nullptr : input_data.data(),
        input_data.size(),
        proof_data,
        &proof_size
    );
    
    if (ret != SGX_SUCCESS || enclave_ret != SGX_SUCCESS) {
        proof.is_valid = false;
        return APP_ERROR_ENCLAVE_CALL;
    }
    
    proof.proof_data.assign(proof_data, proof_data + proof_size);
    proof.is_valid = true;
    
    return APP_SUCCESS;
}

app_status_t SGXSmartContractApp::get_enclave_measurement(std::vector<uint8_t>& measurement) {
    if (!enclave_initialized) {
        return APP_ERROR_ENCLAVE_INIT;
    }
    
    uint8_t measurement_data[32];
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    
    ret = ecall_get_measurement(enclave_id, &enclave_ret, measurement_data);
    
    if (ret != SGX_SUCCESS || enclave_ret != SGX_SUCCESS) {
        return APP_ERROR_ENCLAVE_CALL;
    }
    
    measurement.assign(measurement_data, measurement_data + 32);
    return APP_SUCCESS;
}

app_status_t SGXSmartContractApp::create_attestation_report(const std::vector<uint8_t>& user_data,
                                                           std::vector<uint8_t>& report) {
    if (!enclave_initialized) {
        return APP_ERROR_ENCLAVE_INIT;
    }
    
    uint8_t report_data[64] = {0};
    if (!user_data.empty()) {
        size_t copy_size = std::min(user_data.size(), sizeof(report_data));
        memcpy(report_data, user_data.data(), copy_size);
    }
    
    uint8_t report_buffer[1024];
    size_t report_size = sizeof(report_buffer);
    
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    
    ret = ecall_create_report(
        enclave_id,
        &enclave_ret,
        report_data,
        report_buffer,
        &report_size
    );
    
    if (ret != SGX_SUCCESS || enclave_ret != SGX_SUCCESS) {
        return APP_ERROR_ENCLAVE_CALL;
    }
    
    report.assign(report_buffer, report_buffer + report_size);
    return APP_SUCCESS;
}

// 静态工具函数
SmartContract SGXSmartContractApp::create_sample_contract() {
    SmartContract contract;
    contract.name = "示例加法合约";
    contract.description = "计算两个数的和";
    
    // 简单的计算合约：计算两个数的和
    // PUSH 10
    contract.bytecode.push_back(0x01); // OP_PUSH
    for (int i = 0; i < 8; i++) {
        contract.bytecode.push_back((i == 0) ? 10 : 0);
    }
    
    // PUSH 20
    contract.bytecode.push_back(0x01); // OP_PUSH
    for (int i = 0; i < 8; i++) {
        contract.bytecode.push_back((i == 0) ? 20 : 0);
    }
    
    // ADD
    contract.bytecode.push_back(0x03); // OP_ADD
    
    // HALT
    contract.bytecode.push_back(0x18); // OP_HALT
    
    return contract;
}

std::vector<uint8_t> SGXSmartContractApp::create_sample_input() {
    std::vector<uint8_t> input_data;
    // 添加一些示例输入数据
    input_data.push_back(0x01);
    input_data.push_back(0x02);
    input_data.push_back(0x03);
    input_data.push_back(0x04);
    return input_data;
}

void SGXSmartContractApp::print_execution_result(const ExecutionResult& result) {
    std::cout << "\n=== 执行结果 ===" << std::endl;
    std::cout << "执行状态: " << (result.success ? "成功" : "失败") << std::endl;
    
    if (!result.success) {
        std::cout << "错误信息: " << result.error_message << std::endl;
        return;
    }
    
    std::cout << "Gas消耗: " << result.gas_used << std::endl;
    std::cout << "输出大小: " << result.output.size() << " bytes" << std::endl;
    
    if (!result.output.empty()) {
        std::cout << "输出数据: ";
        for (size_t i = 0; i < std::min(result.output.size(), size_t(32)); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)result.output[i] << " ";
        }
        if (result.output.size() > 32) {
            std::cout << "...";
        }
        std::cout << std::endl;
    }
    
    if (!result.execution_hash.empty()) {
        std::cout << "执行哈希: ";
        for (uint8_t byte : result.execution_hash) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        std::cout << std::endl;
    }
}

void SGXSmartContractApp::print_proof_info(const ExecutionProof& proof) {
    std::cout << "\n=== 执行证明 ===" << std::endl;
    std::cout << "证明状态: " << (proof.is_valid ? "有效" : "无效") << std::endl;
    std::cout << "证明大小: " << proof.proof_data.size() << " bytes" << std::endl;
    
    if (!proof.proof_data.empty()) {
        std::cout << "证明数据: ";
        for (size_t i = 0; i < std::min(proof.proof_data.size(), size_t(64)); i++) {
            if (i % 16 == 0) std::cout << "\n";
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)proof.proof_data[i] << " ";
        }
        std::cout << std::endl;
    }
}

// 全局工具函数
void print_error(const std::string& message) {
    std::cerr << "\033[31m[错误] " << message << "\033[0m" << std::endl;
}

void print_success(const std::string& message) {
    std::cout << "\033[32m[成功] " << message << "\033[0m" << std::endl;
}

void print_warning(const std::string& message) {
    std::cout << "\033[33m[警告] " << message << "\033[0m" << std::endl;
}

void print_app_info() {
    std::cout << "\n=== SGX智能合约验证演示 ===" << std::endl;
    std::cout << "版本: 1.0.0" << std::endl;
    std::cout << "作者: chord233" << std::endl;
    std::cout << "描述: 基于Intel SGX的智能合约安全执行和验证系统" << std::endl;
    std::cout << "==============================" << std::endl;
}