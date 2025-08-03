#!/bin/bash

# SGX Smart Contract Demo Test Script
# Comprehensive testing suite for the SGX smart contract verification demo

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
DEMO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$DEMO_DIR/test_data"
LOGS_DIR="$DEMO_DIR/logs"
BIN_DIR="$DEMO_DIR/bin"
APP_NAME="sgx-smart-contract-demo"
TEST_LOG="$LOGS_DIR/test_$(date +%Y%m%d_%H%M%S).log"

# Test counters
TEST_TOTAL=0
TEST_PASSED=0
TEST_FAILED=0

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

print_test_header() {
    echo ""
    echo "=== $1 ==="
    echo ""
}

print_test_result() {
    local test_name="$1"
    local result="$2"
    
    TEST_TOTAL=$((TEST_TOTAL + 1))
    
    if [ "$result" = "PASS" ]; then
        TEST_PASSED=$((TEST_PASSED + 1))
        print_success "Test $TEST_TOTAL: $test_name - PASSED"
    else
        TEST_FAILED=$((TEST_FAILED + 1))
        print_error "Test $TEST_TOTAL: $test_name - FAILED"
    fi
    
    echo "$(date): Test $TEST_TOTAL: $test_name - $result" >> "$TEST_LOG"
}

# Setup test environment
setup_test_environment() {
    print_status "Setting up test environment..."
    
    # Create test directories
    mkdir -p "$TEST_DIR" "$LOGS_DIR"
    
    # Initialize test log
    echo "SGX Smart Contract Demo Test Suite" > "$TEST_LOG"
    echo "Started at: $(date)" >> "$TEST_LOG"
    echo "Test directory: $TEST_DIR" >> "$TEST_LOG"
    echo "" >> "$TEST_LOG"
    
    # Set SGX environment
    if [ -n "$SGX_SDK" ] && [ -f "$SGX_SDK/environment" ]; then
        source "$SGX_SDK/environment"
    elif [ -f "/opt/intel/sgxsdk/environment" ]; then
        source "/opt/intel/sgxsdk/environment"
        export SGX_SDK=/opt/intel/sgxsdk
    fi
    
    # Create test contracts
    create_test_contracts
    
    print_success "Test environment setup complete"
}

# Create test contracts
create_test_contracts() {
    print_status "Creating test contracts..."
    
    # Simple addition contract
    echo -e "\x01\x0A\x01\x14\x02\x00" > "$TEST_DIR/add_test.bin"
    
    # Factorial contract (simplified)
    echo -e "\x01\x05\x01\x01\x01\x01\x03\x07\x0C\x01\x01\x02\x06\x00" > "$TEST_DIR/factorial_test.bin"
    
    # Hash verification contract
    echo -e "\x01\x20\x12\x34\x56\x78\x9A\xBC\xDE\xF0\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\x10\x00" > "$TEST_DIR/hash_test.bin"
    
    # Invalid contract (for error testing)
    echo -e "\xFF\xFF\xFF\xFF" > "$TEST_DIR/invalid_test.bin"
    
    # Large contract (for stress testing)
    dd if=/dev/zero of="$TEST_DIR/large_test.bin" bs=1024 count=64 2>/dev/null
    
    print_success "Test contracts created"
}

# Test 1: Build verification
test_build_verification() {
    print_test_header "Build Verification"
    
    if [ -f "$BIN_DIR/$APP_NAME" ] && [ -f "$BIN_DIR/enclave.signed.so" ]; then
        print_test_result "Build verification" "PASS"
    else
        print_test_result "Build verification" "FAIL"
        print_error "Application or enclave not found. Please run ./build.sh first"
        return 1
    fi
}

