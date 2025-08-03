#ifndef APP_H
#define APP_H

#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include "sgx_urts.h"
#include "enclave_u.h"

// 常量定义
#define ENCLAVE_FILENAME "enclave.signed.so"
#define MAX_PATH 260
#define MAX_CONTRACT_SIZE 1024 * 1024  // 1MB
#define MAX_INPUT_SIZE 4096
#define MAX_OUTPUT_SIZE 4096

// 错误码定义
typedef enum {
    APP_SUCCESS = 0,
    APP_ERROR_ENCLAVE_INIT = -1,
    APP_ERROR_ENCLAVE_CALL = -2,
    APP_ERROR_FILE_IO = -3,
    APP_ERROR_INVALID_PARAM = -4,
    APP_ERROR_MEMORY = -5
} app_status_t;

// 智能合约结构
struct SmartContract {
    std::vector<uint8_t> bytecode;
    std::string name;
    std::string description;
    size_t gas_limit;
    
    SmartContract() : gas_limit(1000000) {}
    SmartContract(const std::vector<uint8_t>& code, const std::string& contract_name) 
        : bytecode(code), name(contract_name), gas_limit(1000000) {}
};

// 执行结果结构
struct ExecutionResult {
    bool success;
    std::vector<uint8_t> output;
    uint64_t gas_used;
    std::vector<uint8_t> execution_hash;
    std::string error_message;
    
    ExecutionResult() : success(false), gas_used(0) {}
};

// 证明结构
struct ExecutionProof {
    std::vector<uint8_t> proof_data;
    std::vector<uint8_t> measurement;
    std::vector<uint8_t> signature;
    bool is_valid;
    
    ExecutionProof() : is_valid(false) {}
};

// 应用程序类
class SGXSmartContractApp {
private:
    sgx_enclave_id_t enclave_id;
    bool enclave_initialized;
    
public:
    SGXSmartContractApp();
    ~SGXSmartContractApp();
    
    // Enclave管理
    app_status_t initialize_enclave();
    void destroy_enclave();
    bool is_enclave_ready() const { return enclave_initialized; }
    
    // 智能合约操作
    app_status_t load_contract_from_file(const std::string& filename, SmartContract& contract);
    app_status_t execute_contract(const SmartContract& contract, 
                                 const std::vector<uint8_t>& input_data,
                                 ExecutionResult& result);
    
    // 证明和验证
    app_status_t generate_execution_proof(const SmartContract& contract,
                                         const std::vector<uint8_t>& input_data,
                                         ExecutionProof& proof);
    app_status_t verify_execution_proof(const ExecutionProof& proof, bool& is_valid);
    
    // Enclave信息
    app_status_t get_enclave_measurement(std::vector<uint8_t>& measurement);
    app_status_t create_attestation_report(const std::vector<uint8_t>& user_data,
                                          std::vector<uint8_t>& report);
    
    // 工具函数
    static SmartContract create_sample_contract();
    static std::vector<uint8_t> create_sample_input();
    static void print_execution_result(const ExecutionResult& result);
    static void print_proof_info(const ExecutionProof& proof);
};

// 全局函数声明
void show_main_menu();
void run_interactive_mode();
void run_benchmark_test(int iterations = 100);
app_status_t run_contract_from_file(const std::string& filename);
void print_app_info();
void print_error(const std::string& message);
void print_success(const std::string& message);
void print_warning(const std::string& message);

#endif // APP_H