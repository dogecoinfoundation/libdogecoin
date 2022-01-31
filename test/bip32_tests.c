/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2022 bluezr                                          *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <dogecoin/bip32.h>

#include "utest.h"
#include <dogecoin/utils.h>

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
    memcpy(private_key_master,
           utils_hex_to_uint8("26579cdad83446e4a94432d8f466eeac5ee4250aa995f60eb6a5e7f656ce6685"),
           32);
    memcpy(chain_code_master,
           utils_hex_to_uint8("f0fb73b8dcdd948eeaaf032dfbbdea9de00531b0bb08b01d9a510c0925edb5fb"),
           32);
    u_assert_int_eq(node.fingerprint, 0x00000000);
    u_assert_mem_eq(node.chain_code, chain_code_master, 32);
    u_assert_mem_eq(node.private_key, private_key_master, 32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("0310e54bc631afc4f49d9d13f05634a68e07fd7946ef12ee72dbf6a922e72f82bf"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv51eADS3spNJhAKoM6jzKhn2tZnKNSZrkiVat2yHEUrRv8Xmuav8rC8b4uPDRBmxZGNTesizGZPRtF3xuPo7uxfFfDJaCn8u9EC3ErkuvEDt");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));

    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str, "DCyYWHFbDs8Q5yQiW91CAM5D18T42ZqMZZ");

    dogecoin_hdnode_serialize_public(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8kXBZ7ymNWy2TBTW8ycHpdaV2Lh1GgtU16N6C5gDGyUvgatbzggcqRziUdvdHi78yJj2dKvDs7LMbC4YbWxf9Ew6rV6C9bL89Pn2RdJDTmo");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    /* [Chain m/0'] */
    char path0[] = "m/0'";
    dogecoin_hd_generate_key(&node, path0, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x55F2859B);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("77fc3eeb5c334dbf7ded8d4e9a0260a2ee500ddcdf8346a79f5e0f19907e87d6"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("54794f42d7e75488962ec2fcf42e71d1645eec0ab6bbd47209a9ed1755687aa1"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("035f66dcaed6ec1cfe060260131e9046e06b246c0c4ab731f7a21c9bd7c1a3e060"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv549wD3hUpZWPRJU1dxTzxEGoWM19qRP2vzYkXSoaztubE9mq9bBqoXEcCSu8crjSMWzVTNxbRNk8qNJmBixSttSQENKrB3AoY2H26io5BFG");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));

    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str, "DHyraHdYd9dhfnq78S6gQZ5bNqY86besZL");

    dogecoin_hdnode_serialize_public(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8o2xYjdNNiAiiA8AgC5y55pPxuNnfYQkDbKxgZCZo1xbnCtXZMjcSpeFmhrbgdxvdizSudME3kPco8KYxspzxhPvRBxuT7f9J5XcExpJ7v9");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    /* [Chain m/0'/3] */
    char path1[] = "m/0'/3";
    dogecoin_hd_generate_key(&node, path1, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x8CDA3BEF);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("77f86e8795d8e656da6e3fe903c525d1026df9dc8356615ff85f60291971edc4"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("a04bc87762984d25f100a6137355cf5feabf05a75a15e3579d4bf04cba7b25fe"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("02cc26191f66d326d606e44e74d7efa6e122c272fbf6355a39a4a2291ebd1e8c73"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv56SUejy9T8BJSZnZ69suNfXa2Gt2uaEA1sryN31Attt8kq1xU3r7Q8iVxRJu7iqwhtQCoqFHfX4tdcNpzpHQrW3rLWyZLgSWnCU2DjSsJnQ");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str, "D9cgYh1DH9rbemXmSe6of7yDQzcTQiJxbr");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8qKVzRu31GqdjRSi8PVsVX5AUqFfjhFsJUeBX9Q9h1w9Jt8espPt3S89Xea82Vy5CdVhjiqT5WD5mtDw5Fv3oHXuJERXUKWX6uTobBnvzRw");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/1'] */
    char path2[] = "m/0'/3/1'";
    dogecoin_hd_generate_key(&node, path2, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x31181F41);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("0c2e988491481c8872dba4b0330f59bda258d639c64bcddbd1a7b023a8d8f3d0"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("08788bb29c924038ad2a086625f7e7863991c01b9a47d2a9fa78fc45c7740166"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("020f079509811e6f761e4cae276bd6439afca7e1c54b4c3ff0b38659dd9354a0cc"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv57eVhEh8z7kT4sRAaMm6im3zKCAnJLeDUoVQiaUb26PK7SfJq14SvUdM93T93NrUDjdVv4mMQ4iPHgL9pb5w64QswKLuuQDKsmMfTjEC8No");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str, "DEF9ND2VNGuSWVG6BDjHxnNfrGKqjJRL1f");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8rXX2vd2YGQnMj5KcbP4qcbamkYR8TfvmQGcsgsZpDSKfVn1EmcDZn2ziGRwKozMH6wCKytW4kLnDyajZqv16HkCFjcGb9VJ4yNeArXG3bY");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/2'/2] */
    char path3[] = "m/0'/3/2'/2";
    dogecoin_hd_generate_key(&node, path3, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x3FA35903);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("189eb3d2fa48385353b96b1dad719a5b1609dac0e1d6e21013f2def5d6d7f8a6"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("7182493c596486f7d53e01da452f462df9bb1dfda94adb59a318d34a4ccd98d0"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("036596d78fbc4215ecb5766f86e9f90b1d0aff791dba4c7f1847eb7c543ac201b5"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv59dqAUrme8jeXMNd3v13YF3wf4SF3Nq6Y5K2rWRAEX6WdrNCD8LbqFc7YxYerHDBbF1ya3FNnMTL2CgMmd5RxkMhdksDZe2yyP1ZSXDTzkj");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str, "DNRckMf7oj82MUq3Z9RBudafp4UcxkMjhe");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8tWrWAnfCHPypD2n69d1f6bY7cossVropg6F1cp92e9XBuUtcttNUZ1m8DL4JYTXsXXGNh3yV3bBosdiGNzEinK4h1qhhj67gxnSXpfY4UT");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/2'/2/1000000000] */
    char path4[] = "m/0'/3/2'/2/1000000000";
    dogecoin_hd_generate_key(&node, path4, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0xBD9A02B5);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("dd8c3e697cac34e85a4eca184675549b07d2cb76545d363045c502870152a120"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("09c678dcf8e2ac2814a3b0942c783adfa221c651bcebdf0afda7bcfaf4398336"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("033eba6b0b81fee7b20fc744691138cf1a84764478a6e6f8b2ad81067d7f009544"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str, "dgpv5CSfbdmi4KVmECkNMWhpzikcPDxKNvSQLd2qR7ySFiCXjq8Yi91QMoR2JMYdVW5q4q19j5a9msjSQpuB9YdpRHh8ZAxiKUMHoWFKMoo95k1");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str, "D6DzSJ5kdcYUMXz9PPc5jY6xq8R8aDZ13K");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8wKgwKhbcUA6X4QXPkKo7aJCqnKxD3U7dDp3aENR3qFYHtFF7uZB16pfscpc2difrhLqyhiCyUHbnqBawbuFUJUG7ewRMiwstBj5AWLg5bV");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chain_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    char str_pub_ckd[] = "dgub8ni9FJceneQCtAfhezgYfKrYfo1P959gmrhEiD2xxuEZYxhUNxm1f19Gg2EsiZG33ywfxcahMBvAe69gj9h4xad7b7eMWrXZniiB9PBXEdb";

    r = dogecoin_hdnode_deserialize(str_pub_ckd, &dogecoin_chain_main, &node4);
    r = dogecoin_hdnode_public_ckd(&node4, 123);
    u_assert_int_eq(r, true);
    dogecoin_hdnode_serialize_public(&node4, &dogecoin_chain_main, str, sizeof(str));
    u_assert_str_eq(str, "dgub8pbH768fzx5CkM6iJdDqTfrr53Df8wBa7H6jV5N7A4fLjnrhakh99VwCFkXiHLP8sj5jhnn9AfuzFCxecGo8uaigfUjtUJtcHgo9NAWN9yT");


    r = dogecoin_hdnode_public_ckd(&node4, 0x80000000 + 1); //try deriving a hardened key (= must fail)
    u_assert_int_eq(r, false);


    char str_pub_ckd_tn[] = "dgub8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK";

    r = dogecoin_hdnode_deserialize(str_pub_ckd_tn, &dogecoin_chain_test, &node4);
    r = dogecoin_hdnode_public_ckd(&node4, 123);
    u_assert_int_eq(r, true);
    dogecoin_hdnode_get_p2pkh_address(&node4, &dogecoin_chain_test, str, sizeof(str));
    u_assert_str_eq(str, "nY1EsYedSx8tnLAjDpMzgMgrUW245HYXZM");
    size_t size = sizeof(str);
    size_t sizeSmall = 55;
    r = dogecoin_hdnode_get_pub_hex(&node4, str, &sizeSmall);
    u_assert_int_eq(r, false);
    r = dogecoin_hdnode_get_pub_hex(&node4, str, &size);
    u_assert_int_eq(size, 66);
    u_assert_int_eq(r, true);
    u_assert_str_eq(str, "000000000000000000000000000000000000000000000000000000000000000000");
    dogecoin_hdnode_serialize_public(&node4, &dogecoin_chain_test, str, sizeof(str));
    u_assert_str_eq(str, "tpubD8ZxBnPjxpzEyizBotrgS9tzkcxY4tWEH361VT5szbCik3xiWX5k8PjQYypfs5oor9xh8JQGHHYGKYLGZGtZrCDrd1z9g7Qh261yBtj6dQa");

    dogecoin_hdnode *nodeheap;
    nodeheap = dogecoin_hdnode_new();
    dogecoin_hdnode *nodeheap_copy = dogecoin_hdnode_copy(nodeheap);

    u_assert_int_eq(memcmp(nodeheap->private_key, nodeheap_copy->private_key, 32), 0);
    u_assert_int_eq(memcmp(nodeheap->public_key, nodeheap_copy->public_key, 33), 0)

    dogecoin_hdnode_free(nodeheap);
    dogecoin_hdnode_free(nodeheap_copy);
}