# Test 2: Enclave initialization
test_enclave_initialization() {
    print_test_header "Enclave Initialization"
    
    cd "$BIN_DIR"
    
    # Test enclave creation and destruction
    local output
    output=$(echo "0" | timeout 10 ./$APP_NAME 2>&1) || {
        print_test_result "Enclave initialization" "FAIL"
        return 1
    }
    
    if echo "$output" | grep -q "Enclave created successfully"; then
        print_test_result "Enclave initialization" "PASS"
    else
        print_test_result "Enclave initialization" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Test 3: Sample contract execution
test_sample_contract_execution() {
    print_test_header "Sample Contract Execution"
    
    cd "$BIN_DIR"
    
    local output
    output=$(echo -e "1\n0" | timeout 15 ./$APP_NAME 2>&1) || {
        print_test_result "Sample contract execution" "FAIL"
        return 1
    }
    
    if echo "$output" | grep -q "Contract executed successfully"; then
        print_test_result "Sample contract execution" "PASS"
    else
        print_test_result "Sample contract execution" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Test 4: Custom contract loading
test_custom_contract_loading() {
    print_test_header "Custom Contract Loading"
    
    cd "$BIN_DIR"
    
    # Copy test contract to data directory
    cp "$TEST_DIR/add_test.bin" "../data/test_contract.bin"
    
    local output
    output=$(echo -e "2\ntest_contract.bin\n0" | timeout 15 ./$APP_NAME 2>&1) || {
        print_test_result "Custom contract loading" "FAIL"
        return 1
    }
    
    if echo "$output" | grep -q "Contract loaded successfully" || echo "$output" | grep -q "Contract executed"; then
        print_test_result "Custom contract loading" "PASS"
    else
        print_test_result "Custom contract loading" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Test 5: Proof generation
test_proof_generation() {
    print_test_header "Proof Generation"
    
    cd "$BIN_DIR"
    
    local output
    output=$(echo -e "3\n0" | timeout 15 ./$APP_NAME 2>&1) || {
        print_test_result "Proof generation" "FAIL"
        return 1
    }
    
    if echo "$output" | grep -q "Proof generated successfully" || echo "$output" | grep -q "Execution proof"; then
        print_test_result "Proof generation" "PASS"
    else
        print_test_result "Proof generation" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Test 6: Enclave measurement
test_enclave_measurement() {
    print_test_header "Enclave Measurement"
    
    cd "$BIN_DIR"
    
    local output
    output=$(echo -e "4\n0" | timeout 15 ./$APP_NAME 2>&1) || {
        print_test_result "Enclave measurement" "FAIL"
        return 1
    }
    
    if echo "$output" | grep -q "Enclave measurement" || echo "$output" | grep -q "MRENCLAVE"; then
        print_test_result "Enclave measurement" "PASS"
    else
        print_test_result "Enclave measurement" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Test 7: Attestation report
test_attestation_report() {
    print_test_header "Attestation Report"
    
    cd "$BIN_DIR"
    
    local output
    output=$(echo -e "5\n0" | timeout 15 ./$APP_NAME 2>&1) || {
        print_test_result "Attestation report" "FAIL"
        return 1
    }
    
    if echo "$output" | grep -q "Attestation report" || echo "$output" | grep -q "Report created"; then
        print_test_result "Attestation report" "PASS"
    else
        print_test_result "Attestation report" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Test 8: Performance benchmark
test_performance_benchmark() {
    print_test_header "Performance Benchmark"
    
    cd "$BIN_DIR"
    
    local output
    output=$(echo -e "6\n0" | timeout 60 ./$APP_NAME 2>&1) || {
        print_test_result "Performance benchmark" "FAIL"
        return 1
    }
    
    if echo "$output" | grep -q "Benchmark completed" || echo "$output" | grep -q "Throughput"; then
        print_test_result "Performance benchmark" "PASS"
    else
        print_test_result "Performance benchmark" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Test 9: Error handling
test_error_handling() {
    print_test_header "Error Handling"
    
    cd "$BIN_DIR"
    
    # Copy invalid contract
    cp "$TEST_DIR/invalid_test.bin" "../data/invalid_contract.bin"
    
    local output
    output=$(echo -e "2\ninvalid_contract.bin\n0" | timeout 15 ./$APP_NAME 2>&1) || true
    
    if echo "$output" | grep -q "Error" || echo "$output" | grep -q "Invalid" || echo "$output" | grep -q "Failed"; then
        print_test_result "Error handling" "PASS"
    else
        print_test_result "Error handling" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Test 10: Memory stress test
test_memory_stress() {
    print_test_header "Memory Stress Test"
    
    cd "$BIN_DIR"
    
    # Copy large contract
    cp "$TEST_DIR/large_test.bin" "../data/large_contract.bin"
    
    local output
    output=$(echo -e "2\nlarge_contract.bin\n0" | timeout 30 ./$APP_NAME 2>&1) || true
    
    # This test should either succeed or fail gracefully
    if echo "$output" | grep -q "executed" || echo "$output" | grep -q "Error" || echo "$output" | grep -q "too large"; then
        print_test_result "Memory stress test" "PASS"
    else
        print_test_result "Memory stress test" "FAIL"
        echo "Output: $output" >> "$TEST_LOG"
    fi
}

# Run all tests
run_all_tests() {
    print_status "Starting comprehensive test suite..."
    
    setup_test_environment
    
    # Run individual tests
    test_build_verification || true
    test_enclave_initialization || true
    test_sample_contract_execution || true
    test_custom_contract_loading || true
    test_proof_generation || true
    test_enclave_measurement || true
    test_attestation_report || true
    test_performance_benchmark || true
    test_error_handling || true
    test_memory_stress || true
    
    # Print summary
    print_test_summary
}

# Print test summary
print_test_summary() {
    echo ""
    echo "=== Test Summary ==="
    echo ""
    echo "Total tests: $TEST_TOTAL"
    echo "Passed: $TEST_PASSED"
    echo "Failed: $TEST_FAILED"
    
    if [ $TEST_FAILED -eq 0 ]; then
        print_success "All tests passed!"
        echo "Result: SUCCESS" >> "$TEST_LOG"
    else
        print_error "$TEST_FAILED test(s) failed"
        echo "Result: FAILURE ($TEST_FAILED failed)" >> "$TEST_LOG"
    fi
    
    echo "Completed at: $(date)" >> "$TEST_LOG"
    echo ""
    echo "Detailed log: $TEST_LOG"
    echo ""
    
    return $TEST_FAILED
}

# Run specific test
run_specific_test() {
    local test_name="$1"
    
    setup_test_environment
    
    case "$test_name" in
        build)
            test_build_verification
            ;;
        init)
            test_enclave_initialization
            ;;
        execute)
            test_sample_contract_execution
            ;;
        load)
            test_custom_contract_loading
            ;;
        proof)
            test_proof_generation
            ;;
        measure)
            test_enclave_measurement
            ;;
        attest)
            test_attestation_report
            ;;
        benchmark)
            test_performance_benchmark
            ;;
        error)
            test_error_handling
            ;;
        stress)
            test_memory_stress
            ;;
        *)
            print_error "Unknown test: $test_name"
            echo "Available tests: build, init, execute, load, proof, measure, attest, benchmark, error, stress"
            exit 1
            ;;
    esac
    
    print_test_summary
}

# Show usage
show_usage() {
    echo "Usage: $0 [TEST_NAME]"
    echo "Test script for SGX Smart Contract Demo"
    echo ""
    echo "Options:"
    echo "  (no args)     Run all tests"
    echo "  build         Test build verification"
    echo "  init          Test enclave initialization"
    echo "  execute       Test sample contract execution"
    echo "  load          Test custom contract loading"
    echo "  proof         Test proof generation"
    echo "  measure       Test enclave measurement"
    echo "  attest        Test attestation report"
    echo "  benchmark     Test performance benchmark"
    echo "  error         Test error handling"
    echo "  stress        Test memory stress"
    echo "  help          Show this help message"
    echo ""
}

# Main execution
case "${1:-all}" in
    all|"")
        run_all_tests
        ;;
    help|--help|-h)
        show_usage
        ;;
    *)
        run_specific_test "$1"
        ;;
esac

exit $?