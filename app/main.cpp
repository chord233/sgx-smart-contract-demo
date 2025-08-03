#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <chrono>
#include <memory>

#include "sgx_urts.h"
#include "enclave_u.h"
#include "app_utils.h"

// Enclave相关
#define ENCLAVE_FILENAME "enclave.signed.so"
#define MAX_PATH 260

// 全局Enclave ID
sgx_enclave_id_t global_eid = 0;

/**
 * 初始化Enclave
 */
sgx_status_t initialize_enclave() {
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    
    // 创建Enclave
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        std::cerr << "Failed to create enclave: 0x" << std::hex << ret << std::endl;
        return ret;
    }
    
    std::cout << "Enclave created successfully. EID: " << global_eid << std::endl;
    
    // 初始化验证器
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    ret = ecall_init_verifier(global_eid, &enclave_ret);
    if (ret != SGX_SUCCESS || enclave_ret != SGX_SUCCESS) {
        std::cerr << "Failed to initialize verifier: 0x" << std::hex << ret << ", 0x" << enclave_ret << std::endl;
        return (ret != SGX_SUCCESS) ? ret : enclave_ret;
    }
    
    std::cout << "Contract verifier initialized successfully." << std::endl;
    
    return SGX_SUCCESS;
}

/**
 * 销毁Enclave
 */
void destroy_enclave() {
    if (global_eid != 0) {
        sgx_destroy_enclave(global_eid);
        global_eid = 0;
        std::cout << "Enclave destroyed." << std::endl;
    }
}

/**
 * 加载智能合约字节码
 */
std::vector<uint8_t> load_contract_bytecode(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open contract file: " << filename << std::endl;
        return {};
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> bytecode(size);
    file.read(reinterpret_cast<char*>(bytecode.data()), size);
    
    std::cout << "Loaded contract bytecode: " << size << " bytes" << std::endl;
    
    return bytecode;
}

/**
 * 创建示例智能合约
 */
std::vector<uint8_t> create_sample_contract() {
    std::vector<uint8_t> bytecode;
    
    // 简单的计算合约：计算两个数的和
    // PUSH 10
    bytecode.push_back(0x01); // OP_PUSH
    for (int i = 0; i < 8; i++) {
        bytecode.push_back((i == 0) ? 10 : 0); // 推入值10
    }
    
    // PUSH 20
    bytecode.push_back(0x01); // OP_PUSH
    for (int i = 0; i < 8; i++) {
        bytecode.push_back((i == 0) ? 20 : 0); // 推入值20
    }
    
    // ADD
    bytecode.push_back(0x03); // OP_ADD
    
    // HALT
    bytecode.push_back(0x18); // OP_HALT
    
    std::cout << "Created sample contract: " << bytecode.size() << " bytes" << std::endl;
    
    return bytecode;
}

/**
 * 执行智能合约
 */
