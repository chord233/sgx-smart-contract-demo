#!/bin/bash

# SGX智能合约演示运行脚本
# 作者: chord233
# 功能: 自动化运行智能合约演示程序

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_demo() {
    echo -e "${PURPLE}[DEMO]${NC} $1"
}

log_step() {
    echo -e "${CYAN}[STEP]${NC} $1"
}

# 全局变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BIN_DIR="$PROJECT_ROOT/bin"
LOGS_DIR="$PROJECT_ROOT/logs"
DATA_DIR="$PROJECT_ROOT/data"
DEMO_APP="$BIN_DIR/sgx_smart_contract_demo"
TEST_APP="$BIN_DIR/test_enclave"
CONTRACT_TEST_APP="$BIN_DIR/test_contracts"

# 检查项目是否已构建
check_build() {
    log_info "检查项目构建状态..."
    
    if [[ ! -f "$DEMO_APP" ]]; then
        log_warning "演示程序未找到，尝试构建项目..."
        cd "$PROJECT_ROOT"
        if [[ -f "build.sh" ]]; then
            ./build.sh
        else
            log_error "构建脚本不存在，请手动构建项目"
            exit 1
        fi
    fi
    
    if [[ ! -x "$DEMO_APP" ]]; then
        log_error "演示程序不可执行: $DEMO_APP"
        exit 1
    fi
    
    log_success "项目构建检查完成"
}

# 检查SGX环境
check_sgx_environment() {
    log_info "检查SGX环境..."
    
    # 检查SGX SDK
    if [[ -z "$SGX_SDK" ]]; then
        if [[ -d "/opt/intel/sgxsdk" ]]; then
            export SGX_SDK="/opt/intel/sgxsdk"
            export PATH="$PATH:$SGX_SDK/bin/x64"
            export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$SGX_SDK/pkgconfig"
            export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SGX_SDK/sdk_libs"
            log_warning "自动设置SGX_SDK环境变量"
        else
            log_error "SGX SDK未找到，请先安装SGX SDK"
            exit 1
        fi
    fi
    
    # 检查SGX设备
    if [[ ! -c /dev/sgx_enclave ]] && [[ ! -c /dev/sgx/enclave ]] && [[ ! -c /dev/isgx ]]; then
        log_warning "SGX设备节点未找到，演示可能在模拟模式下运行"
    fi
    
    log_success "SGX环境检查完成"
}

# 准备演示数据
prepare_demo_data() {
    log_info "准备演示数据..."
    
    # 创建必要的目录
    mkdir -p "$LOGS_DIR" "$DATA_DIR"
    
    # 创建示例合约数据（如果不存在）
    if [[ ! -f "$DATA_DIR/simple_add.bin" ]]; then
        log_info "创建示例合约数据..."
        # 简单的加法合约字节码: PUSH 5, PUSH 10, ADD, STORE, RETURN
        echo -ne '\x60\x05\x60\x0a\x01\x60\x00\x52\x60\x20\x60\x00\xf3' > "$DATA_DIR/simple_add.bin"
    fi
    
    if [[ ! -f "$DATA_DIR/simple_multiply.bin" ]]; then
        log_info "创建乘法合约数据..."
        # 简单的乘法合约字节码: PUSH 3, PUSH 7, MUL, STORE, RETURN
        echo -ne '\x60\x03\x60\x07\x02\x60\x00\x52\x60\x20\x60\x00\xf3' > "$DATA_DIR/simple_multiply.bin"
    fi
    
    if [[ ! -f "$DATA_DIR/fibonacci.bin" ]]; then
        log_info "创建斐波那契合约数据..."
        # 斐波那契数列合约（简化版）
        echo -ne '\x60\x01\x60\x01\x60\x0a\x80\x82\x01\x90\x91\x80\x60\x01\x03\x80\x82\x10\x15\x60\x16\x57\x60\x00\x52\x60\x20\x60\x00\xf3' > "$DATA_DIR/fibonacci.bin"
    fi
    
    log_success "演示数据准备完成"
}

# 显示演示菜单
show_demo_menu() {
    echo
    echo "====================================="
    echo "     SGX智能合约演示程序菜单"
    echo "====================================="
    echo "1. 交互式演示模式"
    echo "2. 自动化演示模式"
    echo "3. 性能基准测试"
    echo "4. 运行单元测试"
    echo "5. 运行合约测试"
    echo "6. 查看演示日志"
    echo "7. 清理演示数据"
    echo "8. 退出"
    echo "====================================="
    echo
}

# 交互式演示模式
run_interactive_demo() {
    log_demo "启动交互式演示模式..."
    echo
    
    cd "$PROJECT_ROOT"
    
    # 设置日志文件
    LOG_FILE="$LOGS_DIR/interactive_demo_$(date +%Y%m%d_%H%M%S).log"
    
    log_info "演示日志将保存到: $LOG_FILE"
    echo
    
    # 运行演示程序
    "$DEMO_APP" 2>&1 | tee "$LOG_FILE"
    
    log_success "交互式演示完成"
}

