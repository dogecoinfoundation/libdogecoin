/*

 The MIT License (MIT)

 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

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
#include <dogecoin/chainparams.h>
#include <dogecoin/key.h>
#include <dogecoin/random.h>
#include <dogecoin/sha2.h>
#include <dogecoin/base58.h>
#include <dogecoin/tool.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>

#define MAX_INT32_STRINGLEN 12
#define HD_MASTERKEY_STRINGLEN 112
#define P2PKH_ADDR_STRINGLEN 35
#define WIF_UNCOMPRESSED_PRIVKEY_STRINGLEN 53
#define DERIVED_PATH_STRINGLEN 33 
/* NOTE: Path string composed of m/44/3/+32bits_Account+/+bool_ischange+/+32bits_Address + string terminator; for a total of 33 bytes. */

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

    char wif_privkey_internal[WIF_UNCOMPRESSED_PRIVKEY_STRINGLEN]; //MLUMIN: Keylength (51 or 52 chars, depending on uncompressed or compressed) +1 for string termination 'internally'? 
    char p2pkh_pubkey_internal[P2PKH_ADDR_STRINGLEN]; //MLUMIN: no magic numbers. p2pkh address should be 34 characters, +1 though for string termination 'internally'?
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
 * @param wif_privkey_master The generated master private key.
 * @param p2pkh_pubkey_master The generated master public key.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 * 
 * @return 1 if the key pair was generated successfully.
 */
int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)
{
    char hd_privkey_master[HD_MASTERKEY_STRINGLEN];
    char hd_pubkey_master[P2PKH_ADDR_STRINGLEN];

    /* if nothing is passed use internal variables */
    if (wif_privkey_master) {
        memcpy_safe(hd_privkey_master, wif_privkey_master, sizeof(hd_privkey_master));
    }
    if (p2pkh_pubkey_master) {
        memcpy_safe(hd_pubkey_master, p2pkh_pubkey_master, sizeof(hd_pubkey_master));
    }

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* generate a new hd master key */
    if (!hd_gen_master(chain, hd_privkey_master, sizeof(hd_privkey_master))) {
        return false;
    }

    if (!generateDerivedHDPubkey(hd_privkey_master, hd_pubkey_master)) {
        return false;
    }

    if (wif_privkey_master) {
        memcpy_safe(wif_privkey_master, hd_privkey_master, sizeof(hd_privkey_master));
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
 * @param wif_privkey_master The master private key to derive the child key from.
 * @param p2pkh_pubkey The resulting child public key.
 * 
 * @return 1 if the child key was generated successfully, 0 otherwise.
 */
int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey)
{
    /* require master key */
    if (!wif_privkey_master) {
        return false;
    }

    /* determine address prefix for network chainparams */
    const dogecoin_chainparams* chain = chain_from_b58_prefix(wif_privkey_master);

    char str[P2PKH_ADDR_STRINGLEN];

    /* if nothing is passed in use internal variables */
    if (p2pkh_pubkey) {
        memcpy_safe(str, p2pkh_pubkey, sizeof(str));
    }

    dogecoin_hdnode* node = dogecoin_hdnode_new();
    dogecoin_hdnode_deserialize(wif_privkey_master, chain, node);

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

    char new_wif_privkey[WIF_UNCOMPRESSED_PRIVKEY_STRINGLEN];
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
 * @param wif_privkey_master The master private key to check.
 * @param p2pkh_pubkey_master The master public key to check.
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 * 
 * @return 1 if the keys match and are valid on the specified network, 0 otherwise.
 */
int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet) {
    /* require both private and public key */
    if (!wif_privkey_master || !p2pkh_pubkey_master) return false;

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* calculate master pubkey from master privkey */
    dogecoin_hdnode node;
    dogecoin_hdnode_deserialize(wif_privkey_master, chain, &node);
    char new_p2pkh_pubkey_master[HD_MASTERKEY_STRINGLEN];
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
 * @return 1 if a derived address was successfully generated, 0 otherwise
 */
int getDerivedHDAddressByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey) {
    if (!masterkey || !derived_path || !outaddress) {
        debug_print("%s", "missing input\n");
        return false;
    }

    /* determine if mainnet or testnet/regtest */
    const dogecoin_chainparams* chain = chain_from_b58_prefix(masterkey);
    bool ret = true;
    dogecoin_hdnode node, nodenew;
    
    if (!dogecoin_hdnode_deserialize(masterkey, chain, &node)) {
        ret = false;
    }
    
    // dogecoin_hdnode_has_privkey
    bool pubckd = !dogecoin_hdnode_has_privkey(&node);
    /* derive child key, use pubckd or privckd */
    if (!dogecoin_hd_generate_key(&nodenew, derived_path, pubckd ? node.public_key : node.private_key, node.chain_code, pubckd)) {
        ret = false;
    }

    if (outprivkey) dogecoin_hdnode_serialize_private(&nodenew, chain, outaddress, HD_MASTERKEY_STRINGLEN);
    else dogecoin_hdnode_serialize_public(&nodenew, chain, outaddress, HD_MASTERKEY_STRINGLEN);
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

        int ret = getDerivedHDAddressByPath(masterkey, derived_path, outaddress, outprivkey);
        return ret;
}
