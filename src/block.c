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
#include <errno.h>
#include <string.h>
#include <inttypes.h>

#include <dogecoin/auxpow.h>
#include <dogecoin/mem.h>
#include <dogecoin/portable_endian.h>
#include <dogecoin/protocol.h>
#include <dogecoin/serialize.h>
#include <dogecoin/sha2.h>
#include <dogecoin/utils.h>
#include <dogecoin/validation.h>

dogecoin_bool check(void *ctx, uint256* hash, uint32_t chainid, dogecoin_chainparams* params) {
    dogecoin_auxpow_block* block = (dogecoin_auxpow_block*)ctx;

    if (block->parent_merkle_index != 0) {
        printf("Auxpow is not a generate\n");
        return false;
    }

    uint32_t parent_chainid = get_chainid(block->parent_header->version);
    if (params->strict_id && parent_chainid == chainid) {
        printf("Aux POW parent has our chain ID\n");
        return false;
    }

    vector* chain_merkle_branch = vector_new(block->aux_merkle_count, NULL);
    for (size_t p = 0; p < block->aux_merkle_count; p++) {
        vector_add(chain_merkle_branch, block->aux_merkle_branch[p]);
    }

    if (chain_merkle_branch->len > 30) {
        printf("Aux POW chain merkle branch too long\n");
        vector_free(chain_merkle_branch, true);
        return false;
    }

    // First call to check_merkle_branch for the auxiliary blockchain's merkle branch
    uint256* chain_merkle_root = check_merkle_branch(hash, chain_merkle_branch, block->aux_merkle_index);
    vector_free(chain_merkle_branch, true);

    // Convert the root hash to a human-readable format (hex)
    unsigned char vch_roothash[64]; // Make sure it's large enough to hold the hash
    memcpy(vch_roothash, hash_to_string((uint8_t*)chain_merkle_root), 64); // Copy the data
    dogecoin_free(chain_merkle_root); // Free the computed merkle root

    // Compute the Merkle root for the parent block
    vector* parent_merkle_branch = vector_new(block->parent_merkle_count, NULL);
    for (size_t p = 0; p < block->parent_merkle_count; p++) {
        vector_add(parent_merkle_branch, block->parent_coinbase_merkle[p]);
    }

    // Compute the hash of the parent block's coinbase transaction
    uint256 parent_coinbase_hash;
    dogecoin_tx_hash(block->parent_coinbase, parent_coinbase_hash);

    uint256* parent_merkle_root = check_merkle_branch(&parent_coinbase_hash, parent_merkle_branch, block->parent_merkle_index);
    vector_free(parent_merkle_branch, true);

    // Check that the computed Merkle root matches the parent block's Merkle root
    if (memcmp(parent_merkle_root, block->parent_header->merkle_root, sizeof(uint256)) != 0) {
        printf("Aux POW merkle root incorrect\n");
        dogecoin_free(parent_merkle_root);
        return false;
    }
    dogecoin_free(parent_merkle_root);

    dogecoin_tx_in *tx_in = vector_idx(block->parent_coinbase->vin, 0);
    size_t idx = 0, count = 0;
    for (; idx < tx_in->script_sig->len; idx++) {

        bool needle_found = true;
        size_t header_idx = 0;
        for (; header_idx < 4; header_idx++) {
            const char haystack_char = tx_in->script_sig->str[idx + header_idx];
            const char needle_character = pch_merged_mining_header[header_idx];

            if (haystack_char == needle_character) {
                continue;
            } else {
                needle_found = false;
                break;
            }
        }

        if (needle_found) {
            count++;
            if (strncmp((const char*)vch_roothash, utils_uint8_to_hex((uint8_t*)&tx_in->script_sig->str[idx + header_idx], 32), 32) != 0) {
                printf("vch_roothash is not after merge mining header!\n");
                return false;
            }

            uint32_t nSize;
            memcpy(&nSize, &tx_in->script_sig->str[idx + 4 + 32], 4);
            nSize = le32toh(nSize);
            const unsigned int merkleHeight = block->aux_merkle_count;
            if (nSize != (1u << merkleHeight)) {
                printf("Aux POW merkle branch size does not match parent coinbase\n");
                return false;
            }

            uint32_t nNonce;
            memcpy(&nNonce, &tx_in->script_sig->str[idx + 4 + 32 + 4], 4);
            nNonce = le32toh(nNonce);
            uint32_t expected_index = get_expected_index(nNonce, chainid, merkleHeight);
            if (block->aux_merkle_index != expected_index) {
                printf("Aux POW wrong index\n");
                return false;
            }
        }
    }

    if (count > (uint32_t)1) {
        printf("Multiple merged mining headers in coinbase\n");
        return false;
    }

    return true;
}

/**
 * @brief This function allocates a new dogecoin block header->
 *
 * @return A pointer to the new dogecoin block header object.
 */
dogecoin_block_header* dogecoin_block_header_new() {
    dogecoin_block_header* header;
    header = dogecoin_calloc(1, sizeof(*header));
    header->version = 0;
    dogecoin_mem_zero(&header->prev_block, DOGECOIN_HASH_LENGTH);
    dogecoin_mem_zero(&header->merkle_root, DOGECOIN_HASH_LENGTH);
    header->bits = 0;
    header->timestamp = 0;
    header->nonce = 0;
    header->auxpow->check = check;
    header->auxpow->ctx = header;
    header->auxpow->is = false;
    dogecoin_mem_zero(&header->chainwork, DOGECOIN_HASH_LENGTH);
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
    dogecoin_mem_zero(&block->parent_hash, DOGECOIN_HASH_LENGTH);
    block->parent_merkle_count = 0;
    block->parent_coinbase_merkle = NULL;
    block->parent_merkle_index = 0;
    block->aux_merkle_count = 0;
    block->aux_merkle_branch = NULL;
    block->aux_merkle_index = 0;
    block->parent_header = dogecoin_block_header_new();
    block->header->auxpow->ctx = block;
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
    header->version = 0;
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
    dogecoin_free(block->parent_coinbase_merkle);
    dogecoin_free(block->aux_merkle_branch);
    block->parent_merkle_count = 0;
    block->aux_merkle_count = 0;
    block->aux_merkle_index = 0;
    block->parent_merkle_index = 0;
    remove_all_hashes();
    dogecoin_block_header_free(block->parent_header);
    dogecoin_free(block);
    }

void print_transaction(dogecoin_tx* x) {
    // serialize tx & print raw hex:
    cstring* tx = cstr_new_sz(1024);
    dogecoin_tx_serialize(tx, x);
    char tx_hex[2048];
    utils_bin_to_hex((unsigned char *)tx->str, tx->len, tx_hex);
    printf("block->parent_coinbase (hex):                   %s\n", tx_hex); // uncomment to see raw hexadecimal transactions

    // begin deconstruction into objects:
    printf("block->parent_coinbase->version:                %d\n", x->version);

    // parse inputs:
    unsigned int i = 0;
    for (; i < x->vin->len; i++) {
        printf("block->parent_coinbase->tx_in->i:               %d\n", i);
        dogecoin_tx_in* tx_in = vector_idx(x->vin, i);
        printf("block->parent_coinbase->vin->prevout.n:         %d\n", tx_in->prevout.n);
        char* hex_utxo_txid = utils_uint8_to_hex(tx_in->prevout.hash, sizeof tx_in->prevout.hash);
        printf("block->parent_coinbase->tx_in->prevout.hash:    %s\n", hex_utxo_txid);
        char* script_sig = utils_uint8_to_hex((const uint8_t*)tx_in->script_sig->str, tx_in->script_sig->len);
        printf("block->parent_coinbase->tx_in->script_sig:      %s\n", script_sig);

        printf("block->parent_coinbase->tx_in->sequence:        %x\n", tx_in->sequence);
    }

    // parse outputs:
    i = 0;
    for (; i < x->vout->len; i++) {
        printf("block->parent_coinbase->tx_out->i:              %d\n", i);
        dogecoin_tx_out* tx_out = vector_idx(x->vout, i);
        printf("block->parent_coinbase->tx_out->script_pubkey:  %s\n", utils_uint8_to_hex((const uint8_t*)tx_out->script_pubkey->str, tx_out->script_pubkey->len));
        printf("block->parent_coinbase->tx_out->value:          %" PRId64 "\n", tx_out->value);
    }
    printf("block->parent_coinbase->locktime:               %d\n", x->locktime);
    cstr_free(tx, true);
}

void print_block_header(dogecoin_block_header* header) {
    printf("block->header->version:                         %i\n", header->version);
    printf("block->header->prev_block:                      %s\n", hash_to_string(header->prev_block));
    printf("block->header->merkle_root:                     %s\n", hash_to_string(header->merkle_root));
    printf("block->header->timestamp:                       %u\n", header->timestamp);
    printf("block->header->bits:                            %x\n", header->bits);
    printf("block->header->nonce:                           %x\n", header->nonce);
}

void print_parent_header(dogecoin_auxpow_block* block) {
    printf("block->parent_hash:                             %s\n", hash_to_string(block->parent_hash));
    printf("block->parent_merkle_count:                     %d\n", block->parent_merkle_count);
    size_t j = 0;
    for (; j < block->parent_merkle_count; j++) {
        printf("block->parent_coinbase_merkle[%zu]:               "
                "%s\n", j, hash_to_string((uint8_t*)block->parent_coinbase_merkle[j]));
    }
    printf("block->parent_merkle_index:                     %d\n", block->parent_merkle_index);
    printf("block->aux_merkle_count:                        %d\n", block->aux_merkle_count);
    j = 0;
    for (; j < block->aux_merkle_count; j++) {
        printf("block->aux_merkle_branch[%zu]:                    "
                "%s\n", j, hash_to_string((uint8_t*)block->aux_merkle_branch[j]));
    }
    printf("block->aux_merkle_index:                        %d\n", block->aux_merkle_index);
    printf("block->parent_header->version:                  %i\n", block->parent_header->version);
    printf("block->parent_header->prev_block:               %s\n", hash_to_string(block->parent_header->prev_block));
    printf("block->parent_header->merkle_root:              %s\n", hash_to_string(block->parent_header->merkle_root));
    printf("block->parent_header->timestamp:                %u\n", block->parent_header->timestamp);
    printf("block->parent_header->bits:                     %x\n", block->parent_header->bits);
    printf("block->parent_header->nonce:                    %u\n\n", block->parent_header->nonce);
}

void print_block(dogecoin_auxpow_block* block) {
    print_block_header(block->header);
    print_transaction(block->parent_coinbase);
    print_parent_header(block);
}

/**
 * @brief This function takes a raw buffer and deserializes
 * it into a dogecoin block header object += auxpow data.
 *
 * @param header The header object to be constructed.
 * @param buf The buffer to deserialize from.
 * @param params The chain parameters.
 *
 * @return 1 if deserialization was successful, 0 otherwise.
 */
int dogecoin_block_header_deserialize(dogecoin_block_header* header, struct const_buffer* buf, const dogecoin_chainparams *params) {
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
    if ((block->header->version & 0x100) != 0 && buf->len) {
        if (!deserialize_dogecoin_auxpow_block(block, buf, params)) {
            printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
            return false;
        }
        dogecoin_block_header_copy(header, block->header);
    }
    dogecoin_auxpow_block_free(block);
    return true;
    }

int deserialize_dogecoin_auxpow_block(dogecoin_auxpow_block* block, struct const_buffer* buffer, const dogecoin_chainparams *params) {
    if (buffer->len > DOGECOIN_MAX_P2P_MSG_SIZE) {
        return printf("\ntransaction is invalid or to large.\n\n");
        }

    size_t consumedlength = 0;
    if (!dogecoin_tx_deserialize(buffer->p, buffer->len, block->parent_coinbase, &consumedlength)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
        }

    if (consumedlength == 0) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }

    if (!deser_skip(buffer, consumedlength)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }

    block->header->auxpow->is = (block->header->version & 0x100) == 256;

    if (!deser_u256(block->parent_hash, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (!deser_varlen((uint32_t*)&block->parent_merkle_count, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    uint8_t i = 0;
    if (block->parent_merkle_count > 0) {
        block->parent_coinbase_merkle = dogecoin_calloc(block->parent_merkle_count, sizeof(uint256));
    }
    for (; i < block->parent_merkle_count; i++) {
        if (!deser_u256(block->parent_coinbase_merkle[i], buffer)) {
            printf("%d:%s:%d\n", __LINE__, __func__, i);
            return false;
        }
        }

    if (!deser_u32(&block->parent_merkle_index, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (!deser_varlen((uint32_t*)&block->aux_merkle_count, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (block->aux_merkle_count > 0) {
        block->aux_merkle_branch = dogecoin_calloc(block->aux_merkle_count, sizeof(uint256));
    }
    for (i = 0; i < block->aux_merkle_count; i++) {
        if (!deser_u256(block->aux_merkle_branch[i], buffer)) {
            printf("%d:%s:%d\n", __LINE__, __func__, i);
            return false;
        }
        }

    if (!deser_u32(&block->aux_merkle_index, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (!deser_s32(&block->parent_header->version, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (!deser_u256(block->parent_header->prev_block, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (!deser_u256(block->parent_header->merkle_root, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (!deser_u32(&block->parent_header->timestamp, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (!deser_u32(&block->parent_header->bits, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }
    if (!deser_u32(&block->parent_header->nonce, buffer)) {
        printf("%s:%d:%s:%s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }

    if (!check_auxpow(block, (dogecoin_chainparams*)params)) {
        printf("check_auxpow failed!\n");
        return false;
    }

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
    dest->auxpow->check = src->auxpow->check;
    dest->auxpow->ctx = src->auxpow->ctx;
    dest->auxpow->is = src->auxpow->is;
    memcpy_safe(&dest->chainwork, &src->chainwork, sizeof(src->chainwork));
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
