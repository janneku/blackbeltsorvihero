#include <stdint.h>
#include <string.h>

#define SHA1_BLOCK_SIZE  64
#define SHA1_DIGEST_SIZE 20
#define SHA1_HASH_SIZE   SHA1_DIGEST_SIZE

typedef struct sha1_ctx_t {
	uint32_t count[2];
	uint32_t hash[5];
	uint32_t wbuf[16];
} sha1_ctx_t;

void sha1_begin(sha1_ctx_t *ctx);
void sha1_hash(const void *data, size_t length, sha1_ctx_t *ctx);
void* sha1_end(void *resbuf, sha1_ctx_t *ctx);
