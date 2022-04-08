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
#include <dogecoin/net/protocol.h>
#include <dogecoin/serialize.h>
#include <dogecoin/crypto/sha2.h>
#include <dogecoin/utils.h>

/**
 * It allocates a new dogecoin_block_header and returns it
 * 
 * @return A pointer to a new dogecoin_block_header object.
 */
dogecoin_block_header* dogecoin_block_header_new() {
    dogecoin_block_header* header;
    header = dogecoin_calloc(1, sizeof(*header));
    return header;
}

/**
 * It takes a pointer to a dogecoin_block_header and sets all the fields to zero
 * 
 * @param header the pointer to the block header to be freed.
 */
void dogecoin_block_header_free(dogecoin_block_header* header) {
    if (!header) return;
    header->version = 1;
    dogecoin_mem_zero(&header->prev_block, DOGECOIN_HASH_LENGTH);
    dogecoin_mem_zero(&header->merkle_root, DOGECOIN_HASH_LENGTH);
    header->bits = 0;
    header->timestamp = 0;
    header->nonce = 0;
    dogecoin_free(header);
}

/**
 * It deserializes a block header from a buffer
 * 
 * @param header the header object to be filled in
 * @param buf The buffer to deserialize from.
 * 
 * @return int
 */
int dogecoin_block_header_deserialize(dogecoin_block_header* header, struct const_buffer* buf) {
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

/**
 * It serializes a dogecoin block header
 * 
 * @param s The string to write to.
 * @param header the block header to serialize
 */
void dogecoin_block_header_serialize(cstring* s, const dogecoin_block_header* header) {
    ser_s32(s, header->version);
    ser_u256(s, header->prev_block);
    ser_u256(s, header->merkle_root);
    ser_u32(s, header->timestamp);
    ser_u32(s, header->bits);
    ser_u32(s, header->nonce);
}

/**
 * Copy the contents of a dogecoin_block_header struct to another.
 * 
 * @param dest The destination block header.
 * @param src The source block header.
 */
void dogecoin_block_header_copy(dogecoin_block_header* dest, const dogecoin_block_header* src) {
    dest->version = src->version;
    memcpy(&dest->prev_block, &src->prev_block, sizeof(src->prev_block));
    memcpy(&dest->merkle_root, &src->merkle_root, sizeof(src->merkle_root));
    dest->timestamp = src->timestamp;
    dest->bits = src->bits;
    dest->nonce = src->nonce;
}

/**
 * The function takes a block header and returns a hash of the block header
 * 
 * @param header The block header to hash.
 * @param hash The hash of the block header.
 * 
 * @return A boolean value.
 */
dogecoin_bool dogecoin_block_header_hash(dogecoin_block_header* header, uint256 hash) {
    cstring* s = cstr_new_sz(80);
    dogecoin_block_header_serialize(s, header);
    sha256_raw((const uint8_t*)s->str, s->len, hash);
    sha256_raw(hash, SHA256_DIGEST_LENGTH, hash);
    cstr_free(s, true);
    dogecoin_bool ret = true;
    return ret;
}
