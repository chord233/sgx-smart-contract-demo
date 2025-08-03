#ifndef APP_UTILS_H
#define APP_UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <chrono>

#include "sgx_urts.h"
#include "sgx_quote.h"

/**
 * 应用程序工具类
 */
class AppUtils {
public:
    /**
     * 打印十六进制数据
     * @param data 数据指针
     * @param size 数据大小
     * @param title 标题
     */
    static void print_hex_data(const uint8_t* data, size_t size, const std::string& title = "");
    
    /**
     * 打印SGX状态码
     * @param status SGX状态码
     * @param operation 操作描述
     */
    static void print_sgx_status(sgx_status_t status, const std::string& operation);
    
    /**
     * 从文件读取数据
     * @param filename 文件名
     * @return 文件数据
     */
    static std::vector<uint8_t> read_file(const std::string& filename);
    
    /**
     * 写入数据到文件
     * @param filename 文件名
     * @param data 数据
     * @return 是否成功
     */
    static bool write_file(const std::string& filename, const std::vector<uint8_t>& data);
    
    /**
     * 获取当前时间戳
     * @return 时间戳（毫秒）
     */
    static uint64_t get_timestamp_ms();
    
    /**
     * 格式化时间
     * @param timestamp 时间戳（毫秒）
     * @return 格式化的时间字符串
     */
    static std::string format_time(uint64_t timestamp);
    
    /**
     * 计算文件哈希
     * @param filename 文件名
     * @return 文件哈希（十六进制字符串）
     */
    static std::string calculate_file_hash(const std::string& filename);
    
    /**
     * 验证文件完整性
     * @param filename 文件名
     * @param expected_hash 期望的哈希值
     * @return 是否匹配
     */
    static bool verify_file_integrity(const std::string& filename, const std::string& expected_hash);
    
    /**
     * 创建目录
     * @param path 目录路径
     * @return 是否成功
     */
    static bool create_directory(const std::string& path);
    
    /**
     * 检查文件是否存在
     * @param filename 文件名
     * @return 是否存在
     */
    static bool file_exists(const std::string& filename);
    
    /**
     * 获取文件大小
     * @param filename 文件名
     * @return 文件大小（字节）
     */
    static size_t get_file_size(const std::string& filename);
    
    /**
     * 格式化字节大小
     * @param bytes 字节数
     * @return 格式化的大小字符串
     */
    static std::string format_bytes(size_t bytes);
    
    /**
     * 生成随机字符串
     * @param length 长度
     * @return 随机字符串
     */
    static std::string generate_random_string(size_t length);
    
    /**
     * 字符串转十六进制
     * @param data 输入数据
     * @return 十六进制字符串
     */
    static std::string to_hex_string(const std::vector<uint8_t>& data);
    
    /**
     * 十六进制转字符串
     * @param hex_str 十六进制字符串
     * @return 数据向量
     */
    static std::vector<uint8_t> from_hex_string(const std::string& hex_str);
    
    /**
     * Base64编码
     * @param data 输入数据
     * @return Base64字符串
     */
    static std::string base64_encode(const std::vector<uint8_t>& data);
    
    /**
     * Base64解码
     * @param encoded Base64字符串
     * @return 解码后的数据
     */
    static std::vector<uint8_t> base64_decode(const std::string& encoded);
};

/**
 * 性能计时器类
 */
class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    std::string operation_name;
    
public:
    /**
     * 构造函数
     * @param name 操作名称
     */
    explicit PerformanceTimer(const std::string& name);
    
    /**
     * 析构函数，自动打印耗时
     */
    ~PerformanceTimer();
    
    /**
     * 获取已经过的时间（微秒）
     * @return 耗时（微秒）
     */
    uint64_t elapsed_microseconds() const;
    
    /**
     * 获取已经过的时间（毫秒）
     * @return 耗时（毫秒）
     */
    uint64_t elapsed_milliseconds() const;
    
    /**
     * 重置计时器
     */
    void reset();
    
    /**
     * 打印当前耗时
     */
    void print_elapsed() const;
};

/**
 * 日志级别
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

/**
 * 简单日志类
 */
class Logger {
private:
    static LogLevel current_level;
    static bool enable_timestamp;
    
public:
    /**
     * 设置日志级别
     * @param level 日志级别
     */
    static void set_level(LogLevel level);
    
    /**
     * 启用/禁用时间戳
     * @param enable 是否启用
     */
    static void enable_timestamps(bool enable);
    
    /**
     * 记录调试信息
     * @param message 消息
     */
    static void debug(const std::string& message);
    
    /**
     * 记录信息
     * @param message 消息
     */
    static void info(const std::string& message);
    
    /**
     * 记录警告
     * @param message 消息
     */
    static void warning(const std::string& message);
    
    /**
     * 记录错误
     * @param message 消息
     */
    static void error(const std::string& message);
    
private:
    /**
     * 内部日志函数
     * @param level 日志级别
     * @param message 消息
     */
    static void log(LogLevel level, const std::string& message);
    
    /**
     * 获取级别字符串
     * @param level 日志级别
     * @return 级别字符串
     */
    static std::string level_to_string(LogLevel level);
};

/**
 * 配置管理类
 */
class Config {
private:
    static std::string config_file;
    static bool loaded;
    
public:
    // SGX配置
    static std::string enclave_file;
    static bool debug_mode;
    static uint64_t default_gas_limit;
    
    // 网络配置
    static std::string server_host;
    static int server_port;
    static int max_connections;
    
    // 存储配置
    static std::string data_directory;
    static std::string log_directory;
    static size_t max_log_size;
    
    /**
     * 加载配置文件
     * @param filename 配置文件名
     * @return 是否成功
     */
    static bool load(const std::string& filename = "config.json");
    
    /**
     * 保存配置文件
     * @param filename 配置文件名
     * @return 是否成功
     */
    static bool save(const std::string& filename = "config.json");
    
    /**
     * 获取字符串配置
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    static std::string get_string(const std::string& key, const std::string& default_value = "");
    
    /**
     * 获取整数配置
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    static int get_int(const std::string& key, int default_value = 0);
    
    /**
     * 获取布尔配置
     * @param key 配置键
     * @param default_value 默认值
     * @return 配置值
     */
    static bool get_bool(const std::string& key, bool default_value = false);
    
    /**
     * 设置字符串配置
     * @param key 配置键
     * @param value 配置值
     */
    static void set_string(const std::string& key, const std::string& value);
    
    /**
     * 设置整数配置
     * @param key 配置键
     * @param value 配置值
     */
    static void set_int(const std::string& key, int value);
    
    /**
     * 设置布尔配置
     * @param key 配置键
     * @param value 配置值
     */
    static void set_bool(const std::string& key, bool value);
};

// OCall实现声明
extern "C" {
    void ocall_print_string(const char* str);
    void ocall_print_error(const char* str);
    int ocall_read_storage(const char* key, uint8_t* data, size_t* data_size);
    int ocall_write_storage(const char* key, const uint8_t* data, size_t data_size);
    int ocall_network_request(const char* url, const uint8_t* request_data, size_t request_size, 
                             uint8_t* response_data, size_t* response_size);
    void ocall_audit_log(int level, const char* message);
}

#endif // APP_UTILS_H