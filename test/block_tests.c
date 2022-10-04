/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dogecoin/block.h>

#include <dogecoin/cstr.h>
#include <dogecoin/key.h>
#include <dogecoin/mem.h>
#include <dogecoin/utils.h>

#include "utest.h"

struct blockheadertest {
    char hexheader[160];
    char hexhash[64];
    int32_t version;
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;
};

static const struct blockheadertest block_header_tests[] =
        {
                {"010000000000000000000000000000000000000000000000000000000000000000000000696ad20e2dd4365c7459b4a4a5af743d5e92c6da3229e6532cd605f6533f2a5b24a6a152f0ff0f1e67860100", "1a91e3dace36e2be3bf030a65679fe821aa1d6ef92e7c9902eb318182c355691", 1, 1386325540, 504365040, 99943}, // genesis hash
                {"020162000d6f03470d329026cd1fc720c0609cd378ca8691a117bd1aa46f01fb09b1a8468a15bf6f0b0e83f2e5036684169eafb9406468d4f075c999fb5b2a78fbb827ee41fb11548441361b00000000", "60323982f9c5ff1b5a954eac9dc1269352835f47c2c5222691d80f0d50dcf053", 6422786, 1410464577, 456540548, 0}, // 331337
                {"020162002107cd08bec145c55ba8ffcbb4a9c0e836dfca383aa6ca1b380259a670aeb56fe5ea77d4f004afc5a0d31af1b89d5ebd9fd60cd7da7f4dcd96b0db1096a5bb1a7afb115488632e1b00000000","aff80f7b4dc8c667ebf4c76a6a62f9c4479844a37421ca2bf5abb485f4579fb6", 6422786, 1410464634, 456024968, 0}, // 331339
                {"03016200c96fd9d1b98330440082bcc1e58a39fe5a522f42defc501bff9b68f7b67ed99e1144e430166c54e9b911d8e059c03d0f972e7ab971c51f5505ff0bb21fee6fb1d88a9d5be132051a00000000", "c91f5a44a752c7549c1c689af5aeb42639582011d887282f976d550477abe25a", 6422787, 1537051352, 436548321, 0}, // 2391337
                {"0401620057bd4aa5170622b624bff774a087ea879a288226925c7cd5f3ead6ca4b6146e227b0e3699361bf58440971cfb28e16d9bab909769668ef3aac26220c6c0dc5fbda52595f9a97031a00000000", "8d7e4e91b571025ca109f2a0aeaf114ecc6aa2feec7f8bf23d405ac026c65d5e", 6422788, 1599689434, 436443034, 0}, // 3391337
                // end mainnet blocks
                {"020162002770a8b89647bbb542f044754a07dc6e56545793f5dcecdf43826ae0cb7192a12466d048e51b0f8a3cbaaf8a624b9aa1212ce4c2a4feba0750f7ad14feb75f54c69de053837b091e00000000", "8afc65a42c47b5ed5862194fb846171ba4afb999a1b4cce149f56c328d8a90e4", 6422786, 1407229382, 503937923, 0} // 158391
        };

void test_block_header()
{
    size_t outlen;
    char hexbuf[161];
    unsigned int i;
    for (i = 0; i < (sizeof(block_header_tests) / sizeof(block_header_tests[0])); i++) {
        cstring* serialized = cstr_new_sz(80);
        const struct blockheadertest* test = &block_header_tests[i];
        uint8_t header_data[80];
        uint256 hash_data;
        utils_hex_to_bin(test->hexheader, header_data, 160, &outlen);

        utils_hex_to_bin(test->hexhash, hash_data, sizeof(hash_data), &outlen);

        dogecoin_block_header* header = dogecoin_block_header_new();
        struct const_buffer buf = {header_data, 80};
        dogecoin_block_header_deserialize(header, &buf);

        // Check the copies are the same
        dogecoin_block_header* header_copy = dogecoin_block_header_new();
        dogecoin_block_header_copy(header_copy, header);
        assert(memcmp(header_copy, header, sizeof(*header_copy)) == 0);

        // Check the serialized form matches
        dogecoin_block_header_serialize(serialized, header);
        utils_bin_to_hex((unsigned char*) serialized->str, serialized->len, hexbuf);
        
        assert(memcmp(hexbuf, test->hexheader, 160) == 0);

        // Check the block hash
        uint256 blockhash;
        dogecoin_block_header_hash(header, blockhash);

        utils_bin_to_hex(blockhash, DOGECOIN_HASH_LENGTH, hexbuf);
        utils_reverse_hex(hexbuf, DOGECOIN_HASH_LENGTH*2);
        assert(memcmp(hexbuf, test->hexhash, DOGECOIN_HASH_LENGTH*2) == 0);
        // Check version, ts, bits, nonce
        assert(header->version == test->version);
        assert(header->timestamp == test->timestamp);
        assert(header->bits == test->bits);
        assert(header->nonce == test->nonce);

        dogecoin_block_header_free(header);
        dogecoin_block_header_free(header_copy);
        cstr_free(serialized, true);
    }

    /* blockheader */
    dogecoin_block_header bheader, bheaderprev, bheadercheck;
    bheader.version = 6422786; // 371338
    bheader.timestamp = 1410464609; // 371338
    bheader.nonce = 0; // 371338
    bheader.bits = 456184976; // 371338
    char *prevblock_hex_o = "60323982f9c5ff1b5a954eac9dc1269352835f47c2c5222691d80f0d50dcf053"; // 371337
    char *prevblock_hex = dogecoin_malloc(strlen(prevblock_hex_o)+1);
    memcpy_safe(prevblock_hex, prevblock_hex_o, strlen(prevblock_hex_o));
    utils_reverse_hex(prevblock_hex, 64);
    outlen = 0;
    utils_hex_to_bin(prevblock_hex, bheader.prev_block, 64, &outlen);
    dogecoin_free(prevblock_hex);

    char *merkleroot_hex_o = "366747b6b22fab0a5ef71d433c14e5949b601c1f103984181364618b83eef67d"; // 427928
    char *merkleroot_hex = dogecoin_malloc(strlen(merkleroot_hex_o)+1);
    memcpy_safe(merkleroot_hex, merkleroot_hex_o, strlen(merkleroot_hex_o));
    utils_reverse_hex(merkleroot_hex, 64);
    outlen = 0;
    utils_hex_to_bin(merkleroot_hex, bheader.merkle_root, 64, &outlen);
    dogecoin_free(merkleroot_hex);

    bheaderprev.version = 6422786; // 371337
    bheaderprev.timestamp = 1410464577; // 371337
    bheaderprev.nonce = 0; // 371337
    bheaderprev.bits = 456540548; // 371337

    prevblock_hex_o = "46a8b109fb016fa41abd17a19186ca78d39c60c020c71fcd2690320d47036f0d"; // 371336
    prevblock_hex = dogecoin_malloc(strlen(prevblock_hex_o)+1);
    memcpy_safe(prevblock_hex, prevblock_hex_o, strlen(prevblock_hex_o));
    utils_reverse_hex(prevblock_hex, 64);
    outlen = 0;
    utils_hex_to_bin(prevblock_hex, bheaderprev.prev_block, 64, &outlen);
    dogecoin_free(prevblock_hex);

    merkleroot_hex_o = "ee27b8fb782a5bfb99c975f0d4686440b9af9e16846603e5f2830e0b6fbf158a"; // 371337
    merkleroot_hex = dogecoin_malloc(strlen(merkleroot_hex_o)+1);
    memcpy_safe(merkleroot_hex, merkleroot_hex_o, strlen(merkleroot_hex_o));
    utils_reverse_hex(merkleroot_hex, 64);
    outlen = 0;
    utils_hex_to_bin(merkleroot_hex, bheaderprev.merkle_root, 64, &outlen);
    dogecoin_free(merkleroot_hex);

    /* compare blockheaderhex */
    cstring *blockheader_ser = cstr_new_sz(256);
    dogecoin_block_header_serialize(blockheader_ser, &bheader);
    char *blockheader_h371338 = "0201620053f0dc500d0fd8912622c5c2475f83529326c19dac4e955a1bffc5f9823932607df6ee838b616413188439101f1c609b94e5143c431df75e0aab2fb2b647673661fb115490d4301b00000000";
    char *blockheader_hash_h371338 = "6fb5ae70a65902381bcaa63a38cadf36e8c0a9b4cbffa85bc545c1be08cd0721";

    char headercheck[1024];
    utils_bin_to_hex((unsigned char *)blockheader_ser->str, blockheader_ser->len, headercheck);
    u_assert_str_eq(headercheck, blockheader_h371338);

    uint256 checkhash;
    dogecoin_block_header_hash(&bheader, (uint8_t *)&checkhash);
    char hashhex[sizeof(checkhash) * 2 + 1];
    utils_bin_to_hex(checkhash, sizeof(checkhash), hashhex);
    utils_reverse_hex(hashhex, strlen(hashhex));
    u_assert_str_eq(blockheader_hash_h371338, hashhex);

    struct const_buffer buf;
    buf.p = blockheader_ser->str;
    buf.len = blockheader_ser->len;
    dogecoin_block_header_deserialize(&bheadercheck, &buf);
    u_assert_str_eq(utils_uint8_to_hex(bheader.prev_block, sizeof(bheader.prev_block)), utils_uint8_to_hex(bheadercheck.prev_block, sizeof(bheadercheck.prev_block)));
    cstr_free(blockheader_ser, true);
    dogecoin_block_header_hash(&bheaderprev, (uint8_t *)&checkhash);
    u_assert_str_eq(utils_uint8_to_hex(bheader.prev_block, sizeof(bheader.prev_block)), utils_uint8_to_hex(checkhash, sizeof(checkhash)));
}
