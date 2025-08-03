#include "app_utils.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <filesystem>
#include <cstring>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>

// AppUtils实现
void AppUtils::print_hex_data(const uint8_t* data, size_t size, const std::string& title) {
    if (!title.empty()) {
        std::cout << title << ":" << std::endl;
    }
    
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0 && i > 0) {
            std::cout << std::endl;
        }
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i] << " ";
    }
    std::cout << std::dec << std::endl;
}

void AppUtils::print_sgx_status(sgx_status_t status, const std::string& operation) {
    std::cout << operation << ": ";
    
    switch (status) {
        case SGX_SUCCESS:
            std::cout << "Success";
            break;
        case SGX_ERROR_INVALID_PARAMETER:
            std::cout << "Invalid parameter";
            break;
        case SGX_ERROR_OUT_OF_MEMORY:
            std::cout << "Out of memory";
            break;
        case SGX_ERROR_ENCLAVE_LOST:
            std::cout << "Enclave lost";
            break;
        case SGX_ERROR_INVALID_STATE:
            std::cout << "Invalid state";
            break;
        case SGX_ERROR_FEATURE_NOT_SUPPORTED:
            std::cout << "Feature not supported";
            break;
        default:
            std::cout << "Unknown error (0x" << std::hex << status << std::dec << ")";
            break;
    }
    
    std::cout << std::endl;
}

std::vector<uint8_t> AppUtils::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        Logger::error("Failed to open file: " + filename);
        return {};
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> data(size);
    file.read(reinterpret_cast<char*>(data.data()), size);
    
    Logger::info("Read " + std::to_string(size) + " bytes from " + filename);
    
    return data;
}

bool AppUtils::write_file(const std::string& filename, const std::vector<uint8_t>& data) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        Logger::error("Failed to create file: " + filename);
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    
    Logger::info("Wrote " + std::to_string(data.size()) + " bytes to " + filename);
    
    return true;
}

uint64_t AppUtils::get_timestamp_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

std::string AppUtils::format_time(uint64_t timestamp) {
    auto time_point = std::chrono::system_clock::from_time_t(timestamp / 1000);
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << "." << std::setw(3) << std::setfill('0') << (timestamp % 1000);
    
    return ss.str();
}

std::string AppUtils::calculate_file_hash(const std::string& filename) {
    auto data = read_file(filename);
    if (data.empty()) {
        return "";
    }
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

bool AppUtils::verify_file_integrity(const std::string& filename, const std::string& expected_hash) {
    std::string actual_hash = calculate_file_hash(filename);
    return actual_hash == expected_hash;
}

bool AppUtils::create_directory(const std::string& path) {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::exception& e) {
        Logger::error("Failed to create directory " + path + ": " + e.what());
        return false;
    }
}

bool AppUtils::file_exists(const std::string& filename) {
    return std::filesystem::exists(filename);
}

size_t AppUtils::get_file_size(const std::string& filename) {
    try {
        return std::filesystem::file_size(filename);
    } catch (const std::exception& e) {
        Logger::error("Failed to get file size for " + filename + ": " + e.what());
        return 0;
    }
}

std::string AppUtils::format_bytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unit_index];
    return ss.str();
}

std::string AppUtils::generate_random_string(size_t length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; i++) {
        result += chars[dis(gen)];
    }
    
    return result;
}

std::string AppUtils::to_hex_string(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    for (uint8_t byte : data) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

std::vector<uint8_t> AppUtils::from_hex_string(const std::string& hex_str) {
    std::vector<uint8_t> data;
    
    for (size_t i = 0; i < hex_str.length(); i += 2) {
        std::string byte_str = hex_str.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::strtol(byte_str.c_str(), nullptr, 16));
        data.push_back(byte);
    }
    
    return data;
}

std::string AppUtils::base64_encode(const std::vector<uint8_t>& data) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    BIO_write(bio, data.data(), data.size());
    BIO_flush(bio);
    
    BUF_MEM* buffer_ptr;
    BIO_get_mem_ptr(bio, &buffer_ptr);
    
    std::string result(buffer_ptr->data, buffer_ptr->length);
    
    BIO_free_all(bio);
    
    return result;
}

std::vector<uint8_t> AppUtils::base64_decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.c_str(), encoded.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    
    std::vector<uint8_t> result(encoded.length());
    int decoded_length = BIO_read(bio, result.data(), encoded.length());
    
    BIO_free_all(bio);
    
    if (decoded_length > 0) {
        result.resize(decoded_length);
    } else {
        result.clear();
    }
    
    return result;
}

// PerformanceTimer实现
PerformanceTimer::PerformanceTimer(const std::string& name) 
    : operation_name(name) {
    start_time = std::chrono::high_resolution_clock::now();
}

PerformanceTimer::~PerformanceTimer() {
    print_elapsed();
}

uint64_t PerformanceTimer::elapsed_microseconds() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);
    return duration.count();
}

uint64_t PerformanceTimer::elapsed_milliseconds() const {
    return elapsed_microseconds() / 1000;
}

void PerformanceTimer::reset() {
    start_time = std::chrono::high_resolution_clock::now();
}

void PerformanceTimer::print_elapsed() const {
    uint64_t elapsed = elapsed_microseconds();
    std::cout << "[PERF] " << operation_name << ": " << elapsed << " μs";
    
    if (elapsed >= 1000) {
        std::cout << " (" << (elapsed / 1000.0) << " ms)";
    }
    
    std::cout << std::endl;
}

// Logger实现
LogLevel Logger::current_level = LogLevel::INFO;
bool Logger::enable_timestamp = true;