# 自动化演示模式
run_automated_demo() {
    log_demo "启动自动化演示模式..."
    echo
    
    cd "$PROJECT_ROOT"
    
    # 设置日志文件
    LOG_FILE="$LOGS_DIR/automated_demo_$(date +%Y%m%d_%H%M%S).log"
    
    log_info "演示日志将保存到: $LOG_FILE"
    echo
    
    # 自动化演示脚本
    {
        echo "1"  # 执行智能合约
        sleep 2
        echo "2"  # 生成执行证明
        sleep 2
        echo "3"  # 获取Enclave度量
        sleep 2
        echo "4"  # 创建证明报告
        sleep 2
        echo "6"  # 退出
    } | "$DEMO_APP" 2>&1 | tee "$LOG_FILE"
    
    log_success "自动化演示完成"
}

# 性能基准测试
run_performance_benchmark() {
    log_demo "启动性能基准测试..."
    echo
    
    cd "$PROJECT_ROOT"
    
    # 设置日志文件
    LOG_FILE="$LOGS_DIR/benchmark_$(date +%Y%m%d_%H%M%S).log"
    
    log_info "基准测试日志将保存到: $LOG_FILE"
    echo
    
    # 运行基准测试
    {
        echo "5"  # 运行性能基准测试
        sleep 1
        echo "6"  # 退出
    } | "$DEMO_APP" 2>&1 | tee "$LOG_FILE"
    
    log_success "性能基准测试完成"
    
    # 分析基准测试结果
    if grep -q "平均执行时间" "$LOG_FILE"; then
        echo
        log_info "基准测试结果摘要:"
        grep -E "(平均执行时间|吞吐量|总执行时间)" "$LOG_FILE" | while read line; do
            echo "  $line"
        done
    fi
}

# 运行单元测试
run_unit_tests() {
    log_demo "运行单元测试..."
    echo
    
    if [[ ! -f "$TEST_APP" ]]; then
        log_warning "测试程序未找到，尝试构建..."
        cd "$PROJECT_ROOT"
        make test_enclave 2>/dev/null || {
            log_error "无法构建测试程序"
            return 1
        }
    fi
    
    cd "$PROJECT_ROOT"
    
    # 设置日志文件
    LOG_FILE="$LOGS_DIR/unit_tests_$(date +%Y%m%d_%H%M%S).log"
    
    log_info "单元测试日志将保存到: $LOG_FILE"
    echo
    
    # 运行单元测试
    "$TEST_APP" 2>&1 | tee "$LOG_FILE"
    
    # 检查测试结果
    if grep -q "所有测试通过" "$LOG_FILE"; then
        log_success "所有单元测试通过"
    elif grep -q "部分测试失败" "$LOG_FILE"; then
        log_warning "部分单元测试失败，请查看日志"
    else
        log_error "单元测试执行异常"
    fi
}

# 运行合约测试
run_contract_tests() {
    log_demo "运行合约测试..."
    echo
    
    if [[ ! -f "$CONTRACT_TEST_APP" ]]; then
        log_warning "合约测试程序未找到，尝试构建..."
        cd "$PROJECT_ROOT"
        make test_contracts 2>/dev/null || {
            log_error "无法构建合约测试程序"
            return 1
        }
    fi
    
    cd "$PROJECT_ROOT"
    
    # 设置日志文件
    LOG_FILE="$LOGS_DIR/contract_tests_$(date +%Y%m%d_%H%M%S).log"
    
    log_info "合约测试日志将保存到: $LOG_FILE"
    echo
    
    # 运行合约测试
    "$CONTRACT_TEST_APP" 2>&1 | tee "$LOG_FILE"
    
    # 检查测试结果
    if grep -q "所有合约测试通过" "$LOG_FILE"; then
        log_success "所有合约测试通过"
    elif grep -q "部分合约测试失败" "$LOG_FILE"; then
        log_warning "部分合约测试失败，请查看日志"
    else
        log_error "合约测试执行异常"
    fi
}

# 查看演示日志
view_demo_logs() {
    log_info "查看演示日志..."
    echo
    
    if [[ ! -d "$LOGS_DIR" ]] || [[ -z "$(ls -A "$LOGS_DIR" 2>/dev/null)" ]]; then
        log_warning "没有找到演示日志文件"
        return
    fi
    
    echo "可用的日志文件:"
    ls -la "$LOGS_DIR"/
    echo
    
    read -p "请输入要查看的日志文件名（或按Enter查看最新日志）: " log_file
    
    if [[ -z "$log_file" ]]; then
        # 查看最新的日志文件
        latest_log=$(ls -t "$LOGS_DIR"/*.log 2>/dev/null | head -n1)
        if [[ -n "$latest_log" ]]; then
            log_info "显示最新日志: $(basename "$latest_log")"
            echo
            less "$latest_log"
        else
            log_warning "没有找到日志文件"
        fi
    else
        if [[ -f "$LOGS_DIR/$log_file" ]]; then
            log_info "显示日志: $log_file"
            echo
            less "$LOGS_DIR/$log_file"
        else
            log_error "日志文件不存在: $log_file"
        fi
    fi
}

# 清理演示数据
clean_demo_data() {
    log_info "清理演示数据..."
    
    read -p "确定要清理所有演示数据吗？这将删除日志文件和临时数据 (y/N): " -n 1 -r
    echo
    
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        # 清理日志文件
        if [[ -d "$LOGS_DIR" ]]; then
            rm -rf "$LOGS_DIR"/*
            log_success "日志文件已清理"
        fi
        
        # 清理临时文件
        if [[ -d "$PROJECT_ROOT/tmp" ]]; then
            rm -rf "$PROJECT_ROOT/tmp"/*
            log_success "临时文件已清理"
        fi
        
        # 清理对象文件
        if [[ -d "$PROJECT_ROOT/obj" ]]; then
            rm -rf "$PROJECT_ROOT/obj"/*
            log_success "对象文件已清理"
        fi
        
        log_success "演示数据清理完成"
    else
        log_info "清理操作已取消"
    fi
}

# 显示系统信息
show_system_info() {
    echo
    log_info "系统信息:"
    echo "  操作系统: $(uname -s) $(uname -r)"
    echo "  架构: $(uname -m)"
    echo "  CPU信息: $(grep 'model name' /proc/cpuinfo | head -n1 | cut -d':' -f2 | xargs)"
    echo "  内存信息: $(free -h | grep '^Mem:' | awk '{print $2" 总计, "$3" 已用, "$7" 可用"}')"
    
    if [[ -n "$SGX_SDK" ]]; then
        echo "  SGX SDK: $SGX_SDK"
    else
        echo "  SGX SDK: 未设置"
    fi
    
    if [[ -c /dev/sgx_enclave ]] || [[ -c /dev/sgx/enclave ]] || [[ -c /dev/isgx ]]; then
        echo "  SGX设备: 可用"
    else
        echo "  SGX设备: 不可用（模拟模式）"
    fi
    
    echo "  项目路径: $PROJECT_ROOT"
    echo
}

# 主函数
main() {
    echo "====================================="
    echo "    SGX智能合约演示运行脚本"
    echo "====================================="
    
    show_system_info
    
    # 检查环境和构建状态
    check_sgx_environment
    check_build
    prepare_demo_data
    
    # 主循环
    while true; do
        show_demo_menu
        read -p "请选择操作 (1-8): " choice
        
        case $choice in
            1)
                run_interactive_demo
                ;;
            2)
                run_automated_demo
                ;;
            3)
                run_performance_benchmark
                ;;
            4)
                run_unit_tests
                ;;
            5)
                run_contract_tests
                ;;
            6)
                view_demo_logs
                ;;
            7)
                clean_demo_data
                ;;
            8)
                log_info "退出演示程序"
                exit 0
                ;;
            *)
                log_error "无效选择，请输入 1-8"
                ;;
        esac
        
        echo
        read -p "按Enter键继续..." -r
    done
}

# 处理命令行参数
if [[ $# -gt 0 ]]; then
    case "$1" in
        "interactive")
            check_sgx_environment
            check_build
            prepare_demo_data
            run_interactive_demo
            ;;
        "auto")
            check_sgx_environment
            check_build
            prepare_demo_data
            run_automated_demo
            ;;
        "benchmark")
            check_sgx_environment
            check_build
            prepare_demo_data
            run_performance_benchmark
            ;;
        "test")
            check_sgx_environment
            check_build
            prepare_demo_data
            run_unit_tests
            ;;
        "contract-test")
            check_sgx_environment
            check_build
            prepare_demo_data
            run_contract_tests
            ;;
        "clean")
            clean_demo_data
            ;;
        "help")
            echo "用法: $0 [interactive|auto|benchmark|test|contract-test|clean|help]"
            echo "  interactive   - 运行交互式演示"
            echo "  auto         - 运行自动化演示"
            echo "  benchmark    - 运行性能基准测试"
            echo "  test         - 运行单元测试"
            echo "  contract-test - 运行合约测试"
            echo "  clean        - 清理演示数据"
            echo "  help         - 显示此帮助信息"
            echo "  (无参数)      - 显示交互式菜单"
            ;;
        *)
            log_error "未知参数: $1"
            echo "使用 '$0 help' 查看可用选项"
            exit 1
            ;;
    esac
else
    # 无参数时运行交互式菜单
    main
fi