#!/bin/bash

# SGX智能合约演示项目构建脚本
# 此脚本用于构建完整的SGX应用程序，包括Enclave和宿主应用程序
# 支持真实SGX环境和演示模式两种构建方式

set -e  # 遇到任何错误时立即退出

# 终端输出颜色定义
RED='\033[0;31m'     # 红色 - 用于错误信息
GREEN='\033[0;32m'   # 绿色 - 用于成功信息
YELLOW='\033[1;33m'  # 黄色 - 用于警告信息
BLUE='\033[0;34m'    # 蓝色 - 用于状态信息
NC='\033[0m'         # 无颜色 - 重置颜色

# 彩色输出函数定义
# 这些函数用于在终端中显示不同类型的消息

# 打印状态信息（蓝色）
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# 打印成功信息（绿色）
print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# 打印警告信息（黄色）
print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# 打印错误信息（红色）
print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查SGX SDK是否已安装
# 此函数负责验证SGX SDK的可用性，并根据情况设置构建模式
check_sgx_sdk() {
    print_status "正在检查SGX SDK安装状态..."
    
    # 检查是否启用演示模式标志
    # 演示模式允许在没有真实SGX SDK的情况下进行模拟构建
    if [ "$1" = "--demo" ] || [ "$DEMO_MODE" = "1" ]; then
        print_warning "正在运行演示模式 - 创建无需SGX SDK的模拟构建"
        export SGX_MODE=SIM      # 设置为模拟模式
        export DEMO_MODE=1       # 启用演示模式标志
        return 0
    fi
    
    # 检查SGX_SDK环境变量是否已设置
    if [ -z "$SGX_SDK" ]; then
        # 尝试使用默认的SGX SDK安装路径
        if [ -d "/opt/intel/sgxsdk" ]; then
            export SGX_SDK=/opt/intel/sgxsdk
            print_warning "SGX_SDK环境变量未设置，使用默认路径: $SGX_SDK"
        else
            print_warning "未找到SGX SDK。请使用 --demo 标志进入演示模式。"
            print_error "要使用真实SGX构建：请安装Intel SGX SDK并设置SGX_SDK环境变量。"
            print_status "转为运行演示模式..."
            export SGX_MODE=SIM      # 设置模拟模式
            export DEMO_MODE=1       # 启用演示模式
            return 0
        fi
    fi
    
    # 检查SGX SDK环境配置文件是否存在
    if [ ! -f "$SGX_SDK/environment" ]; then
        print_warning "在 $SGX_SDK/environment 未找到SGX SDK环境配置文件"
        print_status "回退到演示模式..."
        export SGX_MODE=SIM      # 设置模拟模式
        export DEMO_MODE=1       # 启用演示模式
        return 0
    fi
    
    # 加载SGX环境变量
    source $SGX_SDK/environment
    
    # 设置SGX工具路径
    export SGX_EDGER8R=$SGX_SDK/bin/x64/sgx_edger8r
    export SGX_SIGN=$SGX_SDK/bin/x64/sgx_sign
    
    print_success "SGX SDK已找到，路径: $SGX_SDK"
}

# 检查构建依赖项
# 此函数验证所有必需的构建工具是否已安装
check_dependencies() {
    print_status "正在检查构建依赖项..."
    
    # 检查必需的构建工具
    local missing_deps=()  # 存储缺失的依赖项
    
    # 逐一检查每个必需的工具是否可用
    command -v gcc >/dev/null 2>&1 || missing_deps+=("gcc")        # C编译器
    command -v g++ >/dev/null 2>&1 || missing_deps+=("g++")        # C++编译器
    command -v make >/dev/null 2>&1 || missing_deps+=("make")      # 构建工具
    command -v openssl >/dev/null 2>&1 || missing_deps+=("openssl") # 加密库
    
    # 如果有缺失的依赖项，显示错误信息并退出
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "缺失依赖项: ${missing_deps[*]}"
        print_status "请安装缺失的依赖项:"
        echo "  Ubuntu/Debian: sudo apt-get install build-essential libssl-dev"
        echo "  CentOS/RHEL: sudo yum install gcc gcc-c++ make openssl-devel"
        exit 1
    fi
    
    print_success "所有依赖项已找到"
}

# 创建必要的目录结构
# 此函数设置项目构建所需的所有目录
setup_directories() {
    print_status "正在设置构建目录..."
    
    mkdir -p bin     # 存放可执行文件和签名的Enclave
    mkdir -p obj     # 存放编译生成的目标文件
    mkdir -p data    # 存放示例数据和合约文件
    mkdir -p logs    # 存放日志文件
    
    print_success "目录创建完成"
}

# 生成EDL（Enclave Definition Language）文件
# EDL文件定义了Enclave与外部应用程序之间的接口
generate_edl() {
    print_status "正在生成EDL文件..."
    
    # 检查EDL文件是否存在
    if [ ! -f "enclave/enclave.edl" ]; then
        print_error "未找到enclave/enclave.edl文件"
        exit 1
    fi
    
    # 生成可信和不可信的代理文件
    # --trusted: 生成Enclave内部使用的代理代码
    # --untrusted: 生成宿主应用程序使用的代理代码
    $SGX_EDGER8R --trusted enclave/enclave.edl --search-path $SGX_SDK/include --search-path $SGX_SDK/include/tlibc --search-path $SGX_SDK/include/libcxx --search-path ./enclave
    $SGX_EDGER8R --untrusted enclave/enclave.edl --search-path $SGX_SDK/include --search-path $SGX_SDK/include/tlibc --search-path $SGX_SDK/include/libcxx --search-path ./enclave
    
    print_success "EDL文件生成完成"
}

# 构建Enclave
# 此函数编译所有Enclave源文件并链接成共享库
build_enclave() {
    print_status "正在构建Enclave..."
    
    # 定义Enclave源文件列表
    local enclave_sources=(
        "enclave/enclave.cpp"           # 主Enclave实现
        "enclave/contract_verifier.cpp" # 智能合约验证器
        "enclave/crypto_utils.cpp"     # 加密工具函数
        "enclave_t.c"                  # EDL生成的可信代理代码
    )
    
    local enclave_objects=()  # 存储编译后的目标文件
    
    # 逐个编译每个源文件
    for source in "${enclave_sources[@]}"; do
        # 检查源文件是否存在
        if [ ! -f "$source" ]; then
            print_error "未找到源文件: $source"
            exit 1
        fi
        
        # 生成目标文件路径
        local obj_file="obj/$(basename ${source%.*}).o"
        enclave_objects+=("$obj_file")
        
        print_status "正在编译 $source..."
        
        # 使用SGX编译器编译源文件
        $SGX_SDK/bin/x64/sgx_gcc -c "$source" -o "$obj_file" \
            -I$SGX_SDK/include \
            -I$SGX_SDK/include/tlibc \
            -I$SGX_SDK/include/libcxx \
            -Ienclave \
            -nostdinc \
            -fvisibility=hidden \
            -fpie \
            -ffunction-sections \
            -fdata-sections \
            -fstack-protector-strong \
            -O2 \
            -D_FORTIFY_SOURCE=2
    done
    
    # 链接Enclave
    print_status "正在链接Enclave..."
    
    # 使用SGX链接器将所有目标文件链接成Enclave共享库
    $SGX_SDK/bin/x64/sgx_gcc "${enclave_objects[@]}" -o bin/enclave.so \
        -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles \
        -L$SGX_SDK/lib64 \
        -Wl,--whole-archive -lsgx_trts -Wl,--no-whole-archive \
        -Wl,--start-group -lsgx_tstdc -lsgx_tcxx -lsgx_tcrypto -lsgx_tservice -Wl,--end-group \
        -Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
        -Wl,-pie,-eenclave_entry -Wl,--export-dynamic \
        -Wl,--defsym,__ImageBase=0 \
        -Wl,--gc-sections \
        -Wl,--version-script=enclave/enclave.lds
    
    print_success "Enclave构建成功"
}

# 签名Enclave
# 此函数使用私钥对Enclave进行数字签名，这是SGX安全机制的重要组成部分
sign_enclave() {
    print_status "正在签名Enclave..."
    
    # 如果不存在Enclave配置文件，则创建默认配置
    if [ ! -f "enclave.config.xml" ]; then
        cat > enclave.config.xml << EOF
<EnclaveConfiguration>
  <ProdID>0</ProdID>                    <!-- 产品ID -->
  <ISVSVN>0</ISVSVN>                    <!-- 独立软件供应商安全版本号 -->
  <StackMaxSize>0x40000</StackMaxSize>  <!-- 栈最大大小：256KB -->
  <HeapMaxSize>0x100000</HeapMaxSize>   <!-- 堆最大大小：1MB -->
  <TCSNum>10</TCSNum>                   <!-- 线程控制结构数量 -->
  <TCSPolicy>1</TCSPolicy>              <!-- TCS策略 -->
  <DisableDebug>0</DisableDebug>        <!-- 启用调试模式 -->
  <MiscSelect>0</MiscSelect>            <!-- 其他选择位 -->
  <MiscMask>0xFFFFFFFF</MiscMask>       <!-- 其他掩码 -->
</EnclaveConfiguration>
EOF
        print_status "已创建默认Enclave配置文件"
    fi
    
    # 如果不存在私钥文件，则生成新的私钥
    if [ ! -f "enclave_private.pem" ]; then
        openssl genrsa -out enclave_private.pem -3 3072
        print_status "已生成Enclave私钥"
    fi
    
    # 使用SGX签名工具对Enclave进行签名
    $SGX_SDK/bin/x64/sgx_sign sign \
        -key enclave_private.pem \
        -enclave bin/enclave.so \
        -out bin/enclave.signed.so \
        -config enclave.config.xml
    
    print_success "Enclave签名成功"
}

# 构建宿主应用程序
# 此函数编译并链接运行在普通操作系统环境中的宿主应用程序
build_app() {
    print_status "正在构建宿主应用程序..."
    
    # 定义应用程序源文件列表
    local app_sources=(
        "app/main.cpp"      # 主程序入口
        "app/app_utils.cpp" # 应用程序工具函数
        "enclave_u.c"       # EDL生成的不可信代理代码
    )
    
    local app_objects=()  # 存储编译后的目标文件
    
    # 逐个编译每个源文件
    for source in "${app_sources[@]}"; do
        # 检查源文件是否存在
        if [ ! -f "$source" ]; then
            print_error "未找到源文件: $source"
            exit 1
        fi
        
        # 生成目标文件路径
        local obj_file="obj/$(basename ${source%.*}).o"
        app_objects+=("$obj_file")
        
        print_status "正在编译 $source..."
        
        # 使用g++编译源文件
        g++ -c "$source" -o "$obj_file" \
            -I$SGX_SDK/include \
            -Iapp \
            -std=c++17 \
            -O2 \
            -D_FORTIFY_SOURCE=2 \
            -fstack-protector-strong
    done
    
    # 链接应用程序
    print_status "正在链接宿主应用程序..."
    
    # 链接所有目标文件和必要的库
    g++ "${app_objects[@]}" -o bin/sgx-smart-contract-demo \
        -L$SGX_SDK/lib64 \
        -lsgx_urts \
        -lsgx_uae_service \
        -lssl \
        -lcrypto \
        -lpthread \
        -ldl
    
    print_success "宿主应用程序构建成功"
}

# 创建示例数据
# 此函数设置测试和演示所需的示例合约和数据文件
setup_sample_data() {
    print_status "正在设置示例数据..."
    
    # 将示例合约复制到数据目录
    if [ -d "contracts" ]; then
        cp -r contracts/* data/ 2>/dev/null || true
    fi
    
    # 创建一个简单的二进制合约用于测试
    # 这个合约实现了简单的加法运算：PUSH 10, PUSH 20, ADD, HALT
    echo -n -e '\x01\x0a\x00\x00\x00\x00\x00\x00\x00\x01\x14\x00\x00\x00\x00\x00\x00\x00\x03\x18' > data/simple_add.bin
    
    print_success "示例数据创建完成"
}

# 运行基础测试
# 此函数验证构建产物是否正确生成并可执行
run_tests() {
    print_status "正在运行基础测试..."
    
    # 检查构建的文件是否存在
    if [ ! -f "bin/enclave.signed.so" ]; then
        print_error "未找到已签名的Enclave文件"
        exit 1
    fi
    
    if [ ! -f "bin/sgx-smart-contract-demo" ]; then
        print_error "未找到宿主应用程序"
        exit 1
    fi
    
    # 设置应用程序为可执行
    chmod +x bin/sgx-smart-contract-demo
    
    print_success "所有测试通过"
}

# 清理构建产物
# 此函数删除所有构建过程中生成的文件和目录
clean() {
    print_status "正在清理构建产物..."
    
    rm -rf obj/                # 删除目标文件目录
    rm -rf bin/                # 删除二进制文件目录
    rm -f enclave_t.c enclave_t.h    # 删除EDL生成的可信代理文件
    rm -f enclave_u.c enclave_u.h    # 删除EDL生成的不可信代理文件
    rm -f enclave.config.xml         # 删除Enclave配置文件
    rm -f enclave_private.pem        # 删除私钥文件
    
    print_success "清理完成"
}

# 显示使用说明
# 此函数打印脚本的使用方法和可用选项
show_usage() {
    echo "用法: $0 [选项]"
    echo "SGX智能合约演示项目构建脚本"
    echo ""
    echo "选项:"
    echo "  build     构建整个项目 (默认)"
    echo "  --demo    演示模式构建 (无需SGX SDK)"
    echo "  clean     清理构建产物"
    echo "  test      运行测试"
    echo "  help      显示此帮助信息"
    echo ""
    echo "环境变量:"
    echo "  SGX_SDK   Intel SGX SDK路径 (默认: /opt/intel/sgxsdk)"
    echo "  SGX_MODE  构建模式: HW 或 SIM (默认: SIM)"
    echo "  SGX_ARCH  架构: x64 或 x86 (默认: x64)"
    echo "  DEMO_MODE 演示模式标志 (设置为1启用演示模式)"
}

# 演示模式构建（无需SGX SDK）
# 此函数在没有真实SGX SDK的环境中创建模拟构建
build_demo() {
    print_status "正在构建无需SGX SDK的演示版本..."
    
    setup_directories
    
    # 创建模拟的Enclave二进制文件
    print_status "正在创建模拟Enclave二进制文件..."
    echo "Mock SGX Enclave for Demo" > bin/enclave.signed.so
    
    # 创建简单的演示应用程序
    print_status "正在创建演示应用程序..."
    cat > bin/sgx-smart-contract-demo << 'EOF'
#!/bin/bash
echo "=== SGX智能合约演示 (模拟模式) ==="
echo "这是一个无需真实SGX支持的演示版本。"
echo ""
echo "可用选项:"
echo "1. 初始化Enclave (模拟)"
echo "2. 执行示例合约 (模拟)"
echo "3. 生成执行证明 (模拟)"
echo "4. 获取Enclave测量值 (模拟)"
echo "5. 创建远程证明报告 (模拟)"
echo "6. 性能基准测试 (模拟)"
echo "0. 退出"
echo ""
read -p "请选择选项 [0-6]: " choice

case $choice in
    1) echo "[模拟] Enclave创建成功"; echo "Enclave ID: 0x12345678" ;;
    2) echo "[模拟] 合约执行成功"; echo "结果: 30 (10 + 20)"; echo "Gas消耗: 8" ;;
    3) echo "[模拟] 证明生成成功"; echo "证明: 0xabcdef123456789..." ;;
    4) echo "[模拟] Enclave测量值已获取"; echo "MRENCLAVE: 0x1234567890abcdef..." ;;
    5) echo "[模拟] 远程证明报告已创建"; echo "报告大小: 432 字节" ;;
    6) echo "[模拟] 基准测试完成"; echo "吞吐量: 1000 合约/秒"; echo "平均执行时间: 1.0ms" ;;
    0) echo "再见!" ;;
    *) echo "无效选项" ;;
esac
EOF
    
    chmod +x bin/sgx-smart-contract-demo
    
    setup_sample_data
    
    print_success "演示构建完成!"
    echo ""
    echo "运行演示:"
    echo "  ./run.sh"
    echo "  或者"
    echo "  cd bin && ./sgx-smart-contract-demo"
}

# 主构建函数
# 此函数协调整个构建过程，根据环境和参数选择构建模式
build_all() {
    print_status "开始SGX智能合约演示项目构建..."
    
    # 检查SGX SDK并确定构建模式
    check_sgx_sdk "$1"
    
    # 如果启用演示模式，则执行演示构建
    if [ "$DEMO_MODE" = "1" ]; then
        build_demo
        return 0
    fi
    
    # 执行完整的SGX构建流程
    check_dependencies    # 检查构建依赖
    setup_directories     # 创建目录结构
    generate_edl         # 生成EDL代理文件
    build_enclave        # 构建Enclave
    sign_enclave         # 签名Enclave
    build_app            # 构建宿主应用程序
    setup_sample_data    # 设置示例数据
    run_tests            # 运行基础测试
    
    print_success "构建成功完成!"
    echo ""
    echo "运行演示:"
    echo "  cd bin"
    echo "  ./sgx-smart-contract-demo"
    echo ""
    echo "如需要，请设置LD_LIBRARY_PATH:"
    echo "  export LD_LIBRARY_PATH=$SGX_SDK/lib64:$LD_LIBRARY_PATH"
}

# 解析命令行参数
# 根据用户输入的参数执行相应的操作
case "${1:-build}" in
    build)
        # 标准构建模式
        build_all "$2"
        ;;
    --demo)
        # 演示模式构建
        export DEMO_MODE=1
        build_all "--demo"
        ;;
    clean)
        # 清理构建产物
        clean
        ;;
    test)
        # 运行测试
        run_tests
        ;;
    help|--help|-h)
        # 显示帮助信息
        show_usage
        ;;
    *)
        # 未知选项，显示错误并退出
        print_error "未知选项: $1"
        show_usage
        exit 1
        ;;
esac