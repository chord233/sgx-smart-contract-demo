#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <chrono>
#include "../app/app.h"
#include "../app/app_utils.h"

/**
 * @file test_enclave.cpp
 * @brief SGX Enclave功能测试
 * @author chord233
 */

class EnclaveTest {
private:
    SGXSmartContractApp app;
    bool test_passed;
    int total_tests;
    int passed_tests;
    
public:
    EnclaveTest() : test_passed(true), total_tests(0), passed_tests(0) {}
    
    void run_all_tests() {
        std::cout << "\n=== SGX Enclave 测试套件 ===" << std::endl;
        
        test_enclave_initialization();
        test_enclave_measurement();
        test_attestation_report();
        test_enclave_destruction();
        
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
    
    void test_enclave_initialization() {
        std::cout << "\n--- 测试 Enclave 初始化 ---" << std::endl;
        
        // 测试初始化
        app_status_t status = app.initialize_enclave();
        assert_test(status == APP_SUCCESS, "Enclave初始化");
        assert_test(app.is_enclave_ready(), "Enclave就绪状态检查");
        
        // 测试重复初始化
        status = app.initialize_enclave();
        assert_test(status == APP_SUCCESS, "重复初始化处理");
    }
    
    void test_enclave_measurement() {
        std::cout << "\n--- 测试 Enclave 测量值 ---" << std::endl;
        
        if (!app.is_enclave_ready()) {
            std::cout << "跳过测量值测试 - Enclave未初始化" << std::endl;
            return;
        }
        
        std::vector<uint8_t> measurement;
        app_status_t status = app.get_enclave_measurement(measurement);
        
        assert_test(status == APP_SUCCESS, "获取Enclave测量值");
        assert_test(measurement.size() == 32, "测量值长度验证");
        
        // 验证测量值不全为零
        bool non_zero = false;
        for (uint8_t byte : measurement) {
            if (byte != 0) {
                non_zero = true;
                break;
            }
        }
        assert_test(non_zero, "测量值非零验证");
        
        // 打印测量值
        std::cout << "Enclave测量值: ";
        for (size_t i = 0; i < std::min(measurement.size(), size_t(16)); i++) {
            printf("%02x", measurement[i]);
        }
        std::cout << "..." << std::endl;
    }
    
    void test_attestation_report() {
        std::cout << "\n--- 测试远程证明报告 ---" << std::endl;
        
        if (!app.is_enclave_ready()) {
            std::cout << "跳过证明报告测试 - Enclave未初始化" << std::endl;
            return;
        }
        
        // 测试用户数据
        std::string user_data_str = "SGX Enclave Test Data";
        std::vector<uint8_t> user_data(user_data_str.begin(), user_data_str.end());
        std::vector<uint8_t> report;
        
        app_status_t status = app.create_attestation_report(user_data, report);
        
        assert_test(status == APP_SUCCESS, "创建证明报告");
        assert_test(!report.empty(), "证明报告非空");
        assert_test(report.size() > 0, "证明报告大小验证");
        
        std::cout << "证明报告大小: " << report.size() << " bytes" << std::endl;
        
        // 测试空用户数据
        std::vector<uint8_t> empty_data;
        std::vector<uint8_t> empty_report;
        status = app.create_attestation_report(empty_data, empty_report);
        assert_test(status == APP_SUCCESS, "空用户数据证明报告");
    }
    
    void test_enclave_destruction() {
        std::cout << "\n--- 测试 Enclave 销毁 ---" << std::endl;
        
        // 销毁Enclave
        app.destroy_enclave();
        assert_test(!app.is_enclave_ready(), "Enclave销毁后状态检查");
        
        // 测试销毁后的操作
        std::vector<uint8_t> measurement;
        app_status_t status = app.get_enclave_measurement(measurement);
        assert_test(status == APP_ERROR_ENCLAVE_INIT, "销毁后操作错误处理");
    }
    
    void print_test_summary() {
        std::cout << "\n=== 测试总结 ===" << std::endl;
        std::cout << "总测试数: " << total_tests << std::endl;
        std::cout << "通过测试: " << passed_tests << std::endl;
        std::cout << "失败测试: " << (total_tests - passed_tests) << std::endl;
        std::cout << "成功率: " << (passed_tests * 100.0 / total_tests) << "%" << std::endl;
        
        if (test_passed) {
            std::cout << "\n🎉 所有测试通过!" << std::endl;
        } else {
            std::cout << "\n❌ 部分测试失败!" << std::endl;
        }
    }
};

/**
 * @brief 性能测试类
 */
class PerformanceTest {
private:
    SGXSmartContractApp app;
    
public:
    void run_performance_tests() {
        std::cout << "\n=== Enclave 性能测试 ===" << std::endl;
        
        if (app.initialize_enclave() != APP_SUCCESS) {
            std::cout << "无法初始化Enclave，跳过性能测试" << std::endl;
            return;
        }
        
        test_measurement_performance();
        test_report_generation_performance();
        
        app.destroy_enclave();
    }
    
private:
    void test_measurement_performance() {
        std::cout << "\n--- 测量值获取性能测试 ---" << std::endl;
        
        const int iterations = 1000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            std::vector<uint8_t> measurement;
            app.get_enclave_measurement(measurement);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::cout << "执行次数: " << iterations << std::endl;
        std::cout << "总时间: " << duration.count() << " 微秒" << std::endl;
        std::cout << "平均时间: " << (duration.count() / iterations) << " 微秒/次" << std::endl;
        std::cout << "吞吐量: " << (iterations * 1000000.0 / duration.count()) << " 次/秒" << std::endl;
    }
    
    void test_report_generation_performance() {
        std::cout << "\n--- 证明报告生成性能测试 ---" << std::endl;
        
        const int iterations = 100;
        std::vector<uint8_t> user_data = {0x01, 0x02, 0x03, 0x04};
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; i++) {
            std::vector<uint8_t> report;
            app.create_attestation_report(user_data, report);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        std::cout << "执行次数: " << iterations << std::endl;
        std::cout << "总时间: " << duration.count() << " 微秒" << std::endl;
        std::cout << "平均时间: " << (duration.count() / iterations) << " 微秒/次" << std::endl;
        std::cout << "吞吐量: " << (iterations * 1000000.0 / duration.count()) << " 次/秒" << std::endl;
    }
};

/**
 * @brief 压力测试类
 */
class StressTest {
private:
    SGXSmartContractApp app;
    
public:
    void run_stress_tests() {
        std::cout << "\n=== Enclave 压力测试 ===" << std::endl;
        
        if (app.initialize_enclave() != APP_SUCCESS) {
            std::cout << "无法初始化Enclave，跳过压力测试" << std::endl;
            return;
        }
        
        test_concurrent_operations();
        test_memory_stress();
        
        app.destroy_enclave();
    }
    
private:
    void test_concurrent_operations() {
        std::cout << "\n--- 并发操作测试 ---" << std::endl;
        
        const int operations = 10000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < operations; i++) {
            std::vector<uint8_t> measurement;
            app.get_enclave_measurement(measurement);
            
            if (i % 2 == 0) {
                std::vector<uint8_t> user_data = {static_cast<uint8_t>(i & 0xFF)};
                std::vector<uint8_t> report;
                app.create_attestation_report(user_data, report);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "完成 " << operations << " 次操作" << std::endl;
        std::cout << "总时间: " << duration.count() << " 毫秒" << std::endl;
        std::cout << "平均时间: " << (duration.count() * 1000.0 / operations) << " 微秒/操作" << std::endl;
    }
    
    void test_memory_stress() {
        std::cout << "\n--- 内存压力测试 ---" << std::endl;
        
        const int large_data_size = 1024; // 1KB用户数据
        std::vector<uint8_t> large_user_data(large_data_size, 0xAA);
        
        for (int i = 0; i < 100; i++) {
            std::vector<uint8_t> report;
            app_status_t status = app.create_attestation_report(large_user_data, report);
            
            if (status != APP_SUCCESS) {
                std::cout << "内存压力测试在第 " << i << " 次迭代失败" << std::endl;
                return;
            }
        }
        
        std::cout << "内存压力测试完成 - 所有操作成功" << std::endl;
    }
};

/**
 * @brief 主测试函数
 */
int main(int argc, char* argv[]) {
    std::cout << "SGX Enclave 测试程序" << std::endl;
    std::cout << "==================" << std::endl;
    
    try {
        // 基础功能测试
        EnclaveTest enclave_test;
        enclave_test.run_all_tests();
        
        // 性能测试
        PerformanceTest perf_test;
        perf_test.run_performance_tests();
        
        // 压力测试
        StressTest stress_test;
        stress_test.run_stress_tests();
        
        std::cout << "\n=== 所有测试完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生异常: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}