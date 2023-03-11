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
#include <dogecoin/ecc.h>
#include <dogecoin/key.h>
#include <dogecoin/random.h>
#include <dogecoin/sha2.h>
#include <dogecoin/base58.h>
#include <dogecoin/tool.h>
#include <dogecoin/utils.h>
#include <dogecoin/mem.h>


/**
 * @brief This function instantiates a new working signature,
 * but does not add it to the hash table.
 * 
 * @return A pointer to the new working signature. 
 */
signature* new_signature() {
    signature* sig = (struct signature*)dogecoin_calloc(1, sizeof *sig);
    sig->content = dogecoin_char_vla(75);
    sig->recid = 0;
    sig->idx = HASH_COUNT(signatures) + 1;
    return sig;
}

/**
 * @brief This function takes a pointer to an existing working
 * signature object and adds it to the hash table.
 * 
 * @param sig The pointer to the working signature.
 * 
 * @return Nothing.
 */
void add_signature(signature *sig) {
    signature* sig_old;
    HASH_FIND_INT(signatures, &sig->idx, sig_old);
    if (sig_old == NULL) {
        HASH_ADD_INT(signatures, idx, sig);
    } else {
        HASH_REPLACE_INT(signatures, idx, sig, sig_old);
    }
    dogecoin_free(sig_old);
}

/**
 * @brief This function takes an index and returns the working
 * signature associated with that index in the hash table.
 * 
 * @param idx The index of the target working signature.
 * 
 * @return The pointer to the working signature associated with
 * the provided index.
 */
signature* find_signature(int idx) {
    signature* sig;
    HASH_FIND_INT(signatures, &idx, sig);
    return sig;
}

/**
 * @brief This function removes the specified working signature
 * from the hash table and frees the signatures in memory.
 * 
 * @param sig The pointer to the signature to remove.
 * 
 * @return Nothing.
 */
void remove_signature(signature* sig) {
    HASH_DEL(signatures, sig); /* delete it (signatures advances to next) */
    sig->recid = 0;
    dogecoin_free(sig);
}

/**
 * @brief This function creates a new sig, places it in
 * the hash table, and returns the index of the new sig,
 * starting from 1 and incrementing each subsequent call.
 * 
 * @return The index of the new sig.
 */
int start_signature() {
    signature* sig = new_signature();
    int index = sig->idx;
    add_signature(sig);
    return index;
}

/**
 * @brief This function instantiates a new working eckey,
 * but does not add it to the hash table.
 * 
 * @return A pointer to the new working eckey. 
 */
eckey* new_eckey() {
    eckey* key = (struct eckey*)dogecoin_calloc(1, sizeof *key);
    dogecoin_privkey_init(&key->private_key);
    assert(dogecoin_privkey_is_valid(&key->private_key) == 0);
    dogecoin_privkey_gen(&key->private_key);
    assert(dogecoin_privkey_is_valid(&key->private_key)==1);
    dogecoin_pubkey_init(&key->public_key);
    dogecoin_pubkey_from_key(&key->private_key, &key->public_key);
    assert(dogecoin_pubkey_is_valid(&key->public_key) == 1);
    printf("privkey: %s\n", utils_uint8_to_hex((const uint8_t *)&key->private_key, 33));
    strcpy(key->public_key_hex, utils_uint8_to_hex((const uint8_t *)&key->public_key, 33));
    printf("pubkey: %s\n", key->public_key_hex);
    uint8_t pkeybase58c[34];
    pkeybase58c[0] = dogecoin_chainparams_main.b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */
    memcpy_safe(&pkeybase58c[1], &key->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    assert(dogecoin_base58_encode_check(pkeybase58c, sizeof(pkeybase58c), key->private_key_wif, sizeof(key->private_key_wif)) != 0);
    printf("key->private_key_wif: %s\n", key->private_key_wif);
    key->recid = 0;
    key->idx = HASH_COUNT(keys) + 1;
    return key;
}

/**
 * @brief This function takes a pointer to an existing working
 * eckey object and adds it to the hash table.
 * 
 * @param key The pointer to the working eckey.
 * 
 * @return Nothing.
 */
void add_eckey(eckey *key) {
    eckey* key_old;
    HASH_FIND_INT(keys, &key->idx, key_old);
    if (key_old == NULL) {
        HASH_ADD_INT(keys, idx, key);
    } else {
        HASH_REPLACE_INT(keys, idx, key, key_old);
    }
    dogecoin_free(key_old);
}

/**
 * @brief This function takes an index and returns the working
 * eckey associated with that index in the hash table.
 * 
 * @param idx The index of the target working eckey.
 * 
 * @return The pointer to the working eckey associated with
 * the provided index.
 */
eckey* find_eckey(int idx) {
    eckey* key;
    HASH_FIND_INT(keys, &idx, key);
    return key;
}

/**
 * @brief This function removes the specified working eckey
 * from the hash table and frees the keys in memory.
 * 
 * @param key The pointer to the eckey to remove.
 * 
 * @return Nothing.
 */
void remove_eckey(eckey* key) {
    HASH_DEL(keys, key); /* delete it (keys advances to next) */
    dogecoin_privkey_cleanse(&key->private_key);
    dogecoin_pubkey_cleanse(&key->public_key);
    dogecoin_free(key);
}

/**
 * @brief This function frees the memory allocated
 * for an eckey.
 * 
 * @param eckey The pointer to the eckey to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_key_free(eckey* eckey)
{
    dogecoin_free(eckey);
}

/**
 * @brief This function creates a new key, places it in
 * the hash table, and returns the index of the new key,
 * starting from 1 and incrementing each subsequent call.
 * 
 * @return The index of the new key.
 */
int start_key() {
    eckey* key = new_eckey();
    int index = key->idx;
    add_eckey(key);
    return index;
}

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
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 * @param wif_privkey_master The master private key to check.
 * @param p2pkh_pubkey_master The master public key to check.
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

    /* Derive the BIP 44 extended key */
    if (derive_bip44_extended_private_key(&node, account, NULL, change_level, NULL, is_testnet, keypath, &bip44_key) == -1) {
        return -1;
    }

    /* Derive the child private key at the index */
    if (derive_bip44_extended_private_key(&node, account, &index, change_level, NULL, is_testnet, keypath, &bip44_key) == -1) {
        return -1;
    }

    /* Generate the address */
    dogecoin_hdnode_get_p2pkh_address(&bip44_key, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey, P2PKH_ADDR_STRINGLEN);

   return 0;

}

/**
 * @brief This function generates a HD master key and p2pkh ready-to-use corresponding dogecoin address from a mnemonic.
 *
 * @param wif_privkey_master The generated master private key.
 * @param p2pkh_pubkey_master The generated master public key.
 * @param mnemonic The mnemonic code words.
 * @param passphrase The passphrase (optional).
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * return: 0 (success), -1 (fail)
 */
int generateHDMasterPubKeypairFromMnemonic(char* wif_privkey_master, char* p2pkh_pubkey_master, const MNEMONIC mnemonic, const PASSPHRASE pass, const dogecoin_bool is_testnet) {

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
    dogecoin_hdnode_serialize_private(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, wif_privkey_master, HD_MASTERKEY_STRINGLEN);

    /* Generate the address */
    dogecoin_hdnode_get_p2pkh_address(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey_master, P2PKH_ADDR_STRINGLEN);

    return 0;
}

/**
 * @brief This function verifies that a HD master key and a dogecoin address matches a mnemonic.
 *
 * @param wif_privkey_master The master private key to check.
 * @param p2pkh_pubkey_master The master public key to check.
 * @param mnemonic The mnemonic code words.
 * @param passphrase The passphrase (optional).
 * @param is_testnet The flag denoting which network, 0 for mainnet and 1 for testnet.
 *
 * return: 0 (success), -1 (fail)
 */
int verifyHDMasterPubKeypairFromMnemonic(const char* wif_privkey_master, const char* p2pkh_pubkey_master, const MNEMONIC mnemonic, const PASSPHRASE pass, const dogecoin_bool is_testnet) {

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
    char wif_privkey_master_calculated[HD_MASTERKEY_STRINGLEN];
    dogecoin_hdnode_serialize_private(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, wif_privkey_master_calculated, HD_MASTERKEY_STRINGLEN);

    /* Compare the calculated private key with the input private key */
    if (strcmp(wif_privkey_master, wif_privkey_master_calculated) != 0) {
        return -1;
    }

    /* Generate the address */
    char p2pkh_pubkey_master_calculated[P2PKH_ADDR_STRINGLEN];
    dogecoin_hdnode_get_p2pkh_address(&node, is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main, p2pkh_pubkey_master_calculated, P2PKH_ADDR_STRINGLEN);

    /* Compare the calculated address with the input address */
    if (strcmp(p2pkh_pubkey_master, p2pkh_pubkey_master_calculated) != 0) {
        return -1;
    }

    return 0;
}

/**
 * @brief Derive p2pkh address from private key
 * 
 * @param privkey Private key needed to derive address.
 * 
 * @return derived address if it was successfully generated, 0 otherwise.
 */
char* addressFromPrivkey(char* privkey) {
    assert(gen_privatekey(&dogecoin_chainparams_main, privkey, 53, NULL)==1);
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    assert(dogecoin_privkey_is_valid(&key) == 0);

    // copy byte array privkeydat to dogecoin_key.privkey:
    memcpy(key.privkey, utils_hex_to_uint8(privkey), strlen(privkey) / 2);

    assert(dogecoin_privkey_is_valid(&key) == 1);

    // init pubkey
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    pubkey.compressed = true;
    assert(dogecoin_pubkey_is_valid(&pubkey) == 0);
    dogecoin_pubkey_from_key(&key, &pubkey);

    // derive p2pkh address from new injected dogecoin_pubkey with known hexadecimal public key:
    char* address = dogecoin_char_vla(35);
    if (!dogecoin_pubkey_getaddr_p2pkh(&pubkey, &dogecoin_chainparams_main, address)) {
        printf("failed to get address from pubkey!\n");
        return false;
    }

    assert(dogecoin_pubkey_is_valid(&pubkey) == 1);
    assert(dogecoin_privkey_verify_pubkey(&key, &pubkey) == 1);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_cleanse(&pubkey);
    return address;
}

/**
 * @brief Sign message with private key
 * 
 * @param privkey The key to sign the message with.
 * @param message The message to be signed.
 * 
 * @return Base64 encoded signature if successful, False (0) if not
 * 
 */
char* signmsgwithprivatekey(char* privkey, char* msg) {
    if (!privkey || !msg) return false;

    // vars for signing
    size_t outlen = 74, outlencmp = 64;
    unsigned char *sig = dogecoin_uchar_vla(outlen), 
    *sigcmp = dogecoin_uchar_vla(outlencmp);

    // retrieve double sha256 hash of msg:
    uint256 msgBytes;
    int ret = dogecoin_dblhash((const unsigned char*)msg, strlen(msg), msgBytes);

    dogecoin_key key;
    dogecoin_privkey_init(&key);
    assert(dogecoin_privkey_is_valid(&key) == 0);

    // copy byte array utils_hex_to_uint8(privkey) to dogecoin_key.privkey:
    memcpy(key.privkey, utils_hex_to_uint8(privkey), strlen(privkey) / 2 + 1);
    ret = dogecoin_privkey_is_valid(&key);
    assert(ret == 1);

    // init pubkey
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    pubkey.compressed = false;
    ret = dogecoin_pubkey_is_valid(&pubkey);
    assert(ret == 0);
    dogecoin_pubkey_from_key(&key, &pubkey);
    ret = dogecoin_pubkey_is_valid(&pubkey);
    if (!ret) return 0;
    ret = dogecoin_privkey_verify_pubkey(&key, &pubkey);
    if (!ret) return 0;

    // sign hash
    ret = dogecoin_key_sign_hash(&key, msgBytes, sig, &outlen);
    if (!ret) {
        printf("dogecoin_key_sign_hash failed!\n");
        return 0;
    }
    int header = (sig[0] & 0xFF);
    printf("header: %d\n", header);
    if (header >= 31) { // this is a compressed key signature
        pubkey.compressed = true;
        header -= 24;
    }
    printf("header: %d\n", header);
    int recid = header - 24;
    printf("recid: %d\n", recid);
    // sign compact for recovery of pubkey and free privkey:
    ret = dogecoin_key_sign_hash_compact_recoverable(&key, msgBytes, sigcmp, &outlencmp, &recid);
    if (!ret) {
        printf("key sign recoverable failed!\n");
        return false;
    }
    printf("recid: %d\n", recid);
    ret = dogecoin_key_sign_recover_pubkey((const unsigned char*)sigcmp, msgBytes, recid, &pubkey);
    if (!ret) {
        printf("key sign recover failed!\n");
        return false;
    }
    printf("recid: %d\n", recid);
    ret = dogecoin_pubkey_verify_sig(&pubkey, msgBytes, sig, outlen);
    if (!ret) {
        printf("pubkey sig verification failed!\n");
        return false;
    }

    dogecoin_free(sigcmp);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_cleanse(&pubkey);

    // base64 encode output and free sig:
    char* out = b64_encode(sig, outlen);
    dogecoin_free(sig);
    return out;
}

/**
 * @brief Verify signed message
 * 
 * @param address
 * @param sig
 * @param msg
 * 
 * @return True (1) if successful, False (0) otherwise
 * 
 */
dogecoin_bool verifymessage(char* address, char* sig, char* msg) {
    if (!(address || sig || msg)) return false;

	size_t out_len = b64_decoded_size(sig);
    unsigned char *out = dogecoin_uchar_vla(out_len), 
    *sigcomp_out = dogecoin_uchar_vla(65);
    int ret = b64_decode(sig, out, out_len);
	if (!ret) {
        printf("b64_decode failed!\n");
        return ret;
    }

    // double sha256 hash message:
    uint256 messageBytes;
    ret = dogecoin_dblhash((const unsigned char*)msg, strlen(msg), messageBytes);
    if (!ret) {
        printf("messageBytes failed\n");
        return ret;
    }
    ret = dogecoin_ecc_der_to_compact(out, out_len, sigcomp_out);
    if (!ret) {
        printf("ecc der to compact failed!\n");
        return ret;
    }

    // initialize empty pubkey
    dogecoin_pubkey pub_key;
    dogecoin_pubkey_init(&pub_key);
    pub_key.compressed = false;

    int header = (out[0] & 0xFF);
    printf("header: %d\n", header);
    if (header >= 31) { // this is a compressed key signature
        pub_key.compressed = true;
        header -= 24;
    }
    printf("header: %d\n", header);
    int recid = header - 24;
    printf("recid: %d\n", recid);

    // recover pubkey
    ret = dogecoin_key_sign_recover_pubkey((const unsigned char*)sigcomp_out, messageBytes, recid, &pub_key);
    dogecoin_free(sigcomp_out);
    if (!ret) {
        printf("key sign recover failed!\n");
        return ret;
    }
    printf("pub_key (vm): %s\n", utils_uint8_to_hex((const uint8_t *)&pub_key, 33));
    ret = dogecoin_pubkey_verify_sig(&pub_key, messageBytes, out, out_len);
    dogecoin_free(out);
    if (!ret) {
        printf("pubkey sig verification failed!\n");
        return ret;
    }

    // derive p2pkh address from new injected dogecoin_pubkey with known hexadecimal public key:
    char* p2pkh_address = dogecoin_char_vla(strlen(address) + 1);
    ret = dogecoin_pubkey_getaddr_p2pkh(&pub_key, &dogecoin_chainparams_main, p2pkh_address);
    if (!ret) {
        printf("derived address from pubkey failed!\n");
        return ret;
    }
    printf("p2pkh_address: %s\n", p2pkh_address);
    dogecoin_pubkey_cleanse(&pub_key);

    // compare address derived from recovered pubkey:
    ret = strncmp(p2pkh_address, address, 35);

    dogecoin_free(p2pkh_address);
    return ret==0;
}

/**
 * @brief Sign message with private key
 * 
 * @param key The key to sign the message with.
 * @param message The message to be signed.
 * 
 * @return signature struct with base64 encoded signature and recid if successful, False (0) if not
 * 
 */
signature* signmsgwitheckey(eckey* key, char* msg) {
    if (!key || !msg) return false;

    printf("----start sign----\n");

    // retrieve double sha256 hash of msg:
    uint256 msgBytes;
    int ret = dogecoin_dblhash((const unsigned char*)msg, strlen(msg), msgBytes);

    // vars for signing
    size_t outlen = 74, outlencmp = 64;
    unsigned char *sig = dogecoin_uchar_vla(outlen), 
    *sigcmp = dogecoin_uchar_vla(outlencmp);
    // uint8_t sig[64];
    // size_t siglen = 0;
    // dogecoin_key_sign_hash_compact(privkey, sighash, sig, &siglen);
    // assert(siglen == sizeof(sig));
    // if (sigcompact_out) {
    //     memcpy_safe(sigcompact_out, sig, siglen);
    // }

    // // form normalized DER signature & hashtype
    // unsigned char sigder_plus_hashtype[74 + 1];
    // size_t sigderlen = 75;
    // dogecoin_ecc_compact_to_der_normalized(sig, sigder_plus_hashtype, &sigderlen);
    // assert(sigderlen <= 74 && sigderlen >= 70);
    // sigder_plus_hashtype[sigderlen] = sighashtype;
    // sigderlen += 1; //+hashtype
    // if (sigcompact_out) {
    //     memcpy_safe(sigder_out, sigder_plus_hashtype, sigderlen);
    // }
    // if (sigder_len_out) {
    //     *sigder_len_out = sigderlen;
    // }

    // sign hash
    ret = dogecoin_key_sign_hash(&key->private_key, msgBytes, sig, &outlen);
    if (!ret) {
        printf("dogecoin_key_sign_hash failed!\n");
        return 0;
    }
    int header = (sig[0] & 0xFF);
    printf("header: %d\n", header);
    if (header >= 31) { // this is a compressed key signature
        header -= 24;
    }
    printf("header: %d\n", header);
    int recid = header - 24;
    printf("recid: %d\n", recid);
    // sign compact for recovery of pubkey and free privkey:
    ret = dogecoin_key_sign_hash_compact_recoverable(&key->private_key, msgBytes, sigcmp, &outlencmp, &recid);
    if (!ret) {
        printf("key sign recoverable failed!\n");
        return false;
    }
    printf("recid: %d\n", recid);
    ret = dogecoin_key_sign_recover_pubkey((const unsigned char*)sigcmp, msgBytes, recid, &key->public_key);
    if (!ret) {
        printf("key sign recover failed!\n");
        return false;
    }
    printf("recid: %d\n", recid);
    ret = dogecoin_pubkey_verify_sig(&key->public_key, msgBytes, sig, outlen);
    if (!ret) {
        printf("pubkey sig verification failed!\n");
        return false;
    }
    key->recid = recid;
    printf("key->recid: %d\n", key->recid);
    signature* working_sig = new_signature();
    working_sig->recid = recid;

    printf("key->public_key: %s\n", utils_uint8_to_hex((const uint8_t *)&key->public_key, 33));
    dogecoin_free(sigcmp);
    // base64 encode output and free sig:
    working_sig->content = b64_encode(sig, outlen);
    dogecoin_free(sig);
    printf("working_sig: %s\n", working_sig->content);
    printf("----end sign----\n\n");

    return working_sig;
}

/**
 * @brief Verify signed message
 * 
 * @param address
 * @param sig
 * @param msg
 * 
 * @return True (1) if successful, False (0) otherwise
 * 
 */
dogecoin_bool verifymessagewitheckey(eckey* key, signature* sig, char* msg) {
    if (!(key || sig || msg)) return false;

    printf("----start verification----\n");
    char* signature_encoded = sig->content;
    printf("sig->content: %s\n", signature_encoded);

	size_t out_len = b64_decoded_size(signature_encoded);
    printf("out_len: %d\n", out_len);
    unsigned char *out = dogecoin_uchar_vla(out_len), 
    *sigcomp_out = dogecoin_uchar_vla(65);
    int ret = b64_decode(signature_encoded, out, out_len);
	if (!ret) {
        printf("b64_decode failed!\n");
        return ret;
    }

    int recid = sig->recid;
    printf("recid (vm): %d\n", recid);
    printf("out: %s\n", utils_uint8_to_hex(out, out_len));

    // double sha256 hash message:
    uint256 messageBytes;
    ret = dogecoin_dblhash((const unsigned char*)msg, strlen(msg), messageBytes);
    if (!ret) {
        printf("messageBytes failed\n");
        return ret;
    }
    printf("out_len: %d\n", out_len);
    ret = dogecoin_ecc_der_to_compact(out, out_len, sigcomp_out);
    if (!ret) {
        printf("ecc der to compact failed!\n");
        return ret;
    }

    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    pubkey.compressed = false;


    // recover pubkey
    ret = dogecoin_key_sign_recover_pubkey((const unsigned char*)sigcomp_out, messageBytes, recid, &pubkey);
    dogecoin_free(sigcomp_out);
    if (!ret) {
        printf("key sign recover failed!\n");
        return ret;
    }
    printf("recid (vm): %d\n", recid);
    printf("pub_key (vm): %s\n", utils_uint8_to_hex((const uint8_t *)&pubkey, 33));
    printf("key->public_key (vm): %s\n", utils_uint8_to_hex((const uint8_t *)&key->public_key, 33));
    ret = dogecoin_pubkey_verify_sig(&pubkey, messageBytes, out, out_len);
    if (!ret) {
        printf("pubkey sig verification failed!\n");
        return ret;
    }
    printf("ret: %d\n", ret);
    printf("pub_key (vm): %s\n", utils_uint8_to_hex((const uint8_t *)&pubkey, 33));
    printf("key->public_key (vm): %s\n", utils_uint8_to_hex((const uint8_t *)&key->public_key, 33));
    ret = memcmp(&pubkey, &key->public_key, 33)==0;
    printf("ret: %d\n", ret);
    printf("strncmp(&pubkey.pubkey, &key->public_key.pubkey, 33): %d\n", strncmp(pubkey.pubkey, key->public_key.pubkey, 33));
    printf("----end verification----\n\n");
    return ret;
}
