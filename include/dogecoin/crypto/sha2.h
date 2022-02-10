/**
 * Copyright (c) 2000-2001 Aaron D. Gifford
 * Copyright (c) 2013 Pavol Rusnak
 * Copyright (c) 2022 bluezr
 * Copyright (c) 2022 The Dogecoin Foundation
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

#ifndef __LIBDOGECOIN_CRYPTO_SHA2_H__
#define __LIBDOGECOIN_CRYPTO_SHA2_H__

#include "../dogecoin.h"

#include <stdint.h>
#include <stddef.h>

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
LIBDOGECOIN_API void sha256_finalize(uint8_t[SHA256_DIGEST_LENGTH], sha256_context*);
LIBDOGECOIN_API void sha256_raw(const uint8_t*, size_t, uint8_t[SHA256_DIGEST_LENGTH]);

LIBDOGECOIN_API void sha512_init(sha512_context*);
LIBDOGECOIN_API void sha512_write(sha512_context*, const uint8_t*, size_t);
LIBDOGECOIN_API void sha512_finalize(uint8_t[SHA512_DIGEST_LENGTH], sha512_context*);
LIBDOGECOIN_API void sha512_raw(const uint8_t*, size_t, uint8_t[SHA512_DIGEST_LENGTH]);

LIBDOGECOIN_API void hmac_sha256(const uint8_t* key, const uint32_t keylen, const uint8_t* msg, const uint32_t msglen, uint8_t* hmac);
LIBDOGECOIN_API void hmac_sha512(const uint8_t* key, const uint32_t keylen, const uint8_t* msg, const uint32_t msglen, uint8_t* hmac);

LIBDOGECOIN_END_DECL

#endif /* __LIBDOGECOIN_CRYPTO_SHA2_H__ */
