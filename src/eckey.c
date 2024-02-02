/*

 The MIT License (MIT)

 Copyright (c) 2023 bluezr
 Copyright (c) 2023-2024 The Dogecoin Foundation

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

#include <dogecoin/base58.h>
#include <dogecoin/eckey.h>
#include <dogecoin/key.h>
#include <dogecoin/mem.h>
#include <dogecoin/utils.h>

/**
 * @brief This function instantiates a new working eckey,
 * but does not add it to the hash table.
 *
 * @return A pointer to the new working eckey.
 */
eckey* new_eckey(dogecoin_bool is_testnet) {
    eckey* key = (struct eckey*)dogecoin_calloc(1, sizeof *key);
    dogecoin_privkey_init(&key->private_key);
    assert(dogecoin_privkey_is_valid(&key->private_key) == 0);
    dogecoin_privkey_gen(&key->private_key);
    assert(dogecoin_privkey_is_valid(&key->private_key)==1);
    dogecoin_pubkey_init(&key->public_key);
    dogecoin_pubkey_from_key(&key->private_key, &key->public_key);
    assert(dogecoin_pubkey_is_valid(&key->public_key) == 1);
    strcpy(key->public_key_hex, utils_uint8_to_hex((const uint8_t *)&key->public_key, 33));
    uint8_t pkeybase58c[34];
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;
    pkeybase58c[0] = chain->b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */
    memcpy_safe(&pkeybase58c[1], &key->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    if (dogecoin_base58_encode_check(pkeybase58c, sizeof(pkeybase58c), key->private_key_wif, sizeof(key->private_key_wif)) == 0) return false;
    if (!dogecoin_pubkey_getaddr_p2pkh(&key->public_key, chain, (char*)&key->address)) return false;
    key->idx = HASH_COUNT(keys) + 1;
    return key;
}

/**
 * @brief This function instantiates a new working eckey,
 * but does not add it to the hash table.
 *
 * @return A pointer to the new working eckey.
 */
eckey* new_eckey_from_privkey(char* private_key) {
    eckey* key = (struct eckey*)dogecoin_calloc(1, sizeof *key);
    dogecoin_privkey_init(&key->private_key);
    const dogecoin_chainparams* chain = chain_from_b58_prefix(private_key);
    if (!dogecoin_privkey_decode_wif(private_key, chain, &key->private_key)) return false;
    assert(dogecoin_privkey_is_valid(&key->private_key)==1);
    dogecoin_pubkey_init(&key->public_key);
    dogecoin_pubkey_from_key(&key->private_key, &key->public_key);
    assert(dogecoin_pubkey_is_valid(&key->public_key) == 1);
    strcpy(key->public_key_hex, utils_uint8_to_hex((const uint8_t *)&key->public_key, 33));
    uint8_t pkeybase58c[34];
    pkeybase58c[0] = chain->b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */
    memcpy_safe(&pkeybase58c[1], &key->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    if (dogecoin_base58_encode_check(pkeybase58c, sizeof(pkeybase58c), key->private_key_wif, sizeof(key->private_key_wif)) == 0) return false;
    if (!dogecoin_pubkey_getaddr_p2pkh(&key->public_key, chain, (char*)&key->address)) return false;
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
    dogecoin_key_free(key);
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
 * @param is_testnet
 *
 * @return The index of the new key.
 */
int start_key(dogecoin_bool is_testnet) {
    eckey* key = new_eckey(is_testnet);
    int index = key->idx;
    add_eckey(key);
    return index;
}