bool execute_smart_contract(const std::vector<uint8_t>& bytecode, const std::vector<uint8_t>& input_data) {
    if (global_eid == 0) {
        std::cerr << "Enclave not initialized" << std::endl;
        return false;
    }
    
    std::cout << "\n=== Executing Smart Contract ===" << std::endl;
    std::cout << "Contract size: " << bytecode.size() << " bytes" << std::endl;
    std::cout << "Input data size: " << input_data.size() << " bytes" << std::endl;
    
    // 准备执行参数
    const uint8_t* contract_code = bytecode.data();
    size_t code_size = bytecode.size();
    const uint8_t* input = input_data.empty() ? nullptr : input_data.data();
    size_t input_size = input_data.size();
    uint64_t gas_limit = 1000000; // 1M Gas
    
    // 结果缓冲区
    uint8_t result_buffer[4096];
    size_t result_size = sizeof(result_buffer);
    uint8_t execution_hash[32];
    
    // 记录开始时间
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 调用Enclave执行合约
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    
    ret = ecall_execute_contract(
        global_eid,
        &enclave_ret,
        contract_code,
        code_size,
        input,
        input_size,
        gas_limit,
        result_buffer,
        &result_size,
        execution_hash
    );
    
    // 记录结束时间
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    if (ret != SGX_SUCCESS) {
        std::cerr << "Failed to call enclave: 0x" << std::hex << ret << std::endl;
        return false;
    }
    
    if (enclave_ret != SGX_SUCCESS) {
        std::cerr << "Contract execution failed: 0x" << std::hex << enclave_ret << std::endl;
        return false;
    }
    
    // 显示执行结果
    std::cout << "\n=== Execution Results ===" << std::endl;
    std::cout << "Execution time: " << duration.count() << " microseconds" << std::endl;
    std::cout << "Result size: " << result_size << " bytes" << std::endl;
    
    if (result_size > 0) {
        std::cout << "Result data: ";
        for (size_t i = 0; i < std::min(result_size, size_t(32)); i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)result_buffer[i] << " ";
        }
        if (result_size > 32) {
            std::cout << "...";
        }
        std::cout << std::endl;
    }
    
    std::cout << "Execution hash: ";
    for (int i = 0; i < 32; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)execution_hash[i];
    }
    std::cout << std::endl;
    
    return true;
}

/**
 * 生成执行证明
 */
bool generate_execution_proof(const std::vector<uint8_t>& bytecode, const std::vector<uint8_t>& input_data) {
    if (global_eid == 0) {
        std::cerr << "Enclave not initialized" << std::endl;
        return false;
    }
    
    std::cout << "\n=== Generating Execution Proof ===" << std::endl;
    
    uint8_t proof_data[1024];
    size_t proof_size = sizeof(proof_data);
    
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    
    ret = ecall_generate_proof(
        global_eid,
        &enclave_ret,
        bytecode.data(),
        bytecode.size(),
        input_data.empty() ? nullptr : input_data.data(),
        input_data.size(),
        proof_data,
        &proof_size
    );
    
    if (ret != SGX_SUCCESS || enclave_ret != SGX_SUCCESS) {
        std::cerr << "Failed to generate proof: 0x" << std::hex << ret << ", 0x" << enclave_ret << std::endl;
        return false;
    }
    
    std::cout << "Proof generated successfully" << std::endl;
    std::cout << "Proof size: " << proof_size << " bytes" << std::endl;
    std::cout << "Proof data: ";
    for (size_t i = 0; i < std::min(proof_size, size_t(64)); i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)proof_data[i];
    }
    if (proof_size > 64) {
        std::cout << "...";
    }
    std::cout << std::endl;
    
    return true;
}

/**
 * 获取Enclave测量值
 */
bool get_enclave_measurement() {
    if (global_eid == 0) {
        std::cerr << "Enclave not initialized" << std::endl;
        return false;
    }
    
    std::cout << "\n=== Enclave Measurement ===" << std::endl;
    
    uint8_t measurement[32];
    
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    
    ret = ecall_get_measurement(global_eid, &enclave_ret, measurement);
    
    if (ret != SGX_SUCCESS || enclave_ret != SGX_SUCCESS) {
        std::cerr << "Failed to get measurement: 0x" << std::hex << ret << ", 0x" << enclave_ret << std::endl;
        return false;
    }
    
    std::cout << "Enclave measurement: ";
    for (int i = 0; i < 32; i++) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)measurement[i];
    }
    std::cout << std::endl;
    
    return true;
}

/**
 * 创建远程证明报告
 */
