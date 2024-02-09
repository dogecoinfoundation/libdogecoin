/**********************************************************************
 * Copyright (c) 2015 Jonas Schnelli                                  *
 * Copyright (c) 2023 bluezr                                          *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#include <test/utest.h>

#include <dogecoin/bip32.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

void test_bip32()
{
    dogecoin_hdnode node, node2, node3, node4;
    char str[HDKEYLEN];
    int r;
    uint8_t private_key_master[32];
    uint8_t chain_code_master[32];

    /* init m */
    dogecoin_hdnode_from_seed(utils_hex_to_uint8("000102030405060708090a0b0c0d0e0f"), 16, &node);

    /* [Chain m] */
    memcpy_safe(private_key_master,
           utils_hex_to_uint8("e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35"),
           32);
    memcpy_safe(chain_code_master,
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
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    /* [Chain m/0'] */
    char path0[] = "m/0'";
    dogecoin_hd_generate_key(&node, path0, private_key_master, 0, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x3442193E);
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
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));


    /* [Chain m/0'/3] */
    char path1[] = "m/0'/3";
    dogecoin_hd_generate_key(&node, path1, private_key_master, 0, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x5C1BD648);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("c354182c136d7c9efc2f215b963b667d8729e6eb6e8f6605929b6771aace080e"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("600f941dbe89353aa2de75a277ff798baf59b7182ac768c73465d903a48a3ca4"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("03d49eb33036d51506ce69f47aa2f0169473b2b5041f9c01b0dc35e370ddba14af"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv565hQvuEJLJkEwqYSh7ragz3PxZi9TmYwM7Kz3SnvzSGZ6p4ZhN2XThrntCGgrXiAqSGr2j3zrkeZFJy6ZpWaW3hYLWZrivDmSHvvrZ4HsH");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DFebVDmXTLVkYAjyxtmFm1CNxU3Ryyyd37");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8pxikcq7rUy5XoVhUvjphYXdrWwLyaoGDwtY99qmj7VH79vkyTuoAm7WN9xFxYmGeAAvhUU7bxWEnfET5CtXLGyexchcd3L7RLkRB2dbWEY");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/2'] */
    char path2[] = "m/0'/3/2'";
    dogecoin_hd_generate_key(&node, path2, private_key_master, 0, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x73457C28);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("8f96add0124b936f0066645b8805a9ddc072155be93f2d5c12ab950e4f678611"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("f02825d27affb0cb4bd153caf143f136ee9e83750ba7ea59e977977d58abc664"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("0229aada88e20faf9f3034fe585ea890927d606b58d5260e76052dae2ca34212ac"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv588hzXFx2YuJm5k5VgFBuQyozYy1MyimkRTgK5z74aYnJtT85Ssd4NS2VPwyyLihQZtVjAE9r3RhXMpB9HSGjErQmSepMSH97PxV83YeaVB");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DFi6KKPPxybRccEte1vU81k84bLVUDiA1A");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8s1jLDBqahZe3wQEXusA2GXQT7LeC6kV32EtUCP5rhbnrwZpVDRPhfqg4bNUaKeX1PWrFnkecizH6bHZ2LLn1PUhtyHd7Rm6ebmG1CziQeF");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/2'/2] */
    char path3[] = "m/0'/3/2'/2";
    dogecoin_hd_generate_key(&node, path3, private_key_master, 0, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x73EECC5F);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("d7dff9258e4d978a05d10a9dfecf8d75c8a862f36bb88c97802a1a78c76cdbe9"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("2b52150da379378764224292eab5c9e7fa1969319d448a681c988b73674cca1b"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("03ceeff91ded2cb2f7a23fdc6f6696542b74d5e4aa0630f7c73beeba55310ca7b7"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgpv5A28CoFfaN2Hy7PM3RAMAJ15kfF2gh6snTiVcmg4icJMYP2FQNipkbWFyTaGey2MgGoyMLhYEM4tTFP4DCfsU9Y8J2ZNpyBhZaGWBQ4s3za");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DB1haVYdHwactwxFAsKwu8iayAucR2pX8F");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8tu9YVBZ8WgdFy3W5enKH9YgDDcfWp8b54Vhmt53WjMN6S8wp9GbPtuuYjgyx9SQSmQr2JRXM6Eag6Qje9B6sZEPDeozr7GAupe4cqNsCPo");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    memcpy_safe(&node3, &node, sizeof(dogecoin_hdnode));
    dogecoin_mem_zero(&node3.private_key, 32);
    u_assert_mem_eq(&node2, &node3, sizeof(dogecoin_hdnode));

    /* [Chain m/0'/3/2'/2/1000000000] */
    char path4[] = "m/0'/3/2'/2/1000000000";
    dogecoin_hd_generate_key(&node, path4, private_key_master, 0, chain_code_master, false);
    u_assert_int_eq(node.fingerprint, 0x406AACFA);
    u_assert_mem_eq(node.chain_code,
                    utils_hex_to_uint8("4c77a7bf5d73e6b8904ac92aaee3c46b8be0669573d4c7836e90b3829fae997f"),
                    32);
    u_assert_mem_eq(node.private_key,
                    utils_hex_to_uint8("f4aa045644402693d3244c8baf39e27f73043a363ccc1dd4a559ea072920da54"),
                    32);
    u_assert_mem_eq(node.public_key,
                    utils_hex_to_uint8("02b833646327a7fc33aea8bcbe80e18593826ff3a8676acfd531768d634b9812f5"),
                    33);
    dogecoin_hdnode_serialize_private(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "dgpv5BXJGu5DrBfYiR59ryD5vFsrHd2FUBgkCcYpk2VtXtbvbVdv7HduJhjciTh5w4E8X5FWNhMdiNm4NepsVpuwgwjC5tvoK3ZjEwq9ws2bdKm");
    r = dogecoin_hdnode_deserialize(str, &dogecoin_chainparams_main, &node2);
    u_assert_int_eq(r, true);
    u_assert_mem_eq(&node, &node2, sizeof(dogecoin_hdnode));
    dogecoin_hdnode_get_p2pkh_address(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str, "DTtvgptdaV3hh2vshVx7oBYXZQFVmUMBgV");
    dogecoin_hdnode_serialize_public(&node, &dogecoin_chainparams_main, str, sizeof(str));
    u_assert_str_eq(str,
                    "dgub8vQKcb17QLKt1GjJuCq437RSkBPtJJiTVDL2u8tsL1ew9YkcX4Bfx19GHgANE597nwSs8ugp4qWRjbVa9fiHLL9ife3DkTSiz3x2QW22QpW");
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

    SEED seed = {0};
    memcpy(seed, utils_hex_to_uint8("3779b041fab425e9c0fd55846b2a03e9a388fb12784067bd8ebdb464c2574a05bcc7a8eb54d7b2a2c8420ff60f630722ea5132d28605dbc996c8ca7d7a8311c0"), 64);
    const int seed_len = 64;
    char masterkey[HDKEYLEN];
    char masterpubkey[HDKEYLEN];
    char extkey[HDKEYLEN], extkey2[HDKEYLEN];
    char extpubkey[HDKEYLEN], extpubkey2[HDKEYLEN];

    u_assert_true(getHDRootKeyFromSeed(seed, seed_len, true, masterkey));
    u_assert_str_eq(masterkey,
                    "tprv8ZgxMBicQKsPdM3GJUGqaS67XFjHNqUC8upXBhNb7UXqyKdLCj6HnTfqrjoEo6x89neRY2DzmKXhjWbAkxYvnb1U7vf4cF4qDicyb7Y2mNa");

    u_assert_true(getHDPubKey(masterkey, true, masterpubkey));
    u_assert_str_eq(masterpubkey,
                    "tpubD6NzVbkrYhZ4Wp54C7wRyqkE6HFDYAf6iDRJUDQtXkLEoot6q7usxxHi2tGW48TfY783vGoZ3ufE5XH9YP86c7X6G3CjMh8Dua1ZTTWyjSa");

    char keypath[KEYPATHMAXLEN] = "m/44'/1'/0'";
    u_assert_true(deriveExtKeyFromHDKey(masterkey, keypath, true, extkey));
    u_assert_str_eq(extkey,
                    "tprv8fRGXa8U2TD8awz5go6QPzPkP2wxabQip9Tmm7qNnuGhqJah42dmffvKCer9tZzS3U23n1jmaxDLFWPVGR2qLbS3Ec9Nskr7eKYBBZGy9N8");

    u_assert_true(getHDPubKey(extkey, true, extpubkey));
    u_assert_str_eq(extpubkey,
                    "tpubDC7JfzAiAptoUR1saSkzoQ3rx4TtjvbdPT4Z3dsgDB56fnqTgRTMrAYBNooSmtJgpxUL7U5wvRtXDkKAx3qnPP33qNdcojAWVSK914c2Kd2");

    char keypath1[KEYPATHMAXLEN] = "m/44'/1'/0'/0";
    u_assert_true(deriveExtKeyFromHDKey(masterkey, keypath1, true, extkey2));
    u_assert_str_eq(extkey2,
                    "tprv8hJrzKEmbFfBx44tsRe1wHh25i5QGztsawJGmxeqryPwdXdKrgxMgJUWn35dY2nrYmomRWWL7Y9wJrA6EvKJ27BfQTX1tWzZVxAXrR2pLLn");

    u_assert_true(getHDPubKey(extkey2, true, extpubkey2));
    u_assert_str_eq(extpubkey2,
                    "tpubDDzu8jH1jdLrqX6gm5JcLhM8ejbLSL5nAEu44Uh9HFCLU1t6V5mwro6NxAXCfR2jUJ9vkYkUazKXQSU7WAaA9cbEkxdWmbLxHQnWqLyQ6uR");

    char keypath2[KEYPATHMAXLEN] = "m/0";
    u_assert_true(deriveExtPubKeyFromHDKey(extpubkey, keypath2, true, extpubkey2));
    u_assert_str_eq(extpubkey2,
                    "tpubDDzu8jH1jdLrqX6gm5JcLhM8ejbLSL5nAEu44Uh9HFCLU1t6V5mwro6NxAXCfR2jUJ9vkYkUazKXQSU7WAaA9cbEkxdWmbLxHQnWqLyQ6uR");


    u_assert_true(getHDRootKeyFromSeed(seed, seed_len, false, masterkey));
    u_assert_str_eq(masterkey,
                    "dgpv51eADS3spNJh8Q6Qzz9L6AABYtvRnEGqvSFKXrxzhwK54fWAHo3NrGHvagERrteHgWo6rifQ9GxswUAYR4Exh9zekTNz6FUhJFAjhae3ptn");

    u_assert_true(getHDPubKey(masterkey, false, masterpubkey));
    u_assert_str_eq(masterpubkey,
                    "dgub8kXBZ7ymNWy2RFka3DmJD1hn1TJ4cMJZD32XgyMyW4N5cicrhZb9VZha9vghubutjw72kayz7p38Q56ZiFvbiVvME8rwCSF6gChmQt4RgLZ");

    char keypath3[KEYPATHMAXLEN] = "m/0'";
    u_assert_true(deriveExtKeyFromHDKey(masterkey, keypath3, false, extkey));
    u_assert_str_eq(extkey,
                    "dgpv53Zmoqj7kNTYigGq7A5cTsfDt1yXAuKdCzUCEdBdqPKoM29mg1zhk11b8iSzH2H6ot8eVY9Bdh3dVsqbEeiXU6Exfm3u3ZkwZ3bbYthRE3V");

    u_assert_true(getHDPubKey(extkey, false, extpubkey));
    u_assert_str_eq(extpubkey,
                    "dgub8nSo9Xf1JX7t1Xvz9PhaajCpLaMA12MLVbFQPjacdWNou5GU5nYUPJREhzWzRqENdG4nrxckhMEv4sKFEiLbm4pkVvDjBP7SrQZeStHNMEQ");

    char keypath4[KEYPATHMAXLEN] = "m/0'/0";
    u_assert_true(deriveExtKeyFromHDKey(masterkey, keypath4, false, extkey2));
    u_assert_str_eq(extkey2,
                    "dgpv55f5vS3Ct4dEo6Z5oZSbdtdvCXPPttTuBCg4DjQfJWx6xoiSm2RMcRuJendsxU5XZRcBCUTPLwKnydME4rWxLXxECUGrnVaHW2wHP53CfBG");

    u_assert_true(getHDPubKey(extkey2, false, extpubkey2));
    u_assert_str_eq(extpubkey2,
                    "dgub8pY7G7y6SDHa5xDEqo4ZkkBWf5m2j1VcToTGNqoe6e17Wrq9Any8FjJxE2hnwdBYM19gxkaA7VWqE2Vw6hGHTeac2neLCHzLR8VojVsdqnA");

    char keypath5[KEYPATHMAXLEN] = "m/0";
    u_assert_true(deriveExtPubKeyFromHDKey(extpubkey, keypath5, false, extpubkey2));
    u_assert_str_eq(extpubkey2,
                    "dgub8pY7G7y6SDHa5xDEqo4ZkkBWf5m2j1VcToTGNqoe6e17Wrq9Any8FjJxE2hnwdBYM19gxkaA7VWqE2Vw6hGHTeac2neLCHzLR8VojVsdqnA");
}
