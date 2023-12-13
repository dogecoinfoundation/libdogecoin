/*

 The MIT License (MIT)

 Copyright (c) 2009-2010 Satoshi Nakamoto
 Copyright (c) 2009-2016 The Bitcoin Core developers
 Copyright (c) 2015 Jonas Schnelli
 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the "Software"),
 to deal in the Software without restriction, including without limitation
 the rights to use, copy, modify, merge, publish, distribute, sublicense,
 and/or sell copies of the Software, and to permit persons to whom the
 Software is furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included
 in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef __LIBDOGECOIN_HASH_H__
#define __LIBDOGECOIN_HASH_H__

#include <dogecoin/cstr.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/mem.h>
#include <dogecoin/scrypt.h>
#include <dogecoin/sha2.h>
#include <dogecoin/utils.h>
#include <dogecoin/vector.h>

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API static inline dogecoin_bool dogecoin_hash_is_empty(uint256 hash)
{
    return hash[0] == 0 && !memcmp(hash, hash + 1, 19);
}

LIBDOGECOIN_API static inline void dogecoin_hash_clear(uint256 hash)
{
    dogecoin_mem_zero(hash, DOGECOIN_HASH_LENGTH);
}

LIBDOGECOIN_API static inline dogecoin_bool dogecoin_hash_equal(uint256 hash_a, uint256 hash_b)
{
    return (memcmp(hash_a, hash_b, DOGECOIN_HASH_LENGTH) == 0);
}

LIBDOGECOIN_API static inline void dogecoin_hash_set(uint256 hash_dest, const uint256 hash_src)
{
    memcpy_safe(hash_dest, hash_src, DOGECOIN_HASH_LENGTH);
}

LIBDOGECOIN_API static inline void dogecoin_hash(const unsigned char* datain, size_t length, uint256 hashout)
{
    sha256_raw(datain, length, hashout);
    sha256_raw(hashout, SHA256_DIGEST_LENGTH, hashout); // dogecoin double sha256 hash
}

LIBDOGECOIN_API static inline dogecoin_bool dogecoin_dblhash(const unsigned char* datain, size_t length, uint256 hashout)
{
    sha256_raw(datain, length, hashout);
    sha256_raw(hashout, SHA256_DIGEST_LENGTH, hashout); // dogecoin double sha256 hash
    return true;
}

LIBDOGECOIN_API static inline void dogecoin_hash_sngl_sha256(const unsigned char* datain, size_t length, uint256 hashout)
{
    sha256_raw(datain, length, hashout); // single sha256 hash
}

LIBDOGECOIN_API static inline void dogecoin_get_auxpow_hash(const uint32_t version, uint256 hashout)
{
    scrypt_1024_1_1_256(BEGIN(version), BEGIN(hashout));
}

DISABLE_WARNING_PUSH
DISABLE_WARNING(-Wunused-function)
DISABLE_WARNING(-Wunused-variable)
typedef uint256 chain_code;

typedef struct _chash256 {
    sha256_context* sha;
    void (*finalize)(sha256_context* ctx, unsigned char hash[SHA256_DIGEST_LENGTH]);
    void (*write)(sha256_context* ctx, const uint8_t* data, size_t len);
    void (*reset)();
} chash256;

static inline chash256* dogecoin_chash256_init() {
    chash256* chash = dogecoin_calloc(1, sizeof(*chash));
    sha256_context* ctx = dogecoin_calloc(1, sizeof(*ctx));
    sha256_init(ctx);
    chash->sha = ctx;
    chash->write = sha256_write;
    chash->finalize = sha256_finalize;
    chash->reset = sha256_reset;
    return chash;
}

// Hashes the data from two uint256 values and returns the double SHA-256 hash.
static inline uint256* Hash(const uint256* p1, const uint256* p2) {
    uint256* result = dogecoin_uint256_vla(1);
    chash256* chash = dogecoin_chash256_init();

    // Write the first uint256 to the hash context
    if (p1) {
        chash->write(chash->sha, (const uint8_t*)p1, sizeof(uint256));
    }

    // Write the second uint256 to the hash context
    if (p2) {
        chash->write(chash->sha, (const uint8_t*)p2, sizeof(uint256));
    }

    // Finalize and reset for double hashing
    chash->finalize(chash->sha, (unsigned char*)result);
    chash->reset(chash->sha);
    chash->write(chash->sha, (const uint8_t*)result, SHA256_DIGEST_LENGTH);
    chash->finalize(chash->sha, (unsigned char*)result);

    // Cleanup
    free(chash->sha);
    free(chash);

    return result;
}

/** Hashwriter Psuedoclass */
static enum ser_type {
    // primary actions
    SER_NETWORK         = (1 << 0),
    SER_DISK            = (1 << 1),
    SER_GETHASH         = (1 << 2),
} ser_type;

