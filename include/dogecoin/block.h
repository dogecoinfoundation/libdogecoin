/*

 The MIT License (MIT)

 Copyright (c) 2016 Thomas Kerin
 Copyright (c) 2016 libbtc developers
 Copyright (c) 2023 bluezr
 Copyright (c) 2023 edtubbs
 Copyright (c) 2023 The Dogecoin Foundation

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

#ifndef __LIBDOGECOIN_BLOCK_H__
#define __LIBDOGECOIN_BLOCK_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#include <dogecoin/buffer.h>
#include <dogecoin/cstr.h>
#include <dogecoin/hash.h>
#include <dogecoin/map.h>
#include <dogecoin/tx.h>

typedef struct _auxpow {
    dogecoin_bool is;
    dogecoin_bool (*check)(void* ctx, uint256* hash, uint32_t chainid, dogecoin_chainparams* params);
    void *ctx;
} auxpow;

typedef struct dogecoin_block_header_ {
    int32_t version;
    uint256 prev_block;
    uint256 merkle_root;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
    auxpow auxpow[1];
    uint256 chainwork;
} dogecoin_block_header;

typedef struct dogecoin_auxpow_block_ {
    dogecoin_block_header* header;
    dogecoin_tx* parent_coinbase;
    uint256 parent_hash;
    uint8_t parent_merkle_count;
    uint256* parent_coinbase_merkle;
    uint32_t parent_merkle_index;
    uint8_t aux_merkle_count;
    uint256* aux_merkle_branch;
    uint32_t aux_merkle_index;
    dogecoin_block_header* parent_header;
} dogecoin_auxpow_block;

LIBDOGECOIN_API dogecoin_block_header* dogecoin_block_header_new();
LIBDOGECOIN_API void dogecoin_block_header_free(dogecoin_block_header* header);
LIBDOGECOIN_API dogecoin_auxpow_block* dogecoin_auxpow_block_new();
LIBDOGECOIN_API void dogecoin_auxpow_block_free(dogecoin_auxpow_block* block);
LIBDOGECOIN_API int dogecoin_block_header_deserialize(dogecoin_block_header* header, struct const_buffer* buf, const dogecoin_chainparams *params);
LIBDOGECOIN_API int deserialize_dogecoin_auxpow_block(dogecoin_auxpow_block* block, struct const_buffer* buffer, const dogecoin_chainparams *params);
LIBDOGECOIN_API void dogecoin_block_header_serialize(cstring* s, const dogecoin_block_header* header);
LIBDOGECOIN_API void dogecoin_block_header_copy(dogecoin_block_header* dest, const dogecoin_block_header* src);
LIBDOGECOIN_API dogecoin_bool dogecoin_block_header_hash(dogecoin_block_header* header, uint256 hash);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_BLOCK_H__
