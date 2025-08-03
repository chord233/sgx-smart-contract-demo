#!/bin/bash

# SGX智能合约演示环境设置脚本
# 作者: chord233
# 功能: 自动检测和安装SGX开发环境依赖

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
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

# 检查是否以root权限运行
check_root() {
    if [[ $EUID -eq 0 ]]; then
        log_warning "检测到以root权限运行，某些操作可能需要调整"
    fi
}

# 检测操作系统
detect_os() {
    if [[ -f /etc/os-release ]]; then
        . /etc/os-release
        OS=$NAME
        VER=$VERSION_ID
        log_info "检测到操作系统: $OS $VER"
    else
        log_error "无法检测操作系统版本"
        exit 1
    fi
}

# 检查SGX硬件支持
check_sgx_hardware() {
    log_info "检查SGX硬件支持..."
    
    # 检查CPU是否支持SGX
    if grep -q sgx /proc/cpuinfo; then
        log_success "CPU支持SGX指令集"
    else
        log_warning "CPU可能不支持SGX指令集，请检查BIOS设置"
    fi
    
    # 检查SGX设备节点
    if [[ -c /dev/sgx_enclave ]] && [[ -c /dev/sgx_provision ]]; then
        log_success "SGX设备节点存在"
    elif [[ -c /dev/sgx/enclave ]] && [[ -c /dev/sgx/provision ]]; then
        log_success "SGX设备节点存在 (新版本路径)"
    elif [[ -c /dev/isgx ]]; then
        log_success "SGX设备节点存在 (旧版本路径)"
    else
        log_warning "未找到SGX设备节点，可能需要安装SGX驱动"
    fi
}

# 安装基础依赖
install_basic_dependencies() {
    log_info "安装基础依赖包..."
    
    case "$OS" in
        *"Ubuntu"*|*"Debian"*)
            sudo apt-get update
            sudo apt-get install -y \
                build-essential \
                cmake \
                git \
                wget \
                curl \
                gnupg \
                software-properties-common \
                apt-transport-https \
                ca-certificates \
                lsb-release \
                python3 \
                python3-pip \
                pkg-config \
                libssl-dev \
                libcurl4-openssl-dev \
                libprotobuf-dev \
                libprotobuf-c-dev \
                protobuf-compiler \
                protobuf-c-compiler \
                autoconf \
                libtool \
                ocaml \
                ocaml-native-compilers \
                camlp4 \
                libncurses5-dev \
                flex \
                bison
            ;;
        *"CentOS"*|*"Red Hat"*|*"Fedora"*)
            sudo yum groupinstall -y "Development Tools"
            sudo yum install -y \
                cmake \
                git \
                wget \
                curl \
                gnupg \
                python3 \
                python3-pip \
                pkgconfig \
                openssl-devel \
                libcurl-devel \
                protobuf-devel \
                protobuf-c-devel \
                protobuf-compiler \
                protobuf-c \
                autoconf \
                libtool \
                ocaml \
                ncurses-devel \
                flex \
                bison
            ;;
        *)
            log_warning "未识别的操作系统，请手动安装依赖"
            ;;
    esac
    
    log_success "基础依赖安装完成"
}

# 安装Intel SGX SDK
install_sgx_sdk() {
    log_info "安装Intel SGX SDK..."
    
    # 设置SGX SDK版本
    SGX_SDK_VERSION="2.19"
    SGX_SDK_URL="https://download.01.org/intel-sgx/sgx-linux/${SGX_SDK_VERSION}/distro/ubuntu20.04-server/sgx_linux_x64_sdk_${SGX_SDK_VERSION}.100.3.bin"
    
    # 创建临时目录
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    
    # 下载SGX SDK
    log_info "下载SGX SDK..."
    wget -O sgx_sdk.bin "$SGX_SDK_URL" || {
        log_error "SGX SDK下载失败"
        return 1
    }
    
    # 安装SGX SDK
    chmod +x sgx_sdk.bin
    echo -e 'no\n/opt/intel' | sudo ./sgx_sdk.bin
    
    # 清理临时文件
    cd /
    rm -rf "$TEMP_DIR"
    
    log_success "SGX SDK安装完成"
}

# 安装Intel SGX PSW (Platform Software)
install_sgx_psw() {
    log_info "安装Intel SGX PSW..."
    
    case "$OS" in
        *"Ubuntu"*|*"Debian"*)
            # 添加Intel SGX APT仓库
            echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu focal main' | sudo tee /etc/apt/sources.list.d/intel-sgx.list
            wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | sudo apt-key add -
            
            sudo apt-get update
            sudo apt-get install -y \
                libsgx-launch \
                libsgx-urts \
                libsgx-quote-ex \
                libsgx-epid \
                libsgx-dcap-ql \
                libsgx-dcap-default-qpl
            ;;
        *)
            log_warning "请手动安装SGX PSW for $OS"
            ;;
    esac
    
    log_success "SGX PSW安装完成"
}

# 配置SGX环境变量
setup_sgx_environment() {
    log_info "配置SGX环境变量..."
    
    # SGX SDK环境变量
    SGX_SDK_PATH="/opt/intel/sgxsdk"
    
    if [[ -d "$SGX_SDK_PATH" ]]; then
        # 添加到bashrc
        if ! grep -q "SGX_SDK" ~/.bashrc; then
            echo "" >> ~/.bashrc
            echo "# Intel SGX SDK Environment" >> ~/.bashrc
            echo "export SGX_SDK=$SGX_SDK_PATH" >> ~/.bashrc
            echo "export PATH=\$PATH:\$SGX_SDK/bin/x64" >> ~/.bashrc
            echo "export PKG_CONFIG_PATH=\$PKG_CONFIG_PATH:\$SGX_SDK/pkgconfig" >> ~/.bashrc
            echo "export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:\$SGX_SDK/sdk_libs" >> ~/.bashrc
        fi
        
        # 为当前会话设置环境变量
        export SGX_SDK="$SGX_SDK_PATH"
        export PATH="$PATH:$SGX_SDK/bin/x64"
        export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$SGX_SDK/pkgconfig"
        export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$SGX_SDK/sdk_libs"
        
        log_success "SGX环境变量配置完成"
    else
        log_error "SGX SDK路径不存在: $SGX_SDK_PATH"
    fi
}

# 验证SGX安装
verify_sgx_installation() {
    log_info "验证SGX安装..."
    
    # 检查SGX SDK工具
    if command -v sgx_edger8r >/dev/null 2>&1; then
        log_success "sgx_edger8r 工具可用"
    else
        log_error "sgx_edger8r 工具不可用"
    fi
    
    if command -v sgx_sign >/dev/null 2>&1; then
        log_success "sgx_sign 工具可用"
    else
        log_error "sgx_sign 工具不可用"
    fi
    
    # 检查SGX库文件
    if [[ -f "/opt/intel/sgxsdk/lib64/libsgx_urts.so" ]]; then
        log_success "SGX运行时库存在"
    else
        log_error "SGX运行时库不存在"
    fi
    
    # 检查SGX服务状态
    if systemctl is-active --quiet aesmd 2>/dev/null; then
        log_success "AESM服务正在运行"
    else
        log_warning "AESM服务未运行，某些SGX功能可能不可用"
    fi
}

# 创建项目目录结构
setup_project_structure() {
    log_info "设置项目目录结构..."
    
    PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/.."
    cd "$PROJECT_ROOT"
    
    # 创建必要的目录
    mkdir -p bin obj logs tmp test_data
    
    # 设置权限
    chmod 755 bin obj logs tmp test_data
    
    # 创建示例数据文件
    if [[ ! -f "data/simple_add.bin" ]]; then
        mkdir -p data
        # 创建一个简单的加法合约字节码
        echo -ne '\x60\x05\x60\x0a\x01\x60\x00\x52\x60\x20\x60\x00\xf3' > data/simple_add.bin
        log_success "创建示例合约数据"
    fi
    
    log_success "项目目录结构设置完成"
}

# 安装开发工具
install_development_tools() {
    log_info "安装开发工具..."
    
    # 安装代码格式化工具
    if ! command -v clang-format >/dev/null 2>&1; then
        case "$OS" in
            *"Ubuntu"*|*"Debian"*)
                sudo apt-get install -y clang-format
                ;;
            *"CentOS"*|*"Red Hat"*|*"Fedora"*)
                sudo yum install -y clang-tools-extra
                ;;
        esac
    fi
    
    # 安装静态分析工具
    if ! command -v cppcheck >/dev/null 2>&1; then
        case "$OS" in
            *"Ubuntu"*|*"Debian"*)
                sudo apt-get install -y cppcheck
                ;;
            *"CentOS"*|*"Red Hat"*|*"Fedora"*)
                sudo yum install -y cppcheck
                ;;
        esac
    fi
    
    log_success "开发工具安装完成"
}

# 主函数
main() {
    echo "====================================="
    echo "  SGX智能合约演示环境设置脚本"
    echo "====================================="
    echo
    
    check_root
    detect_os
    check_sgx_hardware
    
    # 询问用户是否继续
    read -p "是否继续安装SGX开发环境? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        log_info "安装已取消"
        exit 0
    fi
    
    install_basic_dependencies
    install_sgx_sdk
    install_sgx_psw
    setup_sgx_environment
    setup_project_structure
    install_development_tools
    verify_sgx_installation
    
    echo
    log_success "SGX开发环境设置完成!"
    echo
    echo "请运行以下命令重新加载环境变量:"
    echo "  source ~/.bashrc"
    echo
    echo "或者重新打开终端窗口。"
    echo
    echo "然后可以运行以下命令构建项目:"
    echo "  cd $(pwd)"
    echo "  ./build.sh"
    echo
}

# 如果脚本被直接执行（而不是被source）
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi