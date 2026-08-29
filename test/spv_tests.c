/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli
 Copyright (c) 2023 bluezr
 Copyright (c) 2023-2024 The Dogecoin Foundation

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

#include <string.h>
#include <time.h>

#include <event2/event.h>
#include <event2/util.h>

#include <test/utest.h>

#include <dogecoin/arith_uint256.h>
#include <dogecoin/block.h>
#include <dogecoin/compact_filter.h>
#include <dogecoin/headersdb_file.h>
#include <dogecoin/net.h>
#include <dogecoin/protocol.h>
#include <dogecoin/serialize.h>
#include <dogecoin/spv.h>
#include <dogecoin/utils.h>
#include <dogecoin/validation.h>

/* The only way out of the runloop is sync_completed, so a runner that cannot
   reach a peer used to sit here until the CI job timeout killed it, ~90 minutes
   on the macOS runners. Bound it. */
#define TEST_SPV_DEADLINE_S 900

static dogecoin_bool test_spv_synced = false;
static dogecoin_bool test_spv_saw_header = false;

void test_spv_sync_completed(dogecoin_spv_client* client) {
    test_spv_synced = true;
    printf("Sync completed, at height %d\n", client->headers_db->getchaintip(client->headers_db_ctx)->height);
    dogecoin_node_group_shutdown(client->nodegroup);
}

static void test_spv_deadline(evutil_socket_t fd, short event, void* ctx) {
    dogecoin_spv_client* client = (dogecoin_spv_client*)ctx;
    UNUSED(fd);
    UNUSED(event);
    printf("test_spv: no sync after %d s (headers seen: %s); giving up\n",
           TEST_SPV_DEADLINE_S, test_spv_saw_header ? "yes" : "no");
    dogecoin_node_group_shutdown(client->nodegroup);
}

dogecoin_bool test_spv_header_message_processed(struct dogecoin_spv_client_ *client, dogecoin_node *node, dogecoin_blockindex *newtip) {
    UNUSED(node);
    if (newtip) {
        test_spv_saw_header = true;
        printf("New headers tip height %d\n", newtip->height);
        if (newtip->height >= 4008284) {
            test_spv_sync_completed(client);
        }
    }
    return true;
}

/* One construction, two properties, because a mainnet client is expensive to
   build here -- opening the cfheaders db walks the on-disk file, and doing that
   twice pushed the suite past its runtime.

   Property one: the client holds the compiled-in checkpoints before it talks to
   anyone. dogecoin_cf_load_hardcoded_checkpoints() previously had exactly one
   caller in the whole BIP157 stack -- a unit test -- so at runtime the checkpoint
   set came entirely from a peer's cfcheckpt, and a peer answering with a short
   list left every filter header above it unanchored. The suite stayed green
   throughout because the test called the loader itself and nothing asserted the
   client did.

   Property two: a peer cannot take them away. Driven through
   nodegroup->postcmd_cb, which is dogecoin_net_spv_post_cmd, so this is the
   production dispatch path rather than a re-implementation of it. No socket is
   involved -- the handler reads node->nodeid and node->nodegroup, both of which
   dogecoin_node_group_add_node sets. */
static void test_spv_checkpoints_are_ours_and_stay_ours(void)
{
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    size_t table_count = 0;
    const dogecoin_cf_checkpoint *table = dogecoin_cf_get_checkpoints(chain, &table_count);
    u_assert_true(table != NULL);
    u_assert_true(table_count > 1);

    dogecoin_spv_client* client = dogecoin_spv_client_new(chain, false, true, true, false, 8, NULL);
    u_assert_true(client != NULL);
    u_assert_true(client->cfilter_state != NULL);
    u_assert_true(client->cfilter_state->checkpoints != NULL);
    u_assert_true(client->nodegroup != NULL);
    u_assert_true(client->nodegroup->postcmd_cb != NULL);

    /* Loaded, and with the table's values rather than an empty vector. */
    u_assert_uint32_eq((uint32_t)client->cfilter_state->checkpoints->len,
                       (uint32_t)table_count);
    uint256_t want;
    utils_uint256_sethex((char *)table[0].filter_header, want);
    u_assert_int_eq(memcmp(vector_idx(client->cfilter_state->checkpoints, 0), want, 32), 0);

    /* A one-entry cfcheckpt, through the real handler. The entry is genuine, so
       validation passes and the peer is not marked misbehaving -- the attack is
       the truncation, not a forged value. */
    dogecoin_node* node = dogecoin_node_new();
    u_assert_true(node != NULL);
    dogecoin_node_group_add_node(client->nodegroup, node);

    cstring* payload = cstr_new_sz(64);
    uint8_t filter_type = GCS_BASIC_FILTER_TYPE;
    ser_bytes(payload, &filter_type, 1);
    uint256_t stop_hash;
    dogecoin_mem_zero(stop_hash, sizeof(stop_hash));
    ser_u256(payload, stop_hash);
    ser_varlen(payload, 1);
    uint256_t cp0;
    utils_uint256_sethex((char *)table[0].filter_header, cp0);
    ser_u256(payload, cp0);

    dogecoin_p2p_msg_hdr hdr;
    dogecoin_mem_zero(&hdr, sizeof(hdr));
    memcpy(hdr.command, DOGECOIN_MSG_CFCHECKPT, strlen(DOGECOIN_MSG_CFCHECKPT));

    struct const_buffer buf = { payload->str, payload->len };
    client->nodegroup->postcmd_cb(node, &hdr, &buf);

    /* Still the whole table, still the table's values. Before the fix the
       handler freed this vector and rebuilt it from the message, leaving one. */
    u_assert_uint32_eq((uint32_t)client->cfilter_state->checkpoints->len,
                       (uint32_t)table_count);
    uint256_t last;
    utils_uint256_sethex((char *)table[table_count - 1].filter_header, last);
    u_assert_int_eq(memcmp(vector_idx(client->cfilter_state->checkpoints,
                                      table_count - 1), last, 32), 0);

    /* And dogecoin_cf_validate_checkpoints still accepts that truncated list --
       pinned deliberately, so nobody routes anchoring back through it. */
    vector_t *truncated = vector_new(1, dogecoin_free);
    uint256_t *one = dogecoin_calloc(1, sizeof(uint256_t));
    memcpy(one, cp0, 32);
    vector_add(truncated, one);
    u_assert_true(dogecoin_cf_validate_checkpoints(chain, truncated));
    vector_free(truncated, true);

    cstr_free(payload, true);
    dogecoin_spv_client_free(client);
}

void test_spv()
{
    test_spv_checkpoints_are_ours_and_stay_ours();

    // set chain:
    const dogecoin_chainparams* chain = &dogecoin_chainparams_test;

    // concatenate chain to prefix of headers database:
    char* header_suffix = "_headers.db";
    char* header_prefix = (char*)chain->chainname;
    char* headersfile = concat(header_prefix, header_suffix);

    // unlink newly prefixed headers database:
    unlink(headersfile);

    // init new spv client with debugging off and syncing to memory:
#ifndef __APPLE__
    // due to TBD anomaly in ci environment, enable http server if not running Apple
    dogecoin_spv_client* client = dogecoin_spv_client_new(chain, false, true, true, false, 8, "localhost:8888");
#else
    dogecoin_spv_client* client = dogecoin_spv_client_new(chain, false, true, true, false, 8, NULL);
#endif
    client->header_message_processed = test_spv_header_message_processed;
    client->sync_completed = test_spv_sync_completed;
    dogecoin_spv_client_load(client, headersfile, false);
    dogecoin_free(headersfile);

    printf("Discover peers...");
    dogecoin_spv_client_discover_peers(client, NULL);
    printf("done\n");
    printf("Start interacting with the p2p network...\n");

    test_spv_synced = false;
    test_spv_saw_header = false;
    struct timeval deadline_tv = { TEST_SPV_DEADLINE_S, 0 };
    struct event* deadline = evtimer_new(client->nodegroup->event_base, test_spv_deadline, client);
    u_assert_true(deadline != NULL);
    evtimer_add(deadline, &deadline_tv);

    dogecoin_spv_client_runloop(client);

    event_free(deadline);
    /* Deliberately not asserted: an offline runner cannot sync and this test has
       never asserted the outcome. The line says which happened. */
    printf("test_spv: %s\n", test_spv_synced ? "synced" : "did not sync");
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
    dogecoin_spv_client* client = dogecoin_spv_client_new(chain, false, false, false, false, 8, NULL);
    client->header_message_processed = test_spv_header_message_processed;
    client->sync_completed = test_spv_sync_completed;
    dogecoin_spv_client_load(client, headersfile, false);

    // Create headers for the main chain and new chain
    dogecoin_block_header* header1 = dogecoin_block_header_new();
    dogecoin_block_header* header2 = dogecoin_block_header_new();
    dogecoin_block_header* header3 = dogecoin_block_header_new();
    dogecoin_block_header* header4 = dogecoin_block_header_new();
    dogecoin_block_header* header2_stale = dogecoin_block_header_new();
    dogecoin_block_header* header2_fork = dogecoin_block_header_new();
    dogecoin_block_header* header3_fork = dogecoin_block_header_new();
    dogecoin_block_header* header4_fork = dogecoin_block_header_new();
    dogecoin_block_header* header5_fork = dogecoin_block_header_new();
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

    // Initialize header4
    header4->version = 1; // 4
    header4->timestamp = 1386474943; // 4
    header4->nonce = 151130624; // 4
    header4->bits = 0x1e0ffff0; // 2
    char prevblock_hex4[65] = "76f80a8a81e6f6669d340651723b874f97395c4dbda200f8b024df4c6566a92c";
    utils_reverse_hex(prevblock_hex4, 64);
    utils_hex_to_bin(prevblock_hex4, (uint8_t*) header4->prev_block, 64, &outlen);
    char merkleroot_hex4[65] = "9f69a09b940fc7645b0a261e81a1f777e3e6514989eaf15bbc66759fa49b70c2";
    utils_reverse_hex(merkleroot_hex4, 64);
    utils_hex_to_bin(merkleroot_hex4, (uint8_t*) header4->merkle_root, 64, &outlen);

    // Initialize header2_stale
    header2_stale->version = 1; // 2
    header2_stale->timestamp = 1386474933; // 2
    header2_stale->nonce = 3404481231; // 2
    header2_stale->bits = 0x1e0ffff0; // 2
    utils_hex_to_bin(prevblock_hex2, (uint8_t*) header2_stale->prev_block, 64, &outlen);
    utils_hex_to_bin(merkleroot_hex2, (uint8_t*) header2_stale->merkle_root, 64, &outlen);

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

    // Initialize header4_fork
    header4_fork->version = 1;
    header4_fork->timestamp = 1386474935; // 2 + 2
    header4_fork->nonce = 3414880011; //
    header4_fork->bits = 0x1e0ffef0; //
    utils_hex_to_bin(merkleroot_hex2, (uint8_t*) header4_fork->merkle_root, 64, &outlen); // merkle is a don't care

    // Initialize header5_fork
    header5_fork->version = 1;
    header5_fork->timestamp = 1386474936; // 2 + 3
    header5_fork->nonce = 3420420582; //
    header5_fork->bits = 0x1e0ffef0; //
    utils_hex_to_bin(merkleroot_hex2, (uint8_t*) header5_fork->merkle_root, 64, &outlen); // merkle is a don't care

    // Calculate the chainwork for each header
    arith_uint256 chainwork1 = {0}, chainwork2 = {0}, chainwork3 = {0}, chainwork4 = {0}, chainwork2_stale = {0}, chainwork2_fork = {0}, chainwork3_fork = {0}, chainwork4_fork = {0}, chainwork5_fork = {0};
    arith_uint256* target1 = init_arith_uint256();
    arith_uint256* target2 = init_arith_uint256();
    arith_uint256* target3 = init_arith_uint256();
    arith_uint256* target4 = init_arith_uint256();
    arith_uint256* target2_stale = init_arith_uint256();
    arith_uint256* target2_fork = init_arith_uint256();
    arith_uint256* target3_fork = init_arith_uint256();
    arith_uint256* target4_fork = init_arith_uint256();
    arith_uint256* target5_fork = init_arith_uint256();
    cstring* s = cstr_new_sz(64);
    dogecoin_bool f_negative, f_overflow;
    uint256_t* hash = dogecoin_uint256_vla(1);

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

    // Compute the hash of the block header 4
    s = cstr_new_sz(64);
    dogecoin_block_header_serialize(s, header4);
    dogecoin_block_header_scrypt_hash(s, hash);
    cstr_free(s, true);

    // Compute the chainwork 4
    target4 = set_compact(target4, header4->bits, &f_negative, &f_overflow);
    check_pow(hash, header4->bits, chain, &chainwork4);
    dogecoin_free(target4);

    arith_uint256* arith_chainwork2 = init_arith_uint256();
    *arith_chainwork2 = chainwork2;
    arith_uint256* arith_chainwork2_stale = init_arith_uint256();

    // Mine the stale block header 2
    // loop until the chainwork of the stale is equal and the hash passes PoW
    while (true) {
        // Compute the hash of the block header 2
        s = cstr_new_sz(64);
        dogecoin_block_header_serialize(s, header2_stale);
        dogecoin_block_header_scrypt_hash(s, hash);
        cstr_free(s, true);

        // Compute the chainwork 2
        target2_stale = set_compact(target2_stale, header2_stale->bits, &f_negative, &f_overflow);
        bool pow_passed = check_pow(hash, header2_stale->bits, chain, &chainwork2_stale);

        // Update the arith_uint256 chainwork of the stale
        *arith_chainwork2_stale = chainwork2_stale;

        // Check if the chainwork of the stale is equal and the hash passes PoW
        if (arith_uint256_equal(arith_chainwork2_stale, arith_chainwork2) && pow_passed) {
            debug_print("Nonce: %u\n", header2_stale->nonce);
            debug_print("Hash: %s\n", hash_to_string((uint8_t*) hash));
            debug_print("Chainwork: %s\n", hash_to_string((uint8_t*) arith_chainwork2_stale));
            break;
        }

        // Increment the nonce
        header2_stale->nonce++;

    }

    // Free the arith_uint256 chainwork of the stale
    dogecoin_free(arith_chainwork2_stale);

    // Free the target and hash
    dogecoin_free(target2_stale);

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
        *arith_chainwork2_fork = chainwork2_fork;

        // Check if the chainwork of the fork is greater and the hash passes PoW
        if (arith_uint256_greater_than(arith_chainwork2_fork, arith_chainwork2) && pow_passed) {
            debug_print("Nonce: %u\n", header2_fork->nonce);
            debug_print("Hash: %s\n", hash_to_string((uint8_t*) hash));
            debug_print("Chainwork: %s\n", hash_to_string((uint8_t*) arith_chainwork2_fork));
            break;
        }

        // Increment the nonce
        header2_fork->nonce++;

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
        *arith_chainwork3_fork = chainwork3_fork;

        // Check if the hash passes PoW
        if (pow_passed) {
            debug_print("Nonce: %u\n", header3_fork->nonce);
            debug_print("Hash: %s\n", hash_to_string((uint8_t*) hash));
            debug_print("Chainwork: %s\n", hash_to_string((uint8_t*) arith_chainwork3_fork));
            break;
        }

        // Increment the nonce
        header3_fork->nonce++;

    }

    // Free the arith_uint256 chainwork of the fork
    dogecoin_free(arith_chainwork3_fork);

    // Free the target
    dogecoin_free(target3_fork);

    // Compute the sha256d hash of the header3_fork
    s = cstr_new_sz(64);
    dogecoin_block_header_serialize(s, header3_fork);
    dogecoin_block_header_hash(header3_fork, (uint8_t*) hash);
    cstr_free(s, true);

    // Set header4_fork's previous block to header3_fork's hash
    memcpy(&header4_fork->prev_block, hash, DOGECOIN_HASH_LENGTH);

    arith_uint256* arith_chainwork4_fork = init_arith_uint256();

    // Mine the forked block header 4
    // loop until the chainwork of the fork is greater and the hash passes PoW
    while (true) {
        // Compute the hash of the block header 4
        s = cstr_new_sz(64);
        dogecoin_block_header_serialize(s, header4_fork);
        dogecoin_block_header_scrypt_hash(s, hash);
        cstr_free(s, true);

        // Compute the chainwork 4
        target4_fork = set_compact(target4_fork, header4_fork->bits, &f_negative, &f_overflow);
        bool pow_passed = check_pow(hash, header4_fork->bits, chain, &chainwork4_fork);

        // Update the arith_uint256 chainwork of the fork
        *arith_chainwork4_fork = chainwork4_fork;

        // Check if the hash passes PoW
        if (pow_passed) {
            debug_print("Nonce: %u\n", header4_fork->nonce);
            debug_print("Hash: %s\n", hash_to_string((uint8_t*) hash));
            debug_print("Chainwork: %s\n", hash_to_string((uint8_t*) arith_chainwork4_fork));
            break;
        }

        // Increment the nonce
        header4_fork->nonce++;

    }

    // Free the arith_uint256 chainwork of the fork
    dogecoin_free(arith_chainwork4_fork);

    // Free the target
    dogecoin_free(target4_fork);

    // Compute the sha256d hash of the header4_fork
    s = cstr_new_sz(64);
    dogecoin_block_header_serialize(s, header4_fork);
    dogecoin_block_header_hash(header4_fork, (uint8_t*) hash);
    cstr_free(s, true);

    // Set header5_fork's previous block to header4_fork's hash
    memcpy(&header5_fork->prev_block, hash, DOGECOIN_HASH_LENGTH);

    arith_uint256* arith_chainwork5_fork = init_arith_uint256();

    // Mine the forked block header 5
    // loop until the chainwork of the fork is greater and the hash passes PoW
    while (true) {
        // Compute the hash of the block header 5
        s = cstr_new_sz(64);
        dogecoin_block_header_serialize(s, header5_fork);
        dogecoin_block_header_scrypt_hash(s, hash);
        cstr_free(s, true);

        // Compute the chainwork 5
        target5_fork = set_compact(target5_fork, header5_fork->bits, &f_negative, &f_overflow);
        bool pow_passed = check_pow(hash, header5_fork->bits, chain, &chainwork5_fork);

        // Update the arith_uint256 chainwork of the fork
        *arith_chainwork5_fork = chainwork5_fork;

        // Check if the hash passes PoW
        if (pow_passed) {
            debug_print("Nonce: %u\n", header5_fork->nonce);
            debug_print("Hash: %s\n", hash_to_string((uint8_t*) hash));
            debug_print("Chainwork: %s\n", hash_to_string((uint8_t*) arith_chainwork5_fork));
            break;
        }

        // Increment the nonce
        header5_fork->nonce++;

    }

    // Free the arith_uint256 chainwork of the fork
    dogecoin_free(arith_chainwork5_fork);

    // Free the target
    dogecoin_free(target5_fork);

    // Free the hash
    dogecoin_free(hash);

    // Create a cstring for the new block headers
    cstring* cbuf_all = cstr_new_sz(80 * 9);

    // Serialize header1 into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header1);

    // Serialize header2 into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header2);

    // Serialize header3 into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header3);

    // Serialize header4 into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header4);

    // Serialize header5_fork into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header5_fork);

    // Serialize header2_stale into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header2_stale);

    // Serialize header2_fork into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header2_fork);

    // Serialize header3_fork into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header3_fork);

    // Serialize header4_fork into cbuf_all
    dogecoin_block_header_serialize(cbuf_all, header4_fork);

    // Serialize header5_fork into cbuf_all again
    dogecoin_block_header_serialize(cbuf_all, header5_fork);

    // Serialize header5_fork into cbuf_all yet again (duplicate)
    dogecoin_block_header_serialize(cbuf_all, header5_fork);

    // Define a constant buffer for each header
    struct const_buffer cbuf_header1 = {cbuf_all->str, 80};
    struct const_buffer cbuf_header2 = {cbuf_all->str + 80, 80};
    struct const_buffer cbuf_header3 = {cbuf_all->str + 160, 80};
    struct const_buffer cbuf_header4 = {cbuf_all->str + 240, 80};
    struct const_buffer cbuf_header5_fork = {cbuf_all->str + 320, 80};
    struct const_buffer cbuf_header2_stale = {cbuf_all->str + 400, 80};
    struct const_buffer cbuf_header2_fork = {cbuf_all->str + 480, 80};
    struct const_buffer cbuf_header3_fork = {cbuf_all->str + 560, 80};
    struct const_buffer cbuf_header4_fork = {cbuf_all->str + 640, 80};
    struct const_buffer cbuf_header5_fork_again = {cbuf_all->str + 720, 80};
    struct const_buffer cbuf_header5_fork_duplicate = {cbuf_all->str + 800, 80};

    // Connect the headers to the database
    dogecoin_bool connected;
    dogecoin_headers_db *db = client->headers_db_ctx;
    dogecoin_headers_db_connect_hdr(db, &cbuf_header1, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header2, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header3, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header4, false, &connected);
    u_assert_true (connected);
    dogecoin_free(dogecoin_headers_db_connect_hdr(db, &cbuf_header5_fork, false, &connected));
    u_assert_true (!connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header2_stale, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header2_fork, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header3_fork, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header4_fork, false, &connected);
    u_assert_true (connected);
    dogecoin_headers_db_connect_hdr(db, &cbuf_header5_fork_again, false, &connected);
    u_assert_true (connected);
    dogecoin_free(dogecoin_headers_db_connect_hdr(db, &cbuf_header5_fork_duplicate, false, &connected));
    u_assert_true (!connected);

    // Cleanup
    cstr_free(cbuf_all, true);
    dogecoin_block_header_free(header1);
    dogecoin_block_header_free(header2);
    dogecoin_block_header_free(header3);
    dogecoin_block_header_free(header4);
    dogecoin_block_header_free(header2_stale);
    dogecoin_block_header_free(header2_fork);
    dogecoin_block_header_free(header3_fork);
    dogecoin_block_header_free(header4_fork);
    dogecoin_block_header_free(header5_fork);
    dogecoin_spv_client_free(client);
    remove_all_hashes();
    remove_all_maps();

    // Re-initialize SPV client and load the headers database
    client = dogecoin_spv_client_new(chain, false, false, false, false, 8, NULL);
    client->header_message_processed = test_spv_header_message_processed;
    client->sync_completed = test_spv_sync_completed;
    dogecoin_spv_client_load(client, headersfile, false);

    // Cleanup
    dogecoin_spv_client_free(client);
    remove_all_hashes();
    remove_all_maps();
}

// BIP37 filter state tests
void test_bip37_filter_state()
{
    u_assert_true(SPV_HEADERS_FILE_HDR_LEN == 8);
    u_assert_true(SPV_HEADERS_FILE_REC_LEN == 148);

    dogecoin_spv_client* client = dogecoin_spv_client_new(&dogecoin_chainparams_main, false, true, false, false, 1, NULL);
    u_assert_true(client != NULL);

    /* BIP37 and BIP157 are mutually exclusive for privacy reasons: a bloom
     * filter leaks the watched scripts to peers, which is what compact filters
     * avoid.  Compact filters are on by default, and filterload fails closed
     * while they are, so a BIP37 consumer must opt out explicitly first. */
    u_assert_true(!dogecoin_spv_client_filterload(client, (const uint8_t[]){0xaa}, 1, 1, 0, 0));
    dogecoin_spv_enable_compact_filters(client, false);
    u_assert_true(!client->compact_filters_enabled);

    uint8_t filter[3] = {0xaa, 0xbb, 0xcc};
    u_assert_true(dogecoin_spv_client_filterload(client, filter, sizeof(filter), 2, 123, 1));
    u_assert_true(client->bloom_filter != NULL);
    u_assert_true(client->bloom_filter_len == sizeof(filter));
    u_assert_true(client->bloom_nhashfunc == 2);
    u_assert_true(client->bloom_ntweak == 123);
    u_assert_true(client->bloom_flags == 1);

    filter[0] = 0x00;
    u_assert_true(client->bloom_filter[0] == 0xaa);

    u_assert_true(!dogecoin_spv_client_filterload(client, NULL, 0, 0, 0, 0));

    {
        uint8_t empty_filter[8] = {0};
        uint8_t before[8] = {0};
        uint8_t outpoint[36] = {0};
        uint32_t vout = 1;
        outpoint[32] = (uint8_t)(vout & 0xffu);
        outpoint[33] = (uint8_t)((vout >> 8) & 0xffu);
        outpoint[34] = (uint8_t)((vout >> 16) & 0xffu);
        outpoint[35] = (uint8_t)((vout >> 24) & 0xffu);
        u_assert_true(dogecoin_spv_client_filterload(client, empty_filter, sizeof(empty_filter), 2, 123, 1));
        memcpy(before, client->bloom_filter, sizeof(before));
        u_assert_true(dogecoin_spv_client_filteradd(client, outpoint, sizeof(outpoint)));
        u_assert_true(memcmp(client->bloom_filter, before, sizeof(before)) != 0);
    }

    u_assert_true(dogecoin_spv_client_filterclear(client));
    u_assert_true(client->bloom_filter == NULL);
    u_assert_true(client->bloom_filter_len == 0);

    dogecoin_spv_client_free(client);
    remove_all_hashes();
    remove_all_maps();
}

