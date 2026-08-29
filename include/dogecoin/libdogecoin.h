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
#include "cstr.h"


/* Chainparams
--------------------------------------------------------------------------
*/
typedef struct dogecoin_dns_seed_ {
    char domain[256];
} dogecoin_dns_seed;

/* Declared here and again in the internal chainparams.h, so that this header
   stays self-contained. The objects are defined once, in chainparams.c, against
   the internal copy and read through this one -- so the two must describe the
   same layout, field for field and in order. A field added to one and not the
   other shifts every field after it with no compile error and no warning; that
   is what happened to genesisblockchainwork, which left consumers reading
   default_port out of the middle of the chainwork. Change both, in step.
   test/chainparams_abi_tests.c fails if they diverge again. */
typedef struct dogecoin_chainparams_ {
    char chainname[32];
    uint8_t b58prefix_pubkey_address;
    uint8_t b58prefix_script_address;
    const char bech32_hrp[5];
    uint8_t b58prefix_secret_address; //!private key
    uint32_t b58prefix_bip32_privkey;
    uint32_t b58prefix_bip32_pubkey;
    const unsigned char netmagic[4];
    uint256_t genesisblockhash;
    uint256_t genesisblockchainwork;
    int default_port;
    dogecoin_dns_seed dnsseeds[8];
    dogecoin_bool strict_id;
    dogecoin_bool auxpow_id;
    uint256_t pow_limit;
    uint256_t minimumchainwork;
} dogecoin_chainparams;

typedef struct dogecoin_checkpoint_ {
    uint32_t height;
    const char* hash;
    uint32_t timestamp;
    uint32_t target;
} dogecoin_checkpoint;

struct dogecoin_transaction_context;
struct dogecoin_eckey_context;
typedef struct dogecoin_context_ {
    const dogecoin_chainparams* chain_params;
    struct dogecoin_transaction_context* tx_ctx;
    struct dogecoin_eckey_context* key_ctx;
    void* ecc_ctx;
    void* rng_state;
    void* refcount_lock;
    int enable_net;
    int error_code;
    uint32_t refcount;
    int thread_safe; /* nonzero when constructed via dogecoin_ctx_new_ts() (alias in dogecoin.h) */
    char last_error[256];
} dogecoin_context;

/* forward declarations for network/SPV node types */
typedef struct dogecoin_node_ dogecoin_node;
typedef struct dogecoin_spv_client_ dogecoin_spv_client;
typedef struct dogecoin_wallet_ dogecoin_wallet;

/* forward declarations for transaction container types */
typedef struct dogecoin_tx_ dogecoin_tx;
typedef struct dogecoin_smpv_tx_ dogecoin_smpv_tx;
/* forward declaration for PSBT (BIP174/BIP370) container type */
typedef struct dogecoin_psbt dogecoin_psbt;
typedef struct dogecoin_smpv_watcher_ dogecoin_smpv_watcher;

/* forward declaration for the SMPV client context type */
typedef struct dogecoin_smpv_client_ dogecoin_smpv_client;

/* callback invoked for smpv-processed transactions */
typedef void (*dogecoin_smpv_tx_callback)(
    const dogecoin_smpv_tx* tx,
    const char* address,
    void* user_data
);

extern const dogecoin_chainparams dogecoin_chainparams_main;
extern const dogecoin_chainparams dogecoin_chainparams_test;
extern const dogecoin_chainparams dogecoin_chainparams_regtest;

// the mainnet checkpoints, needs a fix size
extern const dogecoin_checkpoint dogecoin_mainnet_checkpoint_array[];
extern const size_t dogecoin_mainnet_checkpoint_count;
extern const dogecoin_checkpoint dogecoin_testnet_checkpoint_array[];
extern const size_t dogecoin_testnet_checkpoint_count;

const dogecoin_chainparams* chain_from_b58_prefix(const char* address);
int chain_from_b58_prefix_bool(char* address);

dogecoin_context* dogecoin_context_new(dogecoin_bool testnet, dogecoin_bool enable_net);
void dogecoin_context_acquire(dogecoin_context* ctx);
void dogecoin_context_release(dogecoin_context* ctx);
const dogecoin_chainparams* dogecoin_context_get_chainparams(const dogecoin_context* ctx);
struct dogecoin_transaction_context* dogecoin_context_get_transaction_context(dogecoin_context* ctx);
struct dogecoin_eckey_context* dogecoin_context_get_eckey_context(dogecoin_context* ctx);
void* dogecoin_context_get_ecc_context(dogecoin_context* ctx);
void* dogecoin_context_get_rng_state(dogecoin_context* ctx);
void dogecoin_context_set_error(dogecoin_context* ctx, int code, const char* msg);
int dogecoin_context_get_error_code(const dogecoin_context* ctx);
const char* dogecoin_context_get_error(const dogecoin_context* ctx);
int dogecoin_generate_keypair_ex(dogecoin_context* ctx, char* wif, size_t* wif_size, char* addr, size_t* addr_size);

/* THREAD-SAFE short-form aliases for the dogecoin_context API.
 * dogecoin_ctx_new() returns a context whose refcount is guarded by a
 * process-wide mutex; dogecoin_ctx_new_ts() additionally tags the context
 * as thread-safe so dependent subsystems (wallet / tx / spv helpers) can
 * select per-object locking when wired through the context. The non-_ts
 * constructor remains thread-compatible: any single owning thread may use
 * it, and reference counting is always atomic. */
dogecoin_ctx* dogecoin_ctx_new(dogecoin_bool testnet, dogecoin_bool enable_net);
dogecoin_ctx* dogecoin_ctx_new_ts(dogecoin_bool testnet, dogecoin_bool enable_net);
void dogecoin_ctx_acquire(dogecoin_ctx* ctx);
void dogecoin_ctx_release(dogecoin_ctx* ctx);
int dogecoin_ctx_is_thread_safe(const dogecoin_ctx* ctx);

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
#define P2PKHLEN 35 // Function expects 35, 34-char address + NUL terminator.

//#define PUBKEYHEXLEN 67 //should be 66 for hex pubkey.  Internally this is cited as 67 for strings that represent it because +stringterm.
#define PUBKEYHEXLEN 67

//#define PUBKEYHASHLEN 40 //should be 40 for pubkeyhash.  Internally this is cited as 41 for strings that represent it because +stringterm.
#define PUBKEYHASHLEN 41 // bare hash160 hex only, not a scriptPubKey

//#define SCRIPTPUBKEYLEN 50 //should be 50 for pubkeyhash.  Internally this is cited as 51 for strings that represent it because +stringterm.
#define SCRIPTPUBKEYLEN 51

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

/* get derived hd extended key at m/44'/3'/<account>'/<ischange>/<addressindex>.
 *
 * NOTE: despite the name, this writes a serialized extended key (dgpv.../dgub...),
 * not a P2PKH address, and it writes up to HDKEYLEN bytes.  The output buffer
 * must be sized accordingly.  See issue #217.  If you want an address, use
 * getDerivedHDAddressAsP2PKH() instead. */
int getDerivedHDAddress(const char masterkey[HDKEYLEN], uint32_t account, dogecoin_bool ischange, uint32_t addressindex, char outaddress[HDKEYLEN], dogecoin_bool outprivkey);

/* get derived hd p2pkh address */
int getDerivedHDAddressAsP2PKH(const char masterkey[HDKEYLEN], uint32_t account, dogecoin_bool ischange, uint32_t addressindex, char outp2pkh[P2PKHLEN]);

/* get derived hd address by custom path */
int getDerivedHDAddressByPath(const char masterkey[HDKEYLEN], const char derived_path[KEYPATHMAXLEN], char outaddress[P2PKHLEN]);

/* get derived hd key by custom path */
int getDerivedHDKeyByPath(const char masterkey[HDKEYLEN], const char derived_path[KEYPATHMAXLEN], char outaddress[HDKEYLEN], dogecoin_bool outprivkey);

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
/* takes raw scriptPubKey bytes, not hex, and the length varies by script */
dogecoin_bool dogecoin_pubkey_hash_to_p2pkh_address(char* script_pubkey, size_t script_pubkey_len, char p2pkh[P2PKHLEN], const dogecoin_chainparams* chain);
dogecoin_bool dogecoin_p2pkh_address_to_pubkey_hash(char p2pkh[P2PKHLEN], char scripthash[SCRIPTPUBKEYLEN]);
char* dogecoin_address_to_pubkey_hash(char p2pkh[P2PKHLEN]);
char* dogecoin_private_key_wif_to_pubkey_hash(char private_key_wif[PRIVKEYWIFLEN]);

/* generate the p2pkh address from a given scriptPubKey hex */
int getAddrFromScriptPubKey(const char script_pubkey_hex[SCRIPTPUBKEYLEN], const dogecoin_bool is_testnet, char p2pkh_address[P2PKHLEN]);

/* generate the p2pkh address from a given hash160 hex, inverts dogecoin_address_to_pubkey_hash */
int getAddrFromPubkeyHash(const char pubkey_hash[PUBKEYHASHLEN], const dogecoin_bool is_testnet, char p2pkh_address[P2PKHLEN]);

/* privkey utilities */
typedef struct dogecoin_key_ {
    uint8_t privkey[DOGECOIN_ECKEY_PKEY_LENGTH];
} dogecoin_key;

/* initialize a private key */
void dogecoin_privkey_init(dogecoin_key* privkey);
/* check if a private key is valid */
dogecoin_bool dogecoin_privkey_is_valid(const dogecoin_key* privkey);
/* securely wipe a private key from memory */
void dogecoin_privkey_cleanse(dogecoin_key* privkey);
/* generate a new random private key */
dogecoin_bool dogecoin_privkey_gen(dogecoin_key* privkey);
/* form a WIF encoded string from the given privkey */
void dogecoin_privkey_encode_wif(const dogecoin_key* privkey, const dogecoin_chainparams* chain, char privkey_wif[PRIVKEYWIFLEN], size_t* strsize_inout);
dogecoin_bool dogecoin_privkey_decode_wif(const char privkey_wif[PRIVKEYWIFLEN], const dogecoin_chainparams* chain, dogecoin_key* privkey);

/* wrappers for wif encoding/decoding */
void getWifEncodedPrivKey(const char privkey[DOGECOIN_ECKEY_PKEY_LENGTH], const dogecoin_bool is_testnet, char privkey_wif[PRIVKEYWIFLEN], size_t* strsize_wif);
int getDecodedPrivKeyWif(const char privkey_wif[PRIVKEYWIFLEN], const dogecoin_bool is_testnet, char privkey_hex[DOGECOIN_ECKEY_PKEY_LENGTH]);

/* bip32 utilities */
#define DOGECOIN_BIP32_CHAINCODE_SIZE 32

/* BIP 32 512-bit seed; MAX_SEED_SIZE and SEED come from dogecoin.h */

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

void dogecoin_hdnode_get_hash160(const dogecoin_hdnode* node, uint160_t hash160_out);
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
void utils_hex_to_bin(const char* str, unsigned char* out, size_t inLen, size_t* outLen);
void utils_bin_to_hex(const unsigned char* bin_in, size_t inlen, char* hex_out);
char* getpass(const char *prompt);

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

/* Verifies the mnemonic phrase */
int dogecoin_verify_mnemonic (const char* mnemonic, const char* language, const char* space, const char* filename);

/* Generates a HD master key and p2pkh ready-to-use corresponding dogecoin address from a mnemonic */
int getDerivedHDAddressFromMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const MNEMONIC mnemonic, const PASS pass, char* p2pkh_pubkey, const bool is_testnet);

/* SLIP-0039 (Shamir's Secret-Sharing for Mnemonic Codes) */
#ifndef SLIP0039_DECLS_DEFINED
#define SLIP0039_DECLS_DEFINED
#define SLIP0039_MAX_SHARES 16
#define SLIP0039_MAX_SHARE_STR_SIZE 320
#define SLIP0039_MIN_SECRET_BYTES 16
#define SLIP0039_MAX_SECRET_BYTES 32

typedef char SLIP0039_SHARE[SLIP0039_MAX_SHARE_STR_SIZE];

/* Splits a secret into SLIP-0039 mnemonic shares */
LIBDOGECOIN_API int dogecoin_slip0039_generate_shares(const uint8_t* secret, size_t secret_len, uint8_t threshold, uint8_t share_count, char shares[][SLIP0039_MAX_SHARE_STR_SIZE]);

/* Recovers a secret from a set of SLIP-0039 mnemonic shares */
LIBDOGECOIN_API int dogecoin_slip0039_recover_secret(const char* shares[], size_t share_count, const uint8_t* passphrase, size_t passphrase_len, uint8_t* secret_out, size_t* secret_len_out);
#endif /* SLIP0039_DECLS_DEFINED */

/* Generates a HD master key and p2pkh address from a mnemonic */
int generateHDMasterPubKeypairFromMnemonic(char hd_privkey_master[HDKEYLEN], char p2pkh_pubkey_master[P2PKHLEN], const MNEMONIC mnemonic, const PASS pass, const dogecoin_bool is_testnet);

/* Verifies HD master key and p2pkh address against mnemonic */
int verifyHDMasterPubKeypairFromMnemonic(const char hd_privkey_master[HDKEYLEN], const char p2pkh_pubkey_master[P2PKHLEN], const MNEMONIC mnemonic, const PASS pass, const dogecoin_bool is_testnet);

/* Generates a HD master key and p2pkh address from encrypted seed */
int generateHDMasterPubKeypairFromEncryptedSeed(char hd_privkey_master[HDKEYLEN], char p2pkh_pubkey_master[P2PKHLEN], const dogecoin_bool is_testnet, const int file_num);

/* Verifies HD master key and p2pkh address against encrypted seed */
int verifyHDMasterPubKeypairFromEncryptedSeed(const char hd_privkey_master[HDKEYLEN], const char p2pkh_pubkey_master[P2PKHLEN], const dogecoin_bool is_testnet, const int file_num);

/* TPM2 utilities */

/* Encrypted file numbers */
#define NO_FILE -1
#define DEFAULT_FILE 0
#define MAX_FILES 1000
#define TEST_FILE 999

/* Encrypted BLOB */
#define MAX_ENCRYPTED_BLOB_SIZE 2048
typedef uint8_t ENCRYPTED_BLOB[MAX_ENCRYPTED_BLOB_SIZE];

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

/* Encrypt a BIP32 seed with software */
dogecoin_bool dogecoin_encrypt_seed_with_sw(const SEED seed, const size_t size, const int file_num, const dogecoin_bool overwrite, const char* test_password, ENCRYPTED_BLOB* encrypted_blob_out, size_t* encrypted_blob_size);

/* Decrypt a BIP32 seed with software */
dogecoin_bool dogecoin_decrypt_seed_with_sw (SEED seed, const int file_num, const char* test_password, ENCRYPTED_BLOB encrypted_blob);

/* Generate a BIP39 mnemonic and encrypt it with software */
dogecoin_bool dogecoin_generate_mnemonic_encrypt_with_sw(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite, const char* lang, const char* space, const char* words, const char* test_password, ENCRYPTED_BLOB* encrypted_blob_out, size_t* encrypted_blob_size);

/* Decrypt a BIP39 mnemonic with software */
dogecoin_bool dogecoin_decrypt_mnemonic_with_sw(MNEMONIC mnemonic, const int file_num, const char* test_password, ENCRYPTED_BLOB encrypted_blob);

/* Generate a BIP32 HD node and encrypt it with software */
dogecoin_bool dogecoin_generate_hdnode_encrypt_with_sw(dogecoin_hdnode* out, const int file_num, const dogecoin_bool overwrite, const char* test_password, ENCRYPTED_BLOB* encrypted_blob_out, size_t* encrypted_blob_size);

/* Decrypt a BIP32 HD node object with software */
dogecoin_bool dogecoin_decrypt_hdnode_with_sw(dogecoin_hdnode* out, const int file_num, const char* test_password, ENCRYPTED_BLOB encrypted_blob);

/* Generate a 256-bit random english mnemonic with software */
dogecoin_bool generateRandomEnglishMnemonicSW(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite, uint8_t** encrypted_mnemonic_out, size_t* encrypted_mnemonic_size);

/* Encrypt a BIP32 seed with software and store it on YubiKey */
dogecoin_bool dogecoin_encrypt_seed_with_sw_to_yubikey(const SEED seed, const size_t size, const int file_num, const dogecoin_bool overwrite, const char* test_password);

/* Decrypt a BIP32 seed with software after retrieving it from YubiKey */
dogecoin_bool dogecoin_decrypt_seed_with_sw_from_yubikey(SEED seed, const int file_num, const char* test_password);

/* Generate a BIP39 mnemonic, encrypt it with software, and store it on YubiKey */
dogecoin_bool dogecoin_generate_mnemonic_encrypt_with_sw_to_yubikey(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite, const char* lang, const char* space, const char* words, const char* test_password);

/* Decrypt a BIP39 mnemonic with software after retrieving it from YubiKey */
dogecoin_bool dogecoin_decrypt_mnemonic_with_sw_from_yubikey(MNEMONIC mnemonic, const int file_num, const char* test_password);

/* Generate a BIP32 HD node, encrypt it with software, and store it on YubiKey */
dogecoin_bool dogecoin_generate_hdnode_encrypt_with_sw_to_yubikey(dogecoin_hdnode* out, const int file_num, const dogecoin_bool overwrite, const char* test_password);

/* Decrypt a BIP32 HD node with software after retrieving it from YubiKey */
dogecoin_bool dogecoin_decrypt_hdnode_with_sw_from_yubikey(dogecoin_hdnode* out, const int file_num, const char* test_password);

/* List all encryption keys in the TPM */
dogecoin_bool dogecoin_list_encryption_keys_in_tpm(wchar_t* names[], size_t* count);

/* generates a new dogecoin address from an encrypted seed and a slip44 key path */
int getDerivedHDAddressFromEncryptedSeed(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const dogecoin_bool is_testnet, const int file_num);

/* generates a new dogecoin address from an encrypted mnemonic and a slip44 key path */
int getDerivedHDAddressFromEncryptedMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const PASS pass, char* p2pkh_pubkey, const bool is_testnet, const int file_num);

/* generates a new dogecoin address from an encrypted HD node and a slip44 key path */
int getDerivedHDAddressFromEncryptedHDNode(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const bool is_testnet, const int file_num);

/* generates a new dogecoin address from an encrypted seed and a custom key path */
int getDerivedHDAddressFromAcctPubKey(const char* ext_pubkey, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const bool is_testnet);

/* Transaction creation functions - builds a dogecoin transaction
----------------------------------------------------------------
*/

//#define TXHEXMAXLEN 200001 // Maximum length of standard tx based on relay limit (100 000).  Internally this is cited as 200001 for strings that represent it because +stringterm.
#define TXHEXMAXLEN 200001

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

/* retrieve the raw transaction at (txindex) as a hex string (char*) in a buffer (buf) of size (buf_cap) */
int get_raw_transaction_ex(int txindex, char* buf, size_t buf_cap);

/* sign a raw transaction in memory at (txindex), sign (inputindex) with (scripthex) of (sighashtype), with (privkey) in a buffer (signedrawtx) of size (signed_size) */
int sign_raw_transaction_ex(int inputindex, const char* incomingrawtx, char* signedrawtx, size_t* signed_size, const char* scripthex, int sighashtype, const char* privkey);

/* finalize the transaction being worked on at (txindex), with the (destinationaddress) paying a fee of (subtractedfee) in a buffer (buf) of size (buf_cap) */
int finalize_transaction_ex(int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* changeaddress, char* buf, size_t buf_cap);

/* sign and store one vin of the working tx at (txindex); writes signed hex into (buf) with capacity (buf_cap) */
int sign_indexed_raw_transaction_ex(int txindex, int inputindex, const char* scripthex, int sighashtype, const char* privkey, char* buf, size_t buf_cap);

/* sign all inputs of the working tx at (txindex) with (script_pubkey)/(privkey); writes signed hex into (buf) with capacity (buf_cap) */
int sign_transaction_ex(int txindex, const char* script_pubkey, const char* privkey, char* buf, size_t buf_cap);

/* convenience wrapper: sign a single-key p2pkh tx at (txindex) using (privkey); writes signed hex into (buf) with capacity (buf_cap) */
int sign_transaction_w_privkey_ex(int txindex, const char* privkey, char* buf, size_t buf_cap);

/* build an M-of-N P2SH multisig address from (n) compressed pubkey hex strings;
   writes the P2SH address into (p2sh_addr_out) (must hold at least P2PKHLEN bytes,
   capacity given by (p2sh_addr_cap)) and the redeem script hex into
   (redeem_script_hex_out) (must hold at least (n*68+6)*2+1 bytes, capacity given
   by (redeem_script_hex_cap));
   returns 1 on success, 0 on error (including insufficient buffer capacity) */
int get_p2sh_multisig_address(const char** pubkeys_hex, int n, int m, int is_testnet,
                               char* p2sh_addr_out, size_t p2sh_addr_cap,
                               char* redeem_script_hex_out, size_t redeem_script_hex_cap);

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
int koinu_to_coins_str(uint64_t koinu, char* str, size_t str_size);
uint64_t coins_to_koinu_str(char* coins);


/* Memory functions
--------------------------------------------------------------------------
*/
char* dogecoin_char_vla(size_t size);
/* allocate zero-initialized memory for count elements of size bytes each */
void* dogecoin_calloc(size_t count, size_t size);
/* allocate an unsigned char variable-length array */
unsigned char* dogecoin_uchar_vla(size_t size);
void dogecoin_free(void* ptr);
volatile void* dogecoin_mem_zero(volatile void* dst, size_t len);


/* Advanced API for signing arbitrary messages
--------------------------------------------------------------------------
*/

typedef struct dogecoin_pubkey_ {
    dogecoin_bool compressed;
    uint8_t pubkey[DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH];
} dogecoin_pubkey;

/* initialize a public key */
void dogecoin_pubkey_init(dogecoin_pubkey* pubkey);
/* check if a public key is valid */
dogecoin_bool dogecoin_pubkey_is_valid(const dogecoin_pubkey* pubkey);
/* securely wipe a public key from memory */
void dogecoin_pubkey_cleanse(dogecoin_pubkey* pubkey);
/* derive a public key from a private key */
void dogecoin_pubkey_from_key(const dogecoin_key* privkey, dogecoin_pubkey* pubkey_inout);
/* sign a 32-byte hash and return a 64-byte compact signature with recovery id (fixed compression) */
dogecoin_bool dogecoin_key_sign_hash_compact_recoverable_fcomp(const dogecoin_key* privkey, const uint256_t hash, unsigned char* sigout, size_t* outlen, int* recid);
/* recover a public key from a compact signature and recovery id */
dogecoin_bool dogecoin_key_recover_pubkey(const unsigned char* sig, const uint256_t hash, int recid, dogecoin_pubkey* pubkey);
/* verify a compact encoded signature with given pubkey and return true if valid */
dogecoin_bool dogecoin_pubkey_verify_sigcmp(const dogecoin_pubkey* pubkey, const uint256_t hash, unsigned char* sigcmp);
/* derive a p2pkh address from a public key */
dogecoin_bool dogecoin_pubkey_getaddr_p2pkh(const dogecoin_pubkey* pubkey, const dogecoin_chainparams* chain, char* addrout);

typedef struct eckey {
    int idx;
    dogecoin_key private_key;
    char private_key_wif[PRIVKEYWIFLEN];
    dogecoin_pubkey public_key;
    char public_key_hex[PUBKEYHEXLEN];
    char address[P2PKHLEN];
    UT_hash_handle hh;
} eckey;

// instantiates a new eckey
eckey* new_eckey(dogecoin_bool is_testnet);
// instantiates a new eckey from a WIF-encoded private key
eckey* new_eckey_from_privkey(char* key);

// adds eckey structure to hash table
void add_eckey(eckey *key);

// find eckey from the hash table
eckey* find_eckey(int idx);

// remove eckey from the hash table
void remove_eckey(eckey *key);
// free the memory allocated for an eckey
void dogecoin_key_free(eckey* eckey);

// instantiates and adds key to the hash table
int start_key(dogecoin_bool is_testnet);

/* sign a message with a private key */
char* sign_message(char* privkey, char* msg);

/* verify a message with a address */
int verify_message(char* sig, char* msg, char* address);


/* Vector API
--------------------------------------------------------------------------
*/

/* create a new vector with initial reserve and optional element destructor */
vector_t* vector_new(size_t res, void (*free_f)(void*));
/* free vector internals and optionally free backing array */
void vector_free(vector_t* vec, dogecoin_bool free_array);
/* append an element pointer to the vector */
dogecoin_bool vector_add(vector_t* vec, void* data);
/* remove first matching element pointer from the vector */
dogecoin_bool vector_remove(vector_t* vec, void* data);
/* remove vector element at index */
void vector_remove_idx(vector_t* vec, size_t idx);
/* remove a contiguous range of vector elements */
void vector_remove_range(vector_t* vec, size_t idx, size_t len);
/* resize vector capacity/length bookkeeping */
dogecoin_bool vector_resize(vector_t* vec, size_t newsz);
/* find index of an element pointer, or -1 if absent */
ssize_t vector_find(vector_t* vec, void* data);


/* Wallet API
--------------------------------------------------------------------------
*/

/* read wallet data for a watched address */
dogecoin_wallet* dogecoin_wallet_read(char* address);
/* unregister address from connected node watch list */
int dogecoin_unregister_watch_address_with_node(char* address);
/* register address with connected node watch list */
int dogecoin_register_watch_address_with_node(char* address);
/* fill provided vector with utxo entries for address */
int dogecoin_get_utxo_vector(char* address, vector_t* utxos);
/* get serialized utxo bytes for address */
uint8_t* dogecoin_get_utxos(char* address);
/* get utxo entry count for address */
unsigned int dogecoin_get_utxos_length(char* address);
/* get utxo txid hex string at index */
char* dogecoin_get_utxo_txid_str(char* address, unsigned int index);
/* get utxo txid bytes at index */
uint8_t* dogecoin_get_utxo_txid(char* address, unsigned int index);
/* get utxo amount value at index */
uint64_t dogecoin_get_utxo_amount(char* address, unsigned int index);
/* get utxo output index (vout) at index */
uint32_t dogecoin_get_utxo_vout(char* address, unsigned int index);
/* get total confirmed balance in koinu for address */
uint64_t dogecoin_get_balance(char* address);
/* get total confirmed balance as decimal string for address */
char* dogecoin_get_balance_str(char* address);

/* SPV API
--------------------------------------------------------------------------
*/
/* create a new spv client instance */
dogecoin_spv_client* dogecoin_spv_client_new(const dogecoin_chainparams* params, dogecoin_bool debug, dogecoin_bool headers_memonly, dogecoin_bool use_checkpoints, dogecoin_bool full_sync, int maxnodes, const char* http_server);
/* free an spv client instance */
void dogecoin_spv_client_free(dogecoin_spv_client* client);
/* load spv client headers and state from file */
dogecoin_bool dogecoin_spv_client_load(dogecoin_spv_client* client, const char* file_path, dogecoin_bool prompt);
/* discover peers from DNS seeds or provided ips */
void dogecoin_spv_client_discover_peers(dogecoin_spv_client* client, const char* ips);
/* run the spv client main loop */
void dogecoin_spv_client_runloop(dogecoin_spv_client* client);
/* request headers from connected peers */
dogecoin_bool dogecoin_net_spv_request_headers(dogecoin_spv_client* client);
/* request either headers or blocks from a peer node */
void dogecoin_net_spv_node_request_headers_or_blocks(dogecoin_node* node, dogecoin_bool blocks);
/* enable or disable simple payment verification tracking */
void dogecoin_spv_enable_smpv(dogecoin_spv_client* client, dogecoin_bool enable);
/* process and track a mempool transaction from hex */
dogecoin_bool dogecoin_spv_handle_mempool_tx_hex(dogecoin_spv_client* client, const char* raw_tx_hex);
/* retrieve smpv transaction and watch statistics */
void dogecoin_spv_get_smpv_stats(dogecoin_spv_client* client, uint32_t* total_txs, uint32_t* watched_addrs);
/* request mempool contents from peers */
void dogecoin_net_spv_request_mempool(dogecoin_spv_client* client);

/* SMPV API
--------------------------------------------------------------------------
*/
/* create an SMPV client instance for chain parameters */
dogecoin_smpv_client* dogecoin_smpv_client_new(const dogecoin_chainparams* chain_params);
/* free an SMPV client instance and owned resources */
void dogecoin_smpv_client_free(dogecoin_smpv_client* client);
/* add an address to SMPV watch list */
dogecoin_bool dogecoin_smpv_add_watcher(dogecoin_smpv_client* client, const char* address);
/* remove an address from SMPV watch list */
dogecoin_bool dogecoin_smpv_remove_watcher(dogecoin_smpv_client* client, const char* address);
/* retrieve a watcher record by address */
dogecoin_smpv_watcher* dogecoin_smpv_get_watcher(const dogecoin_smpv_client* client, const char* address);
/* start SMPV processing state */
dogecoin_bool dogecoin_smpv_start(dogecoin_smpv_client* client);
/* stop SMPV processing state */
void dogecoin_smpv_stop(dogecoin_smpv_client* client);
/* decode, filter and process a raw transaction with callbacks */
dogecoin_bool dogecoin_smpv_process_tx(dogecoin_smpv_client* client, const char* raw_tx_hex, dogecoin_smpv_tx_callback callback, void* user_data);
/* get tracked SMPV transaction by txid */
dogecoin_smpv_tx* dogecoin_smpv_get_tx(const dogecoin_smpv_client* client, const char* txid);
/* decode a raw transaction hex string into a tx object */
dogecoin_tx* dogecoin_smpv_decode_tx(const char* raw_tx_hex);
/* update tracked transaction status and optional block data */
void dogecoin_smpv_update_tx_status(dogecoin_smpv_client* client, const char* txid, dogecoin_bool confirmed, const char* block_hash, uint32_t block_height);
/* retrieve SMPV transaction and watcher totals */
void dogecoin_smpv_get_stats(const dogecoin_smpv_client* client, uint32_t* total_txs, uint32_t* watched_addresses);
/* convert tracked smpv transaction data to json */
char* dogecoin_smpv_tx_to_json(const dogecoin_smpv_tx* tx);
/* convert watcher data to json */
char* dogecoin_smpv_watcher_to_json(const dogecoin_smpv_watcher* watcher);

/* Random API
--------------------------------------------------------------------------
*/

/* fill buffer with cryptographically secure random bytes */
dogecoin_bool dogecoin_random_bytes(uint8_t* buf, uint32_t len, const uint8_t update_seed);

/* Replace the byte source, for callers that have one of their own -- an enclave
   or a trusted application whose platform supplies entropy and where the POSIX
   fallback is not merely unnecessary but unavailable.

   This takes the callback rather than the dogecoin_rnd_mapper struct that
   dogecoin/random.h exposes, deliberately. This header is self-contained and
   an enclave includes it rather than the internal ones, but a struct cannot be
   defined in both: C forbids re-declaring a tag even identically, so any
   translation unit including both headers would fail to compile. A function
   declaration duplicates safely; a struct definition does not.

   Thread-local, like the mapper it sets. Pass NULL to restore the default. */
void dogecoin_rnd_set_bytes_cb(dogecoin_bool (*cb)(uint8_t* buf, uint32_t len, const uint8_t update_seed));

/* Crypto API
--------------------------------------------------------------------------
*/

#define SHA1_DIGEST_LENGTH 20
#define SHA256_DIGEST_LENGTH 32
#define SHA512_DIGEST_LENGTH 64

/* compute hmac-sha1 for the input message */
void hmac_sha1(const uint8_t* key, const size_t keylen, const uint8_t* msg, const size_t msglen, uint8_t* hmac);
/* compute hmac-sha256 for the input message */
void hmac_sha256(const uint8_t* key, const size_t keylen, const uint8_t* msg, const size_t msglen, uint8_t* hmac);
/* compute hmac-sha512 for the input message */
void hmac_sha512(const uint8_t* key, const size_t keylen, const uint8_t* msg, const size_t msglen, uint8_t* hmac);
/* compute sha256 digest for raw input bytes */
void sha256_raw(const uint8_t*, size_t, uint8_t[SHA256_DIGEST_LENGTH]);
/* compute sha1 digest for raw input bytes */
void sha1_Raw(const uint8_t*, size_t, uint8_t[SHA1_DIGEST_LENGTH]);
/* compute sha512 digest for raw input bytes */
void sha512_raw(const uint8_t*, size_t, uint8_t[SHA512_DIGEST_LENGTH]);
/* derive key material with pbkdf2-hmac-sha256 */
void pbkdf2_hmac_sha256(const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen, uint32_t iterations, uint8_t *key, int keylen);
/* derive key material with pbkdf2-hmac-sha512 */
void pbkdf2_hmac_sha512(const uint8_t *pass, int passlen, const uint8_t *salt, int saltlen, uint32_t iterations, uint8_t *key);
/* compute ripemd160 digest for input bytes */
void rmd160(const uint8_t* msg, uint32_t msg_len, uint8_t* hash);

/* Base58 API
--------------------------------------------------------------------------
*/
/* encode binary data to base58 string */
int dogecoin_base58_encode(char* b58, size_t* b58sz, const void* data, size_t binsz);
/* decode base58 string into binary buffer */
int dogecoin_base58_decode(void* bin, size_t* binszp, const char* b58, size_t b58sz);
/* encode binary data to base58check string */
size_t dogecoin_base58_encode_check(const uint8_t* data, size_t datalen, char* str, size_t strsize);
/* decode base58check string and verify checksum */
size_t dogecoin_base58_decode_check(const char* str, uint8_t* data, size_t datalen);

/* Key / ECC API
--------------------------------------------------------------------------
*/
/* derive public key bytes from private key bytes */
void dogecoin_ecc_get_pubkey(const uint8_t* private_key, uint8_t* public_key, size_t* public_key_len, dogecoin_bool compressed);
/* sign a 256-bit hash with a private key */
dogecoin_bool dogecoin_ecc_sign(const uint8_t* private_key, const uint256_t hash, unsigned char* sigder, size_t* outlen);
/* verify a DER signature against a 256-bit hash */
dogecoin_bool dogecoin_ecc_verify_sig(const uint8_t* public_key, dogecoin_bool compressed, const uint256_t hash, unsigned char* sigder, size_t siglen);
/* verify a compact signature against a 256-bit hash */
dogecoin_bool dogecoin_ecc_verify_sigcmp(const uint8_t* public_key, dogecoin_bool compressed, const uint256_t hash, unsigned char* sigcmp);

/* Transaction Object API
--------------------------------------------------------------------------
*/
/* create a new transaction object */
dogecoin_tx* dogecoin_tx_new();
/* free a transaction object */
void dogecoin_tx_free(dogecoin_tx* tx);
/* deep-copy transaction contents */
void dogecoin_tx_copy(dogecoin_tx* dest, const dogecoin_tx* src);
/* deserialize transaction bytes into object */
int dogecoin_tx_deserialize(const unsigned char* tx_serialized, size_t inlen, dogecoin_tx* tx, size_t* consumed_length);
/* compute transaction hash */
void dogecoin_tx_hash(const dogecoin_tx* tx, uint256_t hashout);
/* return whether transaction is coinbase */
dogecoin_bool dogecoin_tx_is_coinbase(dogecoin_tx* tx);
/* add a p2pkh output for an address */
dogecoin_bool dogecoin_tx_add_address_out(dogecoin_tx* tx, const dogecoin_chainparams* chain, int64_t amount, const char* address);
/* add a p2pkh output from hash160 */
dogecoin_bool dogecoin_tx_add_p2pkh_hash160_out(dogecoin_tx* tx, int64_t amount, uint160_t hash160);
/* add a p2sh output from hash160 */
dogecoin_bool dogecoin_tx_add_p2sh_hash160_out(dogecoin_tx* tx, int64_t amount, uint160_t hash160);
/* add an op_return-style data output */
dogecoin_bool dogecoin_tx_add_data_out(dogecoin_tx* tx, const int64_t amount, const uint8_t* data, const size_t datalen);

/* ── Reading a transaction ────────────────────────────────────
   dogecoin_tx is opaque here, so these are the only way to check what a
   transaction actually does: which outpoint an input spends, and what each
   output pays and to what script. Anyone being paid needs them, since a
   signature over a transaction you have not read is a signature over whatever
   the other party chose.
   dogecoin_tx_output_get_scriptpubkey() reports the length it needs in
   (len_out) and returns false when (out) is NULL or (cap) is too small, so
   size then fetch. */
size_t dogecoin_tx_num_inputs(const dogecoin_tx* tx);
size_t dogecoin_tx_num_outputs(const dogecoin_tx* tx);
/* (txid_out) is internal byte order, as dogecoin_tx_hash() produces; reverse to display */
dogecoin_bool dogecoin_tx_input_get_prevout(const dogecoin_tx* tx, size_t idx, uint256_t txid_out, uint32_t* vout_out);
dogecoin_bool dogecoin_tx_output_get_amount(const dogecoin_tx* tx, size_t idx, int64_t* amount_out);
dogecoin_bool dogecoin_tx_output_get_scriptpubkey(const dogecoin_tx* tx, size_t idx, uint8_t* out, size_t cap, size_t* len_out);

/* PSBT API (BIP174 / BIP370)
--------------------------------------------------------------------------
Partially Signed Dogecoin Transactions: a six-role pipeline —
creator → updater → signer → combiner → finalizer → extractor.
All hex and base64 output strings are heap-allocated; free with dogecoin_free().
The extracted dogecoin_tx* is heap-allocated; free with dogecoin_tx_free().
*/

/* wire-format magic length and version constants */
#define PSBT_MAGIC_LEN  5u         /* "psbt\xff" */
#define PSBT_VERSION_0  0x00000000u
#define PSBT_VERSION_2  0x00000002u

/* lifecycle */
dogecoin_psbt* dogecoin_psbt_new(void);
void           dogecoin_psbt_free(dogecoin_psbt* psbt);

/* creator role: wrap an unsigned tx in a new PSBT (all inputs must have empty scriptSigs) */
dogecoin_psbt* dogecoin_psbt_create(const dogecoin_tx* tx);

/* serialization: hex (caller frees with dogecoin_free) */
char*          dogecoin_psbt_to_hex(const dogecoin_psbt* psbt);
dogecoin_bool  dogecoin_psbt_from_hex(const char* hex, dogecoin_psbt** out);

/* serialization: base64 (canonical PSBT wire format; caller frees with dogecoin_free) */
char*          dogecoin_psbt_to_base64(const dogecoin_psbt* psbt);
dogecoin_bool  dogecoin_psbt_from_base64(const char* b64, dogecoin_psbt** out);

/* serialization: raw bytes (caller frees the cstring with cstr_free) */
cstring*       dogecoin_psbt_serialize(const dogecoin_psbt* psbt);
dogecoin_bool  dogecoin_psbt_deserialize(const uint8_t* data, size_t len, dogecoin_psbt** out);

/* updater role: attach full previous tx for input idx (enables sighash derivation) */
dogecoin_bool  dogecoin_psbt_input_set_utxo(dogecoin_psbt* psbt, size_t idx, const dogecoin_tx* utxo);
/* updater role: attach P2SH redeem script for input idx */
dogecoin_bool  dogecoin_psbt_input_set_redeemscript(dogecoin_psbt* psbt, size_t idx, const uint8_t* script, size_t len);
/* updater role: set the sighash type for input idx */
dogecoin_bool  dogecoin_psbt_input_set_sighash(dogecoin_psbt* psbt, size_t idx, uint32_t sighash_type);
/* updater role: attach BIP32 derivation path to a pubkey for input idx */
dogecoin_bool  dogecoin_psbt_input_add_keypath(dogecoin_psbt* psbt, size_t idx, const uint8_t* pubkey, size_t pubkey_len, uint32_t fingerprint, const uint32_t* path, size_t path_len);
/* updater role: attach P2SH redeem script for output idx */
dogecoin_bool  dogecoin_psbt_output_set_redeemscript(dogecoin_psbt* psbt, size_t idx, const uint8_t* script, size_t len);
/* updater role: attach BIP32 derivation path to a pubkey for output idx */
dogecoin_bool  dogecoin_psbt_output_add_keypath(dogecoin_psbt* psbt, size_t idx, const uint8_t* pubkey, size_t pubkey_len, uint32_t fingerprint, const uint32_t* path, size_t path_len);

/* signer role: sign all inputs that can be signed with this key; returns true if ≥1 signed */
dogecoin_bool  dogecoin_psbt_sign(dogecoin_psbt* psbt, const dogecoin_key* privkey);
/* signer role: sign a specific input by index */
dogecoin_bool  dogecoin_psbt_sign_input(dogecoin_psbt* psbt, size_t idx, const dogecoin_key* privkey);

/* combiner role: merge partial signatures from src into dst */
dogecoin_bool  dogecoin_psbt_combine(dogecoin_psbt* dst, const dogecoin_psbt* src);

/* finalizer role: build final_script_sig for all inputs; returns true when all inputs are finalized */
dogecoin_bool  dogecoin_psbt_finalize(dogecoin_psbt* psbt);
/* finalizer role: build final_script_sig for a single input */
dogecoin_bool  dogecoin_psbt_finalize_input(dogecoin_psbt* psbt, size_t idx);

/* extractor role: produce the fully-signed tx; returns NULL if any input lacks final_script_sig */
dogecoin_tx*   dogecoin_psbt_extract(const dogecoin_psbt* psbt);

/* validation helpers */
dogecoin_bool  dogecoin_psbt_is_valid(const dogecoin_psbt* psbt);
dogecoin_bool  dogecoin_psbt_is_finalized(const dogecoin_psbt* psbt);

/* Post-Quantum Cryptography (PQC) API: PQC carrier helpers and Falcon-512 /
   Dilithium2 (USE_LIBOQS) / Raccoon-G-44 (USE_RACCOON_G) signature schemes. */

/* PQC carrier sizing / wire-format constants. */
#define DOGECOIN_PQC_CARRIER_MAX_CHUNKS 3
#define DOGECOIN_PQC_CARRIER_CHUNK_MAX  520
#define DOGECOIN_PQC_CARRIER_HDR_LEN    8
#define DOGECOIN_PQC_CARRIER_TAG_LEN    8

/* P2PKH scriptSig length bounds used during PQC sighash derivation. */
#define DOGECOIN_PQC_MIN_P2PKH_SCRIPTSIG_LEN 106
#define DOGECOIN_PQC_MAX_P2PKH_SCRIPTSIG_LEN 180

/* DER signature push length bounds (includes 1-byte sighash type). */
#define DOGECOIN_PQC_MIN_DER_SIG_PUSH_LEN 9
#define DOGECOIN_PQC_MAX_DER_SIG_PUSH_LEN 73

/* PQC algorithm discriminant used by carrier extraction and SPV validation. */
typedef enum {
    DOGECOIN_PQC_ALGO_FALCON,
    DOGECOIN_PQC_ALGO_DILITHIUM,
    DOGECOIN_PQC_ALGO_RACCOONG /* available only when built with USE_RACCOON_G */
} dogecoin_pqc_algo_t;

/* Per-algorithm tag / commit / push-total constants. */
#define DOGECOIN_PQC_FALCON_TAG          "FLC1"
#define DOGECOIN_PQC_FALCON_TAG_LEN      4
#define DOGECOIN_PQC_FALCON_COMMIT_LEN   32
#define DOGECOIN_PQC_FALCON_PUSH_TOTAL   (DOGECOIN_PQC_FALCON_TAG_LEN + DOGECOIN_PQC_FALCON_COMMIT_LEN)

#define DOGECOIN_PQC_DILITHIUM_TAG        "DIL2"
#define DOGECOIN_PQC_DILITHIUM_TAG_LEN    4
#define DOGECOIN_PQC_DILITHIUM_COMMIT_LEN 32
#define DOGECOIN_PQC_DILITHIUM_PUSH_TOTAL (DOGECOIN_PQC_DILITHIUM_TAG_LEN + DOGECOIN_PQC_DILITHIUM_COMMIT_LEN)

#define DOGECOIN_PQC_RACCOON_TAG          "RCG4"
#define DOGECOIN_PQC_RACCOON_TAG_LEN      4
#define DOGECOIN_PQC_RACCOON_COMMIT_LEN   32
#define DOGECOIN_PQC_RACCOON_PUSH_TOTAL   (DOGECOIN_PQC_RACCOON_TAG_LEN + DOGECOIN_PQC_RACCOON_COMMIT_LEN)
#define DOGECOIN_PQC_RACCOON_CHAINCODE_LEN 32

/* Build a P2SH redeem script / scriptPubKey for PQC carrier outputs. */
dogecoin_bool dogecoin_pqc_carrier_build_redeemscript(cstring** out_redeem);
dogecoin_bool dogecoin_pqc_carrier_build_p2sh_scriptpubkey(const cstring* redeem, cstring** out_spk);

/* Build a carrier scriptSig for one part of a multi-part PQC payload. */
dogecoin_bool dogecoin_pqc_carrier_build_part_scriptsig(
    const char tag8[DOGECOIN_PQC_CARRIER_TAG_LEN],
    uint8_t part_index,
    uint8_t part_total,
    uint16_t pk_len,
    uint16_t full_len,
    const uint8_t* part_data,
    size_t part_data_len,
    const cstring* redeem,
    cstring** out_scriptsig);

/* Parse a carrier scriptSig to extract tag, part metadata, and payload. */
dogecoin_bool dogecoin_pqc_carrier_parse_part_scriptsig(
    const cstring* scriptsig,
    char out_tag8[DOGECOIN_PQC_CARRIER_TAG_LEN + 1],
    uint8_t* out_part_index,
    uint8_t* out_part_total,
    uint16_t* out_pk_len,
    uint16_t* out_full_len,
    uint8_t** out_part_data,
    size_t* out_part_data_len,
    cstring** out_redeem);

/* Add carrier P2SH outputs to a transaction. */
dogecoin_bool dogecoin_tx_add_pqc_carrier_outputs(
    dogecoin_tx* tx,
    const cstring* carrier_spk,
    uint64_t value,
    uint8_t part_total);

/* Reassemble PQC pubkey+sig from carrier-format scriptSigs.
   Caller must free *carrier_buf with dogecoin_free(). */
dogecoin_bool dogecoin_pqc_carrier_extract_scriptsig(
    const dogecoin_tx* tx,
    dogecoin_pqc_algo_t* out_algo,
    const uint8_t** out_pk,
    size_t* out_pk_len,
    const uint8_t** out_sig,
    size_t* out_sig_len,
    size_t* out_vin_index,
    uint8_t** carrier_buf,
    size_t* carrier_buf_len);

/* Verify a PQC carrier reveal by reconstructing TX_BASE from raw TX_C bytes,
   deriving the sighash32, and verifying the PQC signature over it. */
dogecoin_bool dogecoin_pqc_carrier_verify_reveal(
    dogecoin_pqc_algo_t algo,
    const uint8_t* txc_raw,
    size_t txc_raw_len,
    const uint8_t* pk,
    size_t pk_len,
    const uint8_t* sig,
    size_t sig_len,
    uint8_t out_sighash[32]);

/* Falcon-512 (requires USE_LIBOQS).  Caller must free pk, sk and sig with dogecoin_free(). */
dogecoin_bool dogecoin_falcon512_keypair(uint8_t** pk, size_t* pk_len, uint8_t** sk, size_t* sk_len);
dogecoin_bool dogecoin_falcon512_sign(const uint8_t* sk, size_t sk_len, const uint8_t* msg, size_t msg_len, uint8_t** sig, size_t* sig_len);
dogecoin_bool dogecoin_falcon512_verify(const uint8_t* pk, size_t pk_len, const uint8_t* msg, size_t msg_len, const uint8_t* sig, size_t sig_len);
dogecoin_bool dogecoin_falcon512_commit_bytes(const uint8_t* pk, size_t pk_len, const uint8_t* sig, size_t sig_len, uint8_t commit32[DOGECOIN_PQC_FALCON_COMMIT_LEN]);
dogecoin_bool dogecoin_tx_add_falcon512_commit(dogecoin_tx* tx, const uint8_t commit32[DOGECOIN_PQC_FALCON_COMMIT_LEN]);
dogecoin_bool dogecoin_tx_extract_falcon512_commit(const dogecoin_tx* tx, uint8_t out_commit32[DOGECOIN_PQC_FALCON_COMMIT_LEN]);

/* Dilithium2 (requires USE_LIBOQS).  Caller must free pk, sk and sig with dogecoin_free(). */
dogecoin_bool dogecoin_dilithium2_keypair(uint8_t** pk, size_t* pk_len, uint8_t** sk, size_t* sk_len);
dogecoin_bool dogecoin_dilithium2_sign(const uint8_t* sk, size_t sk_len, const uint8_t* msg, size_t msg_len, uint8_t** sig, size_t* sig_len);
dogecoin_bool dogecoin_dilithium2_verify(const uint8_t* pk, size_t pk_len, const uint8_t* msg, size_t msg_len, const uint8_t* sig, size_t sig_len);
dogecoin_bool dogecoin_dilithium2_commit_bytes(const uint8_t* pk, size_t pk_len, const uint8_t* signature, size_t signature_len, uint8_t commit32[DOGECOIN_PQC_DILITHIUM_COMMIT_LEN]);
dogecoin_bool dogecoin_tx_add_dilithium2_commit(dogecoin_tx* tx, const uint8_t commit32[DOGECOIN_PQC_DILITHIUM_COMMIT_LEN]);
dogecoin_bool dogecoin_tx_extract_dilithium2_commit(const dogecoin_tx* tx, uint8_t out_commit32[DOGECOIN_PQC_DILITHIUM_COMMIT_LEN]);

/* Raccoon-G-44 (requires USE_RACCOON_G).  Caller must free pk, sk, sig and child outputs with dogecoin_free(). */
dogecoin_bool dogecoin_raccoong44_is_available(void);
dogecoin_bool dogecoin_raccoong44_keypair(uint8_t** pk, size_t* pk_len, uint8_t** sk, size_t* sk_len);
dogecoin_bool dogecoin_raccoong44_sign(const uint8_t* sk, size_t sk_len, const uint8_t* msg, size_t msg_len, uint8_t** sig, size_t* sig_len);
dogecoin_bool dogecoin_raccoong44_verify(const uint8_t* pk, size_t pk_len, const uint8_t* msg, size_t msg_len, const uint8_t* sig, size_t sig_len);
dogecoin_bool dogecoin_raccoong44_commit_bytes(const uint8_t* pk, size_t pk_len, const uint8_t* signature, size_t signature_len, uint8_t commit32[DOGECOIN_PQC_RACCOON_COMMIT_LEN]);
dogecoin_bool dogecoin_tx_add_raccoong44_commit(dogecoin_tx* tx, const uint8_t commit32[DOGECOIN_PQC_RACCOON_COMMIT_LEN]);
dogecoin_bool dogecoin_tx_extract_raccoong44_commit(const dogecoin_tx* tx, uint8_t out_commit32[DOGECOIN_PQC_RACCOON_COMMIT_LEN]);

/* Derive Raccoon-G-44 child secret + public key from parent (BIP32-style). */
dogecoin_bool dogecoin_raccoong44_hd_derive_priv(const uint8_t* parent_sk, size_t parent_sk_len,
                                                 const uint8_t* parent_pk, size_t parent_pk_len,
                                                 const uint8_t chaincode[DOGECOIN_PQC_RACCOON_CHAINCODE_LEN],
                                                 uint32_t index, dogecoin_bool hardened,
                                                 uint8_t** child_sk, size_t* child_sk_len,
                                                 uint8_t** child_pk, size_t* child_pk_len);

/* Derive Raccoon-G-44 child public key from parent public key (non-hardened only). */
dogecoin_bool dogecoin_raccoong44_hd_derive_pub(const uint8_t* parent_pk, size_t parent_pk_len,
                                                const uint8_t chaincode[DOGECOIN_PQC_RACCOON_CHAINCODE_LEN],
                                                uint32_t index,
                                                uint8_t** child_pk, size_t* child_pk_len);

/* Zero-Knowledge Proof (ZK) Carrier API: Groth16 / PLONK / STARK payload
   encode/decode, commit hashing, TX_C / TX_R helpers (USE_ZK_CARRIER). */

#define DOGECOIN_ZK_CARRIER_MAGIC      "ZKP1"
#define DOGECOIN_ZK_CARRIER_MAGIC_LEN  4
#define DOGECOIN_ZK_CARRIER_TAG8       "ZKP1FULL"
#define DOGECOIN_ZK_CARRIER_HDR_FIXED  (DOGECOIN_ZK_CARRIER_MAGIC_LEN + 1 + 1 + 4 + 2)
#define DOGECOIN_ZK_OPRETURN_TAG       "DZKC"
#define DOGECOIN_ZK_OPRETURN_TAG_LEN   4
#define DOGECOIN_ZK_OPRETURN_DATA_LEN  (DOGECOIN_ZK_OPRETURN_TAG_LEN + 1 + 32)

/* Wire-format version byte: v0 = legacy (no embedded vk); v1 = self-contained
   reveal with verification-key bytes appended after the proof. */
#define DOGECOIN_ZK_PAYLOAD_VERSION_V0     0x00
#define DOGECOIN_ZK_PAYLOAD_VERSION_V1     0x01
#define DOGECOIN_ZK_PAYLOAD_VERSION_MASK   0x01

/* Selectable proof systems; values are stable on-wire mode selectors. */
typedef enum {
    DOGECOIN_ZK_MODE_GROTH16  = 0,
    DOGECOIN_ZK_MODE_PLONK    = 1,
    DOGECOIN_ZK_MODE_STARK_S2 = 2
} dogecoin_zk_mode_t;

typedef enum {
    DOGECOIN_ZK_OK                  = 0,
    DOGECOIN_ZK_ERR_INVALID_ARG     = -1,
    DOGECOIN_ZK_ERR_BAD_MAGIC       = -2,
    DOGECOIN_ZK_ERR_BAD_MODE        = -3,
    DOGECOIN_ZK_ERR_TRUNCATED       = -4,
    DOGECOIN_ZK_ERR_OOM             = -5,
    DOGECOIN_ZK_ERR_NOT_IMPLEMENTED = -6,
    DOGECOIN_ZK_ERR_DELEGATED       = -7,
    DOGECOIN_ZK_ERR_VERIFY_FAIL     = -8,
    DOGECOIN_ZK_ERR_BAD_VERSION     = -9
} dogecoin_zk_err_t;

/* Encode a proof + public inputs (and optional verification key) into the
   canonical ZK carrier payload.  Caller frees *out_payload with dogecoin_free(). */
dogecoin_zk_err_t dogecoin_zk_encode_payload(
    dogecoin_zk_mode_t mode,
    uint32_t circuit_id,
    const uint8_t* public_inputs,
    size_t public_inputs_len,
    const uint8_t* proof,
    size_t proof_len,
    const uint8_t* vk_bytes,
    size_t vk_len,
    uint8_t** out_payload,
    size_t* out_payload_len);

/* Decode a canonical ZK carrier payload.  Out-pointers alias into the input
   buffer (no allocation); caller must keep `payload` alive while in use. */
dogecoin_zk_err_t dogecoin_zk_decode_payload(
    const uint8_t* payload,
    size_t payload_len,
    dogecoin_zk_mode_t* out_mode,
    uint32_t* out_circuit_id,
    const uint8_t** out_public_inputs,
    size_t* out_public_inputs_len,
    const uint8_t** out_proof,
    size_t* out_proof_len,
    const uint8_t** out_vk,
    size_t* out_vk_len);

/* Compute the TX_C commitment value: SHA256d(payload). */
dogecoin_zk_err_t dogecoin_zk_get_commitment_hash(
    const uint8_t* payload,
    size_t payload_len,
    uint8_t out_commitment[32]);

/* Extract the canonical 25-byte P2PKH scriptPubKey of the signer for input 0
   of `tx`, parsing the standard `<sig> <pubkey>` P2PKH scriptSig.  Returns
   NULL on parse failure; caller frees the cstring. */
cstring* dogecoin_zk_extract_signer_p2pkh_spk(const dogecoin_tx* tx);

/* Compute the tx_base sighash for a candidate TX_C.  This is the value the
   ZK prover MUST bind into its `tx_binding` public input, mirroring the PQC
   carrier sighash binding.  The top byte of the returned digest is zeroed
   to give an unambiguous 248-bit field element for BN254 R1CS. */
dogecoin_zk_err_t dogecoin_zk_compute_tx_base_sighash(
    const dogecoin_tx* tx_c,
    const cstring* signer_p2pkh_spk,
    const cstring* carrier_spk,
    uint8_t out_sighash[32]);

/* Build the canonical OP_RETURN scriptPubKey carrying a ZK commitment. */
dogecoin_zk_err_t dogecoin_zk_build_opreturn_scriptpubkey(
    dogecoin_zk_mode_t mode,
    const uint8_t commitment[32],
    cstring** out_spk);

/* Parse the decimal string at index `idx` of a snarkjs-style public-inputs
   JSON array and convert it to a 32-byte big-endian buffer. */
