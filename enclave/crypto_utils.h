#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <sgx_tcrypto.h>
#include <sgx_trts.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 密钥长度定义
#define AES_KEY_SIZE 16
#define RSA_KEY_SIZE 256
#define ECC_KEY_SIZE 32
#define HMAC_KEY_SIZE 32
#define SIGNATURE_SIZE 64
#define NONCE_SIZE 12

// 密钥类型
typedef enum {
    KEY_TYPE_AES,
    KEY_TYPE_RSA,
    KEY_TYPE_ECC,
    KEY_TYPE_HMAC
} key_type_t;

// 密钥对结构
typedef struct {
    uint8_t public_key[RSA_KEY_SIZE];
    uint8_t private_key[RSA_KEY_SIZE];
    key_type_t type;
    bool is_valid;
} key_pair_t;

// 加密上下文
typedef struct {
    uint8_t key[AES_KEY_SIZE];
    uint8_t iv[16];
    sgx_aes_gcm_128bit_tag_t tag;
    bool initialized;
} encryption_context_t;

// 签名上下文
typedef struct {
    key_pair_t key_pair;
    uint8_t signature[SIGNATURE_SIZE];
    bool signed;
} signature_context_t;

// HMAC上下文
typedef struct {
    uint8_t key[HMAC_KEY_SIZE];
    sgx_sha256_hash_t hash;
    bool computed;
} hmac_context_t;

/**
 * 生成随机密钥对
 * @param key_pair 输出的密钥对
 * @param type 密钥类型
 * @return SGX状态码
 */
sgx_status_t generate_key_pair(key_pair_t* key_pair, key_type_t type);

/**
 * 生成随机数
 * @param buffer 输出缓冲区
 * @param size 随机数大小
 * @return SGX状态码
 */
sgx_status_t generate_random(uint8_t* buffer, size_t size);

/**
 * AES-GCM加密
 * @param plaintext 明文数据
 * @param plaintext_len 明文长度
 * @param key AES密钥
 * @param iv 初始化向量
 * @param ciphertext 密文输出
 * @param ciphertext_len 密文长度
 * @param tag 认证标签
 * @return SGX状态码
 */
sgx_status_t aes_gcm_encrypt(
    const uint8_t* plaintext,
    size_t plaintext_len,
    const uint8_t* key,
    const uint8_t* iv,
    uint8_t* ciphertext,
    size_t* ciphertext_len,
    sgx_aes_gcm_128bit_tag_t* tag
);

/**
 * AES-GCM解密
 * @param ciphertext 密文数据
 * @param ciphertext_len 密文长度
 * @param key AES密钥
 * @param iv 初始化向量
 * @param tag 认证标签
 * @param plaintext 明文输出
 * @param plaintext_len 明文长度
 * @return SGX状态码
 */
sgx_status_t aes_gcm_decrypt(
    const uint8_t* ciphertext,
    size_t ciphertext_len,
    const uint8_t* key,
    const uint8_t* iv,
    const sgx_aes_gcm_128bit_tag_t* tag,
    uint8_t* plaintext,
    size_t* plaintext_len
);

/**
 * RSA签名
 * @param data 待签名数据
 * @param data_len 数据长度
 * @param private_key RSA私钥
 * @param signature 签名输出
 * @param signature_len 签名长度
 * @return SGX状态码
 */
sgx_status_t rsa_sign(
    const uint8_t* data,
    size_t data_len,
    const uint8_t* private_key,
    uint8_t* signature,
    size_t* signature_len
);

/**
 * RSA验签
 * @param data 原始数据
 * @param data_len 数据长度
 * @param signature 签名
 * @param signature_len 签名长度
 * @param public_key RSA公钥
 * @param result 验证结果
 * @return SGX状态码
 */
sgx_status_t rsa_verify(
    const uint8_t* data,
    size_t data_len,
    const uint8_t* signature,
    size_t signature_len,
    const uint8_t* public_key,
    bool* result
);

/**
 * ECDSA签名
 * @param data 待签名数据
 * @param data_len 数据长度
 * @param private_key ECC私钥
 * @param signature 签名输出
 * @return SGX状态码
 */
sgx_status_t ecdsa_sign(
    const uint8_t* data,
    size_t data_len,
    const sgx_ec256_private_t* private_key,
    sgx_ec256_signature_t* signature
);

/**
 * ECDSA验签
 * @param data 原始数据
 * @param data_len 数据长度
 * @param signature 签名
 * @param public_key ECC公钥
 * @param result 验证结果
 * @return SGX状态码
 */
sgx_status_t ecdsa_verify(
    const uint8_t* data,
    size_t data_len,
    const sgx_ec256_signature_t* signature,
    const sgx_ec256_public_t* public_key,
    bool* result
);

/**
 * 计算HMAC-SHA256
 * @param data 输入数据
 * @param data_len 数据长度
 * @param key HMAC密钥
 * @param key_len 密钥长度
 * @param hmac 输出HMAC
 * @return SGX状态码
 */
sgx_status_t compute_hmac_sha256(
    const uint8_t* data,
    size_t data_len,
    const uint8_t* key,
    size_t key_len,
    sgx_sha256_hash_t* hmac
);

/**
 * 计算SHA256哈希
 * @param data 输入数据
 * @param data_len 数据长度
 * @param hash 输出哈希
 * @return SGX状态码
 */
sgx_status_t compute_sha256(
    const uint8_t* data,
    size_t data_len,
    sgx_sha256_hash_t* hash
);

/**
 * 计算SHA3-256哈希
 * @param data 输入数据
 * @param data_len 数据长度
 * @param hash 输出哈希
 * @return SGX状态码
 */
sgx_status_t compute_sha3_256(
    const uint8_t* data,
    size_t data_len,
    uint8_t* hash
);

/**
 * 密钥派生函数 (PBKDF2)
 * @param password 密码
 * @param password_len 密码长度
 * @param salt 盐值
 * @param salt_len 盐值长度
 * @param iterations 迭代次数
 * @param key_len 输出密钥长度
 * @param derived_key 派生密钥
 * @return SGX状态码
 */
sgx_status_t derive_key_pbkdf2(
    const uint8_t* password,
    size_t password_len,
    const uint8_t* salt,
    size_t salt_len,
    uint32_t iterations,
    size_t key_len,
    uint8_t* derived_key
);

/**
 * 安全内存比较
 * @param a 内存块A
 * @param b 内存块B
 * @param size 比较大小
 * @return 比较结果 (0表示相等)
 */
int secure_memcmp(const void* a, const void* b, size_t size);

/**
 * 安全内存清零
 * @param ptr 内存指针
 * @param size 清零大小
 */
void secure_memzero(void* ptr, size_t size);

/**
 * 安全内存拷贝
 * @param dest 目标内存
 * @param src 源内存
 * @param size 拷贝大小
 * @return SGX状态码
 */
sgx_status_t secure_memcpy(void* dest, const void* src, size_t size);

/**
 * 初始化加密上下文
 * @param ctx 加密上下文
 * @return SGX状态码
 */
sgx_status_t init_encryption_context(encryption_context_t* ctx);

/**
 * 清理加密上下文
 * @param ctx 加密上下文
 */
void cleanup_encryption_context(encryption_context_t* ctx);

/**
 * 初始化签名上下文
 * @param ctx 签名上下文
 * @param key_type 密钥类型
 * @return SGX状态码
 */
sgx_status_t init_signature_context(signature_context_t* ctx, key_type_t key_type);

/**
 * 清理签名上下文
 * @param ctx 签名上下文
 */
void cleanup_signature_context(signature_context_t* ctx);

/**
 * 验证密钥对有效性
 * @param key_pair 密钥对
 * @return 是否有效
 */
bool validate_key_pair(const key_pair_t* key_pair);

/**
 * 导出公钥
 * @param key_pair 密钥对
 * @param public_key_out 输出公钥
 * @param key_len 公钥长度
 * @return SGX状态码
 */
sgx_status_t export_public_key(
    const key_pair_t* key_pair,
    uint8_t* public_key_out,
    size_t* key_len
);

/**
 * 计算数据指纹
 * @param data 输入数据
 * @param data_len 数据长度
 * @param fingerprint 输出指纹
 * @return SGX状态码
 */
sgx_status_t compute_data_fingerprint(
    const uint8_t* data,
    size_t data_len,
    sgx_sha256_hash_t* fingerprint
);

#ifdef __cplusplus
}
#endif

#endif // CRYPTO_UTILS_H