// BIP37 merkleblock tests
typedef struct bip37_test_ctx_ {
    int tx_calls;
    cstring* expected_txid;
    cstring* seen_txid;
} bip37_test_ctx;

// callback for spv_client to report matched tx during merkleblock processing
static void test_bip37_sync_transaction(void *ctx, dogecoin_tx *tx, unsigned int pos, dogecoin_blockindex *pindex)
{
    UNUSED(pos);
    UNUSED(pindex);

    bip37_test_ctx* tctx = (bip37_test_ctx*)ctx;
    if (!tctx || !tx) return;

    uint8_t txid[DOGECOIN_HASH_LENGTH];
    memset(txid, 0, sizeof(txid));
    dogecoin_tx_hash(tx, txid);

    if (!tctx->seen_txid) {
        tctx->seen_txid = cstr_new_sz(DOGECOIN_HASH_LENGTH);
    }
    tctx->seen_txid->len = 0;
    cstr_append_buf(tctx->seen_txid, txid, DOGECOIN_HASH_LENGTH);

    tctx->tx_calls++;
}

// BIP37 vector parameters
#ifndef BIP37_VECTOR_TIME
#define BIP37_VECTOR_TIME 1700000000u
#endif
#ifndef BIP37_VECTOR_NONCE
#define BIP37_VECTOR_NONCE 202083u
#endif
#define BIP37_VECTOR_BITS 0x1e0ffff0u

