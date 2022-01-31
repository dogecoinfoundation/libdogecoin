/*

 The MIT License (MIT)

 Copyright (c) 2016 Thomas Kerin
 Copyright (c) 2016 libbtc developers
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

#ifndef LIBDOGECOIN_BLOCK_H
#define LIBDOGECOIN_BLOCK_H


#include "dogecoin.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>
#include <dogecoin/hash.h>
#include <dogecoin/cstr.h>

typedef struct dogecoin_block_header_ {
    int32_t version;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
} dogecoin_block_header;

LIBDOGECOIN_API dogecoin_block_header* dogecoin_block_header_new();
LIBDOGECOIN_API void dogecoin_block_header_free(dogecoin_block_header* header);
LIBDOGECOIN_API int dogecoin_block_header_deserialize(const unsigned char* header_serialized, size_t headerlen, dogecoin_block_header* header);
LIBDOGECOIN_API void dogecoin_block_header_serialize(cstring* s, const dogecoin_block_header* header);
LIBDOGECOIN_API void dogecoin_block_header_copy(dogecoin_block_header* dest, const dogecoin_block_header* src);
LIBDOGECOIN_API dogecoin_bool dogecoin_block_header_hash(dogecoin_block_header*header, uint8_t* hash);

#ifdef __cplusplus
}
#endif

#endif //__LIBDOGECOIN_BLOCK_H__
