/**********************************************************************
 * Copyright (c) 2022 edtubbs                                         *
 * Copyright (c) 2022 The Dogecoin Foundation                         *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef __LIBDOGECOIN_BIP44_H__
#define __LIBDOGECOIN_BIP44_H__

#include <dogecoin/bip32.h>

/* BIP 44 string constants */
#define BIP44_PURPOSE "44"  /* Purpose for key derivation according to BIP 44 */
#define BIP44_COIN_TYPE "3"  /* Coin type for Dogecoin (SLIP 44) */
#define BIP44_CHANGE_EXTERNAL "0"  /* Change level for external addresses */
#define BIP44_CHANGE_INTERNAL "1"  /* Change level for internal addresses */

/* BIP 44 literal constants */
#define BIP44_MAX_ADDRESS 2^31 - 1 /* Maximum address is 2^31 - 1 */
#define BIP44_KEY_PATH_MAX_LENGTH 255 /* Maximum length of key path string */
#define BIP44_ADDRESS_GAP_LIMIT 20 /* Maximum gap between unused addresses */
#define BIP44_FIRST_ACCOUNT_NODE 0 /* Index of the first account node */
#define BIP44_HARDENED_ADDRESS 1 /* Use hardened addressess */

/* Derives a BIP 44 extended private key from a master private key. */
/* Master private key to derive from */
/* Account index (literal) */
/* Derived address index, set to NULL to get an extended key */
/* Change level ("0" for external or "1" for internal addresses */
/* Use hardned addresses */
/* Key path string generated */
/* BIP 44 extended private key generated */
/* return 0 (success), -1 (fail) */
int derive_bip44_extended_private_key(const dogecoin_hdnode *master_key, const uint32_t account, const uint32_t* address_index, const char* change_level, const int hardened, char* keypath, dogecoin_hdnode *bip44_key);

/* Derives a BIP 44 extended public key from a master public key. */
/* Master public key to derive from */
/* Account index (literal) */
/* Derived address index, set to NULL to get an extended key */
/* Change level ("0" for external or "1" for internal addresses */
/* Use hardned addresses */
/* Key path string generated */
/* BIP 44 extended public key generated */
/* return 0 (success), -1 (fail) */
int derive_bip44_extended_public_key(const dogecoin_hdnode *master_key, const uint32_t account, const uint32_t* address_index, const char* change_level, const int hardened, char* keypath, dogecoin_hdnode *bip44_key);
#endif // __LIBDOGECOIN_BIP44_H__