// Test parsing of a BIP37 merkleblock message using a known vector
void test_bip37_merkleblock_vector()
{
    const dogecoin_chainparams* chain = &dogecoin_chainparams_main;

    char* headersfile = "test_bip37_headers.db";
    unlink(headersfile);

    // Initialize SPV client
    dogecoin_spv_client* client = dogecoin_spv_client_new(chain, false, true, false, false, 1, NULL);
    dogecoin_spv_client_load(client, headersfile, false);

    bip37_test_ctx tctx;
    memset(&tctx, 0, sizeof(tctx));
    client->sync_transaction = test_bip37_sync_transaction;
    client->sync_transaction_ctx = &tctx;

    // Create a simple transaction (coinbase-like) matching the merkleblock
    const char* tx_hex =
        "01000000"
        "01"
        "1111111111111111111111111111111111111111111111111111111111111111"
        "00000000"
        "00"
        "ffffffff"
        "01"
        "0100000000000000"
        "00"
        "00000000";

    uint8_t tx_raw[256];
    size_t tx_raw_len = 0;
    utils_hex_to_bin((char*)tx_hex, tx_raw, strlen(tx_hex), &tx_raw_len);

    dogecoin_tx* tx = dogecoin_tx_new();
    size_t consumed = 0;
    dogecoin_bool tx_ok = dogecoin_tx_deserialize(tx_raw, tx_raw_len, tx, &consumed);
    u_assert_true(tx_ok);
    u_assert_true(consumed == tx_raw_len);

    uint8_t txid[DOGECOIN_HASH_LENGTH];
    memset(txid, 0, sizeof(txid));
    dogecoin_tx_hash(tx, txid);

    tctx.expected_txid = cstr_new_sz(DOGECOIN_HASH_LENGTH);
    cstr_append_buf(tctx.expected_txid, txid, DOGECOIN_HASH_LENGTH);

    // Build a merkleblock message containing the txid
    dogecoin_block_header* hdr = dogecoin_block_header_new();
    hdr->version = 1;
    hdr->timestamp = (uint32_t)BIP37_VECTOR_TIME;
    hdr->bits = (uint32_t)BIP37_VECTOR_BITS;
    hdr->nonce = (uint32_t)BIP37_VECTOR_NONCE;
    memcpy(hdr->prev_block, &chain->genesisblockhash, DOGECOIN_HASH_LENGTH);
    memcpy(hdr->merkle_root, txid, DOGECOIN_HASH_LENGTH);

    // Serialize merkleblock message
    cstring* mb = cstr_new_sz(80 + 4 + 1 + 32 + 1 + 1);
    dogecoin_block_header_serialize(mb, hdr);
    ser_u32(mb, 1);           /* nTransactions */
    ser_varlen(mb, 1);        /* hashes count */
    ser_u256(mb, txid);       /* hash[0] */
    ser_varlen(mb, 1);        /* flags bytes */
    {
        uint8_t flag = 0x01;  /* bit0 set */
        ser_bytes(mb, &flag, 1);
    }

    // Parse the merkleblock message
    {
        struct const_buffer buf = { (const unsigned char*)mb->str, mb->len };

        uint8_t hdr_raw[80];
        u_assert_true(deser_bytes(hdr_raw, &buf, 80) != 0);

        // verify header is known to headers db
        {
            dogecoin_bool connected = false;
            struct const_buffer cbuf_hdr = { hdr_raw, 80 };
            dogecoin_blockindex* r = dogecoin_headers_db_connect_hdr(client->headers_db_ctx, &cbuf_hdr, false, &connected);
            if (!connected && r) dogecoin_free(r);
            u_assert_true(connected);
        }

        // verify header contents (txid in merkle root)
        u_assert_true(memcmp(hdr_raw + 36, txid, DOGECOIN_HASH_LENGTH) == 0);

        // continue parsing merkleblock message
        uint32_t nTransactions = 0;
        u_assert_true(deser_u32(&nTransactions, &buf) != 0);
        u_assert_true(nTransactions == 1);

        // hashes
        uint32_t hash_count = 0;
        u_assert_true(deser_varlen(&hash_count, &buf) != 0);
        u_assert_true(hash_count == 1);

        // txid
        uint8_t mb_txid[DOGECOIN_HASH_LENGTH];
        memset(mb_txid, 0, sizeof(mb_txid));
        u_assert_true(deser_u256(mb_txid, &buf) != 0);
        u_assert_true(memcmp(mb_txid, txid, DOGECOIN_HASH_LENGTH) == 0);

        // flags
        uint32_t flags_len = 0;
        u_assert_true(deser_varlen(&flags_len, &buf) != 0);
        u_assert_true(flags_len == 1);

        // flags bytes
        uint8_t flags[1];
        u_assert_true(deser_bytes(flags, &buf, flags_len) != 0);
        u_assert_true((flags[0] & 0x01) == 0x01);

        u_assert_true(buf.len == 0);
    }

    // Verify that the sync_transaction callback is invoked correctly
    u_assert_true(client->sync_transaction != NULL);
    test_bip37_sync_transaction(client->sync_transaction_ctx, tx, 0, NULL);

    // Verify expected txid seen
    u_assert_true(tctx.tx_calls == 1);
    u_assert_true(tctx.seen_txid != NULL);
    u_assert_true(tctx.seen_txid->len == DOGECOIN_HASH_LENGTH);
    u_assert_true(tctx.expected_txid != NULL);
    u_assert_true(tctx.expected_txid->len == DOGECOIN_HASH_LENGTH);
    u_assert_true(memcmp(tctx.seen_txid->str, tctx.expected_txid->str, DOGECOIN_HASH_LENGTH) == 0);

    // Cleanup
    cstr_free(mb, true);
    if (tctx.expected_txid) cstr_free(tctx.expected_txid, true);
    if (tctx.seen_txid) cstr_free(tctx.seen_txid, true);
    dogecoin_tx_free(tx);
    dogecoin_block_header_free(hdr);
    dogecoin_spv_client_free(client);
    remove_all_hashes();
    remove_all_maps();
}

