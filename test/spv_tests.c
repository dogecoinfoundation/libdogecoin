/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli
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

#ifdef _MSC_VER
#include <stdio.h>
#else
#include <unistd.h>
#endif

#include <test/utest.h>

#include <dogecoin/arith_uint256.h>
#include <dogecoin/block.h>
#include <dogecoin/headersdb_file.h>
#include <dogecoin/net.h>
#include <dogecoin/spv.h>
#include <dogecoin/utils.h>
#include <dogecoin/validation.h>

void test_spv_sync_completed(dogecoin_spv_client* client) {
    printf("Sync completed, at height %d\n", client->headers_db->getchaintip(client->headers_db_ctx)->height);
    dogecoin_node_group_shutdown(client->nodegroup);
}

dogecoin_bool test_spv_header_message_processed(struct dogecoin_spv_client_ *client, dogecoin_node *node, dogecoin_blockindex *newtip) {
    UNUSED(client);
    UNUSED(node);
    if (newtip) {
        printf("New headers tip height %d\n", newtip->height);
    }
    return true;
}

void test_spv()
{
    // set chain:
    const dogecoin_chainparams* chain = &dogecoin_chainparams_test;

    // concatenate chain to prefix of headers database:
    char* header_suffix = "_headers.db";
    char* header_prefix = (char*)chain->chainname;
    char* headersfile = concat(header_prefix, header_suffix);

    // unlink newly prefixed headers database:
    unlink(headersfile);

    // init new spv client with debugging off and syncing to memory:
    dogecoin_spv_client* client = dogecoin_spv_client_new(chain, false, true, true, false);
    client->header_message_processed = test_spv_header_message_processed;
    client->sync_completed = test_spv_sync_completed;
    dogecoin_spv_client_load(client, headersfile, false);
    dogecoin_free(headersfile);

    printf("Discover peers...");
    dogecoin_spv_client_discover_peers(client, NULL);
    printf("done\n");
    printf("Start interacting with the p2p network...\n");
    dogecoin_spv_client_runloop(client);
    dogecoin_spv_client_free(client);
    remove_all_hashes();
    remove_all_maps();
}

