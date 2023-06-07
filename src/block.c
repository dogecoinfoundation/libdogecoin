/*

 The MIT License (MIT)

 Copyright (c) 2016 Thomas Kerin
 Copyright (c) 2016 Jonas Schnelli
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

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <dogecoin/block.h>
#include <dogecoin/protocol.h>
#include <dogecoin/serialize.h>
#include <dogecoin/sha2.h>
#include <dogecoin/utils.h>

#define BLOCK_VERSION_AUXPOW_BIT 0x100

/**
 * @brief This function allocates a new dogecoin block header->
 *
 * @return A pointer to the new dogecoin block header object.
 */
dogecoin_block_header* dogecoin_block_header_new() {
    dogecoin_block_header* header;
    header = dogecoin_calloc(1, sizeof(*header));
    return header;
    }

/**
 * It allocates a new dogecoin_auxpow_block and returns it
 *
 * @return A pointer to a new dogecoin_auxpow_block object.
 */
dogecoin_auxpow_block* dogecoin_auxpow_block_new() {
    dogecoin_auxpow_block* block = dogecoin_calloc(1, sizeof(*block));
    block->header = dogecoin_block_header_new();
    block->parent_coinbase = dogecoin_tx_new();
    block->parent_header = dogecoin_block_header_new();
    return block;
    }

/**
 * @brief This function sets the memory for the specified block
 * header to zero and then frees the memory.
 *
 * @param header The pointer to the block header to be freed.
 *
 * @return Nothing.
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

void dogecoin_auxpow_block_free(dogecoin_auxpow_block* block) {
    if (!block) return;
    dogecoin_block_header_free(block->header);
    dogecoin_tx_free(block->parent_coinbase);
    block->parent_merkle_count = 0;
    block->aux_merkle_count = 0;
    remove_all_hashes();
    dogecoin_block_header_free(block->parent_header);
    dogecoin_free(block);
    }

/**
 * @brief This function takes a raw buffer and deserializes
 * it into a dogecoin block header object += auxpow data.
 *
 * @param header The header object to be constructed.
 * @param buf The buffer to deserialize from.
 *
 * @return 1 if deserialization was successful, 0 otherwise.
 */
int dogecoin_block_header_deserialize(dogecoin_block_header* header, struct const_buffer* buf) {
    dogecoin_auxpow_block* block = dogecoin_auxpow_block_new();
    if (!deser_s32(&block->header->version, buf))
        return false;
    if (!deser_u256(block->header->prev_block, buf))
        return false;
    if (!deser_u256(block->header->merkle_root, buf))
        return false;
    if (!deser_u32(&block->header->timestamp, buf))
        return false;
    if (!deser_u32(&block->header->bits, buf))
        return false;
    if (!deser_u32(&block->header->nonce, buf))
        return false;
    dogecoin_block_header_copy(header, block->header);
    if ((block->header->version & BLOCK_VERSION_AUXPOW_BIT) != 0) {
        deserialize_dogecoin_auxpow_block(block, buf);
        }
    dogecoin_auxpow_block_free(block);
    return true;
    }

int deserialize_dogecoin_auxpow_block(dogecoin_auxpow_block* block, struct const_buffer* buffer) {
    if (buffer->len > DOGECOIN_MAX_P2P_MSG_SIZE) {
        return printf("\ntransaction is invalid or to large.\n\n");
        }

    size_t consumedlength = 0;
    if (!dogecoin_tx_deserialize(buffer->p, buffer->len, block->parent_coinbase, &consumedlength)) {
        return false;
        }

    if (consumedlength == 0) return false;

    if (!deser_skip(buffer, consumedlength)) return false;

    if (!deser_u256(block->parent_hash, buffer)) return false;

    if (!deser_varlen((uint32_t*)&block->parent_merkle_count, buffer)) return false;

    uint8_t i = 0;
    for (; i < block->parent_merkle_count; i++) {
        hash* parent_cb_merkle_branch = new_hash();
        if (!deser_u256((uint8_t*)parent_cb_merkle_branch->data.u8, buffer)) {
            return false;
        }
        dogecoin_free(parent_cb_merkle_branch);
        }

    if (!deser_u32(&block->parent_merkle_index, buffer)) return false;

    if (!deser_varlen((uint32_t*)&block->aux_merkle_count, buffer)) return false;

    for (i = 0; i < block->aux_merkle_count; i++) {
        hash* aux_merkle_branch = new_hash();
        if (!deser_u256((uint8_t*)aux_merkle_branch->data.u8, buffer)) {
            return false;
        }
        dogecoin_free(aux_merkle_branch);
        }

    if (!deser_u32(&block->aux_merkle_index, buffer)) return false;

    if (!deser_s32(&block->parent_header->version, buffer)) return false;
    if (!deser_u256(block->parent_header->prev_block, buffer)) return false;
    if (!deser_u256(block->parent_header->merkle_root, buffer)) return false;
    if (!deser_u32(&block->parent_header->timestamp, buffer)) return false;
    if (!deser_u32(&block->parent_header->bits, buffer)) return false;
    if (!deser_u32(&block->parent_header->nonce, buffer)) return false;

    return true;
    }

/**
 * @brief This function serializes a dogecoin block header into
 * a cstring object.
 *
 * @param s The cstring to write the serialized header->
 * @param header The block header to be serialized.
 *
 * @return Nothing.
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
 * @brief This function copies the contents of one header object
 * into another.
 *
 * @param dest The pointer to the header object copy.
 * @param src The pointer to the source block header object.
 *
 * @return Nothing.
 */
void dogecoin_block_header_copy(dogecoin_block_header* dest, const dogecoin_block_header* src) {
    dest->version = src->version;
    memcpy_safe(&dest->prev_block, &src->prev_block, sizeof(src->prev_block));
    memcpy_safe(&dest->merkle_root, &src->merkle_root, sizeof(src->merkle_root));
    dest->timestamp = src->timestamp;
    dest->bits = src->bits;
    dest->nonce = src->nonce;
    }

/**
 * @brief This function takes a block header and generates its
 * SHA256 hash.
 *
 * @param header The pointer to the block header to hash.
 * @param hash The SHA256 hash of the block header->
 *
 * @return True.
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
