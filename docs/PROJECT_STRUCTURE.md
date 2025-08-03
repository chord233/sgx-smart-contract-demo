# Project Structure

This document describes the reorganized file structure of the SGX Smart Contract Demo project.

## Directory Layout

```
sgx-smart-contract-demo/
├── .gitignore                # Git ignore rules
├── Makefile                  # Build configuration
├── README.md                 # Main documentation (English)
├── README_CN.md              # Main documentation (Chinese)
├── app/                      # Application layer
│   ├── app.h                # Application header
│   ├── app_utils.cpp        # Application utilities
│   ├── app_utils.h          # Application utilities header
│   ├── main.cpp             # Main application entry
│   └── sgx_utils.cpp        # SGX utility functions
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
├── docs/                     # Documentation
│   ├── DEMO_VERSION.md      # Demo version information
│   ├── PROJECT_STRUCTURE.md # This file - project structure guide
│   └── QUICK_START.md       # Quick start guide
└── enclave/                  # SGX Enclave implementation
    ├── contract_verifier.cpp # Smart contract verifier
    ├── contract_verifier.h  # Contract verifier header
    ├── crypto_utils.cpp     # Cryptographic utility functions
    ├── crypto_utils.h       # Crypto utilities header
    ├── enclave.config.xml   # Enclave configuration
    ├── enclave.cpp          # Main enclave implementation
    ├── enclave.edl          # Enclave Definition Language file
    ├── enclave.h            # Enclave header
    └── enclave_private.pem  # Enclave private key
```

## Directory Descriptions

### `/app/`
Contains the untrusted application code that runs outside the SGX enclave. This includes:
- Main application entry point
- SGX utilities for enclave management
- Application-specific utility functions

### `/bin/`
Executable scripts and compiled binaries:
- `run.sh`: Main execution script
- `run_demo.sh`: Demo-specific execution script
- Generated binaries after compilation

### `/build/`
Build-related scripts and tools:
- `build.sh`: Custom build script with additional functionality
- Future build tools and configurations

### `/config/`
Configuration files for the project:
- `config.json`: Main project configuration
- Future configuration files for different environments

### `/contracts/`
Sample smart contracts and test data:
- Text-based contract examples
- Binary contract files
- Test data for contract execution

### `/docs/`
Project documentation:
- Version information
- Quick start guides
- Architecture documentation
- API references

### `/enclave/`
SGX Enclave trusted code:
- Core enclave implementation
- Smart contract verification logic
- Cryptographic utilities
- Enclave configuration and keys

## Changes from Previous Structure

### Consolidated Directories
- **Merged `data/` and `test_data/` into `contracts/`**: All contract-related files are now in one location
- **Renamed `scripts/` to `bin/`**: More standard naming for executable files

### New Directories
- **Added `build/`**: Centralized location for build scripts and tools
- **Added `config/`**: Dedicated location for configuration files
- **Added `docs/`**: Organized documentation in a separate directory

### Benefits
- **Clearer separation of concerns**: Each directory has a specific purpose
- **Better organization**: Related files are grouped together
- **Standard conventions**: Follows common project structure patterns
- **Easier navigation**: Logical hierarchy makes finding files easier
- **Scalability**: Structure supports future expansion

## Usage

After the reorganization:

```bash
# Build the project
make clean && make

# Run the demo
./bin/run_demo.sh

# Or use the main run script
./bin/run.sh

# View documentation
ls docs/

# Check configuration
cat config/config.json
```

This structure provides a clean, organized, and maintainable codebase that follows industry best practices.