/*

 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
 Copyright (c) 2023 bluezr
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

#ifndef __LIBDOGECOIN_WALLET_H__
#define __LIBDOGECOIN_WALLET_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#include <dogecoin/base58.h>
#include <dogecoin/blockchain.h>
#include <dogecoin/bip32.h>
#include <dogecoin/bip39.h>
#include <dogecoin/bip44.h>
#include <dogecoin/buffer.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/common.h>
#include <dogecoin/constants.h>
#include <dogecoin/koinu.h>
#include <dogecoin/random.h>
#include <dogecoin/serialize.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

#include <stdint.h>
#include <stddef.h>

/** single key/value record */
typedef struct dogecoin_wallet_ {
    const char* filename;
    FILE *dbfile;
    dogecoin_hdnode* masterkey;
    uint32_t next_childindex; //cached next child index
    const dogecoin_chainparams* chain;
    uint32_t bestblockheight;

    /* use binary trees for in-memory mapping for wtxs, keys */
    void* hdkeys_rbtree;
    vector* unspent;
    void* unspent_rbtree;
    vector* spends;
    void* spends_rbtree;
    vector *vec_wtxes;
    void* wtxes_rbtree;
    vector *waddr_vector; //points to the addr objects managed by the waddr_rbtree [in order]
    void* waddr_rbtree;
} dogecoin_wallet;

typedef struct dogecoin_wtx_ {
    uint256 tx_hash_cache;
    uint256 blockhash;
    uint32_t height;
    dogecoin_tx* tx;
    dogecoin_bool ignore; //if set, transaction will be ignored (soft-delete)
} dogecoin_wtx;

typedef struct dogecoin_utxo_ {
    uint256 txid;
    int vout;
    char address[P2PKHLEN];
    char* account;
    char script_pubkey[SCRIPT_PUBKEY_STRINGLEN];
    char amount[KOINU_STRINGLEN];
    int confirmations;
    dogecoin_bool spendable;
    dogecoin_bool solvable;
} dogecoin_utxo;

typedef struct dogecoin_wallet_addr_{
    uint160 pubkeyhash;
    uint8_t type;
    uint32_t childindex;
    dogecoin_bool ignore;
} dogecoin_wallet_addr;

typedef struct dogecoin_output_ {
    uint32_t i;
    dogecoin_wtx* wtx;
} dogecoin_output;

/** wallet transaction (wtx) functions */
LIBDOGECOIN_API dogecoin_wtx* dogecoin_wallet_wtx_new();
LIBDOGECOIN_API void dogecoin_wallet_wtx_free(dogecoin_wtx* wtx);
LIBDOGECOIN_API void dogecoin_wallet_wtx_serialize(cstring* s, const dogecoin_wtx* wtx);
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_wtx_deserialize(dogecoin_wtx* wtx, struct const_buffer* buf);
/** ------------------------------------ */

/** wallet utxo functions */
LIBDOGECOIN_API dogecoin_utxo* dogecoin_wallet_utxo_new();
LIBDOGECOIN_API void dogecoin_wallet_utxo_free(dogecoin_utxo* utxo);
LIBDOGECOIN_API void dogecoin_wallet_scrape_utxos(dogecoin_wallet* wallet, dogecoin_wtx* wtx);
/** ------------------------------------ */

/** wallet addr functions */
LIBDOGECOIN_API extern dogecoin_wallet_addr* dogecoin_wallet_addr_new();
LIBDOGECOIN_API void dogecoin_wallet_addr_free(dogecoin_wallet_addr* waddr);
LIBDOGECOIN_API extern int dogecoin_wallet_addr_compare(const void *l, const void *r);
/** ------------------------------------ */

/** wallet outputs (prev wtx + n) functions */
LIBDOGECOIN_API dogecoin_output* dogecoin_wallet_output_new();
LIBDOGECOIN_API void dogecoin_wallet_output_free(dogecoin_output* output);
/** ------------------------------------ */

LIBDOGECOIN_API dogecoin_wallet* dogecoin_wallet_new(const dogecoin_chainparams *params);
LIBDOGECOIN_API dogecoin_wallet* dogecoin_wallet_init(const dogecoin_chainparams* chain, const char* address, const char* name, const char* mnemonic_in, const char* pass, const dogecoin_bool encrypted, const dogecoin_bool tpm, const int file_num, const dogecoin_bool master_key);
LIBDOGECOIN_API void print_utxos(dogecoin_wallet* wallet);
LIBDOGECOIN_API void dogecoin_wallet_free(dogecoin_wallet* wallet);

/** load the wallet, sets masterkey, sets next_childindex */
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_load(dogecoin_wallet* wallet, const char* file_path, int *error, dogecoin_bool *created);

/** load the wallet and replace a record */
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_replace(dogecoin_wallet* wallet, const char* file_path, cstring* record, uint8_t record_type, int *error);

/** writes the wallet state to disk */
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_flush(dogecoin_wallet* wallet);

/** set the master key of new created wallet
 consuming app needs to ensure that we don't override exiting masterkeys */
LIBDOGECOIN_API void dogecoin_wallet_set_master_key_copy(dogecoin_wallet* wallet, const dogecoin_hdnode* master_xpub);

/** derives the next child hdnode and derives an address (memory is owned by the wallet) */
LIBDOGECOIN_API dogecoin_wallet_addr* dogecoin_wallet_next_addr(dogecoin_wallet* wallet);
LIBDOGECOIN_API dogecoin_wallet_addr* dogecoin_wallet_next_bip44_addr(dogecoin_wallet* wallet);
LIBDOGECOIN_API dogecoin_bool dogecoin_p2pkh_address_to_wallet_pubkeyhash(const char* address_in, dogecoin_wallet_addr* addr, dogecoin_wallet* wallet);
LIBDOGECOIN_API dogecoin_wallet_addr* dogecoin_p2pkh_address_to_wallet(const char* address_in, dogecoin_wallet* wallet);

/** writes all available addresses (P2PKH) to the addr_out vector */
LIBDOGECOIN_API void dogecoin_wallet_get_addresses(dogecoin_wallet* wallet, vector* addr_out);

/** finds wallet address object based on pure addresses (base58/bech32) */
LIBDOGECOIN_API dogecoin_wallet_addr* dogecoin_wallet_find_waddr_byaddr(dogecoin_wallet* wallet, const char* search_addr);

/** adds transaction to the wallet (hands over memory management) */
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_add_wtx_move(dogecoin_wallet* wallet, dogecoin_wtx* wtx);

/** gets credit from given transaction */
LIBDOGECOIN_API int64_t dogecoin_wallet_get_balance(dogecoin_wallet* wallet);

/** gets credit from given transaction */
LIBDOGECOIN_API int64_t dogecoin_wallet_wtx_get_credit(dogecoin_wallet* wallet, dogecoin_wtx* wtx);

LIBDOGECOIN_API int64_t dogecoin_wallet_get_debit_tx(dogecoin_wallet *wallet, const dogecoin_tx *tx);
LIBDOGECOIN_API int64_t dogecoin_wallet_wtx_get_available_credit(dogecoin_wallet* wallet, dogecoin_wtx* wtx);

/** checks if a transaction outpoint is owned by the wallet */
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_txout_is_mine(dogecoin_wallet* wallet, dogecoin_tx_out* tx_out);

/** checks if a transaction outpoint is owned by the wallet */
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_is_spent(dogecoin_wallet* wallet, uint256 hash, uint32_t n);
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_get_unspents(dogecoin_wallet* wallet, vector* unspents);
LIBDOGECOIN_API dogecoin_bool dogecoin_wallet_get_unspent(dogecoin_wallet* wallet, vector* unspent);

/** checks a transaction or relevance to the wallet */
LIBDOGECOIN_API void dogecoin_wallet_check_transaction(void *ctx, dogecoin_tx *tx, unsigned int pos, dogecoin_blockindex *pindex);

/** returns wtx based on given hash
 * may return NULL if transaction could not be found
 * memory is managed by the transaction tree
 */
LIBDOGECOIN_API dogecoin_wtx * dogecoin_wallet_get_wtx(dogecoin_wallet* wallet, const uint256 hash);

LIBDOGECOIN_API dogecoin_wallet* dogecoin_wallet_read(char* address);
LIBDOGECOIN_API int dogecoin_register_watch_address_with_node(char* address);
LIBDOGECOIN_API int dogecoin_unregister_watch_address_with_node(char* address);
LIBDOGECOIN_API int dogecoin_get_utxo_vector(char* address, vector* utxos);
LIBDOGECOIN_API uint8_t* dogecoin_get_utxos(char* address);
LIBDOGECOIN_API unsigned int dogecoin_get_utxos_length(char* address);
LIBDOGECOIN_API char* dogecoin_get_utxo_txid_str(char* address, unsigned int index);
LIBDOGECOIN_API uint8_t* dogecoin_get_utxo_txid(char* address, unsigned int index);
LIBDOGECOIN_API int dogecoin_get_utxo_vout(char* address, unsigned int index);
LIBDOGECOIN_API char* dogecoin_get_utxo_amount(char* address, unsigned int index);
LIBDOGECOIN_API uint64_t dogecoin_get_balance(char* address);
LIBDOGECOIN_API char* dogecoin_get_balance_str(char* address);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_WALLET_H__
