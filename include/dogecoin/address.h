/*
 The MIT License (MIT)

 Copyright (c) 2022 bluezr
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

#ifndef __LIBDOGECOIN_ADDRESS_H__
#define __LIBDOGECOIN_ADDRESS_H__

#include <stdbool.h>

#include <dogecoin/bip39.h>
#include <dogecoin/bip44.h>
#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

/* generate a new private key (wif) and p2pkh public key */
LIBDOGECOIN_API int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* generate HD master key and p2pkh public key */
LIBDOGECOIN_API int generateHDMasterPubKeypair(char* hd_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* generate an extended public key */
LIBDOGECOIN_API int generateDerivedHDPubkey(const char* hd_privkey_master, char* p2pkh_pubkey);

/* verify private and public keys are valid and associated with each other*/
LIBDOGECOIN_API int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* verify private and public masters keys are valid and associated with each other */
LIBDOGECOIN_API int verifyHDMasterPubKeypair(char* hd_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* verify address based on length and checksum */
LIBDOGECOIN_API int verifyP2pkhAddress(char* p2pkh_pubkey, size_t len);

/* get derived hd extended child key and corresponding private key in WIF format */
LIBDOGECOIN_API char* getHDNodePrivateKeyWIFByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey);

/* get derived hd extended address and compendium hdnode */
LIBDOGECOIN_API dogecoin_hdnode* getHDNodeAndExtKeyByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey);

/* generate an extended hd public/private child address */
LIBDOGECOIN_API int getDerivedHDAddress(const char* masterkey, uint32_t account, bool ischange, uint32_t addressindex, char* outaddress, bool outprivkey);

/* generate an extended hd public/private child address as a P2PKH */
LIBDOGECOIN_API int getDerivedHDAddressAsP2PKH(const char* masterkey, uint32_t account, bool ischange, uint32_t addressindex, char* outp2pkh);

/* generate an extended hd public/private child address with a more flexible derived path */
LIBDOGECOIN_API int getDerivedHDAddressByPath(const char* masterkey, const char* derived_path, char* outaddress);

/* generate an extended hd public/private child key with a more flexible derived path */
LIBDOGECOIN_API int getDerivedHDKeyByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey);

bool getDerivedHDNodeByPath(const char* masterkey, const char* derived_path, dogecoin_hdnode *nodenew);

/* generates a new dogecoin address from a mnemonic and a slip44 key path */
LIBDOGECOIN_API int getDerivedHDAddressFromMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const MNEMONIC mnemonic, const PASSPHRASE pass, char* p2pkh_pubkey, const dogecoin_bool is_testnet);

/* generates a HD master key and p2pkh ready-to-use corresponding dogecoin address from a mnemonic */
LIBDOGECOIN_API int generateHDMasterPubKeypairFromMnemonic(char* hd_privkey_master, char* p2pkh_pubkey_master, const MNEMONIC mnemonic, const PASSPHRASE pass, const dogecoin_bool is_testnet);

/* verify that a HD master key and a dogecoin address matches a mnemonic */
LIBDOGECOIN_API int verifyHDMasterPubKeypairFromMnemonic(const char* hd_privkey_master, const char* p2pkh_pubkey_master, const MNEMONIC mnemonic, const PASSPHRASE pass, const dogecoin_bool is_testnet);

/* generates a new dogecoin address from an encrypted seed and a slip44 key path */
LIBDOGECOIN_API int getDerivedHDAddressFromEncryptedSeed(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const dogecoin_bool is_testnet, const int file_num);

/* generates a HD master key and p2pkh ready-to-use corresponding dogecoin address from an encrypted seed */
LIBDOGECOIN_API int generateHDMasterPubKeypairFromEncryptedSeed(char* hd_privkey_master, char* p2pkh_pubkey_master, const dogecoin_bool is_testnet, const int file_num);

/* verify that a HD master key and a dogecoin address matches an encrypted seed */
LIBDOGECOIN_API int verifyHDMasterPubKeypairFromEncryptedSeed(const char* hd_privkey_master, const char* p2pkh_pubkey_master, const dogecoin_bool is_testnet, const int file_num);

/* generates a new dogecoin address from an encrypted mnemonic and a slip44 key path */
LIBDOGECOIN_API int getDerivedHDAddressFromEncryptedMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const PASSPHRASE pass, char* p2pkh_pubkey, const bool is_testnet, const int file_num);

/* generates a new dogecoin address from a encrypted master (HD) key and a slip44 key path */
LIBDOGECOIN_API int getDerivedHDAddressFromEncryptedHDNode(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const bool is_testnet, const int file_num);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_ADDRESS_H__
