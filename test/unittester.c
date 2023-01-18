/**********************************************************************
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#if defined HAVE_CONFIG_H
#include "src/libdogecoin-config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utest.h"

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
extern void test_base58();
extern void test_bip32();
extern void test_bip39();
extern void test_bip44();
extern void test_block_header();
extern void test_buffer();
extern void test_cstr();
extern void test_ecc();
extern void test_hash();
extern void test_key();
extern void test_koinu();
extern void test_memory();
extern void test_op_return();
extern void test_random();
extern void test_rmd160();
extern void test_serialize();
extern void test_sha_256();
extern void test_sha_512();
extern void test_sha_hmac();
extern void test_transaction();
extern void test_tx_serialization();
extern void test_tx_sighash();
extern void test_tx_sighash_ext();
extern void test_tx_negative_version();
extern void test_script_parse();
extern void test_script_op_codeseperator();
extern void test_invalid_tx_deser();
extern void test_tx_sign();
extern void test_scripts();
extern void test_utils();
extern void test_vector();
extern void test_qr();

#ifdef WITH_TOOLS
extern void test_tool();
#endif

#ifdef WITH_NET
extern void test_net_basics_plus_download_block();
extern void test_protocol();
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
    u_run_test(test_base58);
    u_run_test(test_bip32);
    u_run_test(test_bip39);
    u_run_test(test_bip44);
    u_run_test(test_block_header);
    u_run_test(test_buffer);
    u_run_test(test_cstr);
    u_run_test(test_ecc);
    u_run_test(test_hash);
    u_run_test(test_key);
    u_run_test(test_koinu);
    u_run_test(test_memory);
    u_run_test(test_op_return);
    u_run_test(test_random);
    u_run_test(test_rmd160);
    u_run_test(test_serialize);
    u_run_test(test_sha_256);
    u_run_test(test_sha_512);
    u_run_test(test_sha_hmac);
    u_run_test(test_transaction);
    u_run_test(test_tx_serialization);
    u_run_test(test_invalid_tx_deser);
    u_run_test(test_tx_sign);
    u_run_test(test_tx_sighash);
    u_run_test(test_tx_sighash_ext);
    u_run_test(test_tx_negative_version);
    u_run_test(test_scripts);
    u_run_test(test_script_parse);
    u_run_test(test_script_op_codeseperator);
    u_run_test(test_utils);
    u_run_test(test_vector);
    u_run_test(test_qr);
#ifdef WITH_TOOLS
    u_run_test(test_tool);
#endif

#ifdef WITH_NET
    u_run_test(test_net_basics_plus_download_block);
    u_run_test(test_protocol);
#endif

    dogecoin_ecc_stop();

    return U_TESTS_FAIL;
}
