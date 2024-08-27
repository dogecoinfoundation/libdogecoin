/*

 The MIT License (MIT)

 Copyright (c) 2023 bluezr
 Copyright (c) 2023 edtubbs
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

#include <assert.h>
#ifdef HAVE_CONFIG_H
#  include "libdogecoin-config.h"
#endif
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#include <getopt.h>
#endif

#include <dogecoin/address.h>
#include <dogecoin/bip32.h>
#include <dogecoin/bip44.h>
#include <dogecoin/constants.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/key.h>
#include <dogecoin/random.h>
#include <dogecoin/seal.h>
#include <dogecoin/sha2.h>
#include <dogecoin/base58.h>
#include <dogecoin/tool.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

/**
 * @brief This function generates a new basic public-private
 * key pair for the specified network.
 *
 * @param wif_privkey The generated private key.
 * @param p2pkh_pubkey The generated public key.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * @return 1 if the key pair was generated successfully.
 */
int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)
{
    /* internal variables */

    char wif_privkey_internal[PRIVKEYWIFLEN]; //MLUMIN: Keylength (51 or 52 chars, depending on uncompressed or compressed) +1 for string termination 'internally'?
    char p2pkh_pubkey_internal[P2PKHLEN]; //MLUMIN: no magic numbers. p2pkh address should be 34 characters, +1 though for string termination 'internally'?
    size_t privkey_len = sizeof(wif_privkey_internal);


    /* if nothing is passed in use internal variables */
    if (wif_privkey) {
        memcpy_safe(wif_privkey_internal, wif_privkey, sizeof(wif_privkey_internal));
    }
    if (p2pkh_pubkey) {
        memcpy_safe(p2pkh_pubkey_internal, p2pkh_pubkey, sizeof(p2pkh_pubkey_internal));
    }

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* generate a new private key */
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    dogecoin_privkey_gen(&key);
    dogecoin_privkey_encode_wif(&key, chain, wif_privkey_internal, &privkey_len);

    /* generate a new public key and export hex */
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    assert(dogecoin_pubkey_is_valid(&pubkey) == 0);
    dogecoin_pubkey_from_key(&key, &pubkey);

    /* export p2pkh_pubkey address */
    dogecoin_pubkey_getaddr_p2pkh(&pubkey, chain, p2pkh_pubkey_internal);

    if (wif_privkey) {
        memcpy_safe(wif_privkey, wif_privkey_internal, sizeof(wif_privkey_internal));
    }
    if (p2pkh_pubkey) {
        memcpy_safe(p2pkh_pubkey, p2pkh_pubkey_internal, sizeof(p2pkh_pubkey_internal));
    }

    /* reset internal variables */
    dogecoin_mem_zero(wif_privkey_internal, strlen(wif_privkey_internal));
    dogecoin_mem_zero(p2pkh_pubkey_internal, strlen(p2pkh_pubkey_internal));

    /* cleanup memory */
    dogecoin_pubkey_cleanse(&pubkey);
    dogecoin_privkey_cleanse(&key);
    return true;
}

/**
 * @brief This function generates a new master public-private
 * key pair for a hierarchical deterministic wallet on the
 * specified network.
 *
 * @param hd_privkey_master The generated master private key.
 * @param p2pkh_pubkey_master The generated master public key.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * @return 1 if the key pair was generated successfully.
 */
int generateHDMasterPubKeypair(char* hd_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)
{
    char hd_privkey_master_local[HDKEYLEN];
    char hd_pubkey_master[P2PKHLEN];

    /* if nothing is passed use internal variables */
    if (hd_privkey_master) {
        memcpy_safe(hd_privkey_master_local, hd_privkey_master, sizeof(hd_privkey_master_local));
    }
    if (p2pkh_pubkey_master) {
        memcpy_safe(hd_pubkey_master, p2pkh_pubkey_master, sizeof(hd_pubkey_master));
    }

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* generate a new hd master key */
    if (!hd_gen_master(chain, hd_privkey_master_local, sizeof(hd_privkey_master_local))) {
        return false;
    }

    if (!generateDerivedHDPubkey(hd_privkey_master_local, hd_pubkey_master)) {
        return false;
    }

    if (hd_privkey_master) {
        memcpy_safe(hd_privkey_master, hd_privkey_master_local, sizeof(hd_privkey_master_local));
    }
    if (p2pkh_pubkey_master) {
        memcpy_safe(p2pkh_pubkey_master, hd_pubkey_master, sizeof(hd_pubkey_master));
    }

    return true;
}

/**
 * @brief This function takes a wif-encoded HD master
 * private key and derive a new HD public key from it
 * on the specified network. This input should come from
 * the result of generateHDMasterPubKeypair().
 *
 * @param hd_privkey_master The master private key to derive the child key from.
 * @param p2pkh_pubkey The resulting child public key.
 *
 * @return 1 if the child key was generated successfully, 0 otherwise.
 */
int generateDerivedHDPubkey(const char* hd_privkey_master, char* p2pkh_pubkey)
{
    /* require master key */
    if (!hd_privkey_master) {
        return false;
    }

    /* determine address prefix for network chainparams */
    const dogecoin_chainparams* chain = chain_from_b58_prefix(hd_privkey_master);

    char str[P2PKHLEN];

    /* if nothing is passed in use internal variables */
    if (p2pkh_pubkey) {
        memcpy_safe(str, p2pkh_pubkey, sizeof(str));
    }

    dogecoin_hdnode* node = dogecoin_hdnode_new();
    dogecoin_hdnode_deserialize(hd_privkey_master, chain, node);

    dogecoin_hdnode_get_p2pkh_address(node, chain, str, sizeof(str));

    /* pass back to external variable if exists */
    if (p2pkh_pubkey) {
        memcpy_safe(p2pkh_pubkey, str, sizeof(str));
    }

    /* reset internal variables */
    dogecoin_hdnode_free(node);
    dogecoin_mem_zero(str, strlen(str));

    return true;
}

/**
 * @brief This function verifies that a given private key
 * matches a given public key and that both are valid on
 * the specified network.
 *
 * @param wif_privkey The private key to check.
 * @param p2pkh_pubkey The public key to check.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * @return 1 if the keys match and are valid on the specified network, 0 otherwise.
 */
int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet) {
    /* require both private and public key */
    if (!wif_privkey || !p2pkh_pubkey) return false;

    /* set chain */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* verify private key */
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    dogecoin_privkey_decode_wif(wif_privkey, chain, &key);
    if (!dogecoin_privkey_is_valid(&key)) return false;

    char new_wif_privkey[PRIVKEYWIFLEN];
    size_t sizeout = sizeof(new_wif_privkey);
    dogecoin_privkey_encode_wif(&key, chain, new_wif_privkey, &sizeout);

    /* verify public key */
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(&key, &pubkey);
    if (!dogecoin_pubkey_is_valid(&pubkey)) return false;

    /* verify address derived matches provided address */
    char* new_p2pkh_pubkey = dogecoin_char_vla(sizeout);
    dogecoin_pubkey_getaddr_p2pkh(&pubkey, chain, new_p2pkh_pubkey);
    if (strcmp(p2pkh_pubkey, new_p2pkh_pubkey))
    {
        free(new_p2pkh_pubkey);
        return false;
    }

    dogecoin_pubkey_cleanse(&pubkey);
    dogecoin_privkey_cleanse(&key);
    free(new_p2pkh_pubkey);
    return true;
}

/**
 * @brief This function verifies that a given HD master
 * private key matches a given HD master public key and
 * that both are valid on the specified network.
 *
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 * @param hd_privkey_master The master private key to check.
 * @param p2pkh_pubkey_master The master public key to check.
 *
 * @return 1 if the keys match and are valid on the specified network, 0 otherwise.
 */
int verifyHDMasterPubKeypair(char* hd_privkey_master, char* p2pkh_pubkey_master, bool is_testnet) {
    /* require both private and public key */
    if (!hd_privkey_master || !p2pkh_pubkey_master) return false;

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* calculate master pubkey from master privkey */
    dogecoin_hdnode node;
    dogecoin_hdnode_deserialize(hd_privkey_master, chain, &node);
    char new_p2pkh_pubkey_master[HDKEYLEN];
    dogecoin_hdnode_get_p2pkh_address(&node, chain, new_p2pkh_pubkey_master, sizeof(new_p2pkh_pubkey_master));

    /* compare derived and given pubkeys */
    if (strcmp(p2pkh_pubkey_master, new_p2pkh_pubkey_master)) return false;

    return true;
}

/**
 * @brief This function takes a public key and does some basic
 * validation to determine if it is a valid Dogecoin address.
 *
 * @param p2pkh_pubkey The address to check.
 * @param len The length of the address in characters.
 *
 * @return 1 if it is a valid Dogecoin address, 0 otherwise.
 */
int verifyP2pkhAddress(char* p2pkh_pubkey, size_t len)
{
    if (!p2pkh_pubkey || !len) return false;
    /* check length */
    unsigned char* dec = dogecoin_uchar_vla(len);
    unsigned char d1[SHA256_DIGEST_LENGTH];
    unsigned char d2[SHA256_DIGEST_LENGTH];
    if (!dogecoin_base58_decode_check(p2pkh_pubkey, dec, len))
    {
        free(dec);
        return false;
    }
    /* check validity */
    sha256_raw(dec, 21, d1);
    sha256_raw(d1, SHA256_DIGEST_LENGTH, d2);
    if (memcmp(dec + 21, d2, 4) != 0)
    {
        free(dec);
        return false;
    }
    free(dec);
    return true;
}

/**
 * @brief This function generates a derived child key from a masterkey using
 * a custom derived path in string format.
 *
 * @param masterkey The master key from which children are derived from.
 * @param derived_path The path to derive an address from according to BIP-44.
 * e.g. m/44'/3'/1'/1/1 representing m/44'/3'/account'/ischange/index
 * @param outaddress The derived address.
 * @param outprivkey The boolean value used to derive either a public or
 * private address. 'true' for private, 'false' for public
 *
 * @return dogecoin_hdnode private key that is converted to WIF that derived child key belongs to
 */
char* getHDNodePrivateKeyWIFByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey) {
    if (!masterkey || !derived_path || !outaddress) {
        debug_print("%s", "missing input\n");
        exit(EXIT_FAILURE);
    }
    size_t wiflen = PRIVKEYWIFLEN;
    char* privkeywif_main = dogecoin_malloc(wiflen);
    const dogecoin_chainparams* chain = chain_from_b58_prefix(masterkey);
    dogecoin_hdnode* hdnode = getHDNodeAndExtKeyByPath(masterkey, derived_path, outaddress, outprivkey);
    dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, chain, privkeywif_main, &wiflen);
    dogecoin_hdnode_free(hdnode);
    return privkeywif_main;
}

/**
 * @brief This function generates a derived child key from a masterkey using
 * a custom derived path in string format.
 *
 * @param masterkey The master key from which children are derived from.
 * @param derived_path The path to derive an address from according to BIP-44.
 * e.g. m/44'/3'/1'/1/1 representing m/44'/3'/account'/ischange/index
 * @param outaddress The derived address.
 * @param outprivkey The boolean value used to derive either a public or
 * private address. 'true' for private, 'false' for public
 *
 * @return dogecoin_hdnode that derived address belongs to
 */
dogecoin_hdnode* getHDNodeAndExtKeyByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey) {
    if (!masterkey || !derived_path || !outaddress) {
        debug_print("%s", "missing input\n");
        exit(EXIT_FAILURE);
    }
    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = chain_from_b58_prefix(masterkey);
    dogecoin_hdnode *node = dogecoin_hdnode_new(),
    *nodenew = dogecoin_hdnode_new();
    dogecoin_hdnode_deserialize(masterkey, chain, node);
    // dogecoin_hdnode_has_privkey
    bool pubckd = !dogecoin_hdnode_has_privkey(node);
    /* derive child key, use pubckd or privckd */
    dogecoin_hd_generate_key(nodenew, derived_path, pubckd ? node->public_key : node->private_key, node->depth, node->chain_code, pubckd);
    if (outprivkey) dogecoin_hdnode_serialize_private(nodenew, chain, outaddress, HDKEYLEN);
    else dogecoin_hdnode_serialize_public(nodenew, chain, outaddress, HDKEYLEN);
    dogecoin_hdnode_free(node);
    return nodenew;
}

/**
 * @brief This function generates a derived child address from a masterkey using
 * a custom derived path in string format.
 *
 * @param masterkey The master key from which children are derived from.
 * @param derived_path The path to derive an address from according to BIP-44.
 * e.g. m/44'/3'/1'/1/1 representing m/44'/3'/account'/ischange/index
 * @param outaddress The derived address.
 *
 * @return 1 if a derived address was successfully generated, 0 otherwise
 */
int getDerivedHDAddressByPath(const char* masterkey, const char* derived_path, char* outaddress) {
    if (!masterkey || !derived_path || !outaddress) {
        debug_print("%s", "missing input\n");
        return false;
    }

    dogecoin_hdnode node;
    bool ret = getDerivedHDNodeByPath(masterkey, derived_path, &node);

    if (ret) {
        const dogecoin_chainparams* chain = chain_from_b58_prefix(masterkey);
        dogecoin_hdnode_get_p2pkh_address(&node, chain, outaddress, P2PKHLEN);
    }

    return ret;
}

/**
 * @brief This function generates a derived child key from a masterkey using
 * a custom derived path in string format.
 *
 * @param masterkey The master key from which children are derived from.
 * @param derived_path The path to derive an address from according to BIP-44.
 * e.g. m/44'/3'/1'/1/1 representing m/44'/3'/account'/ischange/index
 * @param outaddress The derived key.
 * @param outprivkey The boolean value used to derive either a public or
 * private key. 'true' for private, 'false' for public
 *
 * @return 1 if a derived key was successfully generated, 0 otherwise
 */
int getDerivedHDKeyByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey) {
    if (!masterkey || !derived_path || !outaddress) {
        debug_print("%s", "missing input\n");
        return false;
    }

    dogecoin_hdnode node;
    bool ret = getDerivedHDNodeByPath(masterkey, derived_path, &node);

    if (ret) {
        const dogecoin_chainparams* chain = chain_from_b58_prefix(masterkey);
        if (outprivkey) dogecoin_hdnode_serialize_private(&node, chain, outaddress, HDKEYLEN);
        else dogecoin_hdnode_serialize_public(&node, chain, outaddress, HDKEYLEN);
    }

    return ret;
}

bool getDerivedHDNodeByPath(const char* masterkey, const char* derived_path, dogecoin_hdnode *nodenew) {
    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = chain_from_b58_prefix(masterkey);
    bool ret = true;
    dogecoin_hdnode node;

    if (!dogecoin_hdnode_deserialize(masterkey, chain, &node)) {
        ret = false;
    }

    // dogecoin_hdnode_has_privkey
    bool pubckd = !dogecoin_hdnode_has_privkey(&node);
    /* derive child key, use pubckd or privckd */
    if (!dogecoin_hd_generate_key(nodenew, derived_path, pubckd ? node.public_key : node.private_key, node.depth, node.chain_code, pubckd)) {
        ret = false;
    }

    return ret;
}

/**
 * @brief This function generates a derived child address from a masterkey using
 * a BIP44 standardized static, non hardened path comprised of an account, a change or
 * receiving address and an address index.
 *
 * @param masterkey The master key from which children are derived from.
 * @param account The account that the derived address would belong to.
 * @param ischange Boolean value representing either a change or receiving address.
 * @param addressindex The index of the receiving/change address per account.
 * @param outaddress The derived address.
 * @param outprivkey The boolean value used to derive either a public or
 * private address. 'true' for private, 'false' for public
 *
 * @return 1 if a derived address was successfully generated, 0 otherwise.
 */
int getDerivedHDAddress(const char* masterkey, uint32_t account, bool ischange, uint32_t addressindex, char* outaddress, bool outprivkey) {
        if (!masterkey) {
            debug_print("%s", "no extended key\n");
            return false;
        }

        char derived_path[DERIVED_PATH_STRINGLEN];
        int derived_path_size = snprintf(derived_path, sizeof(derived_path), "m/44'/3'/%u'/%u/%u", account, ischange, addressindex);

        if (derived_path_size >= (int)sizeof(derived_path)) {
            debug_print("%s", "derivation path overflow\n");
            return false;
        }

        int ret = getDerivedHDKeyByPath(masterkey, derived_path, outaddress, outprivkey);
        return ret;
}

/**
 * @brief This function generates a derived child address from a masterkey using
 * a BIP44 standardized static, non hardened path comprised of an account, a change or
 * receiving address and an address index.
 *
 * @param masterkey The master key from which children are derived from.
 * @param account The account that the derived address would belong to.
 * @param ischange Boolean value representing either a change or receiving address.
 * @param addressindex The index of the receiving/change address per account.
 * @param outp2pkh The derived address in P2PSH form.
 *
 * @return 1 if a derived address was successfully generated, 0 otherwise.
 */
int getDerivedHDAddressAsP2PKH(const char* masterkey, uint32_t account, bool ischange, uint32_t addressindex, char* outp2pkh) {
        if (!masterkey) {
            debug_print("%s", "no extended key\n");
            return false;
        }

        char derived_path[DERIVED_PATH_STRINGLEN];
        int derived_path_size = snprintf(derived_path, sizeof(derived_path), "m/44'/3'/%u'/%u/%u", account, ischange, addressindex);

        if (derived_path_size >= (int)sizeof(derived_path)) {
            debug_print("%s", "derivation path overflow\n");
            return false;
        }

        return getDerivedHDAddressByPath(masterkey, derived_path, outp2pkh);
}

/**
 * @brief This function generates a new dogecoin address from a mnemonic by the slip44 key path.
 *
 * @param account The BIP44 account to generate the derived address.
 * @param index The BIP44 index to generate the derived address.
 * @param change_level The BIP44 change level flag to generate derived address.
 * @param mnemonic The mnemonic code words.
 * @param passphrase The passphrase (optional).
 * @param p2pkh_pubkey_master The generated master public key.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * return: 0 (success), -1 (fail)
 */
int getDerivedHDAddressFromMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const MNEMONIC mnemonic, const PASSPHRASE pass, char* p2pkh_pubkey, const dogecoin_bool is_testnet) {

    /* Validate input */
    if (!mnemonic) {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return -1;
    }

    /* Initialize variables */
    dogecoin_hdnode node;
    dogecoin_hdnode bip44_key;
    char keypath[BIP44_KEY_PATH_MAX_LENGTH + 1] = "";

    /* Define seed and initialize */
    SEED seed = {0};

    /* Convert mnemonic to seed */
    if (dogecoin_seed_from_mnemonic (mnemonic, pass, seed) == -1) {
        return -1;
    }

    /* Generate the root key from the seed */
    dogecoin_hdnode_from_seed(seed, MAX_SEED_SIZE, &node);

    /* Derive the child private key at the index */
    if (derive_bip44_extended_key(&node, &account, &index, change_level, NULL, is_testnet, keypath, &bip44_key) == -1) {
        return -1;
    }

    /* Generate the address */
    dogecoin_hdnode_get_p2pkh_address(&bip44_key, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey, P2PKHLEN);

   return 0;

}

/**
 * @brief This function generates a HD master key and p2pkh ready-to-use corresponding dogecoin address from a mnemonic.
 *
 * @param hd_privkey_master The generated master private key.
 * @param p2pkh_pubkey_master The generated master public key.
 * @param mnemonic The mnemonic code words.
 * @param passphrase The passphrase (optional).
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * return: 0 (success), -1 (fail)
 */
int generateHDMasterPubKeypairFromMnemonic(char* hd_privkey_master, char* p2pkh_pubkey_master, const MNEMONIC mnemonic, const PASSPHRASE pass, const dogecoin_bool is_testnet) {

    /* Validate input */
    if (!mnemonic) {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return -1;
    }

    /* Initialize variables */
    dogecoin_hdnode node;

    /* Define seed and initialize */
    SEED seed = {0};

    /* Convert mnemonic to seed */
    if (dogecoin_seed_from_mnemonic (mnemonic, pass, seed) == -1) {
        return -1;
    }

    /* Generate the root key from the seed */
    dogecoin_hdnode_from_seed(seed, MAX_SEED_SIZE, &node);

    /* Serialize the private key */
    dogecoin_hdnode_serialize_private(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, hd_privkey_master, HDKEYLEN);

    /* Generate the address */
    dogecoin_hdnode_get_p2pkh_address(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey_master, P2PKHLEN);

    return 0;
}

/**
 * @brief This function verifies that a HD master key and a dogecoin address matches a mnemonic.
 *
 * @param hd_privkey_master The master private key to check.
 * @param p2pkh_pubkey_master The master public key to check.
 * @param mnemonic The mnemonic code words.
 * @param passphrase The passphrase (optional).
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * return: 0 (success), -1 (fail)
 */
int verifyHDMasterPubKeypairFromMnemonic(const char* hd_privkey_master, const char* p2pkh_pubkey_master, const MNEMONIC mnemonic, const PASSPHRASE pass, const dogecoin_bool is_testnet) {

    /* Validate input */
    if (!mnemonic) {
        fprintf(stderr, "ERROR: Invalid mnemonic\n");
        return -1;
    }

    /* Initialize variables */
    dogecoin_hdnode node;

    /* Define seed and initialize */
    SEED seed = {0};

    /* Convert mnemonic to seed */
    if (dogecoin_seed_from_mnemonic (mnemonic, pass, seed) == -1) {
        return -1;
    }

    /* Generate the root key from the seed */
    dogecoin_hdnode_from_seed(seed, MAX_SEED_SIZE, &node);

    /* Serialize the private key */
    char hd_privkey_master_calculated[HDKEYLEN];
    dogecoin_hdnode_serialize_private(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, hd_privkey_master_calculated, HDKEYLEN);

    /* Compare the calculated private key with the input private key */
    if (strcmp(hd_privkey_master, hd_privkey_master_calculated) != 0) {
        return -1;
    }

    /* Generate the address */
    char p2pkh_pubkey_master_calculated[P2PKHLEN];
    dogecoin_hdnode_get_p2pkh_address(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey_master_calculated, P2PKHLEN);

    /* Compare the calculated address with the input address */
    if (strcmp(p2pkh_pubkey_master, p2pkh_pubkey_master_calculated) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief This function generates a new dogecoin address from a encrypted seed by the slip44 key path.
 *
 * @param account The BIP44 account to generate the derived address.
 * @param index The BIP44 index to generate the derived address.
 * @param change_level The BIP44 change level flag to generate derived address.
 * @param p2pkh_pubkey_master The generated master public key.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 * @param file_num The file number to store the encrypted seed.
 *
 * return: 0 (success), -1 (fail)
 */
int getDerivedHDAddressFromEncryptedSeed(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const dogecoin_bool is_testnet, const int file_num) {

    /* Initialize variables */
    dogecoin_hdnode node;
    dogecoin_hdnode bip44_key;
    char keypath[BIP44_KEY_PATH_MAX_LENGTH + 1] = "";

    /* Define seed and initialize */
    SEED seed = {0};

    /* Decrypt the seed with the TPM */
    if (dogecoin_decrypt_seed_with_tpm (seed, file_num) == false) {
        return -1;
    }

    /* Generate the root key from the seed */
    dogecoin_hdnode_from_seed(seed, MAX_SEED_SIZE, &node);

    /* Derive the child private key at the index */
    if (derive_bip44_extended_key(&node, &account, &index, change_level, NULL, is_testnet, keypath, &bip44_key) == -1) {
        return -1;
    }

    /* Generate the address */
    dogecoin_hdnode_get_p2pkh_address(&bip44_key, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey, P2PKHLEN);

   return 0;

}

/**
 * @brief This function generates a HD master key and p2pkh ready-to-use corresponding dogecoin address from a encrypted seed.
 *
 * @param hd_privkey_master The generated master private key.
 * @param p2pkh_pubkey_master The generated master public key.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * return: 0 (success), -1 (fail)
 */
int generateHDMasterPubKeypairFromEncryptedSeed(char* hd_privkey_master, char* p2pkh_pubkey_master, const dogecoin_bool is_testnet, const int file_num) {

    /* Initialize variables */
    dogecoin_hdnode node;

    /* Define seed and initialize */
    SEED seed = {0};

    /* Decrypt the seed with the TPM */
    if (dogecoin_decrypt_seed_with_tpm (seed, file_num) == false) {
        return -1;
    }

    /* Generate the root key from the seed */
    dogecoin_hdnode_from_seed(seed, MAX_SEED_SIZE, &node);

    /* Serialize the private key */
    dogecoin_hdnode_serialize_private(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, hd_privkey_master, HDKEYLEN);

    /* Generate the address */
    dogecoin_hdnode_get_p2pkh_address(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey_master, P2PKHLEN);

    return 0;
}

/**
 * @brief This function verifies that a HD master key and a dogecoin address matches a encrypted seed.
 *
 * @param hd_privkey_master The master private key to check.
 * @param p2pkh_pubkey_master The master public key to check.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * return: 0 (success), -1 (fail)
 */
int verifyHDMasterPubKeypairFromEncryptedSeed(const char* hd_privkey_master, const char* p2pkh_pubkey_master, const dogecoin_bool is_testnet, const int file_num) {

    /* Initialize variables */
    dogecoin_hdnode node;

    /* Define seed and initialize */
    SEED seed = {0};

    /* Decrypt the seed with the TPM */
    if (dogecoin_decrypt_seed_with_tpm (seed, file_num) == false) {
        return -1;
    }

    /* Generate the root key from the seed */
    dogecoin_hdnode_from_seed(seed, MAX_SEED_SIZE, &node);

    /* Serialize the private key */
    char hd_privkey_master_calculated[HDKEYLEN];
    dogecoin_hdnode_serialize_private(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, hd_privkey_master_calculated, HDKEYLEN);

    /* Compare the calculated private key with the input private key */
    if (strcmp(hd_privkey_master, hd_privkey_master_calculated) != 0) {
        return -1;
    }

    /* Generate the address */
    char p2pkh_pubkey_master_calculated[P2PKHLEN];
    dogecoin_hdnode_get_p2pkh_address(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey_master_calculated, P2PKHLEN);

    /* Compare the calculated address with the input address */
    if (strcmp(p2pkh_pubkey_master, p2pkh_pubkey_master_calculated) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief This function generates a new dogecoin address from a encrypted mnemonic by the slip44 key path.
 *
 * @param account The BIP44 account to generate the derived address.
 * @param index The BIP44 index to generate the derived address.
 * @param change_level The BIP44 change level flag to generate derived address.
 * @param pass The passphrase (optional).
 * @param p2pkh_pubkey_master The generated master public key.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 * @param file_num The file number to store the encrypted seed.
 *
 * return: 0 (success), -1 (fail)
 */
int getDerivedHDAddressFromEncryptedMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const PASSPHRASE pass, char* p2pkh_pubkey, const bool is_testnet, const int file_num) {

    /* Initialize variables */
    dogecoin_hdnode node;
    dogecoin_hdnode bip44_key;
    char keypath[BIP44_KEY_PATH_MAX_LENGTH + 1] = "";

    /* Define seed and initialize */
    SEED seed = {0};

    /* Define mnemonic and initialize */
    MNEMONIC mnemonic = {0};

    /* Decrypt the mnemonic with the TPM */
    if (dogecoin_decrypt_mnemonic_with_tpm (mnemonic, file_num) == false) {
        return -1;
    }

    /* Generate the root key from the mnemonic */
    if (dogecoin_seed_from_mnemonic(mnemonic, pass, seed) == -1) {
        return -1;
    }

    /* Generate the root key from the seed */
    dogecoin_hdnode_from_seed(seed, MAX_SEED_SIZE, &node);

    /* Derive the child private key at the index */
    if (derive_bip44_extended_key(&node, &account, &index, change_level, NULL, is_testnet, keypath, &bip44_key) == -1) {
        return -1;
    }

    /* Generate the address */
    dogecoin_hdnode_get_p2pkh_address(&bip44_key, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey, P2PKHLEN);

   return 0;

}

/**
 * @brief This function generates a new dogecoin address from a encrypted master (HD) key by the slip44 key path.
 *
 * @param account The BIP44 account to generate the derived address.
 * @param index The BIP44 index to generate the derived address.
 * @param change_level The BIP44 change level flag to generate derived address.
 * @param p2pkh_pubkey_master The generated master public key.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 * @param file_num The file number to store the encrypted seed.
 *
 * return: 0 (success), -1 (fail)
 */
int getDerivedHDAddressFromEncryptedHDNode(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const bool is_testnet, const int file_num) {

    /* Initialize variables */
    dogecoin_hdnode node;
    dogecoin_hdnode bip44_key;
    char keypath[BIP44_KEY_PATH_MAX_LENGTH + 1] = "";

    /* Decrypt the hdnode with the TPM */
    if (dogecoin_decrypt_hdnode_with_tpm (&node, file_num) == false) {
        return -1;
    }

    /* Derive the child private key at the index */
    if (derive_bip44_extended_key(&node, &account, &index, change_level, NULL, is_testnet, keypath, &bip44_key) == -1) {
        return -1;
    }

    /* Generate the address */
    dogecoin_hdnode_get_p2pkh_address(&bip44_key, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey, P2PKHLEN);

   return 0;

}
