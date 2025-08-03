#include "crypto_utils.h"
#include "enclave_t.h"

#include <sgx_tcrypto.h>
#include <sgx_trts.h>
#include <string.h>
#include <stdlib.h>

/**
 * 生成随机密钥对
 */
sgx_status_t generate_key_pair(key_pair_t* key_pair, key_type_t type) {
    if (!key_pair) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memset(key_pair, 0, sizeof(key_pair_t));
    key_pair->type = type;
    
    sgx_status_t ret = SGX_SUCCESS;
    
    switch (type) {
        case KEY_TYPE_RSA: {
            // 简化实现：生成随机RSA密钥
            ret = sgx_read_rand(key_pair->private_key, RSA_KEY_SIZE);
            if (ret != SGX_SUCCESS) break;
            
            ret = sgx_read_rand(key_pair->public_key, RSA_KEY_SIZE);
            if (ret != SGX_SUCCESS) break;
            
            key_pair->is_valid = true;
            break;
        }
        
        case KEY_TYPE_ECC: {
            // 生成ECC密钥对
            sgx_ec256_private_t private_key;
            sgx_ec256_public_t public_key;
            
            ret = sgx_ecc256_create_key_pair(&private_key, &public_key, sgx_ecc256_open_context());
            if (ret != SGX_SUCCESS) break;
            
            memcpy(key_pair->private_key, &private_key, sizeof(private_key));
            memcpy(key_pair->public_key, &public_key, sizeof(public_key));
            key_pair->is_valid = true;
            break;
        }
        
        case KEY_TYPE_AES:
        case KEY_TYPE_HMAC: {
            // 生成对称密钥
            size_t key_size = (type == KEY_TYPE_AES) ? AES_KEY_SIZE : HMAC_KEY_SIZE;
            ret = sgx_read_rand(key_pair->private_key, key_size);
            if (ret != SGX_SUCCESS) break;
            
            // 对称密钥的公钥和私钥相同
            memcpy(key_pair->public_key, key_pair->private_key, key_size);
            key_pair->is_valid = true;
            break;
        }
        
        default:
            return SGX_ERROR_INVALID_PARAMETER;
    }
    
    return ret;
}

/**
 * 生成随机数
 */
sgx_status_t generate_random(uint8_t* buffer, size_t size) {
    if (!buffer || size == 0) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    return sgx_read_rand(buffer, size);
}

/**
 * AES-GCM加密
 */
sgx_status_t aes_gcm_encrypt(
    const uint8_t* plaintext,
    size_t plaintext_len,
    const uint8_t* key,
    const uint8_t* iv,
    uint8_t* ciphertext,
    size_t* ciphertext_len,
    sgx_aes_gcm_128bit_tag_t* tag
) {
    if (!plaintext || !key || !iv || !ciphertext || !ciphertext_len || !tag) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    if (*ciphertext_len < plaintext_len) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    sgx_status_t ret = sgx_rijndael128GCM_encrypt(
        (const sgx_aes_gcm_128bit_key_t*)key,
        plaintext,
        (uint32_t)plaintext_len,
        ciphertext,
        iv,
        12, // IV长度
        NULL, // AAD
        0,    // AAD长度
        tag
    );
    
    if (ret == SGX_SUCCESS) {
        *ciphertext_len = plaintext_len;
    }
    
    return ret;
}

/**
 * AES-GCM解密
 */
sgx_status_t aes_gcm_decrypt(
    const uint8_t* ciphertext,
    size_t ciphertext_len,
    const uint8_t* key,
    const uint8_t* iv,
    const sgx_aes_gcm_128bit_tag_t* tag,
    uint8_t* plaintext,
    size_t* plaintext_len
) {
    if (!ciphertext || !key || !iv || !tag || !plaintext || !plaintext_len) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    if (*plaintext_len < ciphertext_len) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    sgx_status_t ret = sgx_rijndael128GCM_decrypt(
        (const sgx_aes_gcm_128bit_key_t*)key,
        ciphertext,
        (uint32_t)ciphertext_len,
        plaintext,
        iv,
        12, // IV长度
        NULL, // AAD
        0,    // AAD长度
        tag
    );
    
    if (ret == SGX_SUCCESS) {
        *plaintext_len = ciphertext_len;
    }
    
    return ret;
}

/**
 * RSA签名 (简化实现)
 */
