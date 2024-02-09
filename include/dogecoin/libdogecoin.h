/*

 The MIT License (MIT)

 Copyright (c) 2023 bluezr
 Copyright (c) 2023 edtubbs
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

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "dogecoin.h"
#include "constants.h"
#include "uthash.h"


/* Chainparams
--------------------------------------------------------------------------
*/
typedef struct dogecoin_dns_seed_ {
    char domain[256];
} dogecoin_dns_seed;

typedef struct dogecoin_chainparams_ {
    char chainname[32];
    uint8_t b58prefix_pubkey_address;
    uint8_t b58prefix_script_address;
    const char bech32_hrp[5];
    uint8_t b58prefix_secret_address; //!private key
    uint32_t b58prefix_bip32_privkey;
    uint32_t b58prefix_bip32_pubkey;
    const unsigned char netmagic[4];
    uint256 genesisblockhash;
    int default_port;
    dogecoin_dns_seed dnsseeds[8];
} dogecoin_chainparams;

typedef struct dogecoin_checkpoint_ {
    uint32_t height;
    const char* hash;
    uint32_t timestamp;
    uint32_t target;
} dogecoin_checkpoint;

extern const dogecoin_chainparams dogecoin_chainparams_main;
extern const dogecoin_chainparams dogecoin_chainparams_test;
extern const dogecoin_chainparams dogecoin_chainparams_regtest;

// the mainnet checkpoints, needs a fix size
extern const dogecoin_checkpoint dogecoin_mainnet_checkpoint_array[22];
extern const dogecoin_checkpoint dogecoin_testnet_checkpoint_array[18];

const dogecoin_chainparams* chain_from_b58_prefix(const char* address);
int chain_from_b58_prefix_bool(char* address);

/* basic address functions: return 1 if succesful
   ----------------------------------------------
*///!init static ecc context
void dogecoin_ecc_start(void);

//!destroys the static ecc context
void dogecoin_ecc_stop(void);

//#define PRIVKEYWIFLEN 51 //WIF length for uncompressed keys is 51 and should start with Q. This can be 52 also for compressed keys. 53 internally to lib (+stringterm)
#define PRIVKEYWIFLEN 53 //Function takes 53 but needs to be fixed to take 51.

//#define HDKEYLEN 111 //should be chaincode + privkey; starts with dgpv51eADS3spNJh8 or dgpv51eADS3spNJh9 (112 internally including stringterm? often 128. check this.)
#define HDKEYLEN 112 // Function expects 128 but needs to be fixed to take 111.

//#define P2PKHLEN 34 //our mainnet addresses are 34 chars if p2pkh and start with D.  Internally this is cited as 35 for strings that represent it because +stringterm.
#define P2PKHLEN 35 // Function expects 35, but needs to be fixed to take 34.

//#define PUBKEYHEXLEN 67 //should be 66 for hex pubkey.  Internally this is cited as 67 for strings that represent it because +stringterm.
#define PUBKEYHEXLEN 67

//#define PUBKEYHASHLEN 40 //should be 40 for pubkeyhash.  Internally this is cited as 41 for strings that represent it because +stringterm.
#define PUBKEYHASHLEN 41

//#define KEYPATHMAXLEN 255 // Maximum length of key path string.  Internally this is cited as 256 for strings that represent it because +stringterm.
#define KEYPATHMAXLEN 256

/* check if a given address is a testnet address */
dogecoin_bool isTestnetFromB58Prefix(const char address[P2PKHLEN]);

/* check if a given address is a mainnet address */
dogecoin_bool isMainnetFromB58Prefix(const char address[P2PKHLEN]);

/* generates a private and public keypair (a wallet import format private key and a p2pkh ready-to-use corresponding dogecoin address)*/
int generatePrivPubKeypair(char wif_privkey[PRIVKEYWIFLEN], char p2pkh_pubkey[P2PKHLEN], dogecoin_bool is_testnet);

/* generates a hybrid deterministic HD master key and p2pkh ready-to-use corresponding dogecoin address. */
int generateHDMasterPubKeypair(char hd_privkey_master[HDKEYLEN], char p2pkh_pubkey_master[P2PKHLEN], dogecoin_bool is_testnet);

/* generates a new dogecoin address from a HD master key */
int generateDerivedHDPubkey(const char hd_privkey_master[HDKEYLEN], char p2pkh_pubkey[P2PKHLEN]);

/* verify that a private key and dogecoin address match */
int verifyPrivPubKeypair(char wif_privkey[PRIVKEYWIFLEN], char p2pkh_pubkey[P2PKHLEN], dogecoin_bool is_testnet);

/* verify that a HD Master key and a dogecoin address matches */
int verifyHDMasterPubKeypair(char hd_privkey_master[HDKEYLEN], char p2pkh_pubkey_master[P2PKHLEN], dogecoin_bool is_testnet);

/* verify that a dogecoin address is valid. */
int verifyP2pkhAddress(char p2pkh_pubkey[P2PKHLEN], size_t len);

/* get derived hd address */
int getDerivedHDAddress(const char masterkey[HDKEYLEN], uint32_t account, dogecoin_bool ischange, uint32_t addressindex, char outaddress[P2PKHLEN], dogecoin_bool outprivkey);

/* get derived hd address by custom path */
int getDerivedHDAddressByPath(const char masterkey[HDKEYLEN], const char derived_path[KEYPATHMAXLEN], char outaddress[P2PKHLEN], dogecoin_bool outprivkey);

/* generate the p2pkh address from a given hex pubkey */
dogecoin_bool addresses_from_pubkey(const dogecoin_chainparams* chain, const char pubkey_hex[PUBKEYHEXLEN], char p2pkh_address[P2PKHLEN]);
int getAddressFromPubkey(const char pubkey_hex[PUBKEYHEXLEN], const dogecoin_bool is_testnet, char p2pkh_address[P2PKHLEN]);

/* generate the hex publickey from a given WIF private key */
dogecoin_bool pubkey_from_privatekey(const dogecoin_chainparams* chain, const char privkey_wif[PRIVKEYWIFLEN], char pubkey_hex[PUBKEYHEXLEN], size_t* sizeout);
int getPubkeyFromPrivkey(const char privkey_wif[PRIVKEYWIFLEN], const dogecoin_bool is_testnet, char pubkey_hex[PUBKEYHEXLEN], size_t* sizeout);

/* generate a new private key (hex) */
dogecoin_bool gen_privatekey(const dogecoin_chainparams* chain, char privkey_wif[PRIVKEYWIFLEN], size_t strsize_wif, char privkey_hex[PRIVKEYHEXLEN]);
int genPrivkey(const dogecoin_bool is_testnet, char privkey_wif[PRIVKEYWIFLEN], size_t strsize_wif, char privkey_hex[PRIVKEYHEXLEN]);

/* p2pkh utilities */
dogecoin_bool dogecoin_pubkey_hash_to_p2pkh_address(char script_pubkey_hex[PUBKEYHEXLEN], size_t script_pubkey_hex_length, char p2pkh[P2PKHLEN], const dogecoin_chainparams* chain);
dogecoin_bool dogecoin_p2pkh_address_to_pubkey_hash(char p2pkh[P2PKHLEN], char scripthash[PUBKEYHASHLEN]);
char* dogecoin_address_to_pubkey_hash(char p2pkh[PUBKEYHEXLEN]);
char* dogecoin_private_key_wif_to_pubkey_hash(char private_key_wif[PRIVKEYWIFLEN]);

/* generate the p2pkh address from a given pubkey hash */
int getAddrFromPubkeyHash(const char pubkey_hash[PUBKEYHASHLEN], const dogecoin_bool is_testnet, char p2pkh_address[P2PKHLEN]);

/* privkey utilities */
typedef struct dogecoin_key_ {
    uint8_t privkey[DOGECOIN_ECKEY_PKEY_LENGTH];
} dogecoin_key;

void dogecoin_privkey_encode_wif(const dogecoin_key* privkey, const dogecoin_chainparams* chain, char privkey_wif[PRIVKEYWIFLEN], size_t* strsize_inout);
dogecoin_bool dogecoin_privkey_decode_wif(const char privkey_wif[PRIVKEYWIFLEN], const dogecoin_chainparams* chain, dogecoin_key* privkey);

/* wrappers for wif encoding/decoding */
void getWifEncodedPrivKey(const char privkey[DOGECOIN_ECKEY_PKEY_LENGTH], const dogecoin_bool is_testnet, char privkey_wif[PRIVKEYWIFLEN], size_t* strsize_wif);
int getDecodedPrivKeyWif(const char privkey_wif[PRIVKEYWIFLEN], const dogecoin_bool is_testnet, char privkey_hex[DOGECOIN_ECKEY_PKEY_LENGTH]);

/* bip32 utilities */
#define DOGECOIN_BIP32_CHAINCODE_SIZE 32

/* BIP 32 512-bit seed */
#define MAX_SEED_SIZE 64
typedef uint8_t SEED [MAX_SEED_SIZE];

typedef struct
{
    uint32_t depth;
    uint32_t fingerprint;
    uint32_t child_num;
    uint8_t chain_code[DOGECOIN_BIP32_CHAINCODE_SIZE];
    uint8_t private_key[DOGECOIN_ECKEY_PKEY_LENGTH];
    uint8_t public_key[DOGECOIN_ECKEY_COMPRESSED_LENGTH];
} dogecoin_hdnode;

dogecoin_hdnode* dogecoin_hdnode_new();
dogecoin_hdnode* dogecoin_hdnode_copy(const dogecoin_hdnode* hdnode);
void dogecoin_hdnode_free(dogecoin_hdnode* node);
dogecoin_bool dogecoin_hdnode_public_ckd(dogecoin_hdnode* inout, uint32_t i);
dogecoin_bool dogecoin_hdnode_from_seed(const uint8_t* seed, int seed_len, dogecoin_hdnode* out);
dogecoin_bool dogecoin_hdnode_private_ckd(dogecoin_hdnode* inout, uint32_t i);
void dogecoin_hdnode_fill_public_key(dogecoin_hdnode* node);
void dogecoin_hdnode_serialize_public(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, size_t strsize);
void dogecoin_hdnode_serialize_private(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, size_t strsize);

void dogecoin_hdnode_get_hash160(const dogecoin_hdnode* node, uint160 hash160_out);
void dogecoin_hdnode_get_p2pkh_address(const dogecoin_hdnode* node, const dogecoin_chainparams* chain, char* str, size_t strsize);
dogecoin_bool dogecoin_hdnode_get_pub_hex(const dogecoin_hdnode* node, char* str, size_t* strsize);
dogecoin_bool dogecoin_hdnode_deserialize(const char* str, const dogecoin_chainparams* chain, dogecoin_hdnode* node);

/* bip32 wrappers for key derivation */
dogecoin_bool getHDRootKeyFromSeed(const SEED seed, const int seed_len, const dogecoin_bool is_testnet, char masterkey[HDKEYLEN]);
dogecoin_bool getHDPubKey(const char hdkey[HDKEYLEN], const dogecoin_bool is_testnet, char hdpubkey[HDKEYLEN]);
dogecoin_bool deriveExtKeyFromHDKey(const char extkey[HDKEYLEN], const char keypath[KEYPATHMAXLEN], const dogecoin_bool is_testnet, char key[HDKEYLEN]);
dogecoin_bool deriveExtPubKeyFromHDKey(const char extpubkey[HDKEYLEN], const char keypath[KEYPATHMAXLEN], const dogecoin_bool is_testnet, char pubkey[HDKEYLEN]);

/* bip32 tools */
int genHDMaster(const dogecoin_bool is_testnet, char masterkey[HDKEYLEN], size_t strsize);
int printNode(const dogecoin_bool is_testnet, const char nodeser[HDKEYLEN]);
int deriveHDExtFromMaster(const dogecoin_bool is_testnet, const char masterkey[HDKEYLEN], const char keypath[KEYPATHMAXLEN], char extkeyout[HDKEYLEN], size_t extkeyout_size);

/* get derived hd extended child key and corresponding private key in WIF format */
char* getHDNodePrivateKeyWIFByPath(const char masterkey[HDKEYLEN], const char derived_path[KEYPATHMAXLEN], char outaddress[P2PKHLEN], bool outprivkey);
/* get derived hd extended address and compendium hdnode */
dogecoin_hdnode* getHDNodeAndExtKeyByPath(const char masterkey[HDKEYLEN], const char derived_path[KEYPATHMAXLEN], char outaddress[P2PKHLEN], bool outprivkey);

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

/* A string representation of change level used to generate a BIP 44 key path */
/* The change level should be a string equal to "0" or "1" with a maximum size of BIP44_CHANGE_LEVEL_SIZE */
typedef char CHANGE_LEVEL [BIP44_CHANGE_LEVEL_SIZE];

/* A string representation of key path used to derive BIP 44 keys */
/* The key path should be a string with a maximum size of BIP44_KEY_PATH_MAX_SIZE */
typedef char KEY_PATH [BIP44_KEY_PATH_MAX_SIZE];

/* Derives a BIP 44 extended key from a master key. */
/* Master key to derive from */
/* Account index, set to NULL to get an extended key */
/* Derived address index, set to NULL to get an extended key */
/* Change level ("0" for external or "1" for internal addresses), set to NULL to get an extended key */
/* Custom path string (optional, account and change_level ignored) */
/* Test net flag */
/* Key path string generated */
/* BIP 44 extended key generated */
/* return 0 (success), -1 (fail) */
int derive_bip44_extended_key(const dogecoin_hdnode *master_key, const uint32_t* account, const uint32_t* address_index, const CHANGE_LEVEL change_level, const KEY_PATH path, const dogecoin_bool is_testnet, KEY_PATH keypath, dogecoin_hdnode *bip44_key);

/* Derives a BIP 44 extended private key from a master key. */
/* Master key to derive from */
/* Account index */
/* Change level ("0" for external or "1" for internal addresses) */
/* Derived address index */
/* Custom path string (optional, account and change_level ignored) */
/* Extended private key generated */
/* Key path string generated */
dogecoin_bool deriveBIP44ExtendedKey(
    const char hd_privkey_master[HDKEYLEN],
    const uint32_t* account,
    const CHANGE_LEVEL change_level,
    const uint32_t* address_index,
    const KEY_PATH path,
    char extkeyout[HDKEYLEN],
    KEY_PATH keypath);

/* Derives a BIP 44 extended public key from a master key. */
/* Master key to derive from */
/* Account index */
/* Change level ("0" for external or "1" for internal addresses) */
/* Derived address index */
/* Custom path string (optional, account and change_level ignored) */
/* Extended public key generated */
/* Key path string generated */
dogecoin_bool deriveBIP44ExtendedPublicKey(
    const char hd_privkey_master[HDKEYLEN],
    const uint32_t* account,
    const CHANGE_LEVEL change_level,
    const uint32_t* address_index,
    const KEY_PATH path,
    char extkeyout[HDKEYLEN],
    KEY_PATH keypath);

/* utilities */
uint8_t* utils_hex_to_uint8(const char* str);
char* utils_uint8_to_hex(const uint8_t* bin, size_t l);

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

/* Generates an English mnemonic phrase from given hex entropy */
int generateEnglishMnemonic(const HEX_ENTROPY entropy, const ENTROPY_SIZE size, MNEMONIC mnemonic);

/* Generates a random (e.g. "128" or "256") English mnemonic phrase */
int generateRandomEnglishMnemonic(const ENTROPY_SIZE size, MNEMONIC mnemonic);

/* Generates a seed from an mnemonic seedphrase */
int dogecoin_seed_from_mnemonic(const MNEMONIC mnemonic, const PASS pass, SEED seed);

/* Generates a HD master key and p2pkh ready-to-use corresponding dogecoin address from a mnemonic */
int getDerivedHDAddressFromMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const MNEMONIC mnemonic, const PASS pass, char* p2pkh_pubkey, const bool is_testnet);

/* TPM2 utilities */

/* Encrypted file numbers */
#define NO_FILE -1
#define DEFAULT_FILE 0
#define MAX_FILES 1000
#define TEST_FILE 999

/* Encrypt a BIP32 seed with the TPM */
dogecoin_bool dogecoin_encrypt_seed_with_tpm (const SEED seed, const size_t size, const int file_num, const dogecoin_bool overwrite);

/* Decrypt a BIP32 seed with the TPM */
dogecoin_bool dogecoin_decrypt_seed_with_tpm (SEED seed, const int file_num);

/* Generate a BIP39 mnemonic and encrypt it with the TPM */
dogecoin_bool dogecoin_generate_mnemonic_encrypt_with_tpm(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite, const char* lang, const char* space, const char* words);

/* Decrypt a BIP39 mnemonic with the TPM */
dogecoin_bool dogecoin_decrypt_mnemonic_with_tpm(MNEMONIC mnemonic, const int file_num);

/* Generate a BIP32 HD node and encrypt it with the TPM */
dogecoin_bool dogecoin_generate_hdnode_encrypt_with_tpm(dogecoin_hdnode* out, const int file_num, const dogecoin_bool overwrite);

/* Decrypt a BIP32 HD node object with the TPM */
dogecoin_bool dogecoin_decrypt_hdnode_with_tpm(dogecoin_hdnode* out, const int file_num);

/* Generate a 256-bit random english mnemonic with the TPM */
dogecoin_bool generateRandomEnglishMnemonicTPM(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite);

/* generates a new dogecoin address from an encrypted seed and a slip44 key path */
int getDerivedHDAddressFromEncryptedSeed(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const dogecoin_bool is_testnet, const int file_num);

/* generates a new dogecoin address from an encrypted mnemonic and a slip44 key path */
int getDerivedHDAddressFromEncryptedMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const PASS pass, char* p2pkh_pubkey, const bool is_testnet, const int file_num);

/* generates a new dogecoin address from an encrypted HD node and a slip44 key path */
int getDerivedHDAddressFromEncryptedHDNode(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const bool is_testnet, const int file_num);

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

/* Sign a formed transaction with working transaction index (txindex), prevout.n index (vout_index) and private key (privkey) */
int sign_transaction_w_privkey(int txindex, int vout_index, char* privkey);

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

dogecoin_bool broadcast_raw_tx(const dogecoin_chainparams* chain, const char* raw_hex_tx);


/* Koinu functions
--------------------------------------------------------------------------
*/
int koinu_to_coins_str(uint64_t koinu, char* str);
uint64_t coins_to_koinu_str(char* coins);


/* Memory functions
--------------------------------------------------------------------------
*/
char* dogecoin_char_vla(size_t size);
void dogecoin_free(void* ptr);


/* Advanced API for signing arbitrary messages
--------------------------------------------------------------------------
*/

typedef struct dogecoin_pubkey_ {
    dogecoin_bool compressed;
    uint8_t pubkey[DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH];
} dogecoin_pubkey;

typedef struct eckey {
    int idx;
    dogecoin_key private_key;
    char private_key_wif[PRIVKEYWIFLEN];
    dogecoin_pubkey public_key;
    char public_key_hex[PUBKEYHEXLEN];
    char address[P2PKHLEN];
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


/* Vector API
--------------------------------------------------------------------------
*/

typedef struct vector {
    void** data;  /* array of pointers */
    size_t len;   /* array element count */
    size_t alloc; /* allocated array elements */

    void (*elem_free_f)(void*);
} vector;

#define vector_idx(vec, idx) vec->data[idx]

vector* vector_new(size_t res, void (*free_f)(void*));
void vector_free(vector* vec, dogecoin_bool free_array);
dogecoin_bool vector_add(vector* vec, void* data);
dogecoin_bool vector_remove(vector* vec, void* data);
void vector_remove_idx(vector* vec, size_t idx);
void vector_remove_range(vector* vec, size_t idx, size_t len);
dogecoin_bool vector_resize(vector* vec, size_t newsz);
ssize_t vector_find(vector* vec, void* data);


/* Wallet API
--------------------------------------------------------------------------
*/

int dogecoin_unregister_watch_address_with_node(char* address);
int dogecoin_get_utxo_vector(char* address, vector* utxos);
uint8_t* dogecoin_get_utxos(char* address);
unsigned int dogecoin_get_utxos_length(char* address);
char* dogecoin_get_utxo_txid_str(char* address, unsigned int index);
uint8_t* dogecoin_get_utxo_txid(char* address, unsigned int index);
uint64_t dogecoin_get_balance(char* address);
char* dogecoin_get_balance_str(char* address);
uint64_t dogecoin_get_balance(char* address);
char* dogecoin_get_balance_str(char* address);
