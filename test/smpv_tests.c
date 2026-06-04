/*
 * Copyright (c) 2024 The Dogecoin Foundation
 *
 * SMPV (Simplified Mempool Payment Verification) Test Suite
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stddef.h>

#include <test/utest.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/base58.h>
#include <dogecoin/cstr.h>
#include <dogecoin/smpv.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/mem.h>
#include <dogecoin/utils.h>

/* Test data */
static const char* TEST_ADDRESS_1 = "D7Y55vD8nNtW7VnT9Xr6Qc4vB8hN3jK2mP";
static const char* TEST_ADDRESS_2 = "D8Y66wE9nOtW8WnU0Xs7Rd5cC9i4kL3nQ";
static const char* TEST_RAW_TX =
    "0100000001a1b2c3d4e5f6789012345678901234567890abcdef1234567890abcdef1234567890000000006a47304402201234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef02201234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef01210234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456ffffffff0100e1f505000000001976a9141234567890abcdef1234567890abcdef1234567890abcdef88ac00000000";

/* Test callback function */
static void test_tx_callback(const dogecoin_smpv_tx* tx, const char* address, void* user_data) {
    (void)user_data; /* Suppress unused parameter warning */
    debug_print("%s", "  Transaction callback received\n");
    debug_print("    TXID: %s\n", tx->txid ? tx->txid : "NULL");
    debug_print("    Address: %s\n", address ? address : "NULL");
    debug_print("    Size: %llu bytes\n", (unsigned long long)tx->size);
    debug_print("    Timestamp: %llu\n", (unsigned long long)tx->timestamp);
}

static dogecoin_tx* build_test_tx_for_address(const char* address) {
    if (!address) return NULL;

    char script_pubkey_hex[64];
    dogecoin_mem_zero(script_pubkey_hex, sizeof(script_pubkey_hex));
    if (!dogecoin_p2pkh_address_to_pubkey_hash((char*)address, script_pubkey_hex)) return NULL;

    uint8_t script_pubkey[32];
    size_t script_pubkey_len = 0;
    utils_hex_to_bin(script_pubkey_hex, script_pubkey, strlen(script_pubkey_hex), &script_pubkey_len);
    if (script_pubkey_len == 0) return NULL;

    dogecoin_tx* tx = dogecoin_tx_new();
    if (!tx) return NULL;
    dogecoin_tx_in* in = dogecoin_tx_in_new();
    vector_add(tx->vin, in);
    dogecoin_tx_out* out = dogecoin_tx_out_new();
    out->value = 1000;
    out->script_pubkey = cstr_new_sz(script_pubkey_len);
    cstr_append_buf(out->script_pubkey, (const void*)script_pubkey, script_pubkey_len);
    vector_add(tx->vout, out);
    return tx;
}