sgx_status_t rsa_sign(
    const uint8_t* data,
    size_t data_len,
    const uint8_t* private_key,
    uint8_t* signature,
    size_t* signature_len
) {
    if (!data || !private_key || !signature || !signature_len) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 简化实现：使用HMAC作为签名
    sgx_sha256_hash_t hash;
    sgx_status_t ret = compute_hmac_sha256(data, data_len, private_key, RSA_KEY_SIZE, &hash);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    if (*signature_len < sizeof(hash)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memcpy(signature, &hash, sizeof(hash));
    *signature_len = sizeof(hash);
    
    return SGX_SUCCESS;
}

/**
 * RSA验签 (简化实现)
 */
sgx_status_t rsa_verify(
    const uint8_t* data,
    size_t data_len,
    const uint8_t* signature,
    size_t signature_len,
    const uint8_t* public_key,
    bool* result
) {
    if (!data || !signature || !public_key || !result) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 简化实现：重新计算HMAC并比较
    sgx_sha256_hash_t expected_hash;
    sgx_status_t ret = compute_hmac_sha256(data, data_len, public_key, RSA_KEY_SIZE, &expected_hash);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    if (signature_len != sizeof(expected_hash)) {
        *result = false;
        return SGX_SUCCESS;
    }
    
    *result = (secure_memcmp(signature, &expected_hash, sizeof(expected_hash)) == 0);
    
    return SGX_SUCCESS;
}

/**
 * ECDSA签名
 */
sgx_status_t ecdsa_sign(
    const uint8_t* data,
    size_t data_len,
    const sgx_ec256_private_t* private_key,
    sgx_ec256_signature_t* signature
) {
    if (!data || !private_key || !signature) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 计算数据哈希
    sgx_sha256_hash_t hash;
    sgx_status_t ret = sgx_sha256_msg(data, (uint32_t)data_len, &hash);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 创建ECC上下文
    sgx_ecc_state_handle_t ecc_handle;
    ret = sgx_ecc256_open_context(&ecc_handle);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 执行ECDSA签名
    ret = sgx_ecdsa_sign((uint8_t*)&hash, sizeof(hash), private_key, signature, ecc_handle);
    
    sgx_ecc256_close_context(ecc_handle);
    
    return ret;
}

/**
 * ECDSA验签
 */
sgx_status_t ecdsa_verify(
    const uint8_t* data,
    size_t data_len,
    const sgx_ec256_signature_t* signature,
    const sgx_ec256_public_t* public_key,
    bool* result
) {
    if (!data || !signature || !public_key || !result) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 计算数据哈希
    sgx_sha256_hash_t hash;
    sgx_status_t ret = sgx_sha256_msg(data, (uint32_t)data_len, &hash);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 创建ECC上下文
    sgx_ecc_state_handle_t ecc_handle;
    ret = sgx_ecc256_open_context(&ecc_handle);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 执行ECDSA验签
    uint8_t verify_result;
    ret = sgx_ecdsa_verify((uint8_t*)&hash, sizeof(hash), public_key, signature, &verify_result, ecc_handle);
    
    sgx_ecc256_close_context(ecc_handle);
    
    if (ret == SGX_SUCCESS) {
        *result = (verify_result == SGX_EC_VALID);
    }
    
    return ret;
}

/**
 * 计算HMAC-SHA256
 */
sgx_status_t compute_hmac_sha256(
    const uint8_t* data,
    size_t data_len,
    const uint8_t* key,
    size_t key_len,
    sgx_sha256_hash_t* hmac
) {
    if (!data || !key || !hmac) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    return sgx_hmac_sha256_msg(data, (uint32_t)data_len, key, (uint32_t)key_len, hmac);
}

/**
 * 计算SHA256哈希
 */
sgx_status_t compute_sha256(
    const uint8_t* data,
    size_t data_len,
    sgx_sha256_hash_t* hash
) {
    if (!data || !hash) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    return sgx_sha256_msg(data, (uint32_t)data_len, hash);
}

/**
 * 计算SHA3-256哈希 (简化实现，使用SHA256代替)
 */
sgx_status_t compute_sha3_256(
    const uint8_t* data,
    size_t data_len,
    uint8_t* hash
) {
    if (!data || !hash) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 简化实现：使用SHA256代替SHA3
    return sgx_sha256_msg(data, (uint32_t)data_len, (sgx_sha256_hash_t*)hash);
}

/**
 * 密钥派生函数 (简化的PBKDF2实现)
 */
sgx_status_t derive_key_pbkdf2(
    const uint8_t* password,
    size_t password_len,
    const uint8_t* salt,
    size_t salt_len,
    uint32_t iterations,
    size_t key_len,
    uint8_t* derived_key
) {
    if (!password || !salt || !derived_key || key_len == 0) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 简化实现：使用HMAC-SHA256进行密钥派生
    sgx_sha256_hash_t temp_key;
    sgx_status_t ret = compute_hmac_sha256(salt, salt_len, password, password_len, &temp_key);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 多次迭代
    for (uint32_t i = 1; i < iterations; i++) {
        ret = compute_hmac_sha256((uint8_t*)&temp_key, sizeof(temp_key), password, password_len, &temp_key);
        if (ret != SGX_SUCCESS) {
            return ret;
        }
    }
    
    // 截取所需长度
    size_t copy_len = (key_len < sizeof(temp_key)) ? key_len : sizeof(temp_key);
    memcpy(derived_key, &temp_key, copy_len);
    
    // 如果需要更多字节，重复过程
    if (key_len > sizeof(temp_key)) {
        size_t remaining = key_len - sizeof(temp_key);
        uint8_t* next_block = derived_key + sizeof(temp_key);
        
        // 简化：用第一个块填充剩余部分
        while (remaining > 0) {
            size_t block_size = (remaining < sizeof(temp_key)) ? remaining : sizeof(temp_key);
            memcpy(next_block, &temp_key, block_size);
            next_block += block_size;
            remaining -= block_size;
        }
    }
    
    // 清理临时密钥
    secure_memzero(&temp_key, sizeof(temp_key));
    
    return SGX_SUCCESS;
}

/**
 * 安全内存比较
 */
int secure_memcmp(const void* a, const void* b, size_t size) {
    if (!a || !b) {
        return -1;
    }
    
    const uint8_t* pa = (const uint8_t*)a;
    const uint8_t* pb = (const uint8_t*)b;
    int result = 0;
    
    // 常时间比较，防止时序攻击
    for (size_t i = 0; i < size; i++) {
        result |= (pa[i] ^ pb[i]);
    }
    
    return result;
}

/**
 * 安全内存清零
 */
void secure_memzero(void* ptr, size_t size) {
    if (!ptr || size == 0) {
        return;
    }
    
    volatile uint8_t* p = (volatile uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

/**
 * 安全内存拷贝
 */
sgx_status_t secure_memcpy(void* dest, const void* src, size_t size) {
    if (!dest || !src || size == 0) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    // 检查内存重叠
    if (((uint8_t*)dest >= (uint8_t*)src && (uint8_t*)dest < (uint8_t*)src + size) ||
        ((uint8_t*)src >= (uint8_t*)dest && (uint8_t*)src < (uint8_t*)dest + size)) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memcpy(dest, src, size);
    return SGX_SUCCESS;
}

/**
 * 初始化加密上下文
 */
sgx_status_t init_encryption_context(encryption_context_t* ctx) {
    if (!ctx) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memset(ctx, 0, sizeof(encryption_context_t));
    
    // 生成随机AES密钥
    sgx_status_t ret = sgx_read_rand(ctx->key, AES_KEY_SIZE);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    // 生成随机IV
    ret = sgx_read_rand(ctx->iv, sizeof(ctx->iv));
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    ctx->initialized = true;
    
    return SGX_SUCCESS;
}

/**
 * 清理加密上下文
 */
void cleanup_encryption_context(encryption_context_t* ctx) {
    if (ctx) {
        secure_memzero(ctx, sizeof(encryption_context_t));
    }
}

/**
 * 初始化签名上下文
 */
sgx_status_t init_signature_context(signature_context_t* ctx, key_type_t key_type) {
    if (!ctx) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memset(ctx, 0, sizeof(signature_context_t));
    
    sgx_status_t ret = generate_key_pair(&ctx->key_pair, key_type);
    if (ret != SGX_SUCCESS) {
        return ret;
    }
    
    return SGX_SUCCESS;
}

/**
 * 清理签名上下文
 */
void cleanup_signature_context(signature_context_t* ctx) {
    if (ctx) {
        secure_memzero(ctx, sizeof(signature_context_t));
    }
}

/**
 * 验证密钥对有效性
 */
bool validate_key_pair(const key_pair_t* key_pair) {
    if (!key_pair) {
        return false;
    }
    
    return key_pair->is_valid;
}

/**
 * 导出公钥
 */
sgx_status_t export_public_key(
    const key_pair_t* key_pair,
    uint8_t* public_key_out,
    size_t* key_len
) {
    if (!key_pair || !public_key_out || !key_len || !key_pair->is_valid) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    size_t required_len;
    
    switch (key_pair->type) {
        case KEY_TYPE_RSA:
            required_len = RSA_KEY_SIZE;
            break;
        case KEY_TYPE_ECC:
            required_len = ECC_KEY_SIZE;
            break;
        case KEY_TYPE_AES:
            required_len = AES_KEY_SIZE;
            break;
        case KEY_TYPE_HMAC:
            required_len = HMAC_KEY_SIZE;
            break;
        default:
            return SGX_ERROR_INVALID_PARAMETER;
    }
    
    if (*key_len < required_len) {
        *key_len = required_len;
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    memcpy(public_key_out, key_pair->public_key, required_len);
    *key_len = required_len;
    
    return SGX_SUCCESS;
}

/**
 * 计算数据指纹
 */
sgx_status_t compute_data_fingerprint(
    const uint8_t* data,
    size_t data_len,
    sgx_sha256_hash_t* fingerprint
) {
    if (!data || !fingerprint) {
        return SGX_ERROR_INVALID_PARAMETER;
    }
    
    return compute_sha256(data, data_len, fingerprint);
}