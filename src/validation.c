/*

 The MIT License (MIT)

 Copyright (c) 2009-2010 Satoshi Nakamoto
 Copyright (c) 2009-2016 The Bitcoin Core developers
 Copyright (c) 2022 The Dogecoin Core developers
 Copyright (c) 2023 bluezr
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

#include <dogecoin/validation.h>

/**
 * @brief This function takes a block header and generates its
 * Scrypt hash.
 *
 * @param header The pointer to the block header to hash.
 * @param hash The Scrypt hash of the block header
 *
 * @return True.
 */
dogecoin_bool dogecoin_block_header_scrypt_hash(cstring* s, uint256* hash) {
    char scratchpad[SCRYPT_SCRATCHPAD_SIZE];
    unsigned char* inputbytes = (unsigned char*)utils_uint8_to_hex((uint8_t*)s->str, s->len);
    scrypt_1024_1_1_256_sp_generic((const char*)&inputbytes[0], hash, scratchpad);
    return true;
    }

uint32_t get_chainid(uint32_t version) {
    return version >> 16;
}

dogecoin_bool is_auxpow(uint32_t version) {
    return (version & (1 << 8)) == 256;
}

dogecoin_bool is_legacy(uint32_t version) {
    return version == 1
        // Dogecoin: We have a random v2 block with no AuxPoW, treat as legacy
        || (version == 2 && get_chainid(version) == 0);
}

dogecoin_bool check_auxpow(dogecoin_auxpow_block block, dogecoin_chainparams* params) {
    /* Except for legacy blocks with full version 1, ensure that
       the chain ID is correct.  Legacy blocks are not allowed since
       the merge-mining start, which is checked in AcceptBlockHeader
       where the height is known.  */
    if (!is_legacy(block.header->version) && params->strict_id && get_chainid(block.header->version) != params->auxpow_id) {
        printf("%s:%d:%s : block does not have our chain ID"
                " (got %d, expected %d, full nVersion %d) : %s\n",
                __FILE__, __LINE__, __func__, get_chainid(block.header->version),
                params->auxpow_id, block.header->version, strerror(errno));
        return false;
    }

    /* If there is no auxpow, just check the block hash.  */
    if (!block.header->auxpow->is) {
        if (is_auxpow(block.header->version)) {
            printf("%s:%d:%s : no auxpow on block with auxpow version : %s\n", __FILE__, __LINE__, __func__, strerror(errno));
            return false;
        }

        uint256 auxpow_hash;
        dogecoin_get_auxpow_hash(block.header->version, auxpow_hash);
        if (!check_pow(&auxpow_hash, block.header->bits, params)) {
            printf("%s:%d:%s : non-AUX proof of work failed : %s\n", __FILE__, __LINE__, __func__, strerror(errno));
            return false;
        }

        return true;
    }

    /* We have auxpow.  Check it.  */

    if (!is_auxpow(block.header->version)) {
        printf("%s:%d:%s : auxpow on block with non-auxpow version : %s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }

    uint256 block_header_hash;
    dogecoin_block_header_hash(block.header, block_header_hash);
    uint32_t chainid = get_chainid(block.header->version);
    if (!block.header->auxpow->check(&block, &block_header_hash, chainid, params)) {
        printf("%s:%d:%s : AUX POW is not valid : %s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }

    uint256 parent_hash;
    cstring* s = cstr_new_sz(80);
    dogecoin_block_header_serialize(s, block.parent_header);
    dogecoin_block_header_scrypt_hash(s, &parent_hash);
    cstr_free(s, true);
    swap_bytes((uint8_t*)&parent_hash, DOGECOIN_HASH_LENGTH);
    if (!check_pow(&parent_hash, block.header->bits, params)) {
        printf("%s:%d:%s : check_pow failure : %s\n", __FILE__, __LINE__, __func__, strerror(errno));
        return false;
    }

    return true;
}
