/**
 * Copyright (c) 2000-2001 Aaron D. Gifford
 * Copyright (c) 2013-2014 Tomas Dzetkulic
 * Copyright (c) 2013-2014 Pavol Rusnak
 * Copyright (c) 2022 bluezr
 * Copyright (c) 2022-2024 The Dogecoin Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTOR(S) ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTOR(S) BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __LIBDOGECOIN_SHA2_H__
#define __LIBDOGECOIN_SHA2_H__

#include <sys/types.h>
#include <stddef.h>
#include <stdint.h>

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#define SHA256_BLOCK_LENGTH 64
#define SHA256_DIGEST_LENGTH 32
#define SHA256_DIGEST_STRING_LENGTH (SHA256_DIGEST_LENGTH * 2 + 1)
#define SHA512_BLOCK_LENGTH 128
#define SHA512_DIGEST_LENGTH 64
#define SHA512_DIGEST_STRING_LENGTH (SHA512_DIGEST_LENGTH * 2 + 1)

typedef struct _sha256_context {
    uint32_t state[8];
    uint64_t bitcount;
    uint8_t buffer[SHA256_BLOCK_LENGTH];
} sha256_context;
typedef struct _sha512_context {
    uint64_t state[8];
    uint64_t bitcount[2];
    uint8_t buffer[SHA512_BLOCK_LENGTH];
} sha512_context;

LIBDOGECOIN_API void sha256_init(sha256_context*);
LIBDOGECOIN_API void sha256_write(sha256_context*, const uint8_t*, size_t);
LIBDOGECOIN_API void sha256_finalize(sha256_context*, uint8_t[SHA256_DIGEST_LENGTH]);
LIBDOGECOIN_API void sha256_raw(const uint8_t*, size_t, uint8_t[SHA256_DIGEST_LENGTH]);
LIBDOGECOIN_API void sha256_reset(sha256_context*);

LIBDOGECOIN_API void sha512_init(sha512_context*);
LIBDOGECOIN_API void sha512_write(sha512_context*, const uint8_t*, size_t);
LIBDOGECOIN_API void sha512_finalize(sha512_context*, uint8_t[SHA512_DIGEST_LENGTH]);
LIBDOGECOIN_API void sha512_raw(const uint8_t*, size_t, uint8_t[SHA512_DIGEST_LENGTH]);

typedef struct _hmac_sha256_context {
	uint8_t o_key_pad[SHA256_BLOCK_LENGTH];
	sha256_context ctx;
} hmac_sha256_context;

typedef struct _hmac_sha512_context {
	uint8_t o_key_pad[SHA512_BLOCK_LENGTH];
	sha512_context ctx;
} hmac_sha512_context;

void hmac_sha256_prepare(const uint8_t *key, const uint32_t keylen,
                         uint32_t *opad_digest, uint32_t *ipad_digest);
LIBDOGECOIN_API void hmac_sha256_init(hmac_sha256_context* hctx, const uint8_t* key, const uint32_t keylen);
LIBDOGECOIN_API void hmac_sha256_write(hmac_sha256_context* hctx, const uint8_t* msg, const uint32_t msglen);
LIBDOGECOIN_API void hmac_sha256_finalize(hmac_sha256_context* hctx, uint8_t* hmac);
LIBDOGECOIN_API void hmac_sha256(const uint8_t* key, const size_t keylen, const uint8_t* msg, const size_t msglen, uint8_t* hmac);

LIBDOGECOIN_API void hmac_sha512_init(hmac_sha512_context* hctx, const uint8_t* key, const uint32_t keylen);
LIBDOGECOIN_API void hmac_sha512_write(hmac_sha512_context* hctx, const uint8_t* msg, const uint32_t msglen);
LIBDOGECOIN_API void hmac_sha512_finalize(hmac_sha512_context* hctx, uint8_t* hmac);
LIBDOGECOIN_API void hmac_sha512(const uint8_t* key, const size_t keylen, const uint8_t* msg, const size_t msglen, uint8_t* hmac);

typedef struct _pbkdf2_hmac_sha256_context {
	uint32_t f[SHA256_DIGEST_LENGTH / sizeof(uint32_t)];
	uint32_t g[SHA256_BLOCK_LENGTH / sizeof(uint32_t)];
	const uint8_t *pass;
	int passlen;
	char first;
	uint32_t odig[SHA256_DIGEST_LENGTH / sizeof(uint32_t)];
	uint32_t idig[SHA256_DIGEST_LENGTH / sizeof(uint32_t)];
} pbkdf2_hmac_sha256_context;

typedef struct _pbkdf2_hmac_sha512_context {
	uint8_t f[SHA512_DIGEST_LENGTH];
	uint8_t g[SHA512_DIGEST_LENGTH];
	const uint8_t *pass;
	int passlen;
	char first;
} pbkdf2_hmac_sha512_context;

LIBDOGECOIN_API void pbkdf2_hmac_sha256_init(pbkdf2_hmac_sha256_context *pctx, const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen, uint32_t blocknr);
LIBDOGECOIN_API void pbkdf2_hmac_sha256_write(pbkdf2_hmac_sha256_context *pctx, uint32_t iterations);
LIBDOGECOIN_API void pbkdf2_hmac_sha256_finalize(pbkdf2_hmac_sha256_context *pctx, uint8_t *key);
LIBDOGECOIN_API void pbkdf2_hmac_sha256(const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen, uint32_t iterations, uint8_t *key, int keylen);

LIBDOGECOIN_API void pbkdf2_hmac_sha512_init(pbkdf2_hmac_sha512_context *pctx, const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen);
LIBDOGECOIN_API void pbkdf2_hmac_sha512_write(pbkdf2_hmac_sha512_context *pctx, uint32_t iterations);
LIBDOGECOIN_API void pbkdf2_hmac_sha512_finalize(pbkdf2_hmac_sha512_context *pctx, uint8_t *key);
LIBDOGECOIN_API void pbkdf2_hmac_sha512(const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen, uint32_t iterations, uint8_t *key);

LIBDOGECOIN_END_DECL

#endif /* __LIBDOGECOIN_SHA2_H__ */
