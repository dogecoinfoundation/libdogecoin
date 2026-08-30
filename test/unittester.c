/**********************************************************************
 * Copyright (c) 2024 bluezr                                          *
 * Copyright (c) 2024 edtubbs                                         *
 * Copyright (c) 2024 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#if defined HAVE_CONFIG_H
#include "libdogecoin-config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <test/utest.h>

#ifdef HAVE_BUILTIN_EXPECT
#define EXPECT(x, c) __builtin_expect((x), (c))
#else
#define EXPECT(x, c) (x)
#endif

#define TEST_FAILURE(msg)                                        \
    do {                                                         \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, msg); \
        abort();                                                 \
    } while (0)

#define CHECK(cond)                                        \
    do {                                                   \
        if (EXPECT(!(cond), 0)) {                          \
            TEST_FAILURE("test condition failed: " #cond); \
        }                                                  \
    } while (0)

extern void test_address();
extern void test_aes();
extern void test_arith_uint256();
extern void test_base58();
extern void test_base64();
extern void test_cf_checkpoints();
extern void test_compact_filter();
extern void test_dit();
extern void test_bip32();
extern void test_bip39();
extern void test_bip39_custom_wordlist_bounds();
extern void test_bip44();
extern void test_block_header();
extern void test_block_merkle_root_edges();
extern void test_compact_block();
extern void test_buffer();
extern void test_chacha20();
extern void test_chainparams_abi();
extern void test_sighash_surface();
extern void test_checkpoints();
extern void test_cstr();
extern void test_ecc();
extern void test_golomb();
extern void test_hash();
extern void test_key();
extern void test_key_recoverable_outlen();
extern void test_context_keypair_ex();
extern void test_context_ts_refcount();
extern void test_koinu();
extern void test_map_idx_not_reused();
extern void test_memory();
extern void test_moon();
extern void test_op_return();
extern void test_rfc6979();
extern void test_rng_entropy();
extern void test_random();
extern void test_random_failure_is_false();
extern void test_random_failure_propagates();
extern void test_rmd160();
extern void test_scrypt();
extern void test_serialize();
extern void test_sweep();
extern void test_sha1();
extern void test_sha1_hmac();
extern void test_sha_256();
extern void test_sha_512();
extern void test_sha_hmac();
extern void test_signmsg();
extern void test_slip0039();
extern void test_smpv();
extern void test_signmsg_ext();
extern void test_signmsg_ts_contexts();
extern void test_eckey_ts_retain_release();
extern void test_eckey_idx_not_reused();
extern void test_tpm();
extern void test_psbt();
extern void test_transaction();
extern void test_transaction_ts_contexts();
extern void test_transaction_ts_wrappers();
#if !defined(_WIN32)
extern void test_transaction_ts_multithread_stress();
#endif
extern void test_transaction_large(void);
extern void test_validation();
extern void test_tx_serialization();
extern void test_tx_sighash();
extern void test_tx_sighash_ext();
extern void test_tx_negative_version();
extern void test_validation_version_signed();
extern void test_script_parse();
extern void test_script_op_codeseperator();
extern void test_invalid_tx_deser();
extern void test_tx_sign();
extern void test_scripts();
extern void test_utils();
extern void test_utils_null_file_guards();
extern void test_utils_hex_to_bin_null_guards();
extern void test_utils_slice();
extern void test_utils_fopen_private();
extern void test_utils_reentrant_time();
extern void test_vector();
extern void test_qr();

#ifdef USE_RACCOON_G
extern void test_raccoong_polyr();
extern void test_raccoong_ntt();
extern void test_raccoong_shake();
extern void test_raccoong_xof_sample_q();
extern void test_raccoong_matvec();
extern void test_raccoong_keygen_t();
extern void test_raccoong_keypair();
extern void test_raccoong_gaussian();
extern void test_raccoong_hd_derive();
extern void test_raccoong_signature_serialize();
extern void test_raccoong_chal_poly();
extern void test_raccoong_hash_vec();
extern void test_raccoong_buff_mu();
extern void test_raccoong_sign();
#endif

#ifdef USE_ZK_CARRIER
extern void test_zk_carrier();
#endif

#ifdef WITH_LOGDB
extern void test_red_black_tree();
extern void test_logdb_memdb();
extern void test_logdb_rbtree();
extern void test_examples();
#endif

#ifdef WITH_WALLET
extern void test_wallet_basics();
extern void test_wallet();
extern void test_wallet_null_tx_guards();
extern void test_wallet_file_is_private();
extern void test_wallet_malformed_reclen();
extern void test_wallet_reorg_utxo_update();
extern void test_wallet_utxo_idx_not_reused();
extern void test_wallet_ts_wrappers();
#if !defined(_WIN32)
extern void test_wallet_ts_multithread_stress();
#endif
extern void test_wallet_balance_accounts_for_spends();
#endif

#ifdef WITH_TOOLS
extern void test_tool();
#endif

#ifdef WITH_NET
extern void test_net_basics_plus_download_block();
extern void test_protocol();
extern void test_net_flag_defined();
extern void test_net_peer_recovery();
extern void test_reorg();
extern void test_spv();
extern void test_headers_db_write_appends();
extern void test_bip37_filter_state();
extern void test_bip37_merkleblock_vector();
extern void test_cfheadersdb();
#else
extern void test_net_flag_not_defined();
#endif

extern void dogecoin_ecc_start();
extern void dogecoin_ecc_stop();

int U_TESTS_RUN = 0;
int U_TESTS_FAIL = 0;

int main()
{
    dogecoin_ecc_start();

    u_run_test(test_address);
    u_run_test(test_aes);
    u_run_test(test_arith_uint256);
    u_run_test(test_base58);
    u_run_test(test_base64);
    u_run_test(test_cf_checkpoints);
    u_run_test(test_compact_filter);
    u_run_test(test_dit);
    u_run_test(test_bip32);
    u_run_test(test_bip39);
    u_run_test(test_bip39_custom_wordlist_bounds);
    u_run_test(test_bip44);
    u_run_test(test_block_header);
    u_run_test(test_block_merkle_root_edges);
    u_run_test(test_compact_block);
    u_run_test(test_buffer);
    u_run_test(test_chacha20);
    u_run_test(test_chainparams_abi);
    u_run_test(test_sighash_surface);
    u_run_test(test_checkpoints);
    u_run_test(test_cstr);
    u_run_test(test_ecc);
    u_run_test(test_golomb);
    u_run_test(test_hash);
    u_run_test(test_key);
    u_run_test(test_key_recoverable_outlen);
    u_run_test(test_context_keypair_ex);
    u_run_test(test_context_ts_refcount);
    u_run_test(test_koinu);
    u_run_test(test_map_idx_not_reused);
    u_run_test(test_memory);
    u_run_test(test_moon);
    u_run_test(test_op_return);
    u_run_test(test_rfc6979);
    u_run_test(test_rng_entropy);
    u_run_test(test_random);
    u_run_test(test_random_failure_is_false);
    u_run_test(test_random_failure_propagates);
    u_run_test(test_rmd160);
    u_run_test(test_scrypt);
    u_run_test(test_serialize);
    u_run_test(test_sweep);
    u_run_test(test_sha1);
    u_run_test(test_sha1_hmac);
    u_run_test(test_sha_256);
    u_run_test(test_sha_512);
    u_run_test(test_sha_hmac);
    u_run_test(test_signmsg);
    u_run_test(test_signmsg_ext);
    u_run_test(test_signmsg_ts_contexts);
    u_run_test(test_eckey_ts_retain_release);
    u_run_test(test_eckey_idx_not_reused);
    u_run_test(test_slip0039);
    u_run_test(test_smpv);
#ifndef USE_OPTEE // TPM is not supported in OPTEE
    u_run_test(test_tpm);
#endif
    u_run_test(test_psbt);
    u_run_test(test_transaction);
    u_run_test(test_transaction_ts_contexts);
#ifndef USE_OPTEE
    u_run_test(test_transaction_ts_wrappers);
#if !defined(_WIN32)
    u_run_test(test_transaction_ts_multithread_stress);
#endif
#endif
    u_run_test(test_transaction_large);
    u_run_test(test_validation);
    u_run_test(test_tx_serialization);
    u_run_test(test_invalid_tx_deser);
    u_run_test(test_tx_sign);
    u_run_test(test_tx_sighash);
    u_run_test(test_tx_sighash_ext);
    u_run_test(test_tx_negative_version);
    u_run_test(test_validation_version_signed);
    u_run_test(test_scripts);
    u_run_test(test_script_parse);
    u_run_test(test_script_op_codeseperator);
    u_run_test(test_utils);
    u_run_test(test_utils_null_file_guards);
    u_run_test(test_utils_hex_to_bin_null_guards);
    u_run_test(test_utils_slice);
    u_run_test(test_utils_fopen_private);
    u_run_test(test_utils_reentrant_time);
    u_run_test(test_vector);
    u_run_test(test_qr);

#ifdef USE_RACCOON_G
    u_run_test(test_raccoong_polyr);
    u_run_test(test_raccoong_ntt);
    u_run_test(test_raccoong_shake);
    u_run_test(test_raccoong_xof_sample_q);
    u_run_test(test_raccoong_matvec);
    u_run_test(test_raccoong_keygen_t);
    u_run_test(test_raccoong_keypair);
    u_run_test(test_raccoong_gaussian);
    u_run_test(test_raccoong_hd_derive);
    u_run_test(test_raccoong_signature_serialize);
    u_run_test(test_raccoong_chal_poly);
    u_run_test(test_raccoong_hash_vec);
    u_run_test(test_raccoong_buff_mu);
    u_run_test(test_raccoong_sign);
#endif

#ifdef USE_ZK_CARRIER
    u_run_test(test_zk_carrier);
#endif

#ifdef WITH_LOGDB
    u_run_test(test_red_black_tree);
    u_run_test(test_logdb_memdb);
    u_run_test(test_logdb_rbtree);
    u_run_test(test_examples);
#endif

#ifdef WITH_WALLET
    u_run_test(test_wallet_basics);
    u_run_test(test_wallet);
    u_run_test(test_wallet_null_tx_guards);
    u_run_test(test_wallet_file_is_private);
    u_run_test(test_wallet_malformed_reclen);
    u_run_test(test_wallet_reorg_utxo_update);
    u_run_test(test_wallet_utxo_idx_not_reused);
#ifndef USE_OPTEE
    u_run_test(test_wallet_ts_wrappers);
#if !defined(_WIN32)
    u_run_test(test_wallet_ts_multithread_stress);
#endif
#endif
    u_run_test(test_wallet_balance_accounts_for_spends);
#endif

#ifdef WITH_TOOLS
    u_run_test(test_tool);
#endif

#ifdef WITH_NET
    u_run_test(test_net_flag_defined);
    u_run_test(test_net_basics_plus_download_block);
    u_run_test(test_net_peer_recovery);
    u_run_test(test_protocol);
    u_run_test(test_reorg);
    u_run_test(test_headers_db_write_appends);
    u_run_test(test_spv);
    u_run_test(test_bip37_filter_state);
    u_run_test(test_bip37_merkleblock_vector);
    u_run_test(test_cfheadersdb);
#else
    u_run_test(test_net_flag_not_defined);
#endif

    dogecoin_ecc_stop();

    return U_TESTS_FAIL;
}
