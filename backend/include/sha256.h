#ifndef SHA256_H
#define SHA256_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const void *data, size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t hash[32]);

/* Helper functions */
void sha256_hash_string(const char *str, char out_hex[65]);
void sha256_hash_password(const char *password, const char *salt, char out_hex[65]);
void sha256_generate_salt(char out_salt[33]);

#ifdef __cplusplus
}
#endif

#endif /* SHA256_H */