void Logger::set_level(LogLevel level) {
    current_level = level;
}

void Logger::enable_timestamps(bool enable) {
    enable_timestamp = enable;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < current_level) {
        return;
    }
    
    std::ostream& stream = (level >= LogLevel::ERROR) ? std::cerr : std::cout;
    
    if (enable_timestamp) {
        stream << "[" << AppUtils::format_time(AppUtils::get_timestamp_ms()) << "] ";
    }
    
    stream << "[" << level_to_string(level) << "] " << message << std::endl;
}

std::string Logger::level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Config实现
std::string Config::config_file = "config.json";
bool Config::loaded = false;

// 默认配置值
std::string Config::enclave_file = "enclave.signed.so";
bool Config::debug_mode = true;
uint64_t Config::default_gas_limit = 1000000;

std::string Config::server_host = "localhost";
int Config::server_port = 8080;
int Config::max_connections = 100;

std::string Config::data_directory = "./data";
std::string Config::log_directory = "./logs";
size_t Config::max_log_size = 10 * 1024 * 1024; // 10MB

bool Config::load(const std::string& filename) {
    config_file = filename;
    
    if (!AppUtils::file_exists(filename)) {
        Logger::warning("Config file not found: " + filename + ", using defaults");
        return save(filename); // 创建默认配置文件
    }
    
    // 简化实现：这里应该解析JSON文件
    // 由于这是演示代码，我们只是标记为已加载
    loaded = true;
    Logger::info("Configuration loaded from " + filename);
    
    return true;
}

bool Config::save(const std::string& filename) {
    // 简化实现：创建一个基本的JSON配置文件
    std::ofstream file(filename);
    if (!file.is_open()) {
        Logger::error("Failed to create config file: " + filename);
        return false;
    }
    
    file << "{\n";
    file << "  \"sgx\": {\n";
    file << "    \"enclave_file\": \"" << enclave_file << "\",\n";
    file << "    \"debug_mode\": " << (debug_mode ? "true" : "false") << ",\n";
    file << "    \"default_gas_limit\": " << default_gas_limit << "\n";
    file << "  },\n";
    file << "  \"network\": {\n";
    file << "    \"server_host\": \"" << server_host << "\",\n";
    file << "    \"server_port\": " << server_port << ",\n";
    file << "    \"max_connections\": " << max_connections << "\n";
    file << "  },\n";
    file << "  \"storage\": {\n";
    file << "    \"data_directory\": \"" << data_directory << "\",\n";
    file << "    \"log_directory\": \"" << log_directory << "\",\n";
    file << "    \"max_log_size\": " << max_log_size << "\n";
    file << "  }\n";
    file << "}\n";
    
    Logger::info("Configuration saved to " + filename);
    
    return true;
}

std::string Config::get_string(const std::string& key, const std::string& default_value) {
    // 简化实现：返回默认值
    return default_value;
}

int Config::get_int(const std::string& key, int default_value) {
    // 简化实现：返回默认值
    return default_value;
}

bool Config::get_bool(const std::string& key, bool default_value) {
    // 简化实现：返回默认值
    return default_value;
}

void Config::set_string(const std::string& key, const std::string& value) {
    // 简化实现：暂不支持运行时修改
}

void Config::set_int(const std::string& key, int value) {
    // 简化实现：暂不支持运行时修改
}

void Config::set_bool(const std::string& key, bool value) {
    // 简化实现：暂不支持运行时修改
}

// OCall实现
extern "C" {
    void ocall_print_string(const char* str) {
        std::cout << str << std::flush;
    }
    
    void ocall_print_error(const char* str) {
        std::cerr << str << std::flush;
    }
    
    int ocall_read_storage(const char* key, uint8_t* data, size_t* data_size) {
        std::string filename = Config::data_directory + "/" + std::string(key) + ".dat";
        
        if (!AppUtils::file_exists(filename)) {
            return -1; // 文件不存在
        }
        
        auto file_data = AppUtils::read_file(filename);
        if (file_data.empty()) {
            return -2; // 读取失败
        }
        
        if (*data_size < file_data.size()) {
            *data_size = file_data.size();
            return -3; // 缓冲区太小
        }
        
        memcpy(data, file_data.data(), file_data.size());
        *data_size = file_data.size();
        
        return 0; // 成功
    }
    
    int ocall_write_storage(const char* key, const uint8_t* data, size_t data_size) {
        // 确保数据目录存在
        AppUtils::create_directory(Config::data_directory);
        
        std::string filename = Config::data_directory + "/" + std::string(key) + ".dat";
        std::vector<uint8_t> file_data(data, data + data_size);
        
        return AppUtils::write_file(filename, file_data) ? 0 : -1;
    }
    
    int ocall_network_request(const char* url, const uint8_t* request_data, size_t request_size,
                             uint8_t* response_data, size_t* response_size) {
        // 简化实现：模拟网络请求
        Logger::info("Network request to: " + std::string(url));
        
        // 模拟响应数据
        const char* mock_response = "{\"status\": \"success\", \"data\": \"mock_data\"}";
        size_t response_len = strlen(mock_response);
        
        if (*response_size < response_len) {
            *response_size = response_len;
            return -1; // 缓冲区太小
        }
        
        memcpy(response_data, mock_response, response_len);
        *response_size = response_len;
        
        return 0; // 成功
    }
    
    void ocall_audit_log(int level, const char* message) {
        LogLevel log_level = static_cast<LogLevel>(level);
        Logger::log(log_level, "[ENCLAVE] " + std::string(message));
    }
}