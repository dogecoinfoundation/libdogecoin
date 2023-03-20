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

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "dogecoin.h"
#include "constants.h"
#include "uthash.h"

/* basic address functions: return 1 if succesful
   ----------------------------------------------
*///!init static ecc context
void dogecoin_ecc_start(void);

//!destroys the static ecc context
void dogecoin_ecc_stop(void);

/* generates a private and public keypair (a wallet import format private key and a p2pkh ready-to-use corresponding dogecoin address)*/
int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* generates a hybrid deterministic WIF master key and p2pkh ready-to-use corresponding dogecoin address. */
int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* generates a new dogecoin address from a HD master key */
int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey);

/* verify that a private key and dogecoin address match */
int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet);

/* verify that a HD Master key and a dogecoin address matches */
int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet);

/* verify that a dogecoin address is valid. */
int verifyP2pkhAddress(char* p2pkh_pubkey, size_t len);

/* get derived hd address */
int getDerivedHDAddress(const char* masterkey, uint32_t account, bool ischange, uint32_t addressindex, char* outaddress, bool outprivkey);

/* get derived hd address by custom path */
int getDerivedHDAddressByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey);

/* Advanced API functions for mnemonic seedphrase generation
--------------------------------------------------------------------------
*/

/* BIP 39 entropy */
#define ENT_STRING_SIZE 3
typedef char ENTROPY_SIZE [ENT_STRING_SIZE];

/* BIP 39 hex entropy */
#define MAX_HEX_ENT_SIZE 64 + 1
typedef char HEX_ENTROPY [MAX_HEX_ENT_SIZE];

/* BIP 39 mnemonic */
#define MAX_MNEMONIC_SIZE 1024
typedef char MNEMONIC [MAX_MNEMONIC_SIZE];

/* BIP 39 passphrase */
#define MAX_PASS_SIZE 256
typedef char PASS [MAX_PASS_SIZE];

/* BIP 32 512-bit seed */
#define MAX_SEED_SIZE 64
typedef uint8_t SEED [MAX_SEED_SIZE];

/* BIP 32 change level */
#define CHG_LEVEL_STRING_SIZE 2
typedef char CHANGE_LEVEL [CHG_LEVEL_STRING_SIZE];

/* Generates an English mnemonic phrase from given hex entropy */
int generateEnglishMnemonic(const HEX_ENTROPY entropy, const ENTROPY_SIZE size, MNEMONIC mnemonic);

/* Generates a random (e.g. "128" or "256") English mnemonic phrase */
int generateRandomEnglishMnemonic(const ENTROPY_SIZE size, MNEMONIC mnemonic);

/* Generates a seed from an mnemonic seedphrase */
int dogecoin_seed_from_mnemonic(const MNEMONIC mnemonic, const PASS pass, SEED seed);

/* Generates a HD master key and p2pkh ready-to-use corresponding dogecoin address from a mnemonic */
int getDerivedHDAddressFromMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const MNEMONIC mnemonic, const PASS pass, char* p2pkh_pubkey, const bool is_testnet);

/* Transaction creation functions - builds a dogecoin transaction
----------------------------------------------------------------
*/

/* create a new dogecoin transaction: Returns the (txindex) in memory of the transaction being worked on. */
int start_transaction();

/* add a utxo to the transaction being worked on at (txindex), specifying the utxo's txid and vout. returns 1 if successful.*/
int add_utxo(int txindex, char* hex_utxo_txid, int vout);

/* add an output to the transaction being worked on at (txindex) of amount (amount) in dogecoins, returns 1 if successful. */
int add_output(int txindex, char* destinationaddress, char* amount);

/* finalize the transaction being worked on at (txindex), with the (destinationaddress) paying a fee of (subtractedfee), */
/* re-specify the amount in dogecoin for verification, and change address for change. If not specified, change will go to the first utxo's address. */
char* finalize_transaction(int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* changeaddress);

/* sign a raw transaction in memory at (txindex), sign (inputindex) with (scripthex) of (sighashtype), with (privkey) */
int sign_transaction(int txindex, char* script_pubkey, char* privkey);

/* clear all internal working transactions */
void remove_all();

/* retrieve the raw transaction at (txindex) as a hex string (char*) */
char* get_raw_transaction(int txindex);

/* clear the transaction at (txindex) in memory */
void clear_transaction(int txindex);

/* QR Code Generation Functions
---------------------------------
*/

//TODO: These are strings not just P2PKH but we need to set a min and max, perhaps only accept wif and p2pkh.

/*populate an array of bits that represent qrcode pixels*/
/* returns size(L or W) in pixels of QR.*/
int qrgen_p2pkh_to_qrbits(const char* in_p2pkh, uint8_t* outQrByteArray);

/* create a QR text formatted string (with line breaks) from an incoming p2pkh*/
int qrgen_p2pkh_to_qr_string(const char* in_p2pkh, char* outString);

/* Prints the given p2pkh addr as QR Code to the console. */
void qrgen_p2pkh_consoleprint_to_qr(char* in_p2pkh);

/* Creates a .png file with the filename outFilename, from string inString, w. size factor of SizeMultiplier.*/
int qrgen_string_to_qr_pngfile(const char* outFilename, const char* inString, uint8_t sizeMultiplier);


/* Creates a .jpg file with the filename outFilename, from string inString, w. size factor of SizeMultiplier.*/
int qrgen_string_to_qr_jpgfile(const char* outFilename, const char* inString, uint8_t sizeMultiplier);


/* Advanced API functions for operating on already formed raw transactions
--------------------------------------------------------------------------
*/

/*Sign a raw transaction hexadecimal string using inputindex, scripthex, sighashtype and privkey. */
int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey);

/*Store a raw transaction that's already formed, and give it a txindex in memory. (txindex) is returned as int. */
int store_raw_transaction(char* incomingrawtx);


/* Memory functions
--------------------------------------------------------------------------
*/
char* dogecoin_char_vla(size_t size);
void dogecoin_free(void* ptr);


/* Advanced API for signing arbitrary messages
--------------------------------------------------------------------------
*/

typedef struct dogecoin_key_ {
    uint8_t privkey[DOGECOIN_ECKEY_PKEY_LENGTH];
} dogecoin_key;

typedef struct dogecoin_pubkey_ {
    dogecoin_bool compressed;
    uint8_t pubkey[DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH];
} dogecoin_pubkey;

typedef struct eckey {
    int idx;
    dogecoin_key private_key;
    char private_key_wif[128];
    dogecoin_pubkey public_key;
    char public_key_hex[128];
    char address[35];
    UT_hash_handle hh;
} eckey;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static eckey *keys = NULL;
#pragma GCC diagnostic pop

// instantiates a new eckey
eckey* new_eckey();

// adds eckey structure to hash table
void add_eckey(eckey *key);

// find eckey from the hash table
eckey* find_eckey(int idx);

// remove eckey from the hash table
void remove_eckey(eckey *key);

// instantiates and adds key to the hash table
int start_key();

/* sign a message with a private key */
char* sign_message(char* privkey, char* msg);

/* verify a message with a address */
int verify_message(char* sig, char* msg, char* address);
