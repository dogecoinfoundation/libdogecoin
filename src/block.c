/*

 The MIT License (MIT)

 Copyright (c) 2016 Thomas Kerin
 Copyright (c) 2016 Jonas Schnelli
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>


#include <dogecoin/block.h>

#include <dogecoin/serialize.h>
#include <dogecoin/sha2.h>
#include <dogecoin/utils.h>

dogecoin_block_header* dogecoin_block_header_new()
{
    dogecoin_block_header* header;
    header = dogecoin_calloc(1, sizeof(*header));

    return header;
}

void dogecoin_block_header_free(dogecoin_block_header* header)
{
    if (!header)
        return;

    header->version = 1;
    memset(&header->prev_block, 0, DOGECOIN_HASH_LENGTH);
    memset(&header->merkle_root, 0, DOGECOIN_HASH_LENGTH);
    header->bits = 0;
    header->timestamp = 0;
    header->nonce = 0;
    dogecoin_free(header);
}

int dogecoin_block_header_deserialize(dogecoin_block_header* header, struct const_buffer* buf)
{
    if (!deser_s32(&header->version, buf))
        return false;
    if (!deser_u256(header->prev_block, buf))
        return false;
    if (!deser_u256(header->merkle_root, buf))
        return false;
    if (!deser_u32(&header->timestamp, buf))
        return false;
    if (!deser_u32(&header->bits, buf))
        return false;
    if (!deser_u32(&header->nonce, buf))
        return false;

    return true;
}

void dogecoin_block_header_serialize(cstring* s, const dogecoin_block_header* header)
{
    ser_s32(s, header->version);
    ser_u256(s, header->prev_block);
    ser_u256(s, header->merkle_root);
    ser_u32(s, header->timestamp);
    ser_u32(s, header->bits);
    ser_u32(s, header->nonce);
}

void dogecoin_block_header_copy(dogecoin_block_header* dest, const dogecoin_block_header* src)
{
    dest->version = src->version;
    memcpy(&dest->prev_block, &src->prev_block, sizeof(src->prev_block));
    memcpy(&dest->merkle_root, &src->merkle_root, sizeof(src->merkle_root));
    dest->timestamp = src->timestamp;
    dest->bits = src->bits;
    dest->nonce = src->nonce;
}

dogecoin_bool dogecoin_block_header_hash(dogecoin_block_header* header, uint256 hash)
{
    cstring* s = cstr_new_sz(80);
    dogecoin_block_header_serialize(s, header);

    sha256_Raw((const uint8_t*)s->str, s->len, hash);
    sha256_Raw(hash, SHA256_DIGEST_LENGTH, hash);
    cstr_free(s, true);

    dogecoin_bool ret = true;
    return ret;
}