void test_reorg() {
    // Initialize the chain parameters for mainnet
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    // Setup headers database file path for testing
    char* headersfile = "test_headers.db";

    // Unlink the headers database file
    unlink(headersfile);

    // Initialize SPV client
    dogecoin_spv_client* client = dogecoin_spv_client_new(chain, false, true, false, false);
    client->header_message_processed = test_spv_header_message_processed;
    client->sync_completed = test_spv_sync_completed;
    dogecoin_spv_client_load(client, headersfile, false);

    // Create headers for the main chain and new chain
    dogecoin_block_header* header1 = dogecoin_block_header_new();
    dogecoin_block_header* header2 = dogecoin_block_header_new();
    dogecoin_block_header* header3 = dogecoin_block_header_new();
    dogecoin_block_header* header2_fork = dogecoin_block_header_new();
    dogecoin_block_header* header3_fork = dogecoin_block_header_new();
    size_t outlen;

    // Initialize header1
    header1->version = 1; // 1
    header1->timestamp = 1386474927; // 1
    header1->nonce = 1417875456; // 1
    header1->bits = 0x1e0ffff0; // 1
    char prevblock_hex1[65] = "1a91e3dace36e2be3bf030a65679fe821aa1d6ef92e7c9902eb318182c355691";
    utils_reverse_hex(prevblock_hex1, 64);
    utils_hex_to_bin(prevblock_hex1, (uint8_t*) header1->prev_block, 64, &outlen);
    char merkleroot_hex1[65] = "5f7e779f7600f54e528686e91d5891f3ae226ee907f461692519e549105f521c";
    utils_reverse_hex(merkleroot_hex1, 64);
    utils_hex_to_bin(merkleroot_hex1, (uint8_t*) header1->merkle_root, 64, &outlen);

    // Initialize header2
    header2->version = 1; // 2
    header2->timestamp = 1386474933; // 2
    header2->nonce = 3404207872; // 2
    header2->bits = 0x1e0ffff0; // 2
    char prevblock_hex2[65] = "82bc68038f6034c0596b6e313729793a887fded6e92a31fbdf70863f89d9bea2";
    utils_reverse_hex(prevblock_hex2, 64);
    utils_hex_to_bin(prevblock_hex2, (uint8_t*) header2->prev_block, 64, &outlen);
    char merkleroot_hex2[65] = "3b14b76d22a3f2859d73316002bc1b9bfc7f37e2c3393be9b722b62bbd786983";
    utils_reverse_hex(merkleroot_hex2, 64);
    utils_hex_to_bin(merkleroot_hex2, (uint8_t*) header2->merkle_root, 64, &outlen);

    // Initialize header3
    header3->version = 1; // 3
    header3->timestamp = 1386474940; // 3
    header3->nonce = 3785361152; // 3
    header3->bits = 0x1e0ffff0; // 2
    char prevblock_hex3[65] = "ea5380659e02a68c073369e502125c634b2fb0aaf351b9360c673368c4f20c96";
    utils_reverse_hex(prevblock_hex3, 64);
    utils_hex_to_bin(prevblock_hex3, (uint8_t*) header3->prev_block, 64, &outlen);
    char merkleroot_hex3[65] = "1e10c28574e3b9d7032329b624ce4ac8064d0e91324aa14634aa2da61146ddfd";
    utils_reverse_hex(merkleroot_hex3, 64);
    utils_hex_to_bin(merkleroot_hex3, (uint8_t*) header3->merkle_root, 64, &outlen);

    // Initialize header2_fork
    header2_fork->version = 1; // 2
    header2_fork->timestamp = 1386474933; // 2
    header2_fork->nonce = 3406419112; // 2    header2_fork->nonce = 3404481231; // 2
    header2_fork->bits = 0x1e0ffef0; // 2
    utils_hex_to_bin(prevblock_hex2, (uint8_t*) header2_fork->prev_block, 64, &outlen);
    utils_hex_to_bin(merkleroot_hex2, (uint8_t*) header2_fork->merkle_root, 64, &outlen);

    // Initialize header3_fork
    header3_fork->version = 1;
    header3_fork->timestamp = 1386474934; // 2 + 1
    header3_fork->nonce = 3407274091; //
    header3_fork->bits = 0x1e0ffef0; //
    utils_hex_to_bin(merkleroot_hex2, (uint8_t*) header3_fork->merkle_root, 64, &outlen); // merkle is a don't care

    // Calculate the chainwork for each header
    uint256 chainwork1, chainwork2, chainwork3, chainwork2_fork, chainwork3_fork = {0};
    arith_uint256* target1 = init_arith_uint256();
    arith_uint256* target2 = init_arith_uint256();
    arith_uint256* target3 = init_arith_uint256();
    arith_uint256* target2_fork = init_arith_uint256();
    arith_uint256* target3_fork = init_arith_uint256();
    cstring* s = cstr_new_sz(64);
    dogecoin_bool f_negative, f_overflow;
    uint256* hash = dogecoin_uint256_vla(1);

    // Compute the hash of the block header 1
    dogecoin_block_header_serialize(s, header1);
    dogecoin_block_header_scrypt_hash(s, hash);
    cstr_free(s, true);

    // Compute the chainwork 1
    target1 = set_compact(target1, header1->bits, &f_negative, &f_overflow);
    check_pow(hash, header1->bits, chain, &chainwork1);
    dogecoin_free(target1);

    // Compute the hash of the block header 2
    s = cstr_new_sz(64);
    dogecoin_block_header_serialize(s, header2);
    dogecoin_block_header_scrypt_hash(s, hash);
    cstr_free(s, true);

    // Compute the chainwork 2
    target2 = set_compact(target2, header2->bits, &f_negative, &f_overflow);
    check_pow(hash, header2->bits, chain, &chainwork2);
    dogecoin_free(target2);

    // Compute the hash of the block header 3
    s = cstr_new_sz(64);
    dogecoin_block_header_serialize(s, header3);
    dogecoin_block_header_scrypt_hash(s, hash);
    cstr_free(s, true);

    // Compute the chainwork 3
    target3 = set_compact(target3, header3->bits, &f_negative, &f_overflow);
    check_pow(hash, header3->bits, chain, &chainwork3);
    dogecoin_free(target3);

    arith_uint256* arith_chainwork2 = init_arith_uint256();
    memcpy(arith_chainwork2, &chainwork2, sizeof(arith_uint256));
    arith_uint256* arith_chainwork2_fork = init_arith_uint256();

    // Mine the forked block header 2
    // loop until the chainwork of the fork is greater and the hash passes PoW
    while (true) {
        // Compute the hash of the block header 2
        s = cstr_new_sz(64);
        dogecoin_block_header_serialize(s, header2_fork);
        dogecoin_block_header_scrypt_hash(s, hash);
        cstr_free(s, true);

        // Compute the chainwork 2
        target2_fork = set_compact(target2_fork, header2_fork->bits, &f_negative, &f_overflow);
        bool pow_passed = check_pow(hash, header2_fork->bits, chain, &chainwork2_fork);

        // Update the arith_uint256 chainwork of the fork
        memcpy(arith_chainwork2_fork, &chainwork2_fork, sizeof(uint256));

        // Check if the chainwork of the fork is greater and the hash passes PoW
        if (arith_uint256_greater_than(arith_chainwork2_fork, arith_chainwork2) && pow_passed) {
            debug_print("Nonce: %u\n", header2_fork->nonce);
            debug_print("Hash: %s\n", hash_to_string((uint8_t*) hash));
            debug_print("Chainwork: %s\n", hash_to_string((uint8_t*) arith_chainwork2_fork));
            break;
        }

        // Increment the nonce
        header2_fork->nonce++;

        // Free the arith_uint256 chainwork
        dogecoin_free(arith_chainwork2_fork);
    }

    // Free the arith_uint256 chainwork of the fork
    dogecoin_free(arith_chainwork2);
    dogecoin_free(arith_chainwork2_fork);

    // Free the target and hash
    dogecoin_free(target2_fork);

    // Compute the sha256d hash of the header2_fork
    s = cstr_new_sz(64);
    dogecoin_block_header_serialize(s, header2_fork);
    dogecoin_block_header_hash(header2_fork, (uint8_t*) hash);
    cstr_free(s, true);

    // Set header3_fork's previous block to header2_fork's hash
    memcpy(&header3_fork->prev_block, hash, DOGECOIN_HASH_LENGTH);

    arith_uint256* arith_chainwork3_fork = init_arith_uint256();

    // Mine the forked block header 3
    // loop until the chainwork of the fork is greater and the hash passes PoW
    while (true) {
        // Compute the hash of the block header 3
        s = cstr_new_sz(64);
        dogecoin_block_header_serialize(s, header3_fork);
        dogecoin_block_header_scrypt_hash(s, hash);
        cstr_free(s, true);

        // Compute the chainwork 3
        target3_fork = set_compact(target3_fork, header3_fork->bits, &f_negative, &f_overflow);
        bool pow_passed = check_pow(hash, header3_fork->bits, chain, &chainwork3_fork);

        // Update the arith_uint256 chainwork of the fork
        memcpy(arith_chainwork3_fork, &chainwork3_fork, sizeof(uint256));

        // Check if the hash passes PoW
        if (pow_passed) {
            debug_print("Nonce: %u\n", header3_fork->nonce);
            debug_print("Hash: %s\n", hash_to_string((uint8_t*) hash));
            debug_print("Chainwork: %s\n", hash_to_string((uint8_t*) arith_chainwork3_fork));
            break;
        }

        // Increment the nonce
        header3_fork->nonce++;

        // Free the arith_uint256 chainwork
        dogecoin_free(arith_chainwork3_fork);
    }

    // Free the arith_uint256 chainwork of the fork
    dogecoin_free(arith_chainwork3_fork);

    // Free the target and hash
    dogecoin_free(target3_fork);
    dogecoin_free(hash);

    // Create a cstring for the new block headers
    cstring* cbuf_all = cstr_new_sz(80 * 5);

    // Serialize header1 into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header1);

    // Serialize header2 into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header2);

    // Serialize header3_fork into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header3_fork);

    // Serialize header2_fork into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header2_fork);

    // Serialize header3_fork into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header3_fork);

    // Define a constant buffer for each header
    struct const_buffer cbuf_header1 = {cbuf_all->str, 80};
    struct const_buffer cbuf_header2 = {cbuf_all->str + 80, 80};
    struct const_buffer cbuf_header3_fork = {cbuf_all->str + 160, 80};
    struct const_buffer cbuf_header2_fork = {cbuf_all->str + 240, 80};
    struct const_buffer cbuf_header3_fork_again = {cbuf_all->str + 320, 80};

    // Connect the headers to the database
    dogecoin_bool connected;
    dogecoin_headers_db *db = client->headers_db_ctx;
    dogecoin_headers_db_connect_hdr(db, &cbuf_header1, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header2, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header3_fork, false, &connected);
    u_assert_true (!connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header2_fork, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header3_fork_again, false, &connected);
    u_assert_true (connected);

    // Cleanup
    cstr_free(cbuf_all, true);
    dogecoin_block_header_free(header1);
    dogecoin_block_header_free(header2);
    dogecoin_block_header_free(header3);
    dogecoin_block_header_free(header2_fork);
    dogecoin_block_header_free(header3_fork);
    dogecoin_spv_client_free(client);
    remove_all_hashes();
    remove_all_maps();
}