typedef struct hashwriter {
    chash256* ctx;
    int n_type;
    int n_version;
    cstring* cstr;
    uint256* hash;
    int (*get_type)(struct hashwriter* hw);
    int (*get_version)(struct hashwriter* hw);
    void (*write_hash)(struct hashwriter* hw, const char* pch, size_t size);
    uint256* (*get_hash)(struct hashwriter* hw);
    void (*ser)(cstring* cstr, const void* obj);
} hashwriter;

static int get_type(struct hashwriter* hw) {
    return hw->n_type;
}

static int get_version(struct hashwriter* hw) {
    return hw->n_version;
}

static void write_hash(struct hashwriter* hw, const char* pch, size_t size) {
    hw->ctx->write(hw->ctx->sha, (const unsigned char*)pch, size);
}

static uint256* get_hash(struct hashwriter* hw) {
    dogecoin_dblhash((const unsigned char*)hw->cstr->str, hw->cstr->len, *hw->hash);
    cstr_free(hw->cstr, true);
    return hw->hash;
}

static hashwriter* init_hashwriter(int n_type, int n_version) {
    hashwriter* hw = dogecoin_calloc(1, sizeof(*hw));
    chash256* chash = dogecoin_chash256_init();
    hw->ctx = chash;
    hw->n_type = n_type;
    hw->n_version = n_version;
    hw->cstr = cstr_new_sz(1024);
    hw->hash = dogecoin_uint256_vla(1);
    hw->get_type = get_type;
    hw->get_version = get_version;
    hw->write_hash = write_hash;
    hw->get_hash = get_hash;
    return hw;
}

static void dogecoin_hashwriter_free(hashwriter* hw) {
    free(hw->ctx->sha);
    free(hw->ctx);
    dogecoin_free(hw->hash);
    dogecoin_free(hw);
}

/** SipHash 2-4 */
typedef struct siphasher {
    uint64_t v[4];
    uint64_t tmp;
    int count;
    void (*write)(struct siphasher* sh, uint64_t data);
    void (*hash)(struct siphasher* sh, const unsigned char* data, size_t size);
    uint64_t (*finalize)(struct siphasher* sh);
} siphasher;

#define ROTL64(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define SIPROUND do { \
    v0 += v1; v1 = ROTL64(v1, 13); v1 ^= v0; \
    v0 = ROTL64(v0, 32); \
    v2 += v3; v3 = ROTL64(v3, 16); v3 ^= v2; \
    v0 += v3; v3 = ROTL64(v3, 21); v3 ^= v0; \
    v2 += v1; v1 = ROTL64(v1, 17); v1 ^= v2; \
    v2 = ROTL64(v2, 32); \
} while (0)

static void siphasher_set(struct siphasher* sh, uint64_t k0, uint64_t k1)
{
    sh->v[0] = 0x736f6d6570736575ULL ^ k0;
    sh->v[1] = 0x646f72616e646f6dULL ^ k1;
    sh->v[2] = 0x6c7967656e657261ULL ^ k0;
    sh->v[3] = 0x7465646279746573ULL ^ k1;
    sh->count = 0;
    sh->tmp = 0;
}

static void siphasher_write(struct siphasher* sh, uint64_t data)
{
    uint64_t v0 = sh->v[0], v1 = sh->v[1], v2 = sh->v[2], v3 = sh->v[3];

    assert(sh->count % 8 == 0);

    v3 ^= data;
    SIPROUND;
    SIPROUND;
    v0 ^= data;

    sh->v[0] = v0;
    sh->v[1] = v1;
    sh->v[2] = v2;
    sh->v[3] = v3;

    sh->count += 8;
}

static void siphasher_hash(struct siphasher* sh, const unsigned char* data, size_t size) {
    uint64_t v0 = sh->v[0], v1 = sh->v[1], v2 = sh->v[2], v3 = sh->v[3];
    uint64_t t = sh->tmp;
    int c = sh->count;

    while (size--) {
        t |= ((uint64_t)(*(data++))) << (8 * (c % 8));
        c++;
        if ((c & 7) == 0) {
            v3 ^= t;
            SIPROUND;
            SIPROUND;
            v0 ^= t;
            t = 0;
        }
    }

    sh->v[0] = v0;
    sh->v[1] = v1;
    sh->v[2] = v2;
    sh->v[3] = v3;
    sh->count = c;
    sh->tmp = t;
}

static uint64_t siphasher_finalize(struct siphasher* sh) {
    uint64_t v0 = sh->v[0], v1 = sh->v[1], v2 = sh->v[2], v3 = sh->v[3];

    uint64_t t = sh->tmp | (((uint64_t)sh->count) << 56);

    v3 ^= t;
    SIPROUND;
    SIPROUND;
    v0 ^= t;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}

typedef union u256 {
    uint256 data;
} u256;

static uint64_t get_uint64(uint256* data, int pos) {
    const uint8_t* ptr = (const uint8_t*)data + pos * 8;
    return ((uint64_t)ptr[0]) | \
            ((uint64_t)ptr[1]) << 8 | \
            ((uint64_t)ptr[2]) << 16 | \
            ((uint64_t)ptr[3]) << 24 | \
            ((uint64_t)ptr[4]) << 32 | \
            ((uint64_t)ptr[5]) << 40 | \
            ((uint64_t)ptr[6]) << 48 | \
            ((uint64_t)ptr[7]) << 56;
}

static inline union u256 init_u256(uint256* val) {
    union u256* u256 = dogecoin_calloc(1, sizeof(*u256));
    memcpy_safe(u256->data, dogecoin_uint256_vla(1), 32);
    if (val != NULL) memcpy(u256->data, val, 32);
    return *u256;
}

static uint64_t siphash_u256(uint64_t k0, uint64_t k1, uint256* val) {
    /* Specialized implementation for efficiency */
    uint64_t d = get_uint64(val, 0);
    uint64_t v0 = 0x736f6d6570736575ULL ^ k0;
    uint64_t v1 = 0x646f72616e646f6dULL ^ k1;
    uint64_t v2 = 0x6c7967656e657261ULL ^ k0;
    uint64_t v3 = 0x7465646279746573ULL ^ k1 ^ d;

    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = get_uint64(val, 1);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = get_uint64(val, 2);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    d = get_uint64(val, 3);
    v3 ^= d;
    SIPROUND;
    SIPROUND;
    v0 ^= d;
    v3 ^= ((uint64_t)4) << 59;
    SIPROUND;
    SIPROUND;
    v0 ^= ((uint64_t)4) << 59;
    v2 ^= 0xFF;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    return v0 ^ v1 ^ v2 ^ v3;
}
DISABLE_WARNING_POP

static inline struct siphasher* init_siphasher() {
    struct siphasher* sh = dogecoin_calloc(1, sizeof(*sh));
    sh->write = siphasher_write;
    sh->hash = siphasher_hash;
    sh->finalize = siphasher_finalize;
    return sh;
}

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_HASH_H__
