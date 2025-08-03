#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include "../app/app.h"
#include "../app/app_utils.h"

/**
 * @file test_contracts.cpp
 * @brief 智能合约执行和验证测试
 * @author chord233
 */

class ContractTest {
private:
    SGXSmartContractApp app;
    bool test_passed;
    int total_tests;
    int passed_tests;
    
public:
    ContractTest() : test_passed(true), total_tests(0), passed_tests(0) {}
    
    void run_all_tests() {
        std::cout << "\n=== 智能合约测试套件 ===" << std::endl;
        
        // 初始化Enclave
        if (app.initialize_enclave() != APP_SUCCESS) {
            std::cout << "无法初始化Enclave，跳过合约测试" << std::endl;
            return;
        }
        
        test_sample_contract_execution();
        test_contract_loading();
        test_contract_execution_with_input();
        test_execution_proof_generation();
        test_invalid_contract_handling();
        test_large_contract_execution();
        
        app.destroy_enclave();
        print_test_summary();
    }
    
private:
    void assert_test(bool condition, const std::string& test_name) {
        total_tests++;
        if (condition) {
            passed_tests++;
            std::cout << "✓ " << test_name << " - 通过" << std::endl;
        } else {
            test_passed = false;
            std::cout << "✗ " << test_name << " - 失败" << std::endl;
        }
    }
    
    void test_sample_contract_execution() {
        std::cout << "\n--- 测试示例合约执行 ---" << std::endl;
        
        SmartContract contract = SGXSmartContractApp::create_sample_contract();
        std::vector<uint8_t> input_data = SGXSmartContractApp::create_sample_input();
        ExecutionResult result;
        
        app_status_t status = app.execute_contract(contract, input_data, result);
        
        assert_test(status == APP_SUCCESS, "示例合约执行状态");
        assert_test(result.success, "示例合约执行结果");
        assert_test(!result.output.empty(), "示例合约输出非空");
        assert_test(!result.execution_hash.empty(), "示例合约执行哈希非空");
        assert_test(result.gas_used > 0, "示例合约Gas消耗");
        
        if (result.success) {
            SGXSmartContractApp::print_execution_result(result);
        }
    }
    
    void test_contract_loading() {
        std::cout << "\n--- 测试合约文件加载 ---" << std::endl;
        
        // 测试加载存在的合约文件
        SmartContract contract;
        app_status_t status = app.load_contract_from_file("data/simple_add.bin", contract);
        
        if (status == APP_SUCCESS) {
            assert_test(true, "加载存在的合约文件");
            assert_test(!contract.bytecode.empty(), "加载的合约字节码非空");
            assert_test(contract.name == "data/simple_add.bin", "合约名称设置正确");
        } else {
            assert_test(false, "加载存在的合约文件");
        }
        
        // 测试加载不存在的合约文件
        SmartContract invalid_contract;
        status = app.load_contract_from_file("nonexistent_contract.bin", invalid_contract);
        assert_test(status == APP_ERROR_FILE_IO, "加载不存在文件的错误处理");
    }
    
    void test_contract_execution_with_input() {
        std::cout << "\n--- 测试带输入数据的合约执行 ---" << std::endl;
        
        SmartContract contract = SGXSmartContractApp::create_sample_contract();
        
        // 测试不同的输入数据
        std::vector<std::vector<uint8_t>> test_inputs = {
            {},  // 空输入
            {0x01, 0x02, 0x03, 0x04},  // 小输入
            std::vector<uint8_t>(100, 0xAA),  // 中等输入
            std::vector<uint8_t>(1000, 0xFF)  // 大输入
        };
        
        for (size_t i = 0; i < test_inputs.size(); i++) {
            ExecutionResult result;
            app_status_t status = app.execute_contract(contract, test_inputs[i], result);
            
            std::string test_name = "输入数据测试 " + std::to_string(i + 1) + 
                                   " (大小: " + std::to_string(test_inputs[i].size()) + ")"; 
            assert_test(status == APP_SUCCESS && result.success, test_name);
        }
    }
    
    void test_execution_proof_generation() {
        std::cout << "\n--- 测试执行证明生成 ---" << std::endl;
        
        SmartContract contract = SGXSmartContractApp::create_sample_contract();
        std::vector<uint8_t> input_data = SGXSmartContractApp::create_sample_input();
        ExecutionProof proof;
        
        app_status_t status = app.generate_execution_proof(contract, input_data, proof);
        
        assert_test(status == APP_SUCCESS, "执行证明生成状态");
        assert_test(proof.is_valid, "执行证明有效性");
        assert_test(!proof.proof_data.empty(), "执行证明数据非空");
        
        if (proof.is_valid) {
            SGXSmartContractApp::print_proof_info(proof);
        }
        
        // 测试空合约的证明生成
        SmartContract empty_contract;
        ExecutionProof empty_proof;
        status = app.generate_execution_proof(empty_contract, input_data, empty_proof);
        assert_test(status == APP_ERROR_INVALID_PARAM, "空合约证明生成错误处理");
    }
    
    void test_invalid_contract_handling() {
        std::cout << "\n--- 测试无效合约处理 ---" << std::endl;
        
        // 测试空合约
        SmartContract empty_contract;
        std::vector<uint8_t> input_data;
        ExecutionResult result;
        
        app_status_t status = app.execute_contract(empty_contract, input_data, result);
        assert_test(status == APP_ERROR_INVALID_PARAM, "空合约执行错误处理");
        
        // 测试无效字节码
        SmartContract invalid_contract;
        invalid_contract.bytecode = {0xFF, 0xFF, 0xFF, 0xFF};  // 无效操作码
        invalid_contract.name = "无效合约";
        
        status = app.execute_contract(invalid_contract, input_data, result);
        // 注意：这里可能成功但result.success为false，或者直接返回错误
        assert_test(status == APP_SUCCESS || status == APP_ERROR_ENCLAVE_CALL, "无效字节码处理");
    }
    
    void test_large_contract_execution() {
        std::cout << "\n--- 测试大型合约执行 ---" << std::endl;
        
        // 创建一个较大的合约（重复示例合约的字节码）
        SmartContract large_contract;
        large_contract.name = "大型测试合约";
        
        SmartContract sample = SGXSmartContractApp::create_sample_contract();
        for (int i = 0; i < 100; i++) {
            large_contract.bytecode.insert(large_contract.bytecode.end(),
                                          sample.bytecode.begin(),
                                          sample.bytecode.end());
        }
        
        std::vector<uint8_t> input_data;
        ExecutionResult result;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        app_status_t status = app.execute_contract(large_contract, input_data, result);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        assert_test(status == APP_SUCCESS, "大型合约执行状态");
        
        std::cout << "大型合约大小: " << large_contract.bytecode.size() << " bytes" << std::endl;
        std::cout << "执行时间: " << duration.count() << " 毫秒" << std::endl;
    }
    
    void print_test_summary() {
        std::cout << "\n=== 合约测试总结 ===" << std::endl;
        std::cout << "总测试数: " << total_tests << std::endl;
        std::cout << "通过测试: " << passed_tests << std::endl;
        std::cout << "失败测试: " << (total_tests - passed_tests) << std::endl;
        std::cout << "成功率: " << (passed_tests * 100.0 / total_tests) << "%" << std::endl;
        
        if (test_passed) {
            std::cout << "\n🎉 所有合约测试通过!" << std::endl;
        } else {
            std::cout << "\n❌ 部分合约测试失败!" << std::endl;
        }
    }
};

/**
 * @brief 合约性能测试类
 */