/* A header write must append no matter where the shared read cursor sits.
   Reads and writes use one FILE*, and a reopened DB is "r+b", so a write left
   at a reader's position overwrites live records instead of extending the log.
   Measured against a real peer before the fix: 15 headers arrived and 15
   records were destroyed starting at the height the last read stopped on. */
void test_headers_db_write_appends()
{
    extern dogecoin_bool dogecoin_headers_db_write(dogecoin_headers_db *db,
                                                   dogecoin_blockindex *blockindex);
    const char *path = "test_headers_append.db";
    unlink(path);

    dogecoin_headers_db *db = dogecoin_headers_db_new(&dogecoin_chainparams_main, false);
    u_assert_true(db != NULL);
    u_assert_true(dogecoin_headers_db_load(db, path, false));

    dogecoin_blockindex bi;
    dogecoin_mem_zero(&bi, sizeof(bi));
    uint32_t h;
    for (h = 1; h <= 8; h++) {
        bi.height = h;
        memset(bi.hash, (int)h, sizeof(uint256_t));
        u_assert_true(dogecoin_headers_db_write(db, &bi));
    }
    fflush(db->headers_tree_file);

    /* Park the cursor mid-file, as the sequential cfilter rescan lookup does. */
    fseek(db->headers_tree_file,
          SPV_HEADERS_FILE_HDR_LEN + 3 * SPV_HEADERS_FILE_REC_LEN, SEEK_SET);

    bi.height = 9;
    memset(bi.hash, 9, sizeof(uint256_t));
    u_assert_true(dogecoin_headers_db_write(db, &bi));
    fflush(db->headers_tree_file);

    FILE *f = fopen(path, "rb");
    u_assert_true(f != NULL);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    u_assert_int_eq((int)sz,
                    (int)(SPV_HEADERS_FILE_HDR_LEN + 9 * SPV_HEADERS_FILE_REC_LEN));

    /* Heights 1..9 in order: record 3 survived, the write went to the end. */
    fseek(f, SPV_HEADERS_FILE_HDR_LEN, SEEK_SET);
    uint8_t rec[SPV_HEADERS_FILE_REC_LEN];
    for (h = 1; h <= 9; h++) {
        u_assert_true(fread(rec, sizeof(rec), 1, f) == 1);
        uint32_t got;
        memcpy(&got, rec + 32, 4);
        u_assert_uint32_eq(le32toh(got), h);
    }
    fclose(f);

    dogecoin_headers_db_free(db);
    unlink(path);
}

/* Looking up the same height twice must succeed twice.
   scan_resume_pos stored the end of the matched record while the resume test
   is "target >= scan_resume_height", so a repeat lookup restarted just past
   the record it wanted and scanned to EOF. Against a real chain that made
   every second lookup of a height miss, which forced the getcfilters stop-hash
   fallback to substitute the chain tip and build a 40751-filter request
   against a limit of 1000. */
void test_headers_db_repeat_lookup()
{
    extern dogecoin_bool dogecoin_headers_db_write(dogecoin_headers_db *db,
                                                   dogecoin_blockindex *bi);
    const char *path = "test_headers_repeat.db";
    unlink(path);

    dogecoin_headers_db *db = dogecoin_headers_db_new(&dogecoin_chainparams_main, false);
    u_assert_true(db != NULL);
    u_assert_true(dogecoin_headers_db_load(db, path, false));

    dogecoin_blockindex bi;
    dogecoin_mem_zero(&bi, sizeof(bi));
    uint32_t h;
    for (h = 1; h <= 20; h++) {
        bi.height = h;
        memset(bi.hash, (int)h, sizeof(uint256_t));
        u_assert_true(dogecoin_headers_db_write(db, &bi));
    }
    fflush(db->headers_tree_file);

    uint256_t got;
    uint8_t want[32];
    memset(want, 10, sizeof(want));

    /* First lookup populates the resume cursor. */
    u_assert_true(dogecoin_headers_db_get_block_hash_at_height(db, 10, got));
    u_assert_mem_eq(got, want, 32);

    /* Same height again: must not resume past its own record. */
    u_assert_true(dogecoin_headers_db_get_block_hash_at_height(db, 10, got));
    u_assert_mem_eq(got, want, 32);

    /* Forward from there still works, and so does going backwards. */
    memset(want, 15, sizeof(want));
    u_assert_true(dogecoin_headers_db_get_block_hash_at_height(db, 15, got));
    u_assert_mem_eq(got, want, 32);
    memset(want, 3, sizeof(want));
    u_assert_true(dogecoin_headers_db_get_block_hash_at_height(db, 3, got));
    u_assert_mem_eq(got, want, 32);

    dogecoin_headers_db_free(db);
    unlink(path);
}