/* Test SMPV client creation and destruction */
void test_smpv_client_creation() {
    debug_print("%s", "Testing SMPV client creation...\n");

    /* Test with mainnet */
    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    if (!client) {
        debug_print("%s", "  Failed to create client\n");
        return;
    }

    if (client->chain_params != &dogecoin_chainparams_main) {
        debug_print("%s", "  Wrong chain params\n");
        return;
    }

    if (client->watcher_count != 0) {
        debug_print("%s", "  Wrong initial watcher count\n");
        return;
    }

    if (client->mempool_tx_count != 0) {
        debug_print("%s", "  Wrong initial tx count\n");
        return;
    }

    if (client->is_running != false) {
        debug_print("%s", "  Should not be running initially\n");
        return;
    }

    debug_print("%s", "  Client created successfully\n");

    /* Test with testnet */
    dogecoin_smpv_client* testnet_client = dogecoin_smpv_client_new(&dogecoin_chainparams_test);
    if (!testnet_client) {
        debug_print("%s", "  Failed to create testnet client\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (testnet_client->chain_params != &dogecoin_chainparams_test) {
        debug_print("%s", "  Wrong testnet chain params\n");
        dogecoin_smpv_client_free(client);
        dogecoin_smpv_client_free(testnet_client);
        return;
    }

    debug_print("%s", "  Testnet client created successfully\n");

    /* Test with NULL chain params */
    dogecoin_smpv_client* null_client = dogecoin_smpv_client_new(NULL);
    if (null_client) {
        debug_print("%s", "  Should not create client with NULL params\n");
        dogecoin_smpv_client_free(client);
        dogecoin_smpv_client_free(testnet_client);
        return;
    }

    debug_print("%s", "  NULL chain params handled correctly\n");

    /* Clean up */
    dogecoin_smpv_client_free(client);
    dogecoin_smpv_client_free(testnet_client);

    debug_print("%s", "  SMPV client creation test passed\n\n");
}

/* Test address watching functionality */
void test_address_watching() {
    debug_print("%s", "Testing address watching functionality...\n");

    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    if (!client) {
        debug_print("%s", "  Failed to create client\n");
        return;
    }

    /* Test adding addresses */
    if (!dogecoin_smpv_add_watcher(client, TEST_ADDRESS_1)) {
        debug_print("%s", "  Failed to add first address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->watcher_count != 1) {
        debug_print("%s", "  Wrong watcher count after adding first address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Added first address\n");

    if (!dogecoin_smpv_add_watcher(client, TEST_ADDRESS_2)) {
        debug_print("%s", "  Failed to add second address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->watcher_count != 2) {
        debug_print("%s", "  Wrong watcher count after adding second address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Added second address\n");

    /* Test adding duplicate address */
    if (!dogecoin_smpv_add_watcher(client, TEST_ADDRESS_1)) {
        debug_print("%s", "  Should handle duplicate address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->watcher_count != 2) {
        debug_print("%s", "  Watcher count should not change for duplicate\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Duplicate address handled correctly\n");

    /* Test getting watcher */
    dogecoin_smpv_watcher* watcher = dogecoin_smpv_get_watcher(client, TEST_ADDRESS_1);
    if (!watcher) {
        debug_print("%s", "  Failed to get watcher\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (strcmp(watcher->address, TEST_ADDRESS_1) != 0) {
        debug_print("%s", "  Wrong watcher address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (watcher->is_active != true) {
        debug_print("%s", "  Watcher should be active\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Retrieved watcher successfully\n");

    /* Test getting non-existent watcher */
    watcher = dogecoin_smpv_get_watcher(client, "D9Z77xF0oPvX9XnV1Yt8Se6dD0j5lM4oR");
    if (watcher) {
        debug_print("%s", "  Should not find non-existent watcher\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Non-existent watcher handled correctly\n");

    /* Test removing address */
    if (!dogecoin_smpv_remove_watcher(client, TEST_ADDRESS_1)) {
        debug_print("%s", "  Failed to remove address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->watcher_count != 1) {
        debug_print("%s", "  Wrong watcher count after removal\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Removed address successfully\n");

    /* Test removing non-existent address */
    if (dogecoin_smpv_remove_watcher(client, "D9Z77xF0oPvX9XnV1Yt8Se6dD0j5lM4oR")) {
        debug_print("%s", "  Should not remove non-existent address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Non-existent address removal handled correctly\n");

    /* Test with NULL parameters */
    if (dogecoin_smpv_add_watcher(NULL, TEST_ADDRESS_1)) {
        debug_print("%s", "  Should not add watcher with NULL client\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (dogecoin_smpv_add_watcher(client, NULL)) {
        debug_print("%s", "  Should not add watcher with NULL address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  NULL parameter handling correct\n");

    dogecoin_smpv_client_free(client);

    debug_print("%s", "  Address watching test passed\n\n");
}

/* Test transaction processing */
void test_transaction_processing() {
    debug_print("%s", "Testing transaction processing...\n");

    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    if (!client) {
        debug_print("%s", "  Failed to create client\n");
        return;
    }

    /* Add test addresses */
    if (!dogecoin_smpv_add_watcher(client, TEST_ADDRESS_1)) {
        debug_print("%s", "  Failed to add test address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (!dogecoin_smpv_add_watcher(client, TEST_ADDRESS_2)) {
        debug_print("%s", "  Failed to add second test address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Added test addresses\n");

    /* Test transaction processing with callback */
    if (!dogecoin_smpv_process_tx(client, TEST_RAW_TX, test_tx_callback, NULL)) {
        debug_print("%s", "  Failed to process transaction\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->mempool_tx_count != 1) {
        debug_print("%s", "  Wrong mempool tx count after processing\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Transaction processed successfully\n");

    /* Test with NULL parameters */
    if (dogecoin_smpv_process_tx(NULL, TEST_RAW_TX, test_tx_callback, NULL)) {
        debug_print("%s", "  Should not process with NULL client\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (dogecoin_smpv_process_tx(client, NULL, test_tx_callback, NULL)) {
        debug_print("%s", "  Should not process with NULL tx data\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  NULL parameter handling correct\n");

    dogecoin_smpv_client_free(client);

    debug_print("%s", "  Transaction processing test passed\n\n");
}

/* Test client start/stop functionality */
void test_client_control() {
    debug_print("%s", "Testing client start/stop functionality...\n");

    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    if (!client) {
        debug_print("%s", "  Failed to create client\n");
        return;
    }

    /* Test starting client */
    if (!dogecoin_smpv_start(client)) {
        debug_print("%s", "  Failed to start client\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->is_running != true) {
        debug_print("%s", "  Client should be running\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Client started successfully\n");

    /* Test stopping client */
    dogecoin_smpv_stop(client);
    if (client->is_running != false) {
        debug_print("%s", "  Client should be stopped\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Client stopped successfully\n");

    /* Test starting NULL client */
    if (dogecoin_smpv_start(NULL)) {
        debug_print("%s", "  Should not start NULL client\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  NULL client handling correct\n");

    dogecoin_smpv_client_free(client);

    debug_print("%s", "  Client control test passed\n\n");
}

/* Test statistics and utility functions */
void test_statistics_and_utils() {
    debug_print("%s", "Testing statistics and utility functions...\n");

    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    if (!client) {
        debug_print("%s", "  Failed to create client\n");
        return;
    }

    /* Add some addresses */
    if (!dogecoin_smpv_add_watcher(client, TEST_ADDRESS_1)) {
        debug_print("%s", "  Failed to add first address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (!dogecoin_smpv_add_watcher(client, TEST_ADDRESS_2)) {
        debug_print("%s", "  Failed to add second address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    /* Test statistics */
    uint32_t total_txs, watched_addresses;

    dogecoin_smpv_get_stats(client, &total_txs, &watched_addresses);
    if (watched_addresses != 2) {
        debug_print("%s", "  Wrong watched addresses count\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (total_txs != 0) {
        debug_print("%s", "  Wrong initial tx count\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Statistics retrieved successfully\n");
    debug_print("    Watched addresses: %u\n", watched_addresses);
    debug_print("    Total transactions: %u\n", total_txs);

    /* Test JSON output */
    dogecoin_smpv_watcher* watcher = dogecoin_smpv_get_watcher(client, TEST_ADDRESS_1);
    if (!watcher) {
        debug_print("%s", "  Failed to get watcher for JSON test\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    char* json = dogecoin_smpv_watcher_to_json(watcher);
    if (!json) {
        debug_print("%s", "  Failed to generate watcher JSON\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Watcher JSON generated successfully\n");
    debug_print("    JSON: %s\n", json);
    dogecoin_free(json);

    dogecoin_smpv_client_free(client);

    debug_print("%s", "  Statistics and utility test passed\n\n");
}

/* Test error handling */
void test_error_handling() {
    debug_print("%s", "Testing error handling...\n");

    /* Test NULL client operations */
    dogecoin_smpv_client_free(NULL);
    if (dogecoin_smpv_add_watcher(NULL, TEST_ADDRESS_1)) {
        debug_print("%s", "  Should not add watcher with NULL client\n");
        return;
    }

    if (dogecoin_smpv_remove_watcher(NULL, TEST_ADDRESS_1)) {
        debug_print("%s", "  Should not remove watcher with NULL client\n");
        return;
    }

    if (dogecoin_smpv_get_watcher(NULL, TEST_ADDRESS_1)) {
        debug_print("%s", "  Should not get watcher with NULL client\n");
        return;
    }

    if (dogecoin_smpv_start(NULL)) {
        debug_print("%s", "  Should not start NULL client\n");
        return;
    }

    dogecoin_smpv_stop(NULL);
    if (dogecoin_smpv_process_tx(NULL, TEST_RAW_TX, NULL, NULL)) {
        debug_print("%s", "  Should not process tx with NULL client\n");
        return;
    }

    if (dogecoin_smpv_get_tx(NULL, "test")) {
        debug_print("%s", "  Should not get tx with NULL client\n");
        return;
    }

    dogecoin_smpv_update_tx_status(NULL, "test", true, "block", 1);
    dogecoin_smpv_get_stats(NULL, NULL, NULL);

    debug_print("%s", "  NULL client operations handled gracefully\n");

    /* Test NULL parameter operations */
    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    if (!client) {
        debug_print("%s", "  Failed to create client for error testing\n");
        return;
    }

    if (dogecoin_smpv_add_watcher(client, NULL)) {
        debug_print("%s", "  Should not add watcher with NULL address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (dogecoin_smpv_remove_watcher(client, NULL)) {
        debug_print("%s", "  Should not remove watcher with NULL address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (dogecoin_smpv_get_watcher(client, NULL)) {
        debug_print("%s", "  Should not get watcher with NULL address\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (dogecoin_smpv_process_tx(client, NULL, NULL, NULL)) {
        debug_print("%s", "  Should not process tx with NULL data\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (dogecoin_smpv_get_tx(client, NULL)) {
        debug_print("%s", "  Should not get tx with NULL txid\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    dogecoin_smpv_update_tx_status(client, NULL, true, NULL, 1);
    dogecoin_smpv_tip_update(NULL, 1);

    debug_print("%s", "  NULL parameter operations handled gracefully\n");

    /* Test utility functions with NULL */
    dogecoin_smpv_tx_free(NULL);
    if (dogecoin_smpv_tx_to_json(NULL)) {
        debug_print("%s", "  Should not generate JSON for NULL tx\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (dogecoin_smpv_watcher_to_json(NULL)) {
        debug_print("%s", "  Should not generate JSON for NULL watcher\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  NULL utility function calls handled gracefully\n");

    dogecoin_smpv_client_free(client);

    debug_print("%s", "  Error handling test passed\n\n");
}

/* Test confirmation tracking */
void test_confirmation_tracking() {
    debug_print("%s", "Testing confirmation tracking...\n");

    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    if (!client) {
        debug_print("%s", "  Failed to create client\n");
        return;
    }

    /* Process a transaction - initially unconfirmed */
    if (!dogecoin_smpv_process_tx(client, TEST_RAW_TX, NULL, NULL)) {
        debug_print("%s", "  Failed to process transaction\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->mempool_tx_count != 1) {
        debug_print("%s", "  Wrong tx count after processing\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    dogecoin_smpv_tx* tx = &client->mempool_txs[0];

    /* Verify initial state - unconfirmed */
    if (tx->is_confirmed != false) {
        debug_print("%s", "  Transaction should be unconfirmed initially\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (tx->confirmations != 0) {
        debug_print("%s", "  Initial confirmations should be 0\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->unconfirmed_count != 1) {
        debug_print("%s", "  Unconfirmed count should be 1\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Transaction initially unconfirmed (0 confirmations)\n");

    /* Confirm the transaction at block height 100 */
    const char* test_block_hash = "0000000000000000000000000000000000000000000000000000000000000abc";
    dogecoin_smpv_update_tx_status(client, tx->txid, true, test_block_hash, 100);

    /* Verify confirmation */
    if (tx->is_confirmed != true) {
        debug_print("%s", "  Transaction should be confirmed\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (tx->confirmations != 1) {
        debug_print("  Confirmations should be 1, got %u\n", tx->confirmations);
        dogecoin_smpv_client_free(client);
        return;
    }

    if (tx->block_height != 100) {
        debug_print("  Block height should be 100, got %u\n", tx->block_height);
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->confirmed_count != 1) {
        debug_print("  Confirmed count should be 1, got %u\n", client->confirmed_count);
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->unconfirmed_count != 0) {
        debug_print("  Unconfirmed count should be 0, got %u\n", client->unconfirmed_count);
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Transaction confirmed at block 100 (1 confirmation)\n");

    /* Simulate new blocks being added - tip advances to 105 */
    dogecoin_smpv_tip_update(client, 105);

    /* Verify confirmation count increased */
    if (tx->confirmations != 6) {
        debug_print("  Confirmations should be 6, got %u\n", tx->confirmations);
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  After tip advances to 105, transaction has 6 confirmations\n");

    /* Simulate more blocks - tip advances to 110 */
    dogecoin_smpv_tip_update(client, 110);

    if (tx->confirmations != 11) {
        debug_print("  Confirmations should be 11, got %u\n", tx->confirmations);
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  After tip advances to 110, transaction has 11 confirmations\n");

    /* Test reorg: tip moves below transaction height → 0 confirmations */
    dogecoin_smpv_tip_update(client, 99);

    if (tx->confirmations != 0) {
        debug_print("  After reorg below tx height, confirmations should be 0, got %u\n", tx->confirmations);
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  After reorg (tip at 99, below tx at 100), confirmations = 0\n");

    /* Test unconfirming a transaction (e.g., during reorg) */
    dogecoin_smpv_update_tx_status(client, tx->txid, false, NULL, 0);

    if (tx->is_confirmed != false) {
        debug_print("%s", "  Transaction should be unconfirmed after reorg\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    if (tx->confirmations != 0) {
        debug_print("  Confirmations should be 0 after unconfirm, got %u\n", tx->confirmations);
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->confirmed_count != 0) {
        debug_print("  Confirmed count should be 0 after unconfirm, got %u\n", client->confirmed_count);
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->unconfirmed_count != 1) {
        debug_print("  Unconfirmed count should be 1 after unconfirm, got %u\n", client->unconfirmed_count);
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Transaction unconfirmed during reorg (back to 0 confirmations)\n");

    /* Test that transactions NOT in mempool are ignored */
    const char* unknown_txid = "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";
    uint32_t tx_count_before = client->mempool_tx_count;
    uint32_t confirmed_count_before = client->confirmed_count;

    /* Try to confirm a transaction that was never in mempool */
    dogecoin_smpv_update_tx_status(client, unknown_txid, true, test_block_hash, 120);

    /* Verify it was NOT added */
    if (client->mempool_tx_count != tx_count_before) {
        debug_print("  Transaction count should remain %u, got %u\n", tx_count_before, client->mempool_tx_count);
        dogecoin_smpv_client_free(client);
        return;
    }

    if (client->confirmed_count != confirmed_count_before) {
        debug_print("  Confirmed count should remain %u, got %u\n", confirmed_count_before, client->confirmed_count);
        dogecoin_smpv_client_free(client);
        return;
    }

    dogecoin_smpv_tx* should_be_null = dogecoin_smpv_get_tx(client, unknown_txid);
    if (should_be_null) {
        debug_print("%s", "  Transaction not in mempool should not be tracked\n");
        dogecoin_smpv_client_free(client);
        return;
    }

    debug_print("%s", "  Transactions not in mempool correctly ignored\n");

    dogecoin_smpv_client_free(client);

    debug_print("%s", "  Confirmation tracking test passed\n\n");

}

void test_smpv_relevance_and_address_lookup() {
    uint160_t h1;
    uint160_t h2;
    for (int i = 0; i < 20; i++) {
        h1[i] = (uint8_t)(i + 1);
        h2[i] = (uint8_t)(i + 21);
    }

    char addr1[P2PKHLEN];
    char addr2[P2PKHLEN];
    u_assert_true(dogecoin_p2pkh_addr_from_hash160(h1, &dogecoin_chainparams_main, addr1, sizeof(addr1)));
    u_assert_true(dogecoin_p2pkh_addr_from_hash160(h2, &dogecoin_chainparams_main, addr2, sizeof(addr2)));

    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    u_assert_true(client != NULL);
    u_assert_true(dogecoin_smpv_add_watcher(client, addr1));

    dogecoin_tx* tx_match = build_test_tx_for_address(addr1);
    dogecoin_tx* tx_other = build_test_tx_for_address(addr2);
    u_assert_true(tx_match != NULL);
    u_assert_true(tx_other != NULL);

    char* relevant = NULL;
    u_assert_true(dogecoin_smpv_is_tx_relevant(client, tx_match, &relevant));
    u_assert_true(relevant != NULL);
    u_assert_true(strcmp(relevant, addr1) == 0);
    dogecoin_free(relevant);
    relevant = NULL;

    u_assert_true(!dogecoin_smpv_is_tx_relevant(client, tx_other, &relevant));
    u_assert_true(relevant == NULL);

    client->mempool_txs = (dogecoin_smpv_tx*)dogecoin_calloc(2, sizeof(dogecoin_smpv_tx));
    u_assert_true(client->mempool_txs != NULL);
    client->mempool_tx_count = 2;
    client->mempool_txs[0].decoded_tx = tx_match;
    client->mempool_txs[0].txid = (char*)dogecoin_calloc(1, strlen("tx_match") + 1);
    strcpy(client->mempool_txs[0].txid, "tx_match");
    client->mempool_txs[1].decoded_tx = tx_other;
    client->mempool_txs[1].txid = (char*)dogecoin_calloc(1, strlen("tx_other") + 1);
    strcpy(client->mempool_txs[1].txid, "tx_other");

    size_t tx_count = 0;
    dogecoin_smpv_tx** txs = dogecoin_smpv_get_address_txs(client, addr1, &tx_count);
    u_assert_true(txs != NULL);
    u_assert_true(tx_count == 1);
    dogecoin_free(txs);

    tx_count = 0;
    /* get_address_txs() is an address query over mempool transactions,
       independent from watcher registration. */
    txs = dogecoin_smpv_get_address_txs(client, addr2, &tx_count);
    u_assert_true(txs != NULL);
    u_assert_true(tx_count == 1);
    dogecoin_free(txs);

    dogecoin_smpv_client_free(client);
}

/* Test input-bound enforcement in add_watcher */
void test_smpv_input_bounds() {
    debug_print("%s", "Testing input-bound enforcement...\n");

    dogecoin_smpv_client* client = dogecoin_smpv_client_new(&dogecoin_chainparams_main);
    u_assert_true(client != NULL);

    /* Empty string must be rejected */
    u_assert_true(!dogecoin_smpv_add_watcher(client, ""));
    u_assert_true(client->watcher_count == 0);

    /* Oversized address (>= 90 chars) must be rejected */
    const char oversized[] =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    u_assert_true(strlen(oversized) >= 90);
    u_assert_true(!dogecoin_smpv_add_watcher(client, oversized));
    u_assert_true(client->watcher_count == 0);

    /* Valid-length address must be accepted */
    u_assert_true(dogecoin_smpv_add_watcher(client, "D7ZRpJYqGvXkLmN2xWfQsT9bCdE4hA6yKp"));
    u_assert_true(client->watcher_count == 1);

    /* Duplicate add must be idempotent */
    u_assert_true(dogecoin_smpv_add_watcher(client, "D7ZRpJYqGvXkLmN2xWfQsT9bCdE4hA6yKp"));
    u_assert_true(client->watcher_count == 1);

    dogecoin_smpv_client_free(client);

    debug_print("%s", "  Input-bound enforcement test passed\n\n");
}

/* Main test function for the test framework */
void test_smpv() {
    debug_print("%s", "SMPV (Simplified Mempool Payment Verification) Test Suite\n");
    debug_print("%s", "========================================================\n\n");

    test_smpv_client_creation();
    test_address_watching();
    test_transaction_processing();
    test_client_control();
    test_statistics_and_utils();
    test_error_handling();
    test_confirmation_tracking();
    test_smpv_relevance_and_address_lookup();
    test_smpv_input_bounds();

    debug_print("%s", "All SMPV tests completed\n");
}