bool create_attestation_report() {
    if (global_eid == 0) {
        std::cerr << "Enclave not initialized" << std::endl;
        return false;
    }
    
    std::cout << "\n=== Creating Attestation Report ===" << std::endl;
    
    uint8_t report_data[64] = {0}; // 用户数据
    uint8_t report[1024];
    size_t report_size = sizeof(report);
    
    // 填充一些示例数据
    const char* user_data = "SGX Smart Contract Demo";
    size_t user_data_len = strlen(user_data);
    if (user_data_len <= sizeof(report_data)) {
        memcpy(report_data, user_data, user_data_len);
    }
    
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    sgx_status_t enclave_ret = SGX_ERROR_UNEXPECTED;
    
    ret = ecall_create_report(
        global_eid,
        &enclave_ret,
        report_data,
        report,
        &report_size
    );
    
    if (ret != SGX_SUCCESS || enclave_ret != SGX_SUCCESS) {
        std::cerr << "Failed to create report: 0x" << std::hex << ret << ", 0x" << enclave_ret << std::endl;
        return false;
    }
    
    std::cout << "Attestation report created successfully" << std::endl;
    std::cout << "Report size: " << report_size << " bytes" << std::endl;
    std::cout << "Report data (first 128 bytes): ";
    for (size_t i = 0; i < std::min(report_size, size_t(128)); i++) {
        if (i % 16 == 0) std::cout << "\n";
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)report[i] << " ";
    }
    std::cout << std::endl;
    
    return true;
}

/**
 * 显示菜单
 */
void show_menu() {
    std::cout << "\n=== SGX Smart Contract Demo ===" << std::endl;
    std::cout << "1. Execute sample contract" << std::endl;
    std::cout << "2. Load and execute contract from file" << std::endl;
    std::cout << "3. Generate execution proof" << std::endl;
    std::cout << "4. Get enclave measurement" << std::endl;
    std::cout << "5. Create attestation report" << std::endl;
    std::cout << "6. Run performance benchmark" << std::endl;
    std::cout << "0. Exit" << std::endl;
    std::cout << "Choose an option: ";
}

/**
 * 性能基准测试
 */
void run_benchmark() {
    std::cout << "\n=== Performance Benchmark ===" << std::endl;
    
    auto bytecode = create_sample_contract();
    std::vector<uint8_t> input_data;
    
    const int iterations = 100;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        if (!execute_smart_contract(bytecode, input_data)) {
            std::cerr << "Benchmark failed at iteration " << i << std::endl;
            return;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    std::cout << "Benchmark completed:" << std::endl;
    std::cout << "Total iterations: " << iterations << std::endl;
    std::cout << "Total time: " << total_duration.count() << " microseconds" << std::endl;
    std::cout << "Average time per execution: " << (total_duration.count() / iterations) << " microseconds" << std::endl;
    std::cout << "Throughput: " << (iterations * 1000000.0 / total_duration.count()) << " executions/second" << std::endl;
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    std::cout << "SGX Smart Contract Verification Demo" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // 初始化Enclave
    if (initialize_enclave() != SGX_SUCCESS) {
        std::cerr << "Failed to initialize enclave" << std::endl;
        return -1;
    }
    
    // 主循环
    int choice;
    while (true) {
        show_menu();
        std::cin >> choice;
        
        switch (choice) {
            case 1: {
                auto bytecode = create_sample_contract();
                std::vector<uint8_t> input_data;
                execute_smart_contract(bytecode, input_data);
                break;
            }
            
            case 2: {
                std::string filename;
                std::cout << "Enter contract file path: ";
                std::cin >> filename;
                
                auto bytecode = load_contract_bytecode(filename);
                if (!bytecode.empty()) {
                    std::vector<uint8_t> input_data;
                    execute_smart_contract(bytecode, input_data);
                }
                break;
            }
            
            case 3: {
                auto bytecode = create_sample_contract();
                std::vector<uint8_t> input_data;
                generate_execution_proof(bytecode, input_data);
                break;
            }
            
            case 4:
                get_enclave_measurement();
                break;
                
            case 5:
                create_attestation_report();
                break;
                
            case 6:
                run_benchmark();
                break;
                
            case 0:
                std::cout << "Exiting..." << std::endl;
                goto exit_loop;
                
            default:
                std::cout << "Invalid option. Please try again." << std::endl;
                break;
        }
    }
    
exit_loop:
    // 清理资源
    destroy_enclave();
    
    return 0;
}