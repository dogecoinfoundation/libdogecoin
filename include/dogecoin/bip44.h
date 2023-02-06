/**********************************************************************
 * Copyright (c) 2023 edtubbs                                         *
 * Copyright (c) 2023 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef __LIBDOGECOIN_BIP44_H__
#define __LIBDOGECOIN_BIP44_H__

#include <dogecoin/bip32.h>

LIBDOGECOIN_BEGIN_DECL

/*
 * Defines
 */

/* BIP 44 string constants */
#define BIP44_PURPOSE "44"       /* Purpose for key derivation according to BIP 44 */
#define BIP44_COIN_TYPE "3"      /* Coin type for Dogecoin (3, SLIP 44) */
#define BIP44_COIN_TYPE_TEST "1" /* Coin type for Testnet (1, SLIP44) */
#define BIP44_CHANGE_EXTERNAL "0"     /* Change level for external addresses */
#define BIP44_CHANGE_INTERNAL "1"     /* Change level for internal addresses */
#define BIP44_CHANGE_LEVEL_SIZE 1 + 1 /* Change level size with a null terminator */
#define SLIP44_KEY_PATH "m/" BIP44_PURPOSE "'/" /* Key path to derive keys */

/* BIP 44 literal constants */
#define BIP44_MAX_ADDRESS 2^31 - 1    /* Maximum address is 2^31 - 1 */
#define BIP44_KEY_PATH_MAX_LENGTH 255 /* Maximum length of key path string */
#define BIP44_KEY_PATH_MAX_SIZE BIP44_KEY_PATH_MAX_LENGTH + 1 /* Key path size with a null terminator */
#define BIP44_ADDRESS_GAP_LIMIT 20    /* Maximum gap between unused addresses */
#define BIP44_FIRST_ACCOUNT_NODE 0    /* Index of the first account node */
#define BIP44_FIRST_ADDRESS_INDEX 0   /* Index of the first address */

/*
 * Type definitions
 */

/* A string representation of change level used to generate a BIP 44 key path */
/* The change level should be a string equal to "0" or "1" with a maximum size of BIP44_CHANGE_LEVEL_SIZE */
typedef char CHANGE_LEVEL [BIP44_CHANGE_LEVEL_SIZE];

/* A string representation of key path used to derive BIP 44 keys */
/* The key path should be a string with a maximum size of BIP44_KEY_PATH_MAX_SIZE */
typedef char KEY_PATH [BIP44_KEY_PATH_MAX_SIZE];

/* Derives a BIP 44 extended private key from a master private key. */
/* Master private key to derive from */
/* Account index (literal) */
/* Derived address index, set to NULL to get an extended key */
/* Change level ("0" for external or "1" for internal addresses */
/* Custom path string (optional, account and change_level ignored) */
/* Test net flag */
/* Key path string generated */
/* BIP 44 extended private key generated */
/* return 0 (success), -1 (fail) */
int derive_bip44_extended_private_key(const dogecoin_hdnode *master_key, const uint32_t account, const uint32_t* address_index, const CHANGE_LEVEL change_level, const KEY_PATH path, const dogecoin_bool is_testnet, KEY_PATH keypath, dogecoin_hdnode *bip44_key);

/* Derives a BIP 44 extended public key from a master public key. */
/* Master public key to derive from */
/* Account index (literal) */
/* Derived address index, set to NULL to get an extended key */
/* Change level ("0" for external or "1" for internal addresses */
/* Custom path string (optional, account and change_level ignored) */
/* Test net flag */
/* Key path string generated */
/* BIP 44 extended public key generated */
/* return 0 (success), -1 (fail) */
int derive_bip44_extended_public_key(const dogecoin_hdnode *master_key, const uint32_t account, const uint32_t* address_index, const CHANGE_LEVEL change_level, const KEY_PATH path, const dogecoin_bool is_testnet, KEY_PATH keypath, dogecoin_hdnode *bip44_key);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_BIP44_H__
