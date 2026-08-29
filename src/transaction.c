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

#include <assert.h>

#include <dogecoin/dogecoin.h>
#include <dogecoin/base58.h>
#include <dogecoin/key.h>
#include <dogecoin/koinu.h>
#include <dogecoin/script.h>
#include <dogecoin/transaction.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>
#include <dogecoin/wallet.h>
#include <dogecoin/vector.h>

static dogecoin_transaction_context* default_transaction_context(void) {
    static DOGECOIN_THREAD_LOCAL dogecoin_transaction_context default_ctx = {0};
    return &default_ctx;
}

/* internal helpers for the thread-safe index API (defined below) */
static int add_address_output_ts(working_transaction* tx, const dogecoin_chainparams* chain, int64_t koinu, const char* address);
static int make_change_ts(dogecoin_transaction_context* ctx, int txindex, char* public_key, uint64_t subtractedfee, uint64_t amount);

dogecoin_transaction_context* dogecoin_transaction_context_default(void) {
    return default_transaction_context();
}

dogecoin_transaction_context* dogecoin_transaction_context_new(void) {
    dogecoin_transaction_context* ctx =
        (dogecoin_transaction_context*)dogecoin_calloc(1, sizeof(dogecoin_transaction_context));
    if (!ctx) return NULL;
#if defined(_WIN32) || defined(DOGECOIN_HAVE_THREADS)
    /* With a threading runtime, a failed mutex init is a real error. Without
       one (e.g. OP-TEE/enclave builds), dogecoin_mutex_init() returns false by
       design and the lock no-ops; the context is still valid, so don't treat
       that as a construction failure. */
    if (!dogecoin_mutex_init(&ctx->lock)) {
        dogecoin_free(ctx);
        return NULL;
    }
#else
    (void)dogecoin_mutex_init(&ctx->lock); /* no-op; lock stays uninitialized */
#endif
    return ctx;
}

void dogecoin_transaction_context_free(dogecoin_transaction_context* ctx) {
    if (!ctx) return;
#ifndef NDEBUG
    /* Guard rail (debug builds): freeing a context while a find_transaction_ts()
       reference is still outstanding indicates a missing release_transaction_ts()
       and would otherwise leak the deferred-delete entry. */
    dogecoin_mutex_lock(&ctx->lock);
    {
        working_transaction* cur;
        working_transaction* tmp;
        HASH_ITER(hh, ctx->transactions, cur, tmp) {
            assert(cur->refcount == 0);
        }
    }
    dogecoin_mutex_unlock(&ctx->lock);
#endif
    remove_all_ts(ctx); /* self-locks; safe to call without holding ctx->lock */
    dogecoin_mutex_destroy(&ctx->lock);
    dogecoin_free(ctx);
}

/* ---- internal registry helpers; caller MUST hold ctx->lock ----
 * These perform the raw uthash operations with no locking so the public _ts
 * entry points (and the composed start/clear/remove_all paths) can take the
 * lock exactly once and stay deadlock-free on the non-recursive mutex. */
static working_transaction* new_transaction_locked(dogecoin_transaction_context* ctx) {
    working_transaction* working_tx = (struct working_transaction*)dogecoin_calloc(1, sizeof *working_tx);
    if (!working_tx) return NULL;
    working_tx->transaction = dogecoin_tx_new_ts();
    if (!working_tx->transaction) {
        dogecoin_free(working_tx);
        return NULL;
    }
    /* Mint a never-reused id under the registry lock. HASH_COUNT()+1 recycled
       ids after removals, letting a later entry collide with a live one. */
    working_tx->idx = (int)++ctx->next_idx;
    return working_tx;
}

/* Dispose of an entry that is already unlinked from the registry. Frees it now
   when unreferenced; otherwise marks it so the last release_transaction_locked()
   performs the free. Caller MUST hold ctx->lock. */
static void dispose_unlinked_transaction_locked(working_transaction *working_tx) {
    if (working_tx->refcount > 0) {
        /* A holder from find_transaction_ts() is still using it; defer the free
           to the last release. The entry is already out of the registry. */
        working_tx->pending_delete = 1;
        return;
    }
    dogecoin_tx_free(working_tx->transaction);
    dogecoin_free(working_tx);
}

static void add_transaction_locked(dogecoin_transaction_context* ctx, working_transaction *working_tx) {
    working_transaction *tx;
    HASH_FIND_INT(ctx->transactions, &working_tx->idx, tx);
    if (tx == NULL) {
        HASH_ADD_INT(ctx->transactions, idx, working_tx);
        return;
    }
    /* With monotonic, never-reused ids (see new_transaction_locked) an id
       collision is impossible for entries minted by the registry. Reaching here
       means a caller supplied a colliding idx via the legacy add_transaction(_ts)
       path. The previous code HASH_REPLACE'd and freed the existing entry, which
       (a) silently destroyed a live transaction and (b) freed it with no regard
       for refcount, producing a use-after-free for any outstanding
       find_transaction_ts() holder. Do neither: keep the existing entry and
       decline the colliding insert. The caller retains ownership of working_tx
       (matching the legacy contract where the object is caller-allocated), so we
       must not free it here either. (Real removals still go through
       remove_transaction_locked -> dispose_unlinked_transaction_locked, which
       preserves the refcount-safe deferred delete.) */
    (void)tx;
}

static working_transaction* find_transaction_locked(dogecoin_transaction_context* ctx, int idx) {
    working_transaction *working_tx;
    HASH_FIND_INT(ctx->transactions, &idx, working_tx);
    return working_tx;
}

static void remove_transaction_locked(dogecoin_transaction_context* ctx, working_transaction *working_tx) {
    HASH_DEL(ctx->transactions, working_tx); /* unlink so no new finder can reach it */
    dispose_unlinked_transaction_locked(working_tx);
}

/* Drop a reference taken by find_transaction_ts(). Caller MUST hold ctx->lock.
   Frees the entry if it was removed while referenced and this was the last ref. */
static void release_transaction_locked(working_transaction *working_tx) {
    if (!working_tx) return;
    if (working_tx->refcount > 0) working_tx->refcount--;
    if (working_tx->refcount == 0 && working_tx->pending_delete) {
        dogecoin_tx_free(working_tx->transaction);
        dogecoin_free(working_tx);
    }
}

/**
 * @brief This function instantiates a new working transaction,
 * but does not add it to the hash table.
 *
 * @return A pointer to the new working transaction.
 */
working_transaction* new_transaction() {
    dogecoin_transaction_context* ctx = default_transaction_context();
    if (!ctx) return NULL;
    working_transaction* working_tx = (struct working_transaction*)dogecoin_calloc(1, sizeof *working_tx);
    if (!working_tx) return NULL;
    working_tx->transaction = dogecoin_tx_new();
    if (!working_tx->transaction) {
        dogecoin_free(working_tx);
        return NULL;
    }
    /* Match new_transaction_locked(): never-reused id from the context counter.
       The legacy default context is thread-local, so this is single-threaded. */
    working_tx->idx = (int)++ctx->next_idx;
    return working_tx;
}

working_transaction* new_transaction_ts(dogecoin_transaction_context* ctx) {
    if (!ctx) return NULL;
    dogecoin_mutex_lock(&ctx->lock);
    working_transaction* working_tx = new_transaction_locked(ctx);
    dogecoin_mutex_unlock(&ctx->lock);
    return working_tx;
}

/**
 * @brief This function takes a pointer to an existing working
 * transaction object and adds it to the hash table.
 *
 * @param working_tx The pointer to the working transaction.
 *
 * @return Nothing.
 */
void add_transaction(working_transaction *working_tx) {
    add_transaction_ts(default_transaction_context(), working_tx);
}

void add_transaction_ts(dogecoin_transaction_context* ctx, working_transaction *working_tx) {
    if (!ctx || !working_tx) return;
    dogecoin_mutex_lock(&ctx->lock);
    add_transaction_locked(ctx, working_tx);
    dogecoin_mutex_unlock(&ctx->lock);
}

/**
 * @brief This function takes an index and returns the working
 * transaction associated with that index in the hash table.
 *
 * @param idx The index of the target working transaction.
 *
 * @return The pointer to the working transaction associated with
 * the provided index.
 */
working_transaction* find_transaction(int idx) {
    /* Legacy lookup on the thread-local default context, which is never shared
       across threads. Return a borrowed pointer without retaining so the
       default context's entries keep refcount == 0 per the documented
       invariant (see working_transaction.refcount); the legacy callers of this
       API have no release_transaction_ts() pairing. */
    dogecoin_transaction_context* ctx = default_transaction_context();
    if (!ctx) return NULL;
    dogecoin_mutex_lock(&ctx->lock);
    working_transaction* working_tx = find_transaction_locked(ctx, idx);
    dogecoin_mutex_unlock(&ctx->lock);
    return working_tx;
}

working_transaction* find_transaction_ts(dogecoin_transaction_context* ctx, int idx) {
    if (!ctx) return NULL;
    dogecoin_mutex_lock(&ctx->lock);
    working_transaction* working_tx = find_transaction_locked(ctx, idx);
    if (working_tx) working_tx->refcount++; /* retain under the registry lock */
    dogecoin_mutex_unlock(&ctx->lock);
    return working_tx;
}

/* Release a reference obtained from find_transaction_ts(). Every successful
   (non-NULL) find_transaction_ts() must be paired with exactly one call here
   once the caller is done dereferencing the returned entry. */
void release_transaction_ts(dogecoin_transaction_context* ctx, working_transaction* working_tx) {
    if (!ctx || !working_tx) return;
    dogecoin_mutex_lock(&ctx->lock);
    release_transaction_locked(working_tx);
    dogecoin_mutex_unlock(&ctx->lock);
}

working_transaction* acquire_transaction_ts(dogecoin_transaction_context* ctx, int idx) {
    /* Owning-name alias; identical retain-under-lock semantics. */
    return find_transaction_ts(ctx, idx);
}

int with_transaction_ts(dogecoin_transaction_context* ctx, int idx, void (*fn)(working_transaction* working_tx, void* arg), void* arg) {
    if (!ctx || !fn) return 0;
    dogecoin_mutex_lock(&ctx->lock);
    working_transaction* working_tx = find_transaction_locked(ctx, idx);
    if (working_tx) fn(working_tx, arg);
    dogecoin_mutex_unlock(&ctx->lock);
    return working_tx != NULL;
}

/**
 * @brief This function removes the specified working transaction
 * from the hash table and frees the transactions in memory.
 *
 * @param working_tx The pointer to the transaction to remove.
 *
 * @return Nothing.
 */
void remove_transaction(working_transaction *working_tx) {
    remove_transaction_ts(default_transaction_context(), working_tx);
}

void remove_transaction_ts(dogecoin_transaction_context* ctx, working_transaction *working_tx) {
    if (!ctx || !working_tx) return;
    dogecoin_mutex_lock(&ctx->lock);
    remove_transaction_locked(ctx, working_tx);
    dogecoin_mutex_unlock(&ctx->lock);
}

/**
 * @brief This function removes all working transactions from
 * the hash table.
 *
 * @return Nothing.
 */
void remove_all() {
    remove_all_ts(default_transaction_context());
}

void remove_all_ts(dogecoin_transaction_context* ctx) {
    if (!ctx) return;
    struct working_transaction *current_tx;
    struct working_transaction *tmp;

    dogecoin_mutex_lock(&ctx->lock);
    HASH_ITER(hh, ctx->transactions, current_tx, tmp) {
        remove_transaction_locked(ctx, current_tx);
    }
    dogecoin_mutex_unlock(&ctx->lock);
}

/**
 * @brief This function prints the raw hex representation of
 * each working transaction in the hash table.
 *
 * @return Nothing.
 */
void print_transactions()
{
    struct working_transaction *s;
    working_transaction* root = default_transaction_context()->transactions;

    for (s = root; s != NULL; s = (struct working_transaction*)(s->hh.next)) {
        printf("\nworking transaction id: %d\nraw transaction (hexadecimal): %s\n", s->idx, get_raw_transaction(s->idx));
    }
}

/**
 * @brief This function counts the number of working
 * transactions currently in the hash table.
 *
 * @return Nothing.
 */
void count_transactions() {
    int temp = get_transaction_count();
    printf("there are %d transactions\n", temp);
}

int get_transaction_count(void) {
    return get_transaction_count_ts(default_transaction_context());
}

int get_transaction_count_ts(dogecoin_transaction_context* ctx) {
    if (!ctx) return 0;
    dogecoin_mutex_lock(&ctx->lock);
    int count = HASH_COUNT(ctx->transactions);
    dogecoin_mutex_unlock(&ctx->lock);
    return count;
}

/* THREAD-SAFE variant - uses internal mutex */
dogecoin_tx* dogecoin_tx_new_ts(void)
{
    dogecoin_tx* tx = dogecoin_tx_new();
    if (!tx) return NULL;
    if (dogecoin_mutex_init(&tx->lock)) {
        tx->thread_safe = true;
    } else {
#ifdef DOGECOIN_HAVE_THREADS
        // a threading runtime is present but the mutex could not be initialized
        // (e.g. resource exhaustion): fail rather than hand back an object that
        // callers expect to be lockable
        dogecoin_tx_free(tx);
        return NULL;
#else
        // no threading runtime (e.g. a freestanding OP-TEE TA): fall back to a
        // lock-free transaction so the _ts registry remains usable on
        // single-threaded targets instead of failing outright (the _ts mutex
        // helpers are no-ops in this configuration)
        tx->thread_safe = false;
#endif
    }
    return tx;
}

/* THREAD-SAFE variant - uses internal mutex */
void dogecoin_tx_free_ts(dogecoin_tx* tx)
{
    dogecoin_tx_free(tx);
}

/* THREAD-SAFE variant - uses internal mutex */
int dogecoin_tx_add_input_ts(dogecoin_tx* tx, const dogecoin_tx_in* tx_in)
{
    if (!tx || !tx_in || !tx->vin) return false;
    if (tx->thread_safe) dogecoin_mutex_lock(&tx->lock);
    dogecoin_tx_in* tx_in_new = dogecoin_tx_in_new();
    if (!tx_in_new) {
        if (tx->thread_safe) dogecoin_mutex_unlock(&tx->lock);
        return false;
    }
    dogecoin_tx_in_copy(tx_in_new, tx_in);
    vector_add(tx->vin, tx_in_new);
    if (tx->thread_safe) dogecoin_mutex_unlock(&tx->lock);
    return true;
}

/* THREAD-SAFE variant - uses internal mutex */
int dogecoin_tx_add_output_ts(dogecoin_tx* tx, const dogecoin_tx_out* tx_out)
{
    if (!tx || !tx_out || !tx->vout) return false;
    if (tx->thread_safe) dogecoin_mutex_lock(&tx->lock);
    dogecoin_tx_out* tx_out_new = dogecoin_tx_out_new();
    if (!tx_out_new) {
        if (tx->thread_safe) dogecoin_mutex_unlock(&tx->lock);
        return false;
    }
    dogecoin_tx_out_copy(tx_out_new, tx_out);
    vector_add(tx->vout, tx_out_new);
    if (tx->thread_safe) dogecoin_mutex_unlock(&tx->lock);
    return true;
}

/* THREAD-SAFE variant - uses internal mutex */
int dogecoin_tx_sign_ts(dogecoin_tx* tx, dogecoin_wallet* wallet, const char* passphrase)
{
    /* Reserved for future encrypted-wallet integration. */
    (void)passphrase;
    if (!tx || !wallet || !wallet->masterkey || !dogecoin_hdnode_has_privkey(wallet->masterkey)) return false;
    /* Lock-order contract enforced via the global lock hierarchy: tx (rank
       DOGECOIN_LOCK_RANK_TX) is always acquired before wallet (rank
       DOGECOIN_LOCK_RANK_WALLET). The ranked helpers assert on inversion in
       debug builds and compile down to the plain lock/unlock in release. */
    if (tx->thread_safe) dogecoin_mutex_lock_ranked(&tx->lock, DOGECOIN_LOCK_RANK_TX);
    if (wallet->thread_safe) dogecoin_mutex_lock_ranked(&wallet->lock, DOGECOIN_LOCK_RANK_WALLET);

    dogecoin_key key;
    dogecoin_privkey_init(&key);
    memcpy(key.privkey, wallet->masterkey->private_key, sizeof(key.privkey));

    size_t i;
    int ok = true;
    for (i = 0; i < tx->vin->len; i++) {
        dogecoin_tx_in* txin = vector_idx(tx->vin, i);
        if (!txin || !txin->script_sig || txin->script_sig->len == 0) continue;
        uint8_t sigcompact[64] = {0};
        uint8_t sigder[75] = {0};
        size_t sigder_len = sizeof(sigder);
        enum dogecoin_tx_sign_result res = dogecoin_tx_sign_input(tx, txin->script_sig, &key, i, 1, sigcompact, sigder, &sigder_len);
        if (res != DOGECOIN_SIGN_OK && res != DOGECOIN_SIGN_NO_KEY_MATCH) {
            ok = false;
            break;
        }
    }

    dogecoin_privkey_cleanse(&key);
    if (wallet->thread_safe) dogecoin_mutex_unlock_ranked(&wallet->lock, DOGECOIN_LOCK_RANK_WALLET);
    if (tx->thread_safe) dogecoin_mutex_unlock_ranked(&tx->lock, DOGECOIN_LOCK_RANK_TX);
    return ok;
}

/* THREAD-SAFE variant - uses internal mutex */
int dogecoin_tx_finalize_ts(dogecoin_tx* tx)
{
    if (!tx) return false;
    if (tx->thread_safe) dogecoin_mutex_lock(&tx->lock);
    /* Finalization currently performs a deterministic serialization+hash pass
     * as an integrity check; no additional state is persisted yet. */
    cstring* tmp = cstr_new_sz(256);
    dogecoin_tx_serialize(tmp, tx);
    uint256_t hash;
    dogecoin_tx_hash(tx, hash);
    cstr_free(tmp, true);
    if (tx->thread_safe) dogecoin_mutex_unlock(&tx->lock);
    return true;
}

/**
 * @brief This function takes two working transactions
 * and returns the difference of their indices to aid
 * in sorting transactions.
 *
 * @param a The pointer to the first working transaction.
 * @param b The pointer to the second working transaction.
 *
 * @return The integer difference between the indices of
 * the two provided transactions.
 */
int by_id(const struct working_transaction *a, const struct working_transaction *b)
{
    return (a->idx - b->idx);
}

/**
 * @brief This function prints a prompt and parses the user's
 * response for a CLI tool.
 *
 * @param prompt The prompt to display to the user.
 *
 * @return The string containing user input.
 */
const char *getl(const char *prompt)
{
    static char buf[100];
    char *p;
    printf("%s? ", prompt); fflush(stdout);
    p = fgets(buf, sizeof(buf), stdin);
    if (p == NULL || (p = strchr(buf, '\n')) == NULL) {
        puts("invalid input!");
        exit(EXIT_FAILURE);
    }
    *p = '\0';
    return buf;
}

/**
 * @brief This function prompts the user to enter a raw
 * transaction and parses it.
 *
 * @param prompt_tx The prompt to display to the user.
 *
 * @return The string containing user input.
 */
const char *get_raw_tx(const char *prompt_tx)
{
    static char buf_tx[1000*100];
    char *p_tx;
    printf("%s? ", prompt_tx); fflush(stdout);
    p_tx = fgets(buf_tx, sizeof(buf_tx), stdin);
    if (p_tx == NULL || (p_tx = strchr(buf_tx, '\n')) == NULL) {
        puts("invalid input!");
        exit(EXIT_FAILURE);
    }
    *p_tx = '\0';
    return buf_tx;
}

/**
 * @brief This function prompts the user to enter a private key
 * and parses it.
 *
 * @param prompt_tx The prompt to display to the user.
 *
 * @return The string containing user input.
 */
const char *get_private_key(const char *prompt_key)
{
    static char buf_key[100];
    char *p_key;
    printf("%s? ", prompt_key); fflush(stdout);
    p_key = fgets(buf_key, sizeof(buf_key), stdin);
    if (p_key == NULL || (p_key = strchr(buf_key, '\n')) == NULL) {
        puts("invalid input!");
        exit(EXIT_FAILURE);
    }
    *p_key = '\0';
    return buf_key;
}

/**
 * @brief This function creates a new transaction, places it in
 * the hash table, and returns the index of the new transaction,
 * starting from 1 and incrementing each subsequent call.
 *
 * @return The index of the new transaction.
 */
int start_transaction() {
    return start_transaction_ts(default_transaction_context());
}

int start_transaction_ts(dogecoin_transaction_context* ctx) {
    if (!ctx) return -1;
    /* Hold the lock across allocation, index assignment and insertion so the
       HASH_COUNT()->HASH_ADD() index allocation is atomic (the original split
       across new_transaction_ts/add_transaction_ts allowed two threads to mint
       the same idx and clobber each other via HASH_REPLACE). */
    dogecoin_mutex_lock(&ctx->lock);
    working_transaction* working_tx = new_transaction_locked(ctx);
    int index = -1;
    if (working_tx) {
        index = working_tx->idx;
        add_transaction_locked(ctx, working_tx);
    }
    dogecoin_mutex_unlock(&ctx->lock);
    return index;
}

/**
 * @brief This function takes a transaction represented in raw
 * hex and serializes it into a transaction which is then saved
 * in the hashtable at the specified index.
 *
 * @param txindex The index to save the transaction to.
 * @param hexadecimal_transaction The raw hex of the transaction to serialize and save.
 *
 * @return 1 if the transaction was saved successfully, 0 otherwise.
 */
int save_raw_transaction(int txindex, const char* hexadecimal_transaction) {
    return save_raw_transaction_ts(default_transaction_context(), txindex, hexadecimal_transaction);
}

int save_raw_transaction_ts(dogecoin_transaction_context* ctx, int txindex, const char* hexadecimal_transaction) {
    debug_print("raw_hexadecimal_transaction: %s\n", hexadecimal_transaction);
    if (!ctx || !hexadecimal_transaction) {
        printf("invalid tx hex\n");
        return false;
    }
    size_t hex_len = strspn(hexadecimal_transaction, VALID_HEX_CHARS);
    if (hex_len == 0 || (hex_len % 2) != 0 || hexadecimal_transaction[hex_len] != '\0' || hex_len > TXHEXMAXLEN) {
        printf("tx hex is invalid or too large (max 100kb)\n");
        return false;
    }

    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    uint8_t* data_bin = dogecoin_malloc(hex_len / 2 + 1);
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(hexadecimal_transaction, data_bin, hex_len, &outlength);
    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex");
        return false;
    }
    // free byte array
    working_transaction* tx_raw = find_transaction_ts(ctx, txindex);
    if (!tx_raw) {
        dogecoin_tx_free(txtmp);
        dogecoin_free(data_bin);
        return false;
    }
    // hold the per-object mutex while overwriting the working transaction
    dogecoin_bool locked = tx_raw->transaction->thread_safe;
    if (locked) dogecoin_mutex_lock(&tx_raw->transaction->lock);
    dogecoin_tx_copy(tx_raw->transaction, txtmp);
    if (locked) dogecoin_mutex_unlock(&tx_raw->transaction->lock);
    dogecoin_tx_free(txtmp);
    dogecoin_free(data_bin);
    release_transaction_ts(ctx, tx_raw);
    return true;
}

/**
 * @brief This function takes a transaction represented in raw
 * hex and adds it as an input to the specified working transaction.
 *
 * @param txindex The index of the transaction to add the input to.
 * @param hex_utxo_txid The raw transaction hex of the input transaction.
 * @param vout The output index of the input transaction containing spendable funds.
 *
 * @return 1 if the transaction input was added successfully, 0 otherwise.
 */
int add_utxo(int txindex, char* hex_utxo_txid, int vout) {
    return add_utxo_ts(default_transaction_context(), txindex, hex_utxo_txid, vout);
}

int add_utxo_ts(dogecoin_transaction_context* ctx, int txindex, char* hex_utxo_txid, int vout) {
    if (!ctx) return false;
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction_ts(ctx, txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    // validate hex txid: must be exactly 64 hex characters
    if (!hex_utxo_txid) { release_transaction_ts(ctx, tx); return false; }
    size_t hex_len = strspn(hex_utxo_txid, VALID_HEX_CHARS);
    if (hex_len != DOGECOIN_HASH_LENGTH * 2 || hex_utxo_txid[hex_len] != '\0') { release_transaction_ts(ctx, tx); return false; }

    size_t flag = tx->transaction->vin->len;

    // instantiate empty dogecoin_tx_in object to set previous output txid and output n:
    dogecoin_tx_in* tx_in = dogecoin_tx_in_new();

    // add prevout hash to tx_in->prevout.hash in prep of adding to tx->transaction-vin vector_t
    utils_uint256_sethex((char *)hex_utxo_txid, (uint8_t *)tx_in->prevout.hash);

    // set index of utxo we want to spend
    tx_in->prevout.n = vout;

    // route the input through the thread-safe primitive, which copies tx_in
    // into the working transaction under its per-object mutex when present:
    int added = dogecoin_tx_add_input_ts(tx->transaction, tx_in);

    // dogecoin_tx_add_input_ts copies the input, so free our local template:
    dogecoin_tx_in_free(tx_in);

    // ensure the length of our working tx inputs length has incremented by 1
    // which will return true if successful:
    int result = added && (flag + 1 == tx->transaction->vin->len);
    release_transaction_ts(ctx, tx);
    return result;
}

/**
 * @brief This function constructs an output sending the specified
 * amount to the specified address and adds it to the transaction
 * with the specified index.
 *
 * @param txindex The index of the transaction where the output will be added.
 * @param destinationaddress The address to send the funds to.
 * @param amount The amount of dogecoin to send.
 *
 * @return 1 if the transaction input was added successfully, 0 otherwise.
 */
int add_output(int txindex, char* destinationaddress, char* amount) {
    return add_output_ts(default_transaction_context(), txindex, destinationaddress, amount);
}

/**
 * @brief Internal helper that appends an address output to a working
 * transaction by routing it through the thread-safe dogecoin_tx_add_output_ts
 * primitive. The output script is built with the existing
 * dogecoin_tx_add_address_out logic into a throwaway transaction, then each
 * resulting output is copied into the working transaction under its per-object
 * mutex (when present).
 *
 * @return 1 if at least one output was added, 0 otherwise.
 */
static int add_address_output_ts(working_transaction* tx, const dogecoin_chainparams* chain, int64_t koinu, const char* address) {
    if (!tx || !tx->transaction || !chain || !address) return false;

    // build the output script(s) using the shared address-out logic in a scratch tx
    dogecoin_tx* scratch = dogecoin_tx_new();
    if (!scratch) return false;
    if (!dogecoin_tx_add_address_out(scratch, chain, koinu, address)) {
        dogecoin_tx_free(scratch);
        return false;
    }

    int added = false;
    size_t i;
    for (i = 0; i < scratch->vout->len; i++) {
        dogecoin_tx_out* tx_out = vector_idx(scratch->vout, i);
        if (dogecoin_tx_add_output_ts(tx->transaction, tx_out)) {
            added = true;
        }
    }
    dogecoin_tx_free(scratch);
    return added;
}

int add_output_ts(dogecoin_transaction_context* ctx, int txindex, char* destinationaddress, char* amount) {
    if (!ctx) return false;
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction_ts(ctx, txindex);
    // guard against null pointer exceptions
    if (tx == NULL) {
        return false;
    }
    // determine intended network by checking address prefix:
    const dogecoin_chainparams* chain = chain_from_b58_prefix(destinationaddress);

    uint64_t koinu = coins_to_koinu_str(amount);
    // calculate total minus fees
    // route through the thread-safe output primitive:
    int result = add_address_output_ts(tx, chain, (int64_t)koinu, destinationaddress);
    release_transaction_ts(ctx, tx);
    return result;
}

/**
 * @brief This function is for internal use and constructs an extra
 * output which returns the change back to the sender so that all of
 * the funds from inputs are spent in the current transaction.
 *
 * @param txindex The transaction which needs the output for returning change.
 * @param public_key The address of the sender for returning the change.
 * @param subtractedfee The amount to set aside for the mining fee.
 * @param amount The remaining funds after outputs have been subtracted from the inputs.
 *
 * @return 1 if the additional output was created successfully, 0 otherwise.
 */
static int make_change(int txindex, char* public_key, uint64_t subtractedfee, uint64_t amount) {
    return make_change_ts(default_transaction_context(), txindex, public_key, subtractedfee, amount);
}

static int make_change_ts(dogecoin_transaction_context* ctx, int txindex, char* public_key, uint64_t subtractedfee, uint64_t amount) {
    if (!ctx) return false;
    if (amount==subtractedfee) return false; // utxos already fully spent, no change needed
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction_ts(ctx, txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return false;

    // determine intended network by checking address prefix:
    const dogecoin_chainparams* chain = chain_from_b58_prefix(public_key);

    // calculate total minus fees
    uint64_t total_change_back = amount - subtractedfee;

    // route the change output through the thread-safe output primitive:
    int result = add_address_output_ts(tx, chain, (int64_t)total_change_back, public_key);

    // release the reference retained by find_transaction_ts() above:
    release_transaction_ts(ctx, tx);
    return result;
}

/**
 * @brief This function 'closes the inputs' by returning change to the recipient
 * after the total amount and desired fee is confirmed.
 *
 * @param txindex The index of the working transaction to finalize.
 * @param destinationaddress The address where the funds are being sent.
 * @param subtractedfee The amount to set aside as a fee to the miner.
 * @param out_dogeamount_for_verification An echo of the total amount to send.
 * @param changeaddress The address of the sender to receive the change.
 *
 * @return The hex of the finalized transaction.
 */
char* finalize_transaction(int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* changeaddress) {
    return finalize_transaction_ts(default_transaction_context(), txindex, destinationaddress, subtractedfee, out_dogeamount_for_verification, changeaddress);
}

char* finalize_transaction_ts(dogecoin_transaction_context* ctx, int txindex, char* destinationaddress, char* subtractedfee, char* out_dogeamount_for_verification, char* changeaddress) {
    if (!ctx) return NULL;
    // find working transaction by index and pass to funciton local variable to manipulate:
    working_transaction* tx = find_transaction_ts(ctx, txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return NULL;

    // determine intended network by checking address prefix:
    int is_testnet = chain_from_b58_prefix_bool(destinationaddress);

    uint64_t subtractedfee_koinu = coins_to_koinu_str(subtractedfee);
    uint64_t out_koinu_for_verification = coins_to_koinu_str(out_dogeamount_for_verification);

    // calculate total minus desired fees
    uint64_t total = (uint64_t)out_koinu_for_verification - (uint64_t)subtractedfee_koinu, tx_out_total = 0;

    int i, p2pkh_count = 0, length = (int)tx->transaction->vout->len;

    // iterate through transaction output values while adding each one to tx_out_total:
    for (i = 0; i < length; i++) {
        dogecoin_tx_out* tx_out_tmp = vector_idx(tx->transaction->vout, i);
        tx_out_total += tx_out_tmp->value;
        char p2pkh[36]; //mlumin: this was originally 17, caused problems if < 25.  p2pkh len is 24-36.
        //MLUMIN:MSVC
        dogecoin_mem_zero(p2pkh, sizeof(p2pkh));
        p2pkh_count = dogecoin_tx_out_pubkey_hash_to_p2pkh_address(tx_out_tmp, (char *)p2pkh, is_testnet);
        if (i == length - 1 && changeaddress) {
            // manually make change and send back to our public key address
            if (make_change_ts(ctx, txindex, changeaddress, subtractedfee_koinu, out_koinu_for_verification - tx_out_total)) {
                p2pkh_count += 1;
                tx_out_tmp = vector_idx(tx->transaction->vout, tx->transaction->vout->len - 1);
                tx_out_total += tx_out_tmp->value;
            }
            break;
        }
    }

    if (p2pkh_count < 1) {
        printf("p2pkh address not found from any output script hash!\n");
        release_transaction_ts(ctx, tx);
        return NULL;
    }

    if (tx_out_total != total) { release_transaction_ts(ctx, tx); return NULL; }

    // run the thread-safe finalization integrity pass before serializing:
    dogecoin_tx_finalize_ts(tx->transaction);

    // pass in transaction obect, network paramters, amount of dogecoin to send to address and finally p2pkh address:
    release_transaction_ts(ctx, tx);
    return get_raw_transaction_ts(ctx, txindex);
}

/**
 * @brief This function takes an index of a working transaction and returns
 * the hex representation of it.
 *
 * @param txindex The index of the working transaction.
 *
 * @return The hex representation of the transaction.
 */
char* get_raw_transaction(int txindex) {
    return get_raw_transaction_ts(default_transaction_context(), txindex);
}

char* get_raw_transaction_ts(dogecoin_transaction_context* ctx, int txindex) {
    if (!ctx) return NULL;
    // find working transaction by index and pass to function local variable to manipulate:
    working_transaction* tx = find_transaction_ts(ctx, txindex);

    // guard against null pointer exceptions
    if (tx == NULL) return NULL;

    // serialize under the per-object mutex when present:
    dogecoin_bool locked = tx->transaction->thread_safe;
    if (locked) dogecoin_mutex_lock(&tx->transaction->lock);

    // new allocated cstring to store hexadeicmal buffer string:
    cstring* serialized_transaction = cstr_new_sz(1024);

    // serialize transaction object to new cstring:
    dogecoin_tx_serialize(serialized_transaction, tx->transaction);

    char* hexadecimal_buffer = utils_uint8_to_hex((unsigned char*)serialized_transaction->str, serialized_transaction->len);

    cstr_free(serialized_transaction, true);

    if (locked) dogecoin_mutex_unlock(&tx->transaction->lock);

    release_transaction_ts(ctx, tx);
    return hexadecimal_buffer;
}

/**
 * @brief This function removes the specified working transaction
 * from the hash table.
 *
 * @param txindex The index of the working transaction to remove.
 *
 * @return Nothing.
 */
void clear_transaction(int txindex) {
    clear_transaction_ts(default_transaction_context(), txindex);
}

void clear_transaction_ts(dogecoin_transaction_context* ctx, int txindex) {
    if (!ctx) return;
    /* find+remove under a single lock so the entry cannot be freed by another
       thread between the lookup and the delete. */
    dogecoin_mutex_lock(&ctx->lock);
    working_transaction* tx = find_transaction_locked(ctx, txindex);
    if (tx) remove_transaction_locked(ctx, tx);
    dogecoin_mutex_unlock(&ctx->lock);
}

/**
 * @brief This function signs the specified input of a working transaction,
 * according to the signing parameters specified.
 *
 * @param inputindex The index of the current transaction input to sign.
 * @param incomingrawtx The hex representation of the transaction to sign.
 * @param scripthex The hex representation of the public key script.
 * @param sighashtype The type of signature hash to perform.
 * @param privkey The private key used to sign the transaction input.
 *
 * @return 1 if the raw transaction was signed successfully, 0 otherwise.
 */
int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey) {
    if(!incomingrawtx || !scripthex || !privkey) return false;

    size_t tx_hex_len = strspn(incomingrawtx, VALID_HEX_CHARS);
    if (tx_hex_len == 0 || (tx_hex_len % 2) != 0 || incomingrawtx[tx_hex_len] != '\0' || tx_hex_len > TXHEXMAXLEN) {
        printf("tx hex is invalid or too large (max 100kb)\n");
        return false;
    }

    size_t script_hex_len = strspn(scripthex, VALID_HEX_CHARS);
    if (script_hex_len == 0 || (script_hex_len % 2) != 0 || scripthex[script_hex_len] != '\0') {
        printf("invalid script hex\n");
        return false;
    }

    const dogecoin_chainparams* chain = (privkey[0] == 'c') ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    uint8_t* data_bin = dogecoin_malloc(tx_hex_len / 2 + 1);
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(incomingrawtx, data_bin, tx_hex_len, &outlength);

    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex\n");
        return false;
    }
    // free byte array
    dogecoin_free(data_bin);

    // if utxo input doesn't exist abort attempt to sign message
    if ((size_t)inputindex >= txtmp->vin->len) {
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("input index out of range");
        return false;
    }

    // initialize byte array with length equal to account for byte size
    uint8_t* script_data = dogecoin_uint8_vla(script_hex_len);
    // convert hex string to byte array
    utils_hex_to_bin(scripthex, script_data, script_hex_len, &outlength);
    cstring* script = cstr_new_buf(script_data, outlength);

    uint256_t sighash;
    dogecoin_mem_zero(sighash, sizeof(sighash));
    dogecoin_free(script_data);

    dogecoin_tx_sighash(txtmp, script, inputindex, sighashtype, sighash);

    char *hex = utils_uint8_to_hex(sighash, DOGECOIN_HASH_LENGTH);
    utils_reverse_hex(hex, DOGECOIN_HASH_LENGTH * 2);

    debug_print("script: %s\n", scripthex);
    debug_print("script-type: %s\n", dogecoin_tx_out_type_to_str(dogecoin_script_classify(script, NULL)));
    debug_print("inputindex: %d\n", inputindex);
    debug_print("sighashtype: %d\n", sighashtype);
    debug_print("hash: %s\n", hex);
    // sign
    dogecoin_bool sign = false;
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    if (dogecoin_privkey_decode_wif(privkey, chain, &key)) {
        sign = true;
    } else {
        // WIF decode failed: the key is unusable. Previously this leaked txtmp
        // and script and fell through to `return true` (claiming success on a
        // bad key) whenever strlen(privkey) <= 50. Always clean up and fail.
        dogecoin_tx_free(txtmp);
        cstr_free(script, true);
        return false;
    }
    if (sign) {
        uint8_t sigcompact[64] = {0};
        size_t sigderlen = 74 + 1; //&hashtype
        uint8_t sigder_plus_hashtype[75] = {0};
        enum dogecoin_tx_sign_result res = dogecoin_tx_sign_input(txtmp, script, &key, inputindex, sighashtype, sigcompact, sigder_plus_hashtype, &sigderlen);
        cstr_free(script, true);

        if (res != DOGECOIN_SIGN_OK) {
            dogecoin_tx_free(txtmp);
            return false;
        }

        char sigcompacthex[64*2+1] = {0};
        utils_bin_to_hex((unsigned char *)sigcompact, 64, sigcompacthex);

        char sigderhex[74*2+2+1]; //74 der, 2 hashtype, 1 nullbyte
        dogecoin_mem_zero(sigderhex, sizeof(sigderhex));
        utils_bin_to_hex((unsigned char *)sigder_plus_hashtype, sigderlen, sigderhex);

        debug_print("\nsignature created:\nsignature compact: %s\n", sigcompacthex);
        debug_print("signature DER (+hashtype): %s\n", sigderhex);

        cstring* signed_tx = cstr_new_sz(1024);
        dogecoin_tx_serialize(signed_tx, txtmp);

        char* signed_tx_hex = dogecoin_char_vla(signed_tx->len * 2 + 1);
        utils_bin_to_hex((unsigned char *)signed_tx->str, signed_tx->len, signed_tx_hex);
        size_t signed_len = strlen(signed_tx_hex);
        if (signed_len >= TO_UINT8_HEX_BUF_LEN) {
            printf("signed tx too large (max 100 kB)\n");
            cstr_free(signed_tx, true);
            dogecoin_tx_free(txtmp);
            dogecoin_free(signed_tx_hex);
            return false;
        }
        // Overwrite the caller's buffer in place. Copy only the actual signed
        // length (+ NUL), never a fixed TO_UINT8_HEX_BUF_LEN: strncpy with a
        // fixed count NUL-fills the destination out to that count, writing
        // 200001 bytes into a buffer the caller sized to the input tx. That
        // overflows any binding/caller buffer not pre-sized to TXHEXMAXLEN
        // (heap corruption -> SIGABRT). The in-place contract requires the
        // caller's buffer to be at least signed_len + 1; the signed tx is only
        // marginally larger than the unsigned input it replaces.
        memcpy(incomingrawtx, signed_tx_hex, signed_len);
        incomingrawtx[signed_len] = '\0';
        debug_print("signed TX: %s\n", incomingrawtx);
        cstr_free(signed_tx, true);
        dogecoin_tx_free(txtmp);
        dogecoin_free(signed_tx_hex);
    }
    return true;
}

/**
 * @brief This function is for internal use and saves the result of
 * sign_raw_transaction to a working transaction in the hash table.
 *
 * @param txindex The index where the signed transaction will be saved.
 * @param inputindex The index of the current transaction input to sign.
 * @param incomingrawtx The hex representation of the transaction to sign.
 * @param scripthex The hex representation of the public key script.
 * @param sighashtype The type of signature hash to perform.
 * @param privkey The private key used to sign the transaction input.
 *
 * @return 1 if the transaction was signed successfully, 0 otherwise.
 */
int sign_indexed_raw_transaction(int txindex, int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, char* privkey) {
    if (!txindex) return false;
    if (!sign_raw_transaction(inputindex, incomingrawtx, scripthex, sighashtype, privkey)) {
        printf("error signing raw transaction\n");
        return false;
    }
    if (!save_raw_transaction(txindex, incomingrawtx)) {
        printf("error saving transaction!\n");
        return false;
    }
    return true;
}

/**
 * @brief This function signs all of the inputs in the specified working
 * transaction using the provided script pubkey and private key.
 *
 * @param txindex The index of the working transaction to sign.
 * @param script_pubkey The hex representation of the public key script.
 * @param privkey The private key used to sign the transaction input.
 *
 * @return 1 if the transaction was signed successfully, 0 otherwise.
 */
int sign_transaction(int txindex, char* script_pubkey, char* privkey) {
    char* raw_hexadecimal_transaction = get_raw_transaction(txindex);
    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    uint8_t* data_bin = dogecoin_malloc(strlen(raw_hexadecimal_transaction) / 2);
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(raw_hexadecimal_transaction, data_bin, strlen(raw_hexadecimal_transaction), &outlength);
    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex\n");
        return false;
    }
    // free byte array
    dogecoin_free(data_bin);
    size_t i = 0, len = txtmp->vin->len;
    for (; i < len; i++) {
        if (!sign_raw_transaction(i, raw_hexadecimal_transaction, script_pubkey, 1, privkey)) {
            printf("error signing raw transaction: %s\n", __func__);
            return false;
        }
    }
    save_raw_transaction(txindex, raw_hexadecimal_transaction);
    dogecoin_tx_free(txtmp);
    return true;
}

/**
 * @brief This function signs a specific vin index in the specified working
 * transaction using the provided private key and vin index.
 *
 * @param txindex The index of the working transaction to sign.
 * @param vout_index The index of the unspent tx output we are spending.
 * @param privkey The private key used to sign the transaction input.
 *
 * @return 1 if the transaction was signed successfully, 0 otherwise.
 */
int sign_transaction_w_privkey(int txindex, int vout_index, char* privkey) {
    char* script_pubkey = dogecoin_private_key_wif_to_pubkey_hash(privkey);
    char* raw_hexadecimal_transaction = get_raw_transaction(txindex);

    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    uint8_t* data_bin = dogecoin_malloc(strlen(raw_hexadecimal_transaction) / 2);
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(raw_hexadecimal_transaction, data_bin, strlen(raw_hexadecimal_transaction), &outlength);
    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex\n");
        return false;
    }
    // free byte array
    dogecoin_free(data_bin);
    if (!sign_indexed_raw_transaction(txindex, vout_index, raw_hexadecimal_transaction, script_pubkey, 1, privkey)) {
        dogecoin_free(script_pubkey);
        dogecoin_tx_free(txtmp);
        printf("error signing raw transaction: %s\n", __func__);
        return false;
    }
    save_raw_transaction(txindex, raw_hexadecimal_transaction);
    dogecoin_free(script_pubkey);
    dogecoin_tx_free(txtmp);
    return true;
}

/**
 * @brief This function stores a raw transaction to the next available
 * working transaction in the hash table.
 *
 * @param incomingrawtx The hex of the raw transaction
 *
 * @return The index of the new working transaction if stored successfully, 0 otherwise.
 */
int store_raw_transaction(char* incomingrawtx) {
    if (strlen(incomingrawtx) > TXHEXMAXLEN) { //don't accept tx larger then 100kb
        printf("tx too large (max 100kb)\n");
        return false;
    }

    // deserialize transaction
    dogecoin_tx* txtmp = dogecoin_tx_new();
    int txindex = start_transaction();
    working_transaction* tx_raw = find_transaction(txindex);
    uint8_t* data_bin = dogecoin_malloc(strlen(incomingrawtx));
    size_t outlength = 0;
    // convert incomingrawtx to byte array to dogecoin_tx and if it fails free from memory
    utils_hex_to_bin(incomingrawtx, data_bin, strlen(incomingrawtx), &outlength);
    if (!dogecoin_tx_deserialize(data_bin, outlength, txtmp, NULL)) {
        // free byte array
        dogecoin_free(data_bin);
        // free dogecoin_tx
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex");
        return false;
    }
    // free byte array
    dogecoin_free(data_bin);
    dogecoin_tx_copy(tx_raw->transaction, txtmp);
    dogecoin_tx_free(txtmp);
    return txindex;
}

/**
 * @brief This function takes an index of a working transaction and writes
 * its hex representation into a caller-provided buffer.
 *
 * @param txindex   The index of the working transaction.
 * @param buf       Buffer to receive the hex string.
 * @param buf_cap   Capacity of buf in bytes.
 *
 * @return Number of bytes written on success, or 0 on error.
 */
int get_raw_transaction_ex(int txindex, char* buf, size_t buf_cap) {
    working_transaction* tx = find_transaction(txindex);
    if (!tx || !buf || buf_cap == 0) return 0;

    // serialize to cstring
    cstring* serialized = cstr_new_sz(1024);
    if (!serialized) return 0;
    dogecoin_tx_serialize(serialized, tx->transaction);

    // hex-encode into provided buffer
    size_t hex_len = serialized->len * 2;
    if (hex_len + 1 > buf_cap) {
        cstr_free(serialized, true);
        return 0;
    }
    utils_bin_to_hex((unsigned char*)serialized->str, serialized->len, buf);
    cstr_free(serialized, true);
    return (int)hex_len;
}

/**
 * @brief This function signs the specified input of a working transaction.
 * It supports a two-step pattern:
 *   1) Call with signedrawtx==NULL to query needed buffer size in *signed_size.
 *   2) Allocate signedrawtx to at least *signed_size, then call again to fill it.
 *
 * @param inputindex    Index of the input within the transaction to sign.
 * @param incomingrawtx The raw transaction hex to be signed.
 * @param signedrawtx   Buffer to receive signed hex, or NULL to query size.
 * @param signed_size   On entry: capacity of signedrawtx; on exit (query mode): required size (including NUL).
 * @param scripthex     The pubkey script for this input, in hex.
 * @param sighashtype   SIGHASH type (e.g. 1 for ALL).
 * @param privkey       WIF-encoded private key.
 *
 * @return 1 on success (or size-report), 0 on error.
 */
int sign_raw_transaction_ex(int    inputindex,
                            const char*  incomingrawtx,
                            char*  signedrawtx,
                            size_t* signed_size,
                            const char*  scripthex,
                            int    sighashtype,
                            const char*  privkey)
{
    if (!incomingrawtx || !scripthex || !signed_size || !privkey) return 0;
    if (strlen(incomingrawtx) > TO_UINT8_HEX_BUF_LEN) {
        printf("tx too large (max 100 KB)\n");
        return 0;
    }

    // deserialize incomingrawtx
    dogecoin_tx* txtmp = dogecoin_tx_new();
    if (!txtmp) return 0;
    size_t outlen = 0;
    uint8_t* data = dogecoin_malloc(strlen(incomingrawtx) / 2);
    if (!data) {
        dogecoin_tx_free(txtmp);
        return 0;
    }
    utils_hex_to_bin(incomingrawtx, data, strlen(incomingrawtx), &outlen);
    if (outlen == 0 || !dogecoin_tx_deserialize(data, outlen, txtmp, NULL)) {
        dogecoin_free(data);
        dogecoin_tx_free(txtmp);
        printf("invalid tx hex\n");
        return 0;
    }
    dogecoin_free(data);

    // bounds-check input index
    if ((size_t)inputindex >= txtmp->vin->len) {
        dogecoin_tx_free(txtmp);
        printf("input index out of range\n");
        return 0;
    }

    // build script cstring
    size_t script_hex_len = strlen(scripthex) / 2;
    uint8_t* script_data = dogecoin_malloc(script_hex_len);
    if (!script_data) {
        dogecoin_tx_free(txtmp);
        return 0;
    }
    utils_hex_to_bin(scripthex, script_data, strlen(scripthex), &outlen);
    if (outlen == 0) {
        dogecoin_free(script_data);
        dogecoin_tx_free(txtmp);
        return 0;
    }
    cstring* script = cstr_new_buf(script_data, outlen);
    dogecoin_free(script_data);
    if (!script) {
        dogecoin_tx_free(txtmp);
        return 0;
    }

    // compute sighash
    uint256_t sighash;
    dogecoin_mem_zero(sighash, sizeof(sighash));
    dogecoin_tx_sighash(txtmp, script, inputindex, sighashtype, sighash);

    // decode private key
    const dogecoin_chainparams* chain = (privkey[0] == 'c')
        ? &dogecoin_chainparams_test
        : &dogecoin_chainparams_main;
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    if (!dogecoin_privkey_decode_wif(privkey, chain, &key)) {
        cstr_free(script, true);
        dogecoin_tx_free(txtmp);
        return 0;
    }

    // sign
    uint8_t sigcompact[64] = {0};
    size_t sigderlen = 75;
    uint8_t sigder_plus_hashtype[75] = {0};
    enum dogecoin_tx_sign_result res = dogecoin_tx_sign_input(
        txtmp, script, &key,
        inputindex, sighashtype,
        sigcompact, sigder_plus_hashtype, &sigderlen
    );
    cstr_free(script, true);
    if (res != DOGECOIN_SIGN_OK) {
        dogecoin_tx_free(txtmp);
        return 0;
    }

    // serialize signed tx
    cstring* signed_tx = cstr_new_sz(1024);
    if (!signed_tx) {
        dogecoin_tx_free(txtmp);
        return 0;
    }
    dogecoin_tx_serialize(signed_tx, txtmp);
    dogecoin_tx_free(txtmp);

    // hex‐encode output
    size_t hexout_len = signed_tx->len * 2 + 1;
    char* hexout = dogecoin_malloc(hexout_len);
    if (!hexout) {
        cstr_free(signed_tx, true);
        return 0;
    }
    utils_bin_to_hex((unsigned char*)signed_tx->str, signed_tx->len, hexout);
    cstr_free(signed_tx, true);

    // determine buffer need
    size_t need = strlen(hexout) + 1;

    // step 1: query mode
    if (!signedrawtx) {
        *signed_size = need;
        dogecoin_free(hexout);
        return 1;
    }

    // step 2: write mode
    if (*signed_size < need) {
        printf("signed buffer too small (need %zu bytes)\n", need);
        dogecoin_free(hexout);
        return 0;
    }
    memcpy(signedrawtx, hexout, need - 1);
    signedrawtx[need - 1] = '\0';
    dogecoin_free(hexout);

    return 1;
}

/**
 * @brief  This function 'closes the inputs' by returning change to the recipient
 * after the total amount and desired fee is confirmed. It uses the same logic as finalize_transaction,
 * but writes the result into a caller-provided buffer.
 *
 * @param txindex The index of the working transaction to finalize.
 * @param destinationaddress The address where the funds are being sent.
 * @param subtractedfee The amount to set aside as a fee to the miner.
 * @param out_dogeamount_for_verification An echo of the total amount to send.
 * @param changeaddress The address of the sender to receive the change.
 * @param buf Buffer to receive the hex string.
 * @param buf_cap Capacity of buf in bytes.
 *
 * @return Number of bytes written on success, or 0 on error.
 */
int finalize_transaction_ex(int   txindex,
                            char* destinationaddress,
                            char* subtractedfee,
                            char* out_dogeamount_for_verification,
                            char* changeaddress,
                            char* buf,
                            size_t buf_cap)
{
    working_transaction* tx = find_transaction(txindex);
    if (!tx || !buf || buf_cap == 0) return 0;

    // fee / totals
    int      is_testnet     = chain_from_b58_prefix_bool(destinationaddress);
    uint64_t fee_koinu      = coins_to_koinu_str(subtractedfee);
    uint64_t total_in_koinu = coins_to_koinu_str(out_dogeamount_for_verification);
    uint64_t expected_total = total_in_koinu - fee_koinu;
    uint64_t tx_out_total   = 0;
    int      p2pkh_hits     = 0;

    int vout_len = (int)tx->transaction->vout->len;
    for (int i = 0; i < vout_len; ++i) {
        dogecoin_tx_out* tout = vector_idx(tx->transaction->vout, i);
        tx_out_total += tout->value;

        char addr[P2PKHLEN]; dogecoin_mem_zero(addr, sizeof addr);
        p2pkh_hits += dogecoin_tx_out_pubkey_hash_to_p2pkh_address(
                          tout, addr, is_testnet);

        // last output: maybe append change
        if (i == vout_len - 1 && changeaddress) {
            if (make_change(txindex, changeaddress, fee_koinu,
                            total_in_koinu - tx_out_total)) {
                dogecoin_tx_out* ch =
                    vector_idx(tx->transaction->vout,
                               tx->transaction->vout->len - 1);
                tx_out_total += ch->value;
                ++p2pkh_hits;
            }
        }
    }

    if (p2pkh_hits < 1 || tx_out_total != expected_total) return 0;

    // stream final hex into caller buffer
    return get_raw_transaction_ex(txindex, buf, buf_cap);
}

/**
 * @brief This function signs a single vin (`inputindex`) of the working
 * transaction at `txindex` without allocating heap memory for the hex.
 * It:
 *   1. Serialises the transaction into `buf`,
 *   2. Calls `sign_raw_transaction_ex()` **in-place** on that buffer,
 *   3. Persists the signed hex back to the hash-table slot.
 *
 * @param txindex     The index of the working transaction in memory.
 * @param inputindex  The vin to sign.
 * @param scripthex   The script PubKey (hex) that locks the referenced UTXO.
 * @param sighashtype Signature hash type (e.g. 1 = SIGHASH_ALL).
 * @param privkey     WIF-encoded private key to sign with.
 * @param buf         Caller-supplied output buffer for the hex.
 * @param buf_cap     Capacity of `buf` in bytes.
 *
 * @return 1 on success, 0 on error.
 */
int sign_indexed_raw_transaction_ex(int  txindex,
                                    int  inputindex,
                                    const char* scripthex,
                                    int  sighashtype,
                                    const char* privkey,
                                    char* buf,
                                    size_t buf_cap)
{
    if (!buf || buf_cap == 0) return 0;
    if (!get_raw_transaction_ex(txindex, buf, buf_cap))                 /* (1) */
        return 0;

    size_t cap = buf_cap;
    if (!sign_raw_transaction_ex(inputindex, buf, buf, &cap,            /* (2) */
                                 scripthex, sighashtype, privkey))
        return 0;

    return save_raw_transaction(txindex, buf);                          /* (3) */
}


/**
 * @brief Signs **all** inputs of the working transaction at `txindex`
 * into the caller-provided buffer `buf`. No heap strings are created.
 *
 * @param txindex        Index of the working transaction.
 * @param script_pubkey  The common script PubKey for all inputs (hex).
 * @param privkey        WIF private key.
 * @param buf            Output buffer receiving the final signed hex.
 * @param buf_cap        Capacity of `buf` in bytes.
 *
 * @return 1 if the transaction was fully signed & stored, 0 otherwise.
 */
int sign_transaction_ex(int  txindex,
                        const char* script_pubkey,
                        const char* privkey,
                        char* buf,
                        size_t buf_cap)
{
    if (!buf || buf_cap == 0 || !script_pubkey || !privkey) return 0;

    working_transaction* tx = find_transaction(txindex);
    if (!tx) return 0;
    size_t vin_cnt = tx->transaction->vin->len;

    if (!get_raw_transaction_ex(txindex, buf, buf_cap))                 /* initial */
        return 0;

    for (size_t i = 0; i < vin_cnt; ++i) {
        size_t cap = buf_cap;
        if (!sign_raw_transaction_ex((int)i, buf, buf, &cap,
                                     script_pubkey, 1, privkey))
            return 0;
    }
    return save_raw_transaction(txindex, buf);
}


/**
 * @brief Convenience wrapper around `sign_transaction_ex()` that derives
 * the P2PKH script PubKey from the supplied WIF key.
 *
 * @param txindex  Index of the working transaction.
 * @param privkey  WIF-encoded private key (used for both script derivation
 *                 and signing).
 * @param buf      Output buffer for the fully-signed hex.
 * @param buf_cap  Capacity of `buf` in bytes.
 *
 * @return 1 on success, 0 on error.
 */
int sign_transaction_w_privkey_ex(int  txindex,
                                  const char* privkey,
                                  char* buf,
                                  size_t buf_cap)
{
    if (!privkey) return 0;
    char* script_pubkey = dogecoin_private_key_wif_to_pubkey_hash((char*)privkey);
    if (!script_pubkey) return 0;

    int ok = sign_transaction_ex(txindex, script_pubkey, privkey, buf, buf_cap);
    dogecoin_free(script_pubkey);
    return ok;
}

/**
 * @brief Build an M-of-N P2SH multisig address from compressed pubkey hex strings.
 *
 * @param pubkeys_hex            Array of (n) compressed pubkey hex strings (33 bytes each = 66 hex chars).
 * @param n                      Total number of cosigners.
 * @param m                      Required signatures.
 * @param is_testnet             Non-zero to use testnet chain params.
 * @param p2sh_addr_out          Output buffer for the P2SH address (must be at least P2PKHLEN bytes).
 * @param p2sh_addr_cap          Capacity of @p p2sh_addr_out in bytes.
 * @param redeem_script_hex_out  Output buffer for the redeem script hex
 *                               (must be at least (n*68+6)*2+1 bytes; 1200 is safe for up to 15-of-15).
 * @param redeem_script_hex_cap  Capacity of @p redeem_script_hex_out in bytes.
 *
 * @return 1 on success, 0 on error (including insufficient buffer capacity).
 */
/* Derive the P2SH address for an arbitrary redeem script.
 *
 * get_p2sh_multisig_address() covers plain M-of-N, which is the only redeem
 * script a caller could previously reach: dogecoin_script_get_scripthash() and
 * the script builders live in script.h, which is not installed. A consumer
 * holding any other redeem script -- a timelocked branch, an escrow, anything
 * assembled by hand -- had no way to compute the address it pays to.
 *
 * Takes the script as hex so no libdogecoin type appears in the signature. */
int get_p2sh_address_from_script(const char* redeem_script_hex, int is_testnet,
                                 char* p2sh_addr_out, size_t p2sh_addr_cap)
{
    if (!redeem_script_hex || !p2sh_addr_out) return 0;
    if (p2sh_addr_cap < P2SHLEN) return 0;

    size_t hexlen = strlen(redeem_script_hex);
    if (hexlen == 0 || (hexlen % 2) != 0) return 0;
    if (strspn(redeem_script_hex, VALID_HEX_CHARS) != hexlen) return 0;

    /* A P2SH redeem script is pushed as a single stack element, so it cannot
       exceed MAX_SCRIPT_ELEMENT_SIZE. Reject here rather than return an address
       for a script that can never be spent. */
    size_t binlen = hexlen / 2;
    if (binlen > 520) return 0;

    unsigned char* bin = (unsigned char*)dogecoin_malloc(binlen);
    size_t outlen = 0;
    utils_hex_to_bin(redeem_script_hex, bin, hexlen, &outlen);
    if (outlen != binlen) { dogecoin_free(bin); return 0; }

    cstring* redeem = cstr_new_buf(bin, binlen);
    dogecoin_free(bin);
    if (!redeem) return 0;

    uint160_t script_hash;
    dogecoin_bool ok = dogecoin_script_get_scripthash(redeem, script_hash);
    cstr_free(redeem, true);
    if (!ok) return 0;

    const dogecoin_chainparams* chain = is_testnet
        ? &dogecoin_chainparams_test
        : &dogecoin_chainparams_main;
    return dogecoin_p2sh_addr_from_hash160(script_hash, chain, p2sh_addr_out, p2sh_addr_cap) ? 1 : 0;
}

int get_p2sh_multisig_address(const char** pubkeys_hex, int n, int m, int is_testnet,
                               char* p2sh_addr_out, size_t p2sh_addr_cap,
                               char* redeem_script_hex_out, size_t redeem_script_hex_cap)
{
    if (!pubkeys_hex || n < 1 || m < 1 || m > n || !p2sh_addr_out || !redeem_script_hex_out)
        return 0;
    if (p2sh_addr_cap < P2PKHLEN)
        return 0;

    /* load pubkeys into a vector */
    vector_t* pubs = vector_new((size_t)n, dogecoin_free);
    for (int i = 0; i < n; ++i) {
        if (!pubkeys_hex[i]) { vector_free(pubs, true); return 0; }
        dogecoin_pubkey* pk = (dogecoin_pubkey*)dogecoin_malloc(sizeof(dogecoin_pubkey));
        dogecoin_pubkey_init(pk);
        pk->compressed = 1;
        size_t outlen = 0;
        utils_hex_to_bin(pubkeys_hex[i], pk->pubkey, strlen(pubkeys_hex[i]), &outlen);
        if (outlen != 33 || !dogecoin_pubkey_is_valid(pk)) {
            dogecoin_free(pk);
            vector_free(pubs, true);
            return 0;
        }
        vector_add(pubs, pk);
    }

    /* build the redeem script */
    cstring* redeem = cstr_new_sz(550);
    if (!dogecoin_script_build_multisig(redeem, (unsigned int)m, pubs)) {
        cstr_free(redeem, true);
        vector_free(pubs, true);
        return 0;
    }
    vector_free(pubs, true);

    /* redeem script → hex (each byte renders as two hex chars + NUL terminator) */
    if (redeem_script_hex_cap < (size_t)redeem->len * 2 + 1) {
        cstr_free(redeem, true);
        return 0;
    }
    utils_bin_to_hex((unsigned char*)redeem->str, redeem->len, redeem_script_hex_out);

    /* redeem script → hash160 → P2SH address */
    uint160_t script_hash;
    dogecoin_script_get_scripthash(redeem, script_hash);
    cstr_free(redeem, true);

    const dogecoin_chainparams* chain = is_testnet
        ? &dogecoin_chainparams_test
        : &dogecoin_chainparams_main;
    if (!dogecoin_p2sh_addr_from_hash160(script_hash, chain, p2sh_addr_out, p2sh_addr_cap))
        return 0;

    return 1;
}
