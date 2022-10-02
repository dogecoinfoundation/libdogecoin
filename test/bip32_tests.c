/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include "utest.h"

#include <dogecoin/bip32.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

void test_bip32()
{
    dogecoin_hdnode node, node2, node3, node4;
    char str[112];
    int r;
    uint8_t private_key_master[32];
    uint8_t chain_code_master[32];

    /* init m */
    dogecoin_hdnode_from_seed(utils_hex_to_uint8("000102030405060708090a0b0c0d0e0f"), 16, &node);

    /* [Chain m] */
    memcpy_safe(private_key_master,
           utils_hex_to_uint8("c6991eeda06c82a61001dd0bed02a1b2597997b684cab51550ad8c0ce75c0a6b"),
           32);
    memcpy_safe(chain_code_master,
           utils_hex_to_uint8("97c57681261f358eb33ae52625d79472e264acfa78c163e98c3db882c1317567"),
           32);
    u_assert_int_eq(node.fingerprint, 0x00000000);
    u_assert_mem_eq(node.chain_code, chain_code_master, 32);
    u_assert_mem_eq(node.private_key, private_key_master, 32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("02c768a99915cf995e8507f5accdef995fd912cd4559def5862d29d229c04d2943"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv51eADS3spNJh9SHVGLuKReia8srv3ripH7j8kAS8PFuRsZQLnaAHpHmRz3Mg2DzyRjJKSSunwYByEhGiJzfWQfqcfnmMqg4WPL6CV9Coww4");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));

    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DQKnfKgsqVDxXjcCUKSs8Xz7bDe2SNcyof");

    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8kXBZ7ymNWy2SHweJaXHYWGAbSEYsykXZiWLuGq7BNxSRcX3CLi4TbB5ZGHwUmjfRxcT6zsN88G4C85duZ13naXKyszHKhvrdPsVjRnCjX5");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    /* [Chain m/0'] */
    char path0[] = "m/0'";
    dogecoin_hd_generate_key(&node, path0, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0xD2700AA0);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("ce0b2fcd904a6d31577926feba13d0794482d1216fb082306c768cffbfb8a8ba"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("9a890ef773091cbd474a3be0a90b04f3925fa2a4f39b9e0bcadfb90926b30657"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("03e0e0e17e610cd45a711a73d2c3149c7475ea3bde422dc70b88427d53773b5854"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv5551MfWQawkQLi5tQZ4Fr1baCKMyE3FSJHBiv9VA27KEJWx3VHYhVQuZZyNjRk3jewTP7Bv33L27hngKMzTzPBwhM1tqjmYadC24PWukcmD");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));

    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DFVFuPWwf4gjNWGDUcr3tnmG4ZybmiePNb");

    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8ox2hMSJ96QjdZk3SngDxs9Aesjc4AH9asxw5Ft8pENEra4ju46U8iKD9EnNAvr5NLgNX847FoiNGrhHj1dXQyAaNTo8WXxk69U2kjojQvL");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    /* [Chain m/0'/3] */
    char path1[] = "m/0'/3";
    dogecoin_hd_generate_key(&node, path1, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x718164DE);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("7895d53ee9a390823afb79e063d5e1782840e891bccd0f74b3fb5ba548e1c782"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("110cb805a25d6570a256da0104b2de72b2e396ea95174597f82d29f60a4ed404"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("02ae81bfefb4329140b8243f1551b2337432251e80d5b7bd65a420353f97054ea7"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv56EpU8W81bdsoh8AztdZLFx9SfV8Po3XmoVcv7m7RwvNDYQdh8fzzGZHiYTGfNig54zAXuyweJjPuHqrYUvRyu5Yr2RBwvEdQ6wrZLi79Ub");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DGhvKQMX2QkeNPhUVc5dkAy2BoET25PMZv");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8q7qopS1ZkJD6YnL38FXT7VjuDrmDv5F4QGq5EA6E4yNmbXL6uDmdZxwHnaXSfTRxCgCfsnGA6io3fMp2VfoWjBj3i4qRf4HT8WeNjin6tp");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/2'] */
    char path2[] = "m/0'/3/2'";
    dogecoin_hd_generate_key(&node, path2, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x7EDE93E8);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("17a607f55523ce2957571e14a6839b3f68e4e521588223f9fa24cee15be1b014"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("1870cb7b973718cd9ef29a7317fd5ce6f9c2d1dfa95ac74e02f27ac1ca70e8b7"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("03c865a25a9424ef611bcaaf56783d9b7b723c5f06f72c3c0859c37737b080a36f"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv58DemLg6LcinHnvT3PkzEci3J2RBE9ZUBQUm76ikJ9fF5MjwF9YeCYZwy3chm5FYrFQVkXB5w7mZHDoAcxUMog2X32JKSfKmTZXgRQgMdn6");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DDXT63uLGA4QhQCb3m65DumohfqqDpg25h");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8s6g72bytmP7aeac5dNxMUFdkanp4GbBU1FyGD7j6GiFdQrdev6QqqybYKprFq8t7qFsj8XNEptADqps6MXeHLBNraU5T3rMVmY1TNQQtka");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/2'/2] */
    char path3[] = "m/0'/3/2'/2";
    dogecoin_hd_generate_key(&node, path3, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x5BFB4F66);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("7f6b54729291ca94ef3eb2bf4b8db40d33e790a14704e0979d7a7e6f1f5041a7"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("9b82184573f68191ff9bb964f1e81bbed98eb3ff73d9c2770d996e721df76dd2"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("031a3a7cb1631b6d0a370572fb98831af33b4c089fd85688015c6fdec3a7aeaf2d"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv59quzHTRWT9s24AfcyhZ334JJSFdandiYZezWoPY5sHp1ZXeFKtriERQ2jQiqS4vo99KR8dFbjdN7mArSd1NrC32cYojs4dNpjkHqz1H5WB");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DB6tpGujCj5k9qqjfD9JtouPwxaDCd5pF2");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8tiwKyPK4bpCJuppfDKX9tbtkzdGQufRqASCfunWszLpZceLf6SdMXq3byJSV4UevEfz2Edbz7HfdtahBg28xW68KLa1Bjm4cv9vFX5NVs5");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/2'/2/1000000000] */
    char path4[] = "m/0'/3/2'/2/1000000000";
    dogecoin_hd_generate_key(&node, path4, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x416622DE);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("8e8c0d0f85dc3e9161ccadd9038a111add340dcc6d3dcfdd287803d0f63a69e3"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("2c2baa6f87ae9f76d3107251a28921938435718305bc44b57ea7d8fa84e7e1e5"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("028088f0790134b82c55b91be40390445b8dcbd6ece4c2797b8973008e15cfef98"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "dgpv5BXiZaRGoK7z4izJycDj77vQMXoDAanWwnZgzBfMb6Fwu2xwzxPM8MSw2MJht1iLbjLbHUhnQ1JiP895gAV9wGSDhYN2eayt9GepTapZf8p");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DAPP7ux2W7Sog1bZEcR1Rv8hdLY3CzJ88v");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8vQjuGMAMTnKMaeU1qqhDyTzp6AqzhpEEPLu9J4LPDJxT65eQiw7merabasmbxvdiMJmWh9ppamwqY2Uy3z1prBKfVJAQmhBo2a9kdYGmuU");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    char str_pub_ckd[] = "dgub8kXBZ7ymNWy2SDyf2FW3u9Y29xNHSqXEAdJer8Zh4pXKS61eCFPLByJeX2NyGaNVNXBjMHE9NpXfH4u9JUJKbrRCNFPeJ54gQN9RQTzUNDx";

    dogecoin_hdnode_deserialize(str_pub_ckd, &dogecoin_chainparams_main, &node4);
    r = dogecoin_hdnode_public_ckd(&node4, 124); // double check i >= 2 to the 31st power + 3 = 0x80000000 dogecoin coin_type bip44
    u_assert_int_eq(r, true);
    dogecoin_hdnode_serialize_public(&node4, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "dgub8o73HfBFaVpyuR1D8qzAAmqerNH17DaJTY9afFenUKWKhgiP6eo2DbiUqYS4mMqsnwBMbAyJMbH2acX1H778iUcTUphzR38Ck2rSRgV12Fz");

    r = dogecoin_hdnode_public_ckd(&node4, 0x80000000 + 1); //try deriving a hardened key (= must fail)
    u_assert_int_eq(r, false);

    char str_pub_ckd_tn[] = "tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK";

    dogecoin_hdnode_deserialize(str_pub_ckd_tn, &dogecoin_chainparams_test, &node4);
    r = dogecoin_hdnode_public_ckd(&node4, 124); // double check i >= 2 to the 31st power + 3 = 0x80000000 dogecoin coin_type bip44
    u_assert_int_eq(r, true);
    dogecoin_hdnode_get_p2pkh_address(&node4, &dogecoin_chainparams_test, str, sizeof(str));
    u_assert_str_eq(str, "nbsFtuY3Yxxe1SqcuFCxZc9GXqHERoxTmp");
    size_t size = sizeof(str);
    size_t sizeSmall = 55;
    r = dogecoin_hdnode_get_pub_hex(&node4, str, &sizeSmall);
    u_assert_int_eq(r, false);
    r = dogecoin_hdnode_get_pub_hex(&node4, str, &size);
    u_assert_uint32_eq(size, 66);
    u_assert_int_eq(r, true);
    u_assert_str_eq(str, "0345717c8722bd243ec5c7109ce52e95a353588403684057c2664f7ad3d7065ed5");
    dogecoin_hdnode_serialize_public(&node4, &dogecoin_chainparams_test, str, sizeof(str));
    u_assert_str_eq(str, "tpubD8MQJFN9LVzG9L2CzDwdBRfnyvoJWr8zGR8UrAsMjq89BqGwLQihzyrMJVaMm1WE91LavvHKqfWtk6Ce5Rr8mdPEacB1R2Ln6mc92FNPihs");

    dogecoin_hdnode* nodeheap;
    nodeheap = dogecoin_hdnode_new();
    dogecoin_hdnode* nodeheap_copy = dogecoin_hdnode_copy(nodeheap);

    u_assert_int_eq(memcmp(nodeheap->private_key, nodeheap_copy->private_key, 32), 0);
    u_assert_int_eq(memcmp(nodeheap->public_key, nodeheap_copy->public_key, 33), 0)

        dogecoin_hdnode_free(nodeheap);
    dogecoin_hdnode_free(nodeheap_copy);
}
