/*
 The MIT License (MIT)

 Copyright (c) 2022 bluezr
 Copyright (c) 2022-2024 The Dogecoin Foundation

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

#ifndef __LIBDOGECOIN_TRANSACTION_H__
#define __LIBDOGECOIN_TRANSACTION_H__

#include <stdlib.h>    /* malloc       */
#include <stddef.h>    /* offsetof     */
#include <stdint.h>    /* uint32_t     */
#include <stdio.h>     /* printf       */
#include <string.h>    /* memset       */
#include <dogecoin/uthash.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/tx.h>

LIBDOGECOIN_BEGIN_DECL

// Maximum length of standard tx based on relay limit (100 000).
// Internally this is cited as 200001 for strings that represent it because +stringterm.
#define TXHEXMAXLEN 200001

/* hashmap functions */
typedef struct working_transaction {
    int idx;
    dogecoin_tx* transaction;
    /* Lifetime fields for the _ts retain-under-lock model; both guarded by the
       owning dogecoin_transaction_context->lock. refcount counts outstanding
       holders handed out by find_transaction_ts(); pending_delete marks an
       entry unlinked from the registry but not yet freed because a holder is
       still using it. Unused (stay zero) for entries in the thread-local
       default context, which is never shared. */
    int refcount;
    int pending_delete;
    UT_hash_handle hh;
} working_transaction;

typedef struct dogecoin_transaction_context {
    working_transaction* transactions;
    dogecoin_mutex_t lock; /* guards the registry root above; no-op for the
                              zero-initialized per-thread default context */
    uint32_t next_idx;     /* monotonic id source, guarded by lock. Never reused:
                              deriving ids from HASH_COUNT()+1 recycled an id after
                              any removal, so a later start_transaction could mint
                              the id of a still-live entry and evict it via
                              HASH_REPLACE. Zero-initialized; first minted id is 1. */
} dogecoin_transaction_context;

struct dogecoin_wallet_;

// instantiates a new transaction
LIBDOGECOIN_API working_transaction* new_transaction();
LIBDOGECOIN_API working_transaction* new_transaction_ts(dogecoin_transaction_context* ctx);

LIBDOGECOIN_API void add_transaction(working_transaction *working_tx);
LIBDOGECOIN_API void add_transaction_ts(dogecoin_transaction_context* ctx, working_transaction *working_tx);

LIBDOGECOIN_API working_transaction* find_transaction(int idx);
LIBDOGECOIN_API working_transaction* find_transaction_ts(dogecoin_transaction_context* ctx, int idx);
/* Owning-name alias of find_transaction_ts(): returns an entry with a reference
   held under the registry lock. The name makes the "acquire => must release"
   obligation explicit; it behaves identically to find_transaction_ts(). */
LIBDOGECOIN_API working_transaction* acquire_transaction_ts(dogecoin_transaction_context* ctx, int idx);
LIBDOGECOIN_API void release_transaction_ts(dogecoin_transaction_context* ctx, working_transaction* working_tx);
/* Callback-under-lock convenience: looks up idx and, if found, invokes fn(tx,
   arg) while the registry lock is held, so the entry cannot be removed/freed
   for the duration of the callback and no find/release bookkeeping is needed.
   fn must not call back into the same context (the lock is non-recursive).
   Returns 1 if an entry was found and fn was invoked, 0 otherwise. */
LIBDOGECOIN_API int with_transaction_ts(dogecoin_transaction_context* ctx, int idx, void (*fn)(working_transaction* working_tx, void* arg), void* arg);

LIBDOGECOIN_API void remove_transaction(working_transaction *working_tx);
LIBDOGECOIN_API void remove_transaction_ts(dogecoin_transaction_context* ctx, working_transaction *working_tx);

LIBDOGECOIN_API void remove_all();
LIBDOGECOIN_API void remove_all_ts(dogecoin_transaction_context* ctx);

LIBDOGECOIN_API void print_transactions();

LIBDOGECOIN_API void count_transactions();
LIBDOGECOIN_API int get_transaction_count(void);
LIBDOGECOIN_API int get_transaction_count_ts(dogecoin_transaction_context* ctx);

LIBDOGECOIN_API int by_id();

LIBDOGECOIN_API const char *getl(const char *prompt);

LIBDOGECOIN_API const char *get_raw_tx(const char *prompt_tx);

LIBDOGECOIN_API const char *get_private_key(const char *prompt_key);

LIBDOGECOIN_API int start_transaction(); // #returns  an index of a transaction to build in memory.  (1, 2, etc) ..
LIBDOGECOIN_API int start_transaction_ts(dogecoin_transaction_context* ctx);

LIBDOGECOIN_API dogecoin_transaction_context* dogecoin_transaction_context_new(void);
LIBDOGECOIN_API void dogecoin_transaction_context_free(dogecoin_transaction_context* ctx);

/* Returns the per-thread default transaction context that the non-`_ts`
 * convenience wrappers (and the index-based transaction API) operate on. This
 * lets callers invoke the `_ts` transaction-context API explicitly against the
 * same registry, rather than going through the non-`_ts` wrappers. */
LIBDOGECOIN_API dogecoin_transaction_context* dogecoin_transaction_context_default(void);

LIBDOGECOIN_API int save_raw_transaction(int txindex, const char* hexadecimal_transaction);
/* THREAD-SAFE variant - operates on the supplied transaction context and
   acquires the working transaction's per-object mutex. */
LIBDOGECOIN_API int save_raw_transaction_ts(dogecoin_transaction_context* ctx, int txindex, const char* hexadecimal_transaction);

LIBDOGECOIN_API int add_utxo(int txindex, char* hex_utxo_txid, int vout); // #returns 1 if success.
/* THREAD-SAFE variant - routes the input through dogecoin_tx_add_input_ts. */
LIBDOGECOIN_API int add_utxo_ts(dogecoin_transaction_context* ctx, int txindex, char* hex_utxo_txid, int vout);

LIBDOGECOIN_API int add_output(int txindex, char* destinationaddress, char* amount);
/* THREAD-SAFE variant - routes the output through dogecoin_tx_add_output_ts. */
LIBDOGECOIN_API int add_output_ts(dogecoin_transaction_context* ctx, int txindex, char* destinationaddress, char* amount);

// 'closes the inputs', specifies the recipient, specifies the amnt-to-subtract-as-fee, and returns the raw tx..
// out_dogeamount == just an echoback of the total amount specified in the addutxos for verification
LIBDOGECOIN_API char* finalize_transaction(int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* public_key);
/* THREAD-SAFE variant - adds change via dogecoin_tx_add_output_ts and runs the
   dogecoin_tx_finalize_ts integrity pass before serializing. */
LIBDOGECOIN_API char* finalize_transaction_ts(dogecoin_transaction_context* ctx, int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* public_key);

LIBDOGECOIN_API char* get_raw_transaction(int txindex); // #returns 0 if not closed, returns rawtx again if closed/created.
/* THREAD-SAFE variant - serializes under the working transaction's mutex. */
LIBDOGECOIN_API char* get_raw_transaction_ts(dogecoin_transaction_context* ctx, int txindex);

LIBDOGECOIN_API void clear_transaction(int txindex); // #clears a tx in memory. (overwrites)
/* THREAD-SAFE variant - removes the entry from the supplied context. */
LIBDOGECOIN_API void clear_transaction_ts(dogecoin_transaction_context* ctx, int txindex);

// sign a given inputted transaction with a given private key, and return a hex signed transaction.
// we may want to add such things to 'advanced' section:
// locktime, possibilities for multiple outputs, data, sequence.
LIBDOGECOIN_API int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey);

LIBDOGECOIN_API int sign_indexed_raw_transaction(int txindex, int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey);

LIBDOGECOIN_API int sign_transaction(int txindex, char* script_pubkey, char* privkey);

LIBDOGECOIN_API int sign_transaction_w_privkey(int txindex, int vout_index, char* privkey);

LIBDOGECOIN_API int store_raw_transaction(char* incomingrawtx);

LIBDOGECOIN_API int get_raw_transaction_ex(int txindex, char* buf, size_t buf_cap);

LIBDOGECOIN_API int sign_raw_transaction_ex(int inputindex, const char* incomingrawtx, char* signedrawtx, size_t* signed_size, const char* scripthex, int sighashtype, const char* privkey);

LIBDOGECOIN_API int finalize_transaction_ex(int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* changeaddress, char* buf, size_t buf_cap);

LIBDOGECOIN_API int sign_indexed_raw_transaction_ex(int txindex, int inputindex, const char* scripthex, int sighashtype, const char* privkey, char* buf, size_t buf_cap);

LIBDOGECOIN_API int sign_transaction_ex(int txindex, const char* script_pubkey, const char* privkey, char* buf, size_t buf_cap);

LIBDOGECOIN_API int sign_transaction_w_privkey_ex(int txindex, const char* privkey, char* buf, size_t buf_cap);

/* THREAD-SAFE variant - uses internal mutex */
LIBDOGECOIN_API dogecoin_tx* dogecoin_tx_new_ts(void);
/* THREAD-SAFE variant - uses internal mutex */
LIBDOGECOIN_API void dogecoin_tx_free_ts(dogecoin_tx* tx);
/* THREAD-SAFE variant - uses internal mutex */
LIBDOGECOIN_API int dogecoin_tx_add_input_ts(dogecoin_tx* tx, const dogecoin_tx_in* tx_in);
/* THREAD-SAFE variant - uses internal mutex */
LIBDOGECOIN_API int dogecoin_tx_add_output_ts(dogecoin_tx* tx, const dogecoin_tx_out* tx_out);
/* THREAD-SAFE variant - uses internal mutex */
/* passphrase is currently reserved for future encrypted-wallet integration */
LIBDOGECOIN_API int dogecoin_tx_sign_ts(dogecoin_tx* tx, struct dogecoin_wallet_* wallet, const char* passphrase);
/* THREAD-SAFE variant - uses internal mutex */
LIBDOGECOIN_API int dogecoin_tx_finalize_ts(dogecoin_tx* tx);

/* P2SH address helpers. Declared here as well as in libdogecoin.h so in-tree
   callers that include this header rather than the public one get a prototype;
   without it clang and the optee build reject the call outright while gcc only
   warns. */
LIBDOGECOIN_API int get_p2sh_multisig_address(const char** pubkeys_hex, int n, int m,
                                              int is_testnet,
                                              char* p2sh_addr_out, size_t p2sh_addr_cap,
                                              char* redeem_script_hex_out,
                                              size_t redeem_script_hex_cap);
LIBDOGECOIN_API int get_p2sh_address_from_script(const char* redeem_script_hex,
                                                 int is_testnet,
                                                 char* p2sh_addr_out, size_t p2sh_addr_cap);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_TRANSACTION_H__
