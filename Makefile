######## SGX SDK Settings ########

SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 1

ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_FLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_FLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
	SGX_COMMON_FLAGS += -O0 -g
else
	SGX_COMMON_FLAGS += -O2
endif

SGX_COMMON_FLAGS += -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type \
                    -Waddress -Wsequence-point -Wformat-security \
                    -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow \
                    -Wcast-align -Wcast-qual -Wconversion -Wredundant-decls

SGX_COMMON_CFLAGS := $(SGX_COMMON_FLAGS) -Wjump-misses-init -Wstrict-prototypes -Wunsuffixed-float-constants
SGX_COMMON_CXXFLAGS := $(SGX_COMMON_FLAGS) -Wnon-virtual-dtor -std=c++11

######## App Settings ########

ifneq ($(SGX_MODE), HW)
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

App_Cpp_Files := app/main.cpp app/sgx_utils.cpp
App_Include_Paths := -Iapp -I$(SGX_SDK)/include

App_C_Flags := -fPIC -Wno-attributes $(App_Include_Paths)

ifeq ($(SGX_DEBUG), 1)
	App_C_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	App_C_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
	App_C_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_Cpp_Flags := $(App_C_Flags)
App_Link_Flags := -L$(SGX_LIBRARY_PATH) -l$(Urts_Library_Name) -lpthread

ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif

App_Cpp_Objects := $(App_Cpp_Files:.cpp=.o)

App_Name := sgx_contract_verifier

######## Enclave Settings ########

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif

Crypto_Library_Name := sgx_tcrypto

Enclave_Cpp_Files := enclave/enclave.cpp enclave/contract_verifier.cpp enclave/crypto_utils.cpp
Enclave_Include_Paths := -Ienclave -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc -I$(SGX_SDK)/include/libcxx

Enclave_C_Flags := -nostdinc -fvisibility=hidden -fpie -ffunction-sections -fdata-sections $(Enclave_Include_Paths)
Enclave_Cpp_Flags := $(Enclave_C_Flags) -nostdinc++

# Enable the security flags
Enclave_Security_Link_Flags := -Wl,-z,relro,-z,now,-z,noexecstack

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts before other libraries.
#    2. Link sgx_tservice, sgx_tcrypto, sgx_tkey_exchange libraries.
#    3. Link other libraries if needed.
#    4. Link sgx_trts_sim last.
# Warning: Link order is important, wrong order will lead to link failure.
Enclave_Link_Flags := $(Enclave_Security_Link_Flags) \
    -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -l$(Crypto_Library_Name) -l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 -Wl,--gc-sections   \
	-Wl,--version-script=enclave/enclave.lds

Enclave_Cpp_Objects := $(addprefix enclave/, $(notdir $(Enclave_Cpp_Files:.cpp=.o)))

Enclave_Name := enclave.so
Signed_Enclave_Name := enclave.signed.so
Enclave_Config_File := enclave/enclave.config.xml

ifeq ($(SGX_MODE), HW)
ifeq ($(SGX_DEBUG), 1)
	Build_Mode = HW_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	Build_Mode = HW_PRERELEASE
else
	Build_Mode = HW_RELEASE
endif
else
ifeq ($(SGX_DEBUG), 1)
	Build_Mode = SIM_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
	Build_Mode = SIM_PRERELEASE
else
	Build_Mode = SIM_RELEASE
endif
endif

.PHONY: all target run
all: .config_$(Build_Mode)_$(SGX_ARCH)
	@echo "The project has been built in $(Build_Mode) mode."

target: $(App_Name) $(Signed_Enclave_Name)

run: all
	@echo "RUN  =>  $(App_Name) [$(SGX_MODE)|$(SGX_ARCH), OK]"
	@./$(App_Name)

######## App Objects ########

app/enclave_u.h: $(SGX_EDGER8R) enclave/enclave.edl
	@cd app && $(SGX_EDGER8R) --untrusted ../enclave/enclave.edl --search-path ../enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

app/enclave_u.c: app/enclave_u.h

app/enclave_u.o: app/enclave_u.c
	@$(CC) $(SGX_COMMON_CFLAGS) $(App_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

app/%.o: app/%.cpp app/enclave_u.h
	@$(CXX) $(SGX_COMMON_CXXFLAGS) $(App_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(App_Name): app/enclave_u.o $(App_Cpp_Objects)
	@$(CXX) $^ -o $@ $(App_Link_Flags)
	@echo "LINK =>  $@"

######## Enclave Objects ########

enclave/enclave_t.h: $(SGX_EDGER8R) enclave/enclave.edl
	@cd enclave && $(SGX_EDGER8R) --trusted ../enclave/enclave.edl --search-path ../enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

enclave/enclave_t.c: enclave/enclave_t.h

enclave/enclave_t.o: enclave/enclave_t.c
	@$(CC) $(SGX_COMMON_CFLAGS) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"

enclave/%.o: enclave/%.cpp enclave/enclave_t.h
	@$(CXX) $(SGX_COMMON_CXXFLAGS) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

$(Enclave_Name): enclave/enclave_t.o $(Enclave_Cpp_Objects)
	@$(CXX) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK =>  $@"

$(Signed_Enclave_Name): $(Enclave_Name)
	@$(SGX_ENCLAVE_SIGNER) sign -key enclave/enclave_private.pem -enclave $(Enclave_Name) -out $@ -config $(Enclave_Config_File)
	@echo "SIGN =>  $@"

.config_$(Build_Mode)_$(SGX_ARCH):
	@rm -f .config_*_$(SGX_ARCH)
	@touch .config_$(Build_Mode)_$(SGX_ARCH)

######## Clean ########
.PHONY: clean

clean:
	@rm -f .config_*
	@rm -f $(App_Name) $(Enclave_Name) $(Signed_Enclave_Name)
	@rm -f $(App_Cpp_Objects) app/enclave_u.* 
	@rm -f $(Enclave_Cpp_Objects) enclave/enclave_t.*
	@rm -f tests/test_runner tests/*.o

######## Tests ########
.PHONY: test

test: $(App_Name) $(Signed_Enclave_Name)
	@echo "Building tests..."
	@$(CXX) $(SGX_COMMON_CXXFLAGS) $(App_Cpp_Flags) -Itests tests/test_enclave.cpp tests/test_contracts.cpp app/enclave_u.o app/sgx_utils.cpp -o tests/test_runner $(App_Link_Flags)
	@echo "Running tests..."
	@./tests/test_runner

######## Help ########
.PHONY: help

help:
	@echo "Available targets:"
	@echo "  all      - Build the project"
	@echo "  target   - Build app and signed enclave"
	@echo "  run      - Build and run the application"
	@echo "  test     - Build and run tests"
	@echo "  clean    - Clean build artifacts"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Build modes:"
	@echo "  SGX_MODE=HW|SIM    - Hardware or simulation mode (default: HW)"
	@echo "  SGX_DEBUG=1|0      - Debug or release build (default: 1)"
	@echo "  SGX_ARCH=x64|x86   - Target architecture (default: x64)"