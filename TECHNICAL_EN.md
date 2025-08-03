# SGX Smart Contract Demo - Technical Documentation

## Overview

This project demonstrates the use of Intel SGX (Software Guard Extensions) to create a Trusted Execution Environment (TEE) for verifying smart contract execution. The demo showcases how SGX can be used to ensure the integrity and confidentiality of smart contract computations in a blockchain context.

## Architecture

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Host Application                         │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│  │   User Interface│  │   File I/O      │  │   Logging   │ │
│  └─────────────────┘  └─────────────────┘  └─────────────┘ │
│                           │                                 │
│                           ▼                                 │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                SGX Enclave                              │ │
│  │  ┌─────────────────┐  ┌─────────────────┐              │ │
│  │  │Contract Verifier│  │  Crypto Utils   │              │ │
│  │  └─────────────────┘  └─────────────────┘              │ │
│  │  ┌─────────────────┐  ┌─────────────────┐              │ │
│  │  │   VM Engine     │  │ Attestation     │              │ │
│  │  └─────────────────┘  └─────────────────┘              │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Component Details

#### 1. SGX Enclave
The secure execution environment that contains:
- **Contract Verifier**: Validates and executes smart contract bytecode
- **Crypto Utils**: Provides cryptographic functions (hashing, signing, encryption)
- **VM Engine**: Simple virtual machine for contract execution
- **Attestation**: Generates proofs and attestation reports

#### 2. Host Application
The untrusted application that:
- Manages user interaction
- Handles file I/O operations
- Provides logging and monitoring
- Interfaces with the enclave through ECALLs/OCALLs

## Smart Contract Virtual Machine

### Instruction Set

The demo implements a simple stack-based virtual machine with the following instructions:

| Opcode | Instruction | Description | Gas Cost |
|--------|-------------|-------------|----------|
| 0x00   | HALT        | Stop execution | 0 |
| 0x01   | PUSH        | Push value to stack | 3 |
| 0x02   | POP         | Pop value from stack | 2 |
| 0x03   | ADD         | Add two values | 3 |
| 0x04   | SUB         | Subtract two values | 3 |
| 0x05   | MUL         | Multiply two values | 5 |
| 0x06   | DIV         | Divide two values | 5 |
| 0x07   | MOD         | Modulo operation | 5 |
| 0x08   | AND         | Bitwise AND | 3 |
| 0x09   | OR          | Bitwise OR | 3 |
| 0x0A   | XOR         | Bitwise XOR | 3 |
| 0x0B   | NOT         | Bitwise NOT | 3 |
| 0x0C   | EQ          | Equality check | 3 |
| 0x0D   | LT          | Less than | 3 |
| 0x0E   | GT          | Greater than | 3 |
| 0x0F   | JMP         | Unconditional jump | 8 |
| 0x10   | JMPIF       | Conditional jump | 10 |
| 0x11   | CALL        | Function call | 40 |
| 0x12   | RET         | Return from function | 5 |
| 0x13   | LOAD        | Load from storage | 200 |
| 0x14   | STORE       | Store to storage | 5000 |
| 0x15   | HASH        | Compute hash | 30 |
| 0x16   | VERIFY      | Verify signature | 3000 |
| 0x17   | ECRECOVER   | Recover public key | 3000 |
| 0x18   | BALANCE     | Get balance | 400 |
| 0x19   | TRANSFER    | Transfer tokens | 9000 |

### Gas System

The VM implements a gas system similar to Ethereum:
- Each instruction consumes a specific amount of gas
- Execution stops when gas is exhausted
- Gas limits prevent infinite loops and DoS attacks

### Example Contract

```assembly
; Simple addition contract
PUSH 10     ; Push 10 to stack
PUSH 20     ; Push 20 to stack
ADD         ; Add top two values
HALT        ; Stop execution
```

Bytecode: `01 0A 01 14 02 00`

## Security Features

### 1. Memory Protection
- All contract execution happens within the SGX enclave
- Memory is protected from external access
- Secure heap and stack management

### 2. Cryptographic Verification
- Contract execution results are cryptographically signed
- Hash-based integrity verification
- Support for multiple signature algorithms (RSA, ECDSA)

### 3. Remote Attestation
- Generates SGX attestation reports
- Proves the integrity of the enclave
- Enables remote verification of execution environment

### 4. Data Sealing
- Sensitive data can be sealed to the enclave
- Data remains encrypted when stored outside the enclave
- Only the same enclave can unseal the data

## API Reference

### ECALLs (Enclave Calls)

#### `initialize_verifier()`
Initializes the contract verifier within the enclave.

**Returns:** Status code

#### `verify_contract_execution(contract_data, input_data, gas_limit)`
Executes and verifies a smart contract.

**Parameters:**
- `contract_data`: Contract bytecode
- `input_data`: Input parameters
- `gas_limit`: Maximum gas to consume

**Returns:** Execution result and gas used

#### `generate_execution_proof(execution_hash)`
Generates a cryptographic proof of contract execution.

**Parameters:**
- `execution_hash`: Hash of the execution context

**Returns:** Signed proof

#### `get_enclave_measurement()`
Retrieves the enclave measurement (MRENCLAVE).

**Returns:** 32-byte measurement hash

#### `create_attestation_report(user_data)`
Creates an SGX attestation report.

**Parameters:**
- `user_data`: Custom data to include in report

**Returns:** Attestation report

### OCALLs (Outside Calls)

#### `ocall_print_string(message)`
Prints a message to the console.

#### `ocall_read_storage(key)`
Reads data from external storage.

#### `ocall_write_storage(key, value)`
Writes data to external storage.

#### `ocall_network_request(url, data)`
Makes a network request (for attestation services).

## Performance Characteristics

### Benchmarks

Typical performance on a modern Intel processor with SGX support:

- **Enclave Creation**: ~50ms
- **Simple Contract Execution**: ~1ms
- **Proof Generation**: ~10ms
- **Attestation Report**: ~100ms
- **Throughput**: ~1000 contracts/second

### Optimization Strategies

1. **Batch Processing**: Execute multiple contracts in a single enclave call
2. **Memory Pool**: Pre-allocate memory to reduce allocation overhead
3. **Instruction Caching**: Cache frequently used instruction sequences
4. **Parallel Execution**: Use multiple enclaves for parallel processing

## Build System

### Dependencies

- Intel SGX SDK 2.15+
- GCC 7.5+
- OpenSSL 1.1.1+
- Make 4.1+

### Build Process

1. **EDL Generation**: Generate trusted/untrusted bridge code
2. **Enclave Compilation**: Compile enclave code with SGX-specific flags
3. **Enclave Signing**: Sign the enclave with a private key
4. **Host Compilation**: Compile the host application
5. **Linking**: Link all components together

### Build Targets

- `make all`: Build everything
- `make enclave`: Build only the enclave
- `make app`: Build only the host application
- `make clean`: Clean build artifacts
- `make test`: Run tests

## Testing Strategy

### Unit Tests
- Individual function testing
- Crypto utility validation
- VM instruction verification

### Integration Tests
- End-to-end contract execution
- Attestation workflow
- Error handling scenarios

### Performance Tests
- Throughput measurement
- Latency analysis
- Memory usage profiling

### Security Tests
- Side-channel attack resistance
- Memory safety verification
- Cryptographic correctness

## Deployment Considerations

### Hardware Requirements
- Intel processor with SGX support
- Sufficient EPC (Enclave Page Cache) memory
- BIOS/UEFI SGX enablement

### Software Requirements
- Linux kernel with SGX driver
- Intel SGX Platform Software (PSW)
- Attestation service connectivity (for production)

### Configuration
- Enclave heap/stack size tuning
- Gas limit configuration
- Logging level adjustment
- Network timeout settings

## Troubleshooting

### Common Issues

1. **SGX Not Available**
   - Check processor support
   - Verify BIOS settings
   - Install SGX driver

2. **Enclave Launch Failure**
   - Check enclave signature
   - Verify memory limits
   - Review debug output

3. **Performance Issues**
   - Monitor EPC usage
   - Check for memory swapping
   - Optimize contract code

### Debug Mode

Enable debug mode for detailed logging:
```bash
export SGX_MODE=SIM
export SGX_DEBUG=1
./build.sh
```

## Future Enhancements

### Planned Features
1. **Multi-threading Support**: Parallel contract execution
2. **Advanced Cryptography**: Post-quantum algorithms
3. **Network Integration**: Direct blockchain connectivity
4. **Formal Verification**: Mathematical proof of correctness
5. **Performance Optimization**: JIT compilation for contracts

### Research Areas
1. **Scalability**: Handling thousands of concurrent contracts
2. **Privacy**: Zero-knowledge proof integration
3. **Interoperability**: Cross-chain contract execution
4. **Governance**: Decentralized enclave management

## References

1. [Intel SGX Developer Guide](https://software.intel.com/content/www/us/en/develop/documentation/sgx-developer-guide/top.html)
2. [SGX SDK Documentation](https://github.com/intel/linux-sgx)
3. [Ethereum Virtual Machine](https://ethereum.github.io/yellowpaper/paper.pdf)
4. [TEE Security Analysis](https://arxiv.org/abs/1805.08724)

## License

This project is licensed under the MIT License. See LICENSE file for details.

## Contributing

Contributions are welcome! Please read the contributing guidelines and submit pull requests for any improvements.

## Contact

For questions or support, please contact the development team or create an issue in the project repository.

---

## Language Versions

- [中文版本 (Chinese)](TECHNICAL.md)
- [English Version](TECHNICAL_EN.md)