dogecoin_zk_err_t dogecoin_zk_parse_public_input_be32(
    const uint8_t* public_inputs,
    size_t public_inputs_len,
    size_t idx,
    uint8_t out_be32[32],
    size_t* out_token_count);

/* Append the OP_RETURN commit output and the P2SH carrier outputs (one per
   reveal-part) to an in-progress TX_C.  *out_carrier_spk is the P2SH spk of
   the carrier outputs (caller frees) and *out_part_total is how many parts
   TX_R will spend. */
dogecoin_zk_err_t dogecoin_zk_build_carrier_tx_c(
    dogecoin_tx* tx,
    const uint8_t* payload,
    size_t payload_len,
    dogecoin_zk_mode_t mode,
    uint64_t carrier_value,
    cstring** out_carrier_spk,
    uint8_t* out_part_total);

/* Build the per-part scriptSigs for TX_R; caller frees each scriptsig with
   cstr_free(..., true) and the array itself with dogecoin_free(). */
dogecoin_zk_err_t dogecoin_zk_build_carrier_tx_r_scriptsigs(
    const uint8_t* payload,
    size_t payload_len,
    cstring*** out_scriptsigs,
    uint8_t* out_part_total);

/* Reassemble a previously-revealed payload from a TX_R by walking its inputs.
   Caller frees *out_payload with dogecoin_free(). */
dogecoin_zk_err_t dogecoin_zk_extract_carrier_payload(
    const dogecoin_tx* tx_r,
    uint8_t** out_payload,
    size_t* out_payload_len);

/* Walk a transaction's outputs for the canonical TX_C OP_RETURN commitment.
   Mirrors dogecoin_tx_extract_falcon512_commit for SPV detection. */
dogecoin_bool dogecoin_tx_extract_zk_commit(
    const dogecoin_tx* tx,
    dogecoin_zk_mode_t* out_mode,
    uint8_t out_commit32[32]);

/* Verify a Groth16 proof using the rapidsnark verifier when HAVE_RAPIDSNARK
   is enabled at build time; otherwise returns DOGECOIN_ZK_ERR_NOT_IMPLEMENTED. */
dogecoin_zk_err_t dogecoin_zk_verify_groth16(
    const uint8_t* vk_json,
    size_t vk_json_len,
    const uint8_t* public_json,
    size_t public_json_len,
    const uint8_t* proof_json,
    size_t proof_json_len);

/* Verify any ZK payload by mode, dispatching to the proof-system specific
   verifier.  v1 payloads carry their own vk and ignore `vk_blob`; for v0
   payloads the caller-supplied vk_blob is used. */
dogecoin_zk_err_t dogecoin_zk_verify_proof(
    const uint8_t* payload,
    size_t payload_len,
    const uint8_t* vk_blob,
    size_t vk_blob_len);

/* Proof generation surface — always returns DOGECOIN_ZK_ERR_DELEGATED in
   this build because proving lives outside libdogecoin (snarkjs / rapidsnark
   CLI driven by contrib/zk_carrier/witness_helper.py). */
dogecoin_zk_err_t dogecoin_zk_generate_groth16_proof(
    const uint8_t* witness_json,
    size_t witness_json_len,
    const char* circuit_path,
    uint8_t** out_proof,
    size_t* out_proof_len,
    uint8_t** out_public,
    size_t* out_public_len);

dogecoin_zk_err_t dogecoin_zk_generate_plonk_proof(
    const uint8_t* witness_json,
    size_t witness_json_len,
    const char* circuit_path,
    uint8_t** out_proof,
    size_t* out_proof_len,
    uint8_t** out_public,
    size_t* out_public_len);

/* Human-readable error string.  Never returns NULL. */
const char* dogecoin_zk_strerror(dogecoin_zk_err_t err);

/* BIP38 API
--------------------------------------------------------------------------
*/

/* Encrypt a private key using BIP38 */
dogecoin_bool dogecoin_bip38_encrypt(
    const uint8_t* private_key,
    const char* passphrase,
    const char* address,
    dogecoin_bool compressed,
    char* encrypted_key_out,
    size_t* encrypted_key_size);
/* Decrypt a BIP38 encrypted private key */
dogecoin_bool dogecoin_bip38_decrypt(
    const char* encrypted_key,
    const char* passphrase,
    uint8_t* private_key_out,
    dogecoin_bool* compressed_out);
/* Decrypt with explicit address-hash matching mode (see BIP38_ADDRESS_MATCH_* in bip38.h) */
dogecoin_bool dogecoin_bip38_decrypt_ex(
    const char* encrypted_key,
    const char* passphrase,
    unsigned int address_match_mode,
    uint8_t* private_key_out,
    dogecoin_bool* compressed_out);
/* Decrypt with explicit passphrase byte length (NFC applied; supports embedded NUL) */
dogecoin_bool dogecoin_bip38_decrypt_passphrase(
    const char* encrypted_key,
    const uint8_t* passphrase,
    size_t passphrase_len,
    uint8_t* private_key_out,
    dogecoin_bool* compressed_out);
/* Decrypt with explicit passphrase length and address-hash matching mode */
dogecoin_bool dogecoin_bip38_decrypt_passphrase_ex(
    const char* encrypted_key,
    const uint8_t* passphrase,
    size_t passphrase_len,
    unsigned int address_match_mode,
    uint8_t* private_key_out,
    dogecoin_bool* compressed_out);
/* Non-EC encrypt with explicit passphrase byte length (NFC applied; supports embedded NUL) */
dogecoin_bool dogecoin_bip38_encrypt_passphrase(
    const uint8_t* private_key,
    const uint8_t* passphrase,
    size_t passphrase_len,
    const char* address,
    dogecoin_bool compressed,
    char* encrypted_key_out,
    size_t* encrypted_key_size);
/* Decrypt a BIP38 encrypted private key with lot/sequence */
dogecoin_bool dogecoin_bip38_decrypt_with_lot_sequence(
    const char* encrypted_key,
    const char* passphrase,
    uint8_t* private_key_out,
    dogecoin_bool* compressed_out,
    uint32_t* lot_out,
    uint32_t* sequence_out);
/* Decrypt with lot/sequence and explicit address-hash matching mode */
dogecoin_bool dogecoin_bip38_decrypt_with_lot_sequence_ex(
    const char* encrypted_key,
    const char* passphrase,
    unsigned int address_match_mode,
    uint8_t* private_key_out,
    dogecoin_bool* compressed_out,
    uint32_t* lot_out,
    uint32_t* sequence_out);
/* Check if a string is a valid BIP38 encrypted key */
dogecoin_bool dogecoin_bip38_is_valid(const char* encrypted_key);
/* Get the address hash from a BIP38 encrypted key */
dogecoin_bool dogecoin_bip38_get_address_hash(
    const char* encrypted_key,
    uint8_t* address_hash_out);
/* Verify the address hash matches the given address */
dogecoin_bool dogecoin_bip38_verify_address_hash(
    const char* encrypted_key,
    const char* address);
/* Get the flag byte from a BIP38 encrypted key */
dogecoin_bool dogecoin_bip38_get_flag_byte(
    const char* encrypted_key,
    uint8_t* flag_byte_out);
/* Check if a BIP38 encrypted key is compressed */
dogecoin_bool dogecoin_bip38_is_compressed(const char* encrypted_key);
/* Check if a BIP38 encrypted key has lot/sequence */
dogecoin_bool dogecoin_bip38_has_lot_sequence(const char* encrypted_key);
/* Check if a BIP38 encrypted key is EC multiplied */
dogecoin_bool dogecoin_bip38_is_ec_multiplied(const char* encrypted_key);
/* Generate a random lot and sequence for BIP38 */
void dogecoin_bip38_generate_lot_sequence(
    uint32_t* lot_out,
    uint32_t* sequence_out);
/* Convert a private key to WIF format */
dogecoin_bool dogecoin_bip38_private_key_to_wif(
    const uint8_t* private_key,
    const dogecoin_chainparams* chain,
    dogecoin_bool compressed,
    char* wif_out,
    size_t* wif_size);
/* Convert a WIF private key to raw format */
dogecoin_bool dogecoin_bip38_wif_to_private_key(
    const char* wif,
    const dogecoin_chainparams* chain,
    uint8_t* private_key_out,
    dogecoin_bool* compressed_out);
/* Owner-side intermediate passphrase string (starts with "passphrase") */
dogecoin_bool dogecoin_bip38_generate_intermediate_code(
    const char* passphrase,
    dogecoin_bool use_lot_sequence,
    uint32_t lot,
    uint32_t sequence,
    const uint8_t* ownerentropy_override,
    char* intermediate_code_out,
    size_t* intermediate_code_size);
/* Printer-side: create encrypted key (+ optional confirmation code) from intermediate code */
dogecoin_bool dogecoin_bip38_encrypt_from_intermediate(
    const char* intermediate_code,
    dogecoin_bool compressed,
    const uint8_t* seedb_override,
    const char* address_chain_hint,
    char* encrypted_key_out,
    size_t* encrypted_key_size,
    char* confirmation_code_out,
    size_t* confirmation_code_size);
/* Owner verifies confirmation code matches passphrase (and optional lot/sequence) */
dogecoin_bool dogecoin_bip38_confirm_passphrase(
    const char* passphrase,
    const char* confirmation_code,
    char* address_out,
    size_t address_size,
    dogecoin_bool* compressed_out,
    uint32_t* lot_out,
    uint32_t* sequence_out);
/* Owner verifies confirmation code (Dogecoin mainnet address output by default) */
dogecoin_bool dogecoin_bip38_confirm_passphrase_ex(
    const char* passphrase,
    const char* confirmation_code,
    unsigned int address_match_mode,
    char* address_out,
    size_t address_size,
    dogecoin_bool* compressed_out,
    uint32_t* lot_out,
    uint32_t* sequence_out);
/* One-shot EC-multiplied encrypt (optional lot/sequence, optional confirmation code) */
dogecoin_bool dogecoin_bip38_encrypt_ec_multiplied(
    const char* passphrase,
    dogecoin_bool compressed,
    dogecoin_bool use_lot_sequence,
    uint32_t lot,
    uint32_t sequence,
    const char* address_chain_hint,
    uint8_t* private_key_out,
    char* encrypted_key_out,
    size_t* encrypted_key_size,
    char* confirmation_code_out,
    size_t* confirmation_code_size);
/* Check if a string is a BIP38 intermediate passphrase code */
dogecoin_bool dogecoin_bip38_is_intermediate_code(const char* code);
/* Check if a string is a BIP38 confirmation code */
dogecoin_bool dogecoin_bip38_is_confirmation_code(const char* code);

/* Paper wallet sweep API
--------------------------------------------------------------------------
*/

typedef struct dogecoin_paper_wallet_ {
    char* private_key_wif;
    char* private_key_hex;
    char* encrypted_private_key; /* BIP38 encrypted */
    char* passphrase; /* For BIP38 decryption */
    char* address;
    dogecoin_bool compressed;
    dogecoin_bool is_encrypted;
    const dogecoin_chainparams* chain_params;
} dogecoin_paper_wallet;
typedef struct dogecoin_sweep_utxo_ {
    char* txid;
    int vout;
    char* amount_doge;
} dogecoin_sweep_utxo;
typedef struct dogecoin_sweep_options_ {
    char* destination_address;
    uint64_t fee_per_byte; /* Fee in koinu per byte (see chain) */
    uint64_t min_fee; /* Minimum fee in koinu */
    uint64_t max_fee; /* Maximum fee in koinu */
    dogecoin_bool use_rbf; /* Replace-by-fee (nSequence 0xfffffffd on inputs) */
    uint32_t locktime; /* Locktime for transaction */
    const dogecoin_chainparams* chain_params;
    /*
     * UTXO list: populate with dogecoin_sweep_options_set_utxo() (one input) or
     * dogecoin_sweep_options_add_utxo() (several inputs, same private key).
     * Amounts are Dogecoin decimal strings (same as transaction.c / finalize_transaction).
     * libdogecoin does not query the chain; the application supplies prevouts.
     */
    dogecoin_sweep_utxo* utxos;
    size_t utxo_count;
    /* Mirrors first entry for backward compatibility */
    char* utxo_txid;
    int utxo_vout;
    char* utxo_total_doge;
} dogecoin_sweep_options;
typedef struct dogecoin_sweep_result_ {
    dogecoin_bool success;
    char* error_message;
    char* transaction_hex;
    char* transaction_id;
    uint64_t amount_swept;
    uint64_t fee_paid;
    char* destination_address;
} dogecoin_sweep_result;
typedef dogecoin_tx dogecoin_transaction;

/* Initialize a paper wallet structure */
dogecoin_paper_wallet* dogecoin_paper_wallet_new(void);
/* Free a paper wallet structure */
void dogecoin_paper_wallet_free(dogecoin_paper_wallet* wallet);
/* Initialize a sweep result structure */
dogecoin_sweep_result* dogecoin_sweep_result_new(void);
/* Free a sweep result structure */
void dogecoin_sweep_result_free(dogecoin_sweep_result* result);
/* Initialize sweep options with defaults */
dogecoin_sweep_options* dogecoin_sweep_options_new(const dogecoin_chainparams* chain_params);
/* Free sweep options */
void dogecoin_sweep_options_free(dogecoin_sweep_options* options);
/* Set paper wallet from WIF private key */
dogecoin_bool dogecoin_paper_wallet_set_wif(
    dogecoin_paper_wallet* wallet,
    const char* wif_private_key,
    const dogecoin_chainparams* chain_params);
/* Set paper wallet from hex private key (compressed selects P2PKH address format) */
dogecoin_bool dogecoin_paper_wallet_set_hex(
    dogecoin_paper_wallet* wallet,
    const char* hex_private_key,
    dogecoin_bool compressed,
    const dogecoin_chainparams* chain_params);
/* Set paper wallet from a BIP38 encrypted private key (Dogecoin-mainnet decrypt semantics) */
dogecoin_bool dogecoin_paper_wallet_set_encrypted(
    dogecoin_paper_wallet* wallet,
    const char* encrypted_private_key,
    const char* passphrase,
    const dogecoin_chainparams* chain_params);
/* Get the address from a paper wallet */
dogecoin_bool dogecoin_paper_wallet_get_address(
    const dogecoin_paper_wallet* wallet,
    char* address_out,
    size_t address_size);
/* Get the private key from a paper wallet */
dogecoin_bool dogecoin_paper_wallet_get_private_key(
    const dogecoin_paper_wallet* wallet,
    uint8_t* private_key_out);
/* Get the WIF private key from a paper wallet */
dogecoin_bool dogecoin_paper_wallet_get_wif(
    const dogecoin_paper_wallet* wallet,
    char* wif_out,
    size_t wif_size);
/* Check if a paper wallet is valid */
dogecoin_bool dogecoin_paper_wallet_is_valid(const dogecoin_paper_wallet* wallet);
/* Sweep a paper wallet to a destination address (requires UTXO on options) */
dogecoin_sweep_result* dogecoin_sweep_paper_wallet(
    const dogecoin_paper_wallet* wallet,
    const dogecoin_sweep_options* options);
/* Sweep multiple paper wallets to a destination address */
dogecoin_sweep_result* dogecoin_sweep_multiple_paper_wallets(
    const dogecoin_paper_wallet* wallets,
    size_t wallet_count,
    const dogecoin_sweep_options* options);
/* Estimate the fee for sweeping a paper wallet */
uint64_t dogecoin_sweep_estimate_fee(
    const dogecoin_paper_wallet* wallet,
    const dogecoin_sweep_options* options);
/* Get the balance of a paper wallet address */
dogecoin_bool dogecoin_sweep_get_balance(
    const char* address,
    const dogecoin_chainparams* chain_params,
    uint64_t* balance_out);
/* Create an unsigned sweep transaction (free with dogecoin_tx_free) */
dogecoin_transaction* dogecoin_sweep_create_transaction(
    const dogecoin_paper_wallet* wallet,
    const dogecoin_sweep_options* options);
/* Sign a sweep transaction */
dogecoin_bool dogecoin_sweep_sign_transaction(
    dogecoin_transaction* transaction,
    const dogecoin_paper_wallet* wallet);
/* Broadcast a sweep transaction */
dogecoin_bool dogecoin_sweep_broadcast_transaction(
    const dogecoin_transaction* transaction,
    const dogecoin_chainparams* chain_params,
    char* transaction_id_out,
    size_t transaction_id_size);
/* Validate a sweep transaction */
dogecoin_bool dogecoin_sweep_validate_transaction(
    const dogecoin_transaction* transaction,
    const dogecoin_paper_wallet* wallet,
    const dogecoin_sweep_options* options);
/* Get sweep transaction statistics */
dogecoin_bool dogecoin_sweep_get_stats(
    const dogecoin_transaction* transaction,
    const dogecoin_sweep_options* options,
    uint64_t* input_count_out,
    uint64_t* output_count_out,
    uint64_t* total_input_value_out,
    uint64_t* total_output_value_out,
    uint64_t* fee_out);
/* Set sweep options destination address */
dogecoin_bool dogecoin_sweep_options_set_destination(
    dogecoin_sweep_options* options,
    const char* destination_address);
/* Set sweep options fee */
dogecoin_bool dogecoin_sweep_options_set_fee(
    dogecoin_sweep_options* options,
    uint64_t fee_per_byte,
    uint64_t min_fee,
    uint64_t max_fee);
/* Set sweep options RBF */
void dogecoin_sweep_options_set_rbf(
    dogecoin_sweep_options* options,
    dogecoin_bool use_rbf);
/* Set sweep options locktime */
void dogecoin_sweep_options_set_locktime(
    dogecoin_sweep_options* options,
    uint32_t locktime);
/* Replace all UTXOs with a single prevout */
dogecoin_bool dogecoin_sweep_options_set_utxo(
    dogecoin_sweep_options* options,
    const char* txid_hex,
    int vout,
    const char* total_input_doge);
/* Append a prevout for multi-UTXO sweeps */
dogecoin_bool dogecoin_sweep_options_add_utxo(
    dogecoin_sweep_options* options,
    const char* txid_hex,
    int vout,
    const char* amount_doge);
/* Number of prevouts configured on options */
size_t dogecoin_sweep_options_utxo_count(const dogecoin_sweep_options* options);
/* Map fee-per-kB to fee-per-byte for sweep options */
uint64_t dogecoin_sweep_fee_per_kb_to_per_byte(uint64_t fee_per_kb);
/* Get sweep result error message */
const char* dogecoin_sweep_result_get_error(const dogecoin_sweep_result* result);
/* Get sweep result transaction hex */
const char* dogecoin_sweep_result_get_transaction_hex(const dogecoin_sweep_result* result);
/* Get sweep result transaction ID */
const char* dogecoin_sweep_result_get_transaction_id(const dogecoin_sweep_result* result);
/* Get sweep result amount swept */
uint64_t dogecoin_sweep_result_get_amount_swept(const dogecoin_sweep_result* result);
/* Get sweep result fee paid */
uint64_t dogecoin_sweep_result_get_fee_paid(const dogecoin_sweep_result* result);
/* Get sweep result destination address */
const char* dogecoin_sweep_result_get_destination_address(const dogecoin_sweep_result* result);
