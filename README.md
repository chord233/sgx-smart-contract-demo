# Intel SGX Smart Contract Verification TEE Demo

**Note: This is a simplified demonstration version for educational purposes.**

This is a demonstration project that uses Intel SGX technology to show basic smart contract execution simulation in a Trusted Execution Environment (TEE). The project includes mock implementations for proof generation and simplified contract execution to demonstrate SGX capabilities.

## Project Overview

This project demonstrates how to use Intel SGX to create a secure trusted execution environment for verifying smart contract execution results and data integrity. SGX enclave provides hardware-level security guarantees, ensuring that smart contract execution processes cannot be tampered with.

## Technical Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
├─────────────────────────────────────────────────────────────┤
│                SGX Enclave (TEE)                           │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │Contract Verifier│  │Data Integrity   │  │Key Management│ │
│  │                 │  │Checker          │  │Module        │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────────┤
│                Intel SGX Hardware Layer                     │
└─────────────────────────────────────────────────────────────┘
```

## Core Features

1. **Smart Contract Verification**: Verify smart contract execution logic within SGX enclave
2. **Data Integrity Protection**: Use SGX to protect sensitive data from tampering
3. **Remote Attestation**: Provide remote attestation functionality for enclave
4. **Secure Communication**: Establish secure communication channels with external systems

## Project Structure

```
sgx-smart-contract-demo/
├── README.md                 # Project documentation
├── README_EN.md              # English documentation
├── Makefile                  # Build script
├── app/                      # Application code
│   ├── main.cpp             # Main program entry
│   ├── app.h                # Application header file
│   └── sgx_utils.cpp        # SGX utility functions
├── enclave/                  # SGX Enclave code
│   ├── enclave.cpp          # Enclave main logic
│   ├── enclave.h            # Enclave header file
│   ├── enclave.edl          # Enclave Definition Language file
│   ├── contract_verifier.cpp # Smart contract verifier
│   └── crypto_utils.cpp     # Cryptographic utility functions
├── bin/                      # Executable scripts and binaries
│   ├── run.sh               # Main run script
│   └── run_demo.sh          # Demo execution script
├── build/                    # Build scripts and tools
│   └── build.sh             # Build script
├── config/                   # Configuration files
│   └── config.json          # Project configuration
├── contracts/                # Sample smart contracts and test data
│   ├── add_test.bin         # Addition test binary
│   ├── sample_contract.txt  # Sample contract text
│   └── simple_add.bin       # Simple addition contract binary
└── docs/                     # Documentation
    ├── DEMO_VERSION.md      # Demo version information
    └── QUICK_START.md       # Quick start guide
```

## Requirements

- Intel SGX supported CPU
- Ubuntu 24.04 LTS
- Intel SGX SDK 2.15+
- Intel SGX PSW (Platform Software)
- GCC 7.5+
- Make

## Quick Start

### 1. Prerequisites

- Intel SGX SDK installed and configured
- Environment variables set: `source /opt/intel/sgxsdk/environment`

### 2. Build and Run

```bash
# Build the demo
make clean && make

# Run the demo
./bin/run_demo.sh

# Or use the main run script
./bin/run.sh
```

## Core Features

### 🔒 Security Guarantees
- **Hardware-level Protection**: Leverage Intel SGX hardware-level security guarantees
- **Memory Encryption**: Enclave memory is automatically encrypted, preventing memory dump attacks
- **Integrity Verification**: Ensure code and data integrity

### 🚀 Performance Optimization
- **Minimized Enclave Size**: Execute only critical security operations within the enclave
- **Efficient Data Transfer**: Optimize data exchange between application and enclave
- **Concurrent Processing**: Support multi-threaded concurrent verification

### 🔧 Usability
- **Simple API**: Provide easy-to-use smart contract verification interface
- **Detailed Documentation**: Complete usage instructions and API documentation
- **Example Code**: Include multiple real-world usage scenario examples

## Demo Use Cases

1. **Smart Contract Simulation**: Demonstrate basic smart contract execution in SGX enclave
2. **Proof Generation**: Show how to generate execution proofs (mock implementation)
3. **Enclave Attestation**: Demonstrate enclave measurement and attestation capabilities
4. **Secure Computation**: Basic example of secure computation in trusted environment

## API Documentation

### Demo Enclave Interface

```cpp
// Initialize contract verifier (demo version)
sgx_status_t ecall_init_contract_verifier();

// Execute smart contract (simulation)
sgx_status_t ecall_execute_contract(
    const uint8_t* contract_code,
    size_t code_size,
    const uint8_t* input_data,
    size_t input_size,
    uint64_t gas_limit,
    uint8_t* result,
    size_t* result_size,
    uint8_t* execution_hash
);

// Generate execution proof (mock implementation)
sgx_status_t ecall_generate_proof(
    uint8_t* data,
    size_t data_size,
    uint8_t* proof,
    size_t proof_size
);

// Get enclave measurement
sgx_status_t ecall_get_enclave_measurement(
    uint8_t* measurement
);
```

## Security Considerations

1. **Side-channel Attack Protection**: Implemented protection against timing attacks and power analysis
2. **Key Management**: Use SGX sealing functionality to securely store keys
3. **Remote Attestation**: Support Intel Attestation Service for remote attestation
4. **Rollback Attack Protection**: Implement monotonic counters to prevent rollback attacks

## Demo Features

This is a simplified demonstration version that includes:
- Basic smart contract execution simulation
- Simple proof generation (mock implementation)
- Enclave measurement retrieval
- Basic attestation report creation

## Contributing

1. Fork this project
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contact

- **Author**: Chord
- **Email**: chord244@gmail.com
- **GitHub**: https://github.com/chord233
- **Twitter**: @chord244
- **LinkedIn**: https://linkedin.com/in/chord233

## Acknowledgments

- Intel SGX development team
- Open source community contributors
- All users who provided testing and feedback

---

**Language Versions:**
- [English Version (Main)](README.md)
- [中文版本 (Chinese)](README_CN.md)