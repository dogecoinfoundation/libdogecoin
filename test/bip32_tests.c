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
           utils_hex_to_uint8("e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35"),
           32);
    memcpy(chain_code_master,
           utils_hex_to_uint8("873dff81c02f525623fd1fe5167eac3a55a049de3d314bb42ee227ffed37d508"),
           32);
    u_assert_int_eq(node.fingerprint, 0x00000000);
    u_assert_mem_eq(node.chain_code, chain_code_master, 32);
    u_assert_mem_eq(node.private_key, private_key_master, 32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("0339a36013301597daef41fbe593a02cc513d0b55527ec2df1050e2e8ff49c85c2"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv51eADS3spNJh9Gjth94XcPwAczvQaDJs9rqx11kvxKs6r3Ek8AgERHhjLs6mzXQFHRzQqGwqdeoDkZmr8jQMBfi43b7sT3sx3cCSk5fGeUR");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));

    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "D9uQrqyJ7Guz3aHVTTcxVhNnthobME3o4w");

    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8kXBZ7ymNWy2S8Q3jNgVjFUm5ZJ3QLLaSTdAA89ukSv7Q6MSXwE14b7Nv6eDpE9JJXinTKc8LeLVu19uDPrm5uJuhpKNzV2kAgncwo6bNpP");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    /* [Chain m/0'] */
    char path0[] = "m/0'";
    dogecoin_hd_generate_key(&node, path0, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x3442193e);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("47fdacbd0f1097043b78c63c20c34ef4ed9a111d980047ad16282c7ae6236141"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("edb2e14f9ee77d26dd93b4ecede8d16ed408ce149b6cd80b0715a2d911a0afea"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("035a784662a4a20a65bf6aab9ae98a6c068a81c52e4b032c0fb5400c706cfccc56"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv53uaD9MLudRgHssbttwAVS3GwpUkxHnsqUGqy793vX4PDKXvYQDKYS4988T7QEnCzUt7CaGi21e6UKoZnKgXyjna7To1h1aqkcqJBDM65ur");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));

    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DDY844NizrLNz8TLRu7t76XHeeK8MwFJ9Z");

    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8nnbYqHETn61ajXkw8Z8cHasQNrPnQpb85448DY2ie7PmNecxAm6BjTnhNCvZY3qJk1MKZ9Z5HQasQ83ARb99nmduT7dunvxgcvBFVHuvrq");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    /* [Chain m/0'/1] */
    char path1[] = "m/0'/1";
    dogecoin_hd_generate_key(&node, path1, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x5c1bd648);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("2a7857631386ba23dacac34180dd1983734e444fdbf774041578e9b6adb37c19"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("3c6cb8d0f6a264c91ea8b5030fadaa8e538b020f0a387421a12de9319dc93368"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("03501e454bf00751f24b1b489aa925215d66af2234e3891c3b21a52bedb3cd711c"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv565hQvuEJLJk8Kv3d9q36Avw1CTrxKXAmnwgZNurs9rbSs34GCddVzxNYBeB1AZFSZdo1Ps96ibWcGKnufUWkuH1dEkjkmMhRR9fi7Po6B2");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DNYoBqYyh3FNWSPMb9k3drRd3wtpwEwrV2");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8pxikcq7rUy5RBaCfPT1D2UXTkqVnSYt4PitiVJqfGubzv9kfyBQ9JN27SfVyUmBGTdQ6ybfBsu4Thrrdkm2qSbaCexVPRwEKMSxYLP2A41");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/1/2'] */
    char path2[] = "m/0'/1/2'";
    dogecoin_hd_generate_key(&node, path2, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0xbef5a2f9);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("04466b9cc8e161e966409ca52986c584f07e9dc81f735db683c3ff6ec7b1503f"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("cbce0d719ecf7431d88e6a89fa1483e02e35092af60c042b1df2ff59fa424dca"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("0357bfe1e341d01c69fe5654309956cbea516822fba8a601743a012a7896ee8dc2"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv58gyTTj61DA9zVi8skEQTAy5EMLPDs7A7LBMoiD232E2riEB4xU4QSWJ6DrnyQ4jx2fBbrp4X8RQqU4YVgPhszifyrKHuhbe2gttLnRB4a6");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DSt4Nr6nsyR5E1JRk4VcPwDqH2qTGw9N6h");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8sZzo9eyZMpVHMNHuyrNa2Wfgui23z8sPvxZxpbzq9H3QmLsUj1q3juwfTrLRMCVcyj8iMaGZpU2v319LrJZttkQnYvdUNzv33N6dcqeZ8X");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/1/2'/2] */
    char path3[] = "m/0'/1/2'/2";
    dogecoin_hd_generate_key(&node, path3, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0xee7ab90c);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("cfb71883f01676f587d023cc53a35bc7f88f724b1f8c2892ac1275ac822a3edd"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("0f479245fb19a38a1954c5c7c0ebab2f9bdfd96a17563ef28a6a4b1a2a764ef4"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("02e8445082a72f29b75ca48748a914df60622a609cacfce8ed0e35804560741d29"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv5AvNHtr3Bgq94yBra1SVLg8PKAd7rTRMYp4f4fjVMTneDorY8jARc1yDmYGFS4UB1pntDn3dRwsaJexzh6w45PJiP6QPTnRMBfN3rDUiyyH");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DQsrqsa35dByuTfHb6yFt2x26TghzUvKno");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8uoPdamvjqVUMpr1cF4TTXfymizkgaT4qQqsDn8U9aqemryEYViCFKNsLnqiq9ME6HrJrN4DcZN9UTM9S9jmcVDfhLUpJZtk3jGwnGkhd8u");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/1/2'/2/1000000000] */
    char path4[] = "m/0'/1/2'/2/1000000000";
    dogecoin_hd_generate_key(&node, path4, private_key_master, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0xd880d7d8);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("c783e67b921d2beb8f6b389cc646d7263b4145701dadd2161548a8b078e65e9e"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("471b76e389e528d6de6d816857e012c5455051cad6660850e58372a6c3e6e7c8"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("022a471424da5e657499d1ff51cb43c47481a03b1e77f951fe64cec9f5a48f7011"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "dgpv5Ce8maTHJpDLbJyJgZ1DeP8P7QCRfxEPM4TDJx7dZYB8vwFvf9R5s88HQQ3TLybFdEC9192aGzQhJpyNEAwnCLxFibAcahB4TzvQbJyp2im");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DQhpP7kTKhAhbr2sk4L7wjMRMDtmhtKA7G");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8wXA7GPArxsftAdTindBmEfyZxa4W5G6dfERU4WcMfE9UzNd4uxrWRXvyckfgQRwZz8rMhz29m4k4skAY1EcTkNnZstu73UNrgts2MA5evC");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy(&node3, &node, sizeof(dogecoin_hdnode));
    memset(&node3.private_key, 0, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    char str_pub_ckd[] = "dgub8ni9FJceneQCtAfhezgYfKrYfo1P959gmrhEiD2xxuEZYxhUNxm1f19Gg2EsiZG33ywfxcahMBvAe69gj9h4xad7b7eMWrXZniiB9PBXEdb";

    r = dogecoin_hdnode_deserialize(str_pub_ckd, &dogecoin_chainparams_main, &node4);
    r = dogecoin_hdnode_public_ckd(&node4, 123);
    u_assert_int_eq(r, true);
    dogecoin_hdnode_serialize_public(&node4, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "dgub8pbH768fzx5CkM6iJdDqTfrr53Df8wBa7H6jV5N7A4fLjnrhakh99VwCFkXiHLP8sj5jhnn9AfuzFCxecGo8uaigfUjtUJtcHgo9NAWN9yT");


    r = dogecoin_hdnode_public_ckd(&node4, 0x80000000 + 1); //try deriving a hardened key (= must fail)
    u_assert_int_eq(r, false);


    char str_pub_ckd_tn[] = "tprv8ZgxMBicQKsPd7Uf69XL1XwhmjHopUGep8GuEiJDZmbQz6o58LninorQAfcKZWARbtRtfnLcJ5MQ2AtHcQJCCRUcMRvmDUjyEmNUWwx8UbK";

    r = dogecoin_hdnode_deserialize(str_pub_ckd_tn, &dogecoin_chainparams_test, &node4);
    r = dogecoin_hdnode_public_ckd(&node4, 123);
    u_assert_int_eq(r, true);
    dogecoin_hdnode_get_p2pkh_address(&node4, &dogecoin_chainparams_test, str, sizeof(str));
    u_assert_str_eq(str, "ncjhiYnSD1pUiD2t2DWXezjZZww5H76n3P");
    size_t size = sizeof(str);
    size_t sizeSmall = 55;
    r = dogecoin_hdnode_get_pub_hex(&node4, str, &sizeSmall);
    u_assert_int_eq(r, false);
    r = dogecoin_hdnode_get_pub_hex(&node4, str, &size);
    u_assert_int_eq(size, 66);
    u_assert_int_eq(r, true);
    u_assert_str_eq(str, "0391a9964e79f39cebf9b59eb2151b500bd462e589682d6ceebe8e15970bfebf8b");
    dogecoin_hdnode_serialize_public(&node4, &dogecoin_chainparams_test, str, sizeof(str));
    u_assert_str_eq(str, "tpubD8MQJFN9LVzG8pktwoQ7ApWWKLfUUhonQkeXe8gqi9tFMtMdC34g6Ntj5K6V1hdzR3to2z7dGnQbXaoZSsFkVky7TFWZjmC9Ez4Gog6ujaD");

    dogecoin_hdnode *nodeheap;
    nodeheap = dogecoin_hdnode_new();
    dogecoin_hdnode *nodeheap_copy = dogecoin_hdnode_copy(nodeheap);

    u_assert_int_eq(memcmp(nodeheap->private_key, nodeheap_copy->private_key, 32), 0);
    u_assert_int_eq(memcmp(nodeheap->public_key, nodeheap_copy->public_key, 33), 0)

    dogecoin_hdnode_free(nodeheap);
    dogecoin_hdnode_free(nodeheap_copy);
}
