/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <dogecoin/wallet.h>
#include <dogecoin/base58.h>

#include <logdb/logdb.h>

#include "utest.h"
#include <dogecoin/utils.h>
#include <unistd.h>

static const char *wallettmpfile = "/tmp/dummy";

void test_wallet()
{
    unlink(wallettmpfile);
    dogecoin_wallet *wallet = dogecoin_wallet_new();
    enum logdb_error error;
    u_assert_int_eq(dogecoin_wallet_load(wallet, wallettmpfile, &error), true);

    char *xpriv = "dgpv51eADS3spNJh9sBD9rPnvatnXfcT7a12RBwuhww4Jt82yHfso4v2XhiDRzL3FxfzxNQNZDSY1PyqTPHZCfWPqVDBtSKemjpHJpELnZGwodC";

    dogecoin_hdnode node;
    dogecoin_hdnode *node2, *node3;
    dogecoin_bool suc = dogecoin_hdnode_deserialize(xpriv, &dogecoin_chain_main, &node);
    u_assert_int_eq(suc, 1); // should be 1
    dogecoin_wallet_set_master_key_copy(wallet, &node);

    node2 = dogecoin_wallet_next_key_new(wallet);

    dogecoin_wallet_free(wallet);

    wallet = dogecoin_wallet_new();
    u_assert_int_eq(dogecoin_wallet_load(wallet, wallettmpfile, &error), true);
    node3 = dogecoin_wallet_next_key_new(wallet);

    //should not be equal because we autoincrementing child index
    u_assert_int_eq(memcmp(node2->private_key, node3->private_key, sizeof(node2->private_key)) != 0, 1);
    dogecoin_hdnode_free(node3);

    //force to regenerate child 0
    wallet->next_childindex = 0;
    node3 = dogecoin_wallet_next_key_new(wallet);
    dogecoin_hdnode_free(node3);
    wallet->next_childindex = 0;
    node3 = dogecoin_wallet_next_key_new(wallet);

    //now it should be equal
    u_assert_int_eq(memcmp(node2->private_key, node3->private_key, sizeof(node2->private_key)), 0);

    uint8_t hash160[20];

    vector *addrs = vector_new(1, free);
    dogecoin_wallet_get_addresses(wallet, addrs);
    u_assert_int_eq(addrs->len, 4);
    u_assert_int_eq(strcmp(addrs->data[3],"DSwntSVA9sniePeWhu84msGKvYnuLTHA6X"), 0)
    u_assert_int_eq(strcmp(addrs->data[0],"DTCL4spXqoUrVtPVsviV81xkd8KKi4FjdE"), 0)

    vector_free(addrs, true);
    dogecoin_wallet_flush(wallet);
    dogecoin_wallet_free(wallet);
    dogecoin_hdnode_free(node2);
    dogecoin_hdnode_free(node3);

    wallet = dogecoin_wallet_new();
    u_assert_int_eq(dogecoin_wallet_load(wallet, wallettmpfile, &error), true);
    addrs = vector_new(1, free);
    dogecoin_wallet_get_addresses(wallet, addrs);

    u_assert_int_eq(addrs->len, 4);
    u_assert_int_eq(strcmp(addrs->data[3],"DSwntSVA9sniePeWhu84msGKvYnuLTHA6X"), 0)
    u_assert_int_eq(strcmp(addrs->data[0],"DTCL4spXqoUrVtPVsviV81xkd8KKi4FjdE"), 0)

    // get a hdnode and compare
    dogecoin_hdnode *checknode = dogecoin_wallet_find_hdnode_byaddr(wallet, addrs->data[0]);

    hash160[0] = wallet->chain->b58prefix_pubkey_address;
    dogecoin_hdnode_get_hash160(checknode, hash160+1);

    size_t addrsize = 98;
    char addr[addrsize];
    dogecoin_base58_encode_check(hash160, 21, addr, addrsize);
    u_assert_int_eq(strcmp(addr, addrs->data[0]), 0);
    vector_free(addrs, true);

    // add some txes
    static const char *hextx_coinbase = "01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff6403c4210637e4b883e5bda9e7a59ee4bb99e9b1bc468ae3c311fe570bbbaadade4d0c6ae4fd009f2045e7808d8c569b1eb63ecdb802000000f09f909f0f4d696e6564206279206368656e626f000000000000000000000000000000000000000000000000c036008601bb734c95000000001976a914bef5a2f9a56a94aab12459f72ad9cf8cf19c7bbe88aca7525e3a";

    static const char *hextx_ntx = "0100000001f48eef277d1338def6e6656b9226a82cb63b0591d15844e896fb875d95990edb000000006b483045022100ed3681313a3a52c1beb2f94f4cbba80d341652676463516cfd3e7fcfb00fdb8902201ff1acfba71bbb4436a990eac8f2ec3944a917859e2b02c9c113445147f23b9c0121021b8f3b66d044fabca1295e6ed16558909ebea941404ff376dcaec106cefe3526feffffff02e5b32400000000001976a91444d6af9359434935f1e9a0f43643eae59bf64d1388ace417541a030000001976a914d69367208e54bfdfa8ed1c77e4d8f6730b9777e388acb8210600";

    // form coinbase tx
    uint8_t tx_data[strlen(hextx_coinbase) / 2];
    int outlen ;
    utils_hex_to_bin(hextx_coinbase, tx_data, strlen(hextx_coinbase), &outlen);
    dogecoin_wtx* wtx = dogecoin_wallet_wtx_new();
    dogecoin_tx_deserialize(tx_data, outlen, wtx->tx);

    // add coinbase tx
    wtx->height = 0;
    dogecoin_wallet_add_wtx(wallet, wtx);

    uint64_t amount = dogecoin_wallet_wtx_get_credit(wallet, wtx);
    u_assert_uint32_eq(amount, 0);
    wallet->bestblockheight = 200;
    amount = dogecoin_wallet_wtx_get_credit(wallet, wtx);
    u_assert_uint32_eq(amount, 0); // 2504815547
    dogecoin_wallet_wtx_free(wtx);

    // form normal tx
    uint8_t tx_data_n[strlen(hextx_ntx) / 2];
    utils_hex_to_bin(hextx_ntx, tx_data_n, strlen(hextx_ntx), &outlen);
    wtx = dogecoin_wallet_wtx_new();
    dogecoin_tx_deserialize(tx_data_n, outlen, wtx->tx);
    
    // add normal tx
    wtx->height = 0;
    dogecoin_wallet_add_wtx(wallet, wtx);

    amount = dogecoin_wallet_wtx_get_credit(wallet, wtx);
    dogecoin_wallet_wtx_free(wtx);

    u_assert_uint32_eq(amount, 0); // 13326620644

    dogecoin_wallet_flush(wallet);
    dogecoin_wallet_free(wallet);

    wallet = dogecoin_wallet_new();
    u_assert_int_eq(dogecoin_wallet_load(wallet, wallettmpfile, &error), true);

    amount = dogecoin_wallet_get_balance(wallet);

    vector *unspents = vector_new(10, (void (*)(void*))dogecoin_wallet_output_free);
    dogecoin_wallet_get_unspent(wallet, unspents);

    amount = dogecoin_wallet_get_balance(wallet);

    unsigned int i;
    unsigned int found = 0;
    for (i = 0; i < unspents->len; i++)
    {
        dogecoin_output *output = unspents->data[i];
        uint8_t hash[32];
        dogecoin_tx_hash(output->wtx->tx, hash);
        char str[65];
        utils_bin_to_hex(hash, 32, str);
        utils_reverse_hex(str, 64);
        if (strcmp(str, "963b8b8e2d2025b64fd8144557604e98d2fa67a5386f8a06597d810f27ab60d7") == 0)
            found++;
        if (strcmp(str, "b99c4c532643a376c440b3cc612ec2fd96c15d1f50a6c40b112e4fd0c880d661") == 0)
            found++;
    }
    vector_free(unspents, true);
    u_assert_int_eq(found, 0); // 2

    amount = dogecoin_wallet_get_balance(wallet);
    u_assert_uint32_eq(amount, 0); // 13326620644
    wallet->bestblockheight = 200;
    amount = dogecoin_wallet_get_balance(wallet);
    u_assert_uint32_eq(amount, 13326620644+2504815547); // 13326620644+2504815547

    dogecoin_wallet_free(wallet);
}