class ContractPerformanceTest {
private:
    SGXSmartContractApp app;
    
public:
    void run_performance_tests() {
        std::cout << "\n=== 合约性能测试 ===" << std::endl;
        
        if (app.initialize_enclave() != APP_SUCCESS) {
            std::cout << "无法初始化Enclave，跳过性能测试" << std::endl;
            return;
        }
        
        test_execution_performance();
        test_proof_generation_performance();
        test_throughput_benchmark();
        
        app.destroy_enclave();
    }
    
private:
    void test_execution_performance() {
        std::cout << "\n--- 合约执行性能测试 ---" << std::endl;
        
        SmartContract contract = SGXSmartContractApp::create_sample_contract();
        std::vector<uint8_t> input_data = SGXSmartContractApp::create_sample_input();
        
        const int iterations = 1000;
        std::vector<double> execution_times;
        
        for (int i = 0; i < iterations; i++) {
            auto start_time = std::chrono::high_resolution_clock::now();
            
            ExecutionResult result;
            app.execute_contract(contract, input_data, result);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            execution_times.push_back(duration.count());
        }
        
        // 计算统计信息
        double total_time = 0;
        double min_time = execution_times[0];
        double max_time = execution_times[0];
        
        for (double time : execution_times) {
            total_time += time;
            min_time = std::min(min_time, time);
            max_time = std::max(max_time, time);
        }
        
        double avg_time = total_time / iterations;
        
        std::cout << "执行次数: " << iterations << std::endl;
        std::cout << "平均执行时间: " << avg_time << " 微秒" << std::endl;
        std::cout << "最小执行时间: " << min_time << " 微秒" << std::endl;
        std::cout << "最大执行时间: " << max_time << " 微秒" << std::endl;
        std::cout << "总执行时间: " << total_time << " 微秒" << std::endl;
        std::cout << "吞吐量: " << (iterations * 1000000.0 / total_time) << " 次/秒" << std::endl;
    }
    
    void test_proof_generation_performance() {
        std::cout << "\n--- 证明生成性能测试 ---" << std::endl;
        
        SmartContract contract = SGXSmartContractApp::create_sample_contract();
        std::vector<uint8_t> input_data = SGXSmartContractApp::create_sample_input();
        
        const int iterations = 100;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            ExecutionProof proof;
            app.generate_execution_proof(contract, input_data, proof);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::cout << "证明生成次数: " << iterations << std::endl;
        std::cout << "总时间: " << duration.count() << " 微秒" << std::endl;
        std::cout << "平均时间: " << (duration.count() / iterations) << " 微秒/次" << std::endl;
        std::cout << "吞吐量: " << (iterations * 1000000.0 / duration.count()) << " 次/秒" << std::endl;
    }
    
    void test_throughput_benchmark() {
        std::cout << "\n--- 吞吐量基准测试 ---" << std::endl;
        
        SmartContract contract = SGXSmartContractApp::create_sample_contract();
        std::vector<uint8_t> input_data;
        
        const int test_duration_seconds = 10;
        const auto test_duration = std::chrono::seconds(test_duration_seconds);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        auto end_time = start_time + test_duration;
        
        int successful_executions = 0;
        int failed_executions = 0;
        
        while (std::chrono::high_resolution_clock::now() < end_time) {
            ExecutionResult result;
            app_status_t status = app.execute_contract(contract, input_data, result);
            
            if (status == APP_SUCCESS && result.success) {
                successful_executions++;
            } else {
                failed_executions++;
            }
        }
        
        auto actual_end_time = std::chrono::high_resolution_clock::now();
        auto actual_duration = std::chrono::duration_cast<std::chrono::milliseconds>(actual_end_time - start_time);
        
        std::cout << "测试持续时间: " << actual_duration.count() << " 毫秒" << std::endl;
        std::cout << "成功执行次数: " << successful_executions << std::endl;
        std::cout << "失败执行次数: " << failed_executions << std::endl;
        std::cout << "总执行次数: " << (successful_executions + failed_executions) << std::endl;
        std::cout << "成功率: " << (successful_executions * 100.0 / (successful_executions + failed_executions)) << "%" << std::endl;
        std::cout << "吞吐量: " << (successful_executions * 1000.0 / actual_duration.count()) << " 次/秒" << std::endl;
    }
};

/**
 * @brief 合约兼容性测试类
 */
