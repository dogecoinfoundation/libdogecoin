/*

 The MIT License (MIT)

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

#ifndef __LIBDOGECOIN_CRYPTO_HASH_H__
#define __LIBDOGECOIN_CRYPTO_HASH_H__

#include <dogecoin/crypto/sha2.h>
#include <dogecoin/cstr.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/mem.h>
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

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_CRYPTO_HASH_H__
