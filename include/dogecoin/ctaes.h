 /*********************************************************************
 * Copyright (c) 2016 Pieter Wuille                                   *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef __LIBDOGECOIN_CTAES_H__
#define __LIBDOGECOIN_CTAES_H__

#include "dogecoin.h"

LIBDOGECOIN_BEGIN_DECL

typedef struct {
    uint16_t slice[8];
} AES_state;

typedef struct {
    AES_state rk[11];
} AES128_ctx;

typedef struct {
    AES_state rk[13];
} AES192_ctx;

typedef struct {
    AES_state rk[15];
} AES256_ctx;

LIBDOGECOIN_API void AES128_init(AES128_ctx* ctx, const unsigned char* key16);
LIBDOGECOIN_API void AES128_encrypt(const AES128_ctx* ctx, size_t blocks, unsigned char* cipher16, const unsigned char* plain16);
LIBDOGECOIN_API void AES128_decrypt(const AES128_ctx* ctx, size_t blocks, unsigned char* plain16, const unsigned char* cipher16);

LIBDOGECOIN_API void AES192_init(AES192_ctx* ctx, const unsigned char* key24);
LIBDOGECOIN_API void AES192_encrypt(const AES192_ctx* ctx, size_t blocks, unsigned char* cipher16, const unsigned char* plain16);
LIBDOGECOIN_API void AES192_decrypt(const AES192_ctx* ctx, size_t blocks, unsigned char* plain16, const unsigned char* cipher16);

LIBDOGECOIN_API void AES256_init(AES256_ctx* ctx, const unsigned char* key32);
LIBDOGECOIN_API void AES256_encrypt(const AES256_ctx* ctx, size_t blocks, unsigned char* cipher16, const unsigned char* plain16);
LIBDOGECOIN_API void AES256_decrypt(const AES256_ctx* ctx, size_t blocks, unsigned char* plain16, const unsigned char* cipher16);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_CTAES_H__