class ContractCompatibilityTest {
private:
    SGXSmartContractApp app;
    
public:
    void run_compatibility_tests() {
        std::cout << "\n=== 合约兼容性测试 ===" << std::endl;
        
        if (app.initialize_enclave() != APP_SUCCESS) {
            std::cout << "无法初始化Enclave，跳过兼容性测试" << std::endl;
            return;
        }
        
        test_different_contract_sizes();
        test_various_input_formats();
        test_edge_cases();
        
        app.destroy_enclave();
    }
    
private:
    void test_different_contract_sizes() {
        std::cout << "\n--- 不同大小合约测试 ---" << std::endl;
        
        std::vector<size_t> contract_sizes = {1, 10, 100, 1000, 10000};
        
        for (size_t size : contract_sizes) {
            SmartContract contract;
            contract.name = "大小测试合约_" + std::to_string(size);
            contract.bytecode.resize(size, 0x18); // 填充HALT指令
            
            std::vector<uint8_t> input_data;
            ExecutionResult result;
            
            app_status_t status = app.execute_contract(contract, input_data, result);
            
            std::cout << "合约大小 " << size << " bytes: ";
            if (status == APP_SUCCESS) {
                std::cout << "✓ 执行成功" << std::endl;
            } else {
                std::cout << "✗ 执行失败 (" << status << ")" << std::endl;
            }
        }
    }
    
    void test_various_input_formats() {
        std::cout << "\n--- 各种输入格式测试 ---" << std::endl;
        
        SmartContract contract = SGXSmartContractApp::create_sample_contract();
        
        // 测试不同的输入格式
        std::vector<std::pair<std::string, std::vector<uint8_t>>> test_cases = {
            {"空输入", {}},
            {"单字节", {0x42}},
            {"递增序列", {0x01, 0x02, 0x03, 0x04, 0x05}},
            {"递减序列", {0x05, 0x04, 0x03, 0x02, 0x01}},
            {"全零", std::vector<uint8_t>(50, 0x00)},
            {"全一", std::vector<uint8_t>(50, 0xFF)},
            {"随机模式", {0xAA, 0x55, 0xAA, 0x55, 0xAA}}
        };
        
        for (const auto& test_case : test_cases) {
            ExecutionResult result;
            app_status_t status = app.execute_contract(contract, test_case.second, result);
            
            std::cout << test_case.first << ": ";
            if (status == APP_SUCCESS && result.success) {
                std::cout << "✓ 成功" << std::endl;
            } else {
                std::cout << "✗ 失败" << std::endl;
            }
        }
    }
    
    void test_edge_cases() {
        std::cout << "\n--- 边界情况测试 ---" << std::endl;
        
        // 测试最大输入大小
        SmartContract contract = SGXSmartContractApp::create_sample_contract();
        std::vector<uint8_t> max_input(MAX_INPUT_SIZE - 1, 0xAB);
        
        ExecutionResult result;
        app_status_t status = app.execute_contract(contract, max_input, result);
        
        std::cout << "最大输入大小测试: ";
        if (status == APP_SUCCESS) {
            std::cout << "✓ 成功" << std::endl;
        } else {
            std::cout << "✗ 失败" << std::endl;
        }
        
        // 测试超大输入（应该失败或被截断）
        std::vector<uint8_t> oversized_input(MAX_INPUT_SIZE + 1000, 0xCD);
        status = app.execute_contract(contract, oversized_input, result);
        
        std::cout << "超大输入测试: ";
        if (status != APP_SUCCESS) {
            std::cout << "✓ 正确拒绝" << std::endl;
        } else {
            std::cout << "? 意外接受" << std::endl;
        }
    }
};

/**
 * @brief 主测试函数
 */
int main(int argc, char* argv[]) {
    std::cout << "智能合约测试程序" << std::endl;
    std::cout << "================" << std::endl;
    
    try {
        // 基础合约测试
        ContractTest contract_test;
        contract_test.run_all_tests();
        
        // 性能测试
        ContractPerformanceTest perf_test;
        perf_test.run_performance_tests();
        
        // 兼容性测试
        ContractCompatibilityTest compat_test;
        compat_test.run_compatibility_tests();
        
        std::cout << "\n=== 所有合约测试完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生异常: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}