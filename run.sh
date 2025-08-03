#!/bin/bash

# SGX Smart Contract Demo Run Script
# This script provides various ways to run and test the SGX smart contract demo

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Configuration
DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_DIR="$DEMO_DIR/bin"
DATA_DIR="$DEMO_DIR/data"
LOGS_DIR="$DEMO_DIR/logs"
APP_NAME="sgx-smart-contract-demo"

# Check if the application is built
check_build() {
    if [ ! -f "$BIN_DIR/$APP_NAME" ]; then
        print_error "Application not found. Please build first:"
        echo "  ./build.sh"
        exit 1
    fi
    
    if [ ! -f "$BIN_DIR/enclave.signed.so" ]; then
        print_error "Signed enclave not found. Please build first:"
        echo "  ./build.sh"
        exit 1
    fi
}

# Setup environment
setup_environment() {
    print_status "Setting up environment..."
    
    # Create necessary directories
    mkdir -p "$DATA_DIR"
    mkdir -p "$LOGS_DIR"
    
    # Set SGX environment if available
    if [ -n "$SGX_SDK" ] && [ -f "$SGX_SDK/environment" ]; then
        source "$SGX_SDK/environment"
        print_status "SGX environment loaded from $SGX_SDK"
    elif [ -f "/opt/intel/sgxsdk/environment" ]; then
        source "/opt/intel/sgxsdk/environment"
        export SGX_SDK=/opt/intel/sgxsdk
        print_status "SGX environment loaded from /opt/intel/sgxsdk"
    else
        print_warning "SGX SDK not found, some features may not work"
    fi
    
    # Set library path
    if [ -n "$SGX_SDK" ]; then
        export LD_LIBRARY_PATH="$SGX_SDK/lib64:$LD_LIBRARY_PATH"
    fi
    
    # Change to binary directory
    cd "$BIN_DIR"
    
    print_success "Environment setup complete"
}

# Run the demo interactively
run_interactive() {
    print_status "Starting SGX Smart Contract Demo (Interactive Mode)..."
    
    setup_environment
    check_build
    
    echo ""
    echo "=== SGX Smart Contract Verification Demo ==="
    echo "This demo showcases Intel SGX for smart contract verification"
    echo "in a Trusted Execution Environment (TEE)."
    echo ""
    
    ./$APP_NAME
}

# Run automated tests
run_tests() {
    print_status "Running automated tests..."
    
    setup_environment
    check_build
    
    local test_log="$LOGS_DIR/test_$(date +%Y%m%d_%H%M%S).log"
    
    echo "Test started at $(date)" > "$test_log"
    
    # Test 1: Basic functionality
    print_status "Test 1: Basic enclave initialization"
    echo "1" | timeout 30 ./$APP_NAME >> "$test_log" 2>&1 || {
        print_error "Test 1 failed"
        return 1
    }
    print_success "Test 1 passed"
    
    # Test 2: Contract execution
    print_status "Test 2: Sample contract execution"
    echo -e "1\n0" | timeout 30 ./$APP_NAME >> "$test_log" 2>&1 || {
        print_error "Test 2 failed"
        return 1
    }
    print_success "Test 2 passed"
    
    # Test 3: Proof generation
    print_status "Test 3: Execution proof generation"
    echo -e "3\n0" | timeout 30 ./$APP_NAME >> "$test_log" 2>&1 || {
        print_error "Test 3 failed"
        return 1
    }
    print_success "Test 3 passed"
    
    # Test 4: Enclave measurement
    print_status "Test 4: Enclave measurement"
    echo -e "4\n0" | timeout 30 ./$APP_NAME >> "$test_log" 2>&1 || {
        print_error "Test 4 failed"
        return 1
    }
    print_success "Test 4 passed"
    
    # Test 5: Attestation report
    print_status "Test 5: Attestation report creation"
    echo -e "5\n0" | timeout 30 ./$APP_NAME >> "$test_log" 2>&1 || {
        print_error "Test 5 failed"
        return 1
    }
    print_success "Test 5 passed"
    
    echo "Test completed at $(date)" >> "$test_log"
    
    print_success "All tests passed! Log saved to $test_log"
}

# Run performance benchmark
run_benchmark() {
    print_status "Running performance benchmark..."
    
    setup_environment
    check_build
    
    local bench_log="$LOGS_DIR/benchmark_$(date +%Y%m%d_%H%M%S).log"
    
    echo "Benchmark started at $(date)" > "$bench_log"
    
    print_status "Running 100 contract executions..."
    echo -e "6\n0" | timeout 120 ./$APP_NAME >> "$bench_log" 2>&1 || {
        print_error "Benchmark failed"
        return 1
    }
    
    echo "Benchmark completed at $(date)" >> "$bench_log"
    
    print_success "Benchmark completed! Results saved to $bench_log"
    
    # Extract and display key metrics
    if grep -q "Throughput:" "$bench_log"; then
        local throughput=$(grep "Throughput:" "$bench_log" | tail -1 | awk '{print $2, $3}')
        print_status "Throughput: $throughput"
    fi
    
    if grep -q "Average time per execution:" "$bench_log"; then
        local avg_time=$(grep "Average time per execution:" "$bench_log" | tail -1 | awk '{print $6, $7}')
        print_status "Average execution time: $avg_time"
    fi
}

# Run with custom contract
run_custom_contract() {
    local contract_file="$1"
    
    if [ -z "$contract_file" ]; then
        print_error "Please specify a contract file"
        echo "Usage: $0 custom <contract_file>"
        exit 1
    fi
    
    if [ ! -f "$contract_file" ]; then
        print_error "Contract file not found: $contract_file"
        exit 1
    fi
    
    print_status "Running custom contract: $contract_file"
    
    setup_environment
    check_build
    
    # Copy contract to data directory
    cp "$contract_file" "$DATA_DIR/custom_contract.bin"
    
    # Run with option 2 (load from file)
    echo -e "2\ncustom_contract.bin\n0" | ./$APP_NAME
}

# Monitor system resources
monitor_resources() {
    print_status "Monitoring system resources during execution..."
    
    setup_environment
    check_build
    
    local monitor_log="$LOGS_DIR/monitor_$(date +%Y%m%d_%H%M%S).log"
    
    # Start resource monitoring in background
    {
        echo "Resource monitoring started at $(date)"
        while true; do
            echo "$(date): $(ps aux | grep $APP_NAME | grep -v grep | awk '{print "CPU: " $3 "%, MEM: " $4 "%"}')"
            echo "$(date): $(free -h | grep Mem | awk '{print "Total: " $2 ", Used: " $3 ", Free: " $4}')"
            sleep 1
        done
    } > "$monitor_log" &
    
    local monitor_pid=$!
    
    # Run the application
    echo -e "6\n0" | timeout 60 ./$APP_NAME
    
    # Stop monitoring
    kill $monitor_pid 2>/dev/null || true
    
    echo "Resource monitoring completed at $(date)" >> "$monitor_log"
    
    print_success "Resource monitoring completed! Log saved to $monitor_log"
}

# Generate demo report
generate_report() {
    print_status "Generating demo report..."
    
    local report_file="$LOGS_DIR/demo_report_$(date +%Y%m%d_%H%M%S).md"
    
    cat > "$report_file" << EOF
# SGX Smart Contract Demo Report

Generated on: $(date)
System: $(uname -a)
SGX SDK: ${SGX_SDK:-"Not found"}

## Demo Overview

This report summarizes the execution of the SGX Smart Contract Verification Demo.
The demo showcases the use of Intel SGX (Software Guard Extensions) to create
a Trusted Execution Environment (TEE) for verifying smart contract execution.

## Key Features Demonstrated

1. **Enclave Initialization**: Secure creation and initialization of SGX enclave
2. **Contract Execution**: Execution of smart contracts within the TEE
3. **Proof Generation**: Creation of cryptographic proofs for contract execution
4. **Remote Attestation**: Generation of attestation reports for verification
5. **Data Integrity**: Ensuring data integrity through cryptographic hashing

## Technical Architecture

- **Enclave**: Secure execution environment for smart contracts
- **Host Application**: Interface for interacting with the enclave
- **Contract Verifier**: Core logic for validating and executing contracts
- **Crypto Utils**: Cryptographic functions for security operations

## Security Features

- Memory protection through SGX
- Cryptographic verification of contract execution
- Secure key generation and management
- Tamper-resistant execution environment
- Remote attestation capabilities

## Performance Characteristics

EOF
    
    # Run a quick benchmark for the report
    setup_environment
    check_build
    
    echo "Running benchmark for report..." >> "$report_file"
    echo -e "6\n0" | timeout 60 ./$APP_NAME 2>&1 | grep -E "(Throughput|Average time|Total time)" >> "$report_file" || true
    
    cat >> "$report_file" << EOF

## Files Generated

- Enclave binary: bin/enclave.signed.so
- Host application: bin/$APP_NAME
- Sample contracts: data/
- Logs: logs/

## Usage Instructions

1. Build the project: \`./build.sh\`
2. Run interactively: \`./run.sh\`
3. Run tests: \`./run.sh test\`
4. Run benchmark: \`./run.sh benchmark\`

## Conclusion

This demo successfully demonstrates the integration of Intel SGX with smart
contract verification, providing a secure and verifiable execution environment
for blockchain applications.

For more information, see the project README and documentation.
EOF
    
    print_success "Demo report generated: $report_file"
}

# Show system information
show_info() {
    echo "=== SGX Smart Contract Demo - System Information ==="
    echo ""
    echo "Demo Directory: $DEMO_DIR"
    echo "Binary Directory: $BIN_DIR"
    echo "Data Directory: $DATA_DIR"
    echo "Logs Directory: $LOGS_DIR"
    echo ""
    echo "System Information:"
    echo "  OS: $(uname -s)"
    echo "  Kernel: $(uname -r)"
    echo "  Architecture: $(uname -m)"
    echo ""
    echo "SGX Information:"
    if [ -n "$SGX_SDK" ]; then
        echo "  SGX SDK: $SGX_SDK"
        if [ -f "$SGX_SDK/bin/x64/sgx_cap" ]; then
            echo "  SGX Capabilities:"
            "$SGX_SDK/bin/x64/sgx_cap" 2>/dev/null | head -10 || echo "    Unable to query SGX capabilities"
        fi
    else
        echo "  SGX SDK: Not found"
    fi
    echo ""
    echo "Build Status:"
    if [ -f "$BIN_DIR/$APP_NAME" ]; then
        echo "  Application: Built ($(stat -c%s "$BIN_DIR/$APP_NAME") bytes)"
    else
        echo "  Application: Not built"
    fi
    if [ -f "$BIN_DIR/enclave.signed.so" ]; then
        echo "  Enclave: Built and signed ($(stat -c%s "$BIN_DIR/enclave.signed.so") bytes)"
    else
        echo "  Enclave: Not built"
    fi
    echo ""
}

# Show usage
show_usage() {
    echo "Usage: $0 [COMMAND] [OPTIONS]"
    echo "Run script for SGX Smart Contract Demo"
    echo ""
    echo "Commands:"
    echo "  (no args)     Run demo interactively (default)"
    echo "  test          Run automated tests"
    echo "  benchmark     Run performance benchmark"
    echo "  custom FILE   Run with custom contract file"
    echo "  monitor       Monitor system resources during execution"
    echo "  report        Generate demo report"
    echo "  info          Show system and build information"
    echo "  help          Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                              # Run interactively"
    echo "  $0 test                         # Run all tests"
    echo "  $0 benchmark                    # Run performance test"
    echo "  $0 custom contracts/my_contract.bin  # Run custom contract"
    echo "  $0 monitor                      # Monitor resources"
    echo ""
}

# Main execution
case "${1:-interactive}" in
    interactive|"")
        run_interactive
        ;;
    test)
        run_tests
        ;;
    benchmark)
        run_benchmark
        ;;
    custom)
        run_custom_contract "$2"
        ;;
    monitor)
        monitor_resources
        ;;
    report)
        generate_report
        ;;
    info)
        show_info
        ;;
    help|--help|-h)
        show_usage
        ;;
    *)
        print_error "Unknown command: $1"
        show_usage
        exit 1
        ;;
esac