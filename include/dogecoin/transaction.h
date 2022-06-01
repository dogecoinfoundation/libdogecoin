/*
 The MIT License (MIT)
 
 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

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
#include <stdio.h>     /* printf       */
#include <string.h>    /* memset       */
#include <contrib/uthash/uthash.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/tool.h>

LIBDOGECOIN_BEGIN_DECL

/* hashmap functions */
typedef struct working_transaction {
    int idx;
    dogecoin_tx* transaction;
    UT_hash_handle hh;
} working_transaction;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static working_transaction *transactions = NULL;
#pragma GCC diagnostic pop
// instantiates a new transaction
LIBDOGECOIN_API working_transaction* new_transaction();

LIBDOGECOIN_API void add_transaction(working_transaction *working_tx);

LIBDOGECOIN_API working_transaction* find_transaction(int idx);

LIBDOGECOIN_API void remove_transaction(working_transaction *working_tx);

LIBDOGECOIN_API void remove_all();

LIBDOGECOIN_API void print_transactions();

LIBDOGECOIN_API void count_transactions();

LIBDOGECOIN_API int by_id();

LIBDOGECOIN_API const char *getl(const char *prompt);

LIBDOGECOIN_API const char *get_raw_tx(const char *prompt_tx);

LIBDOGECOIN_API const char *get_private_key(const char *prompt_key);

LIBDOGECOIN_API int start_transaction(); // #returns  an index of a transaction to build in memory.  (1, 2, etc) ..   

LIBDOGECOIN_API int save_raw_transaction(int txindex, const char* hexadecimal_transaction);

LIBDOGECOIN_API int add_utxo(int txindex, char* hex_utxo_txid, int vout); // #returns 1 if success.

LIBDOGECOIN_API int add_output(int txindex, char* destinationaddress, long double amount);

// 'closes the inputs', specifies the recipient, specifies the amnt-to-subtract-as-fee, and returns the raw tx..
// out_dogeamount == just an echoback of the total amount specified in the addutxos for verification
LIBDOGECOIN_API char* finalize_transaction(int txindex, char* destinationaddress, long double subtractedfee, long double out_dogeamount_for_verification, char* public_key);

LIBDOGECOIN_API char* get_raw_transaction(int txindex); // #returns 0 if not closed, returns rawtx again if closed/created.

LIBDOGECOIN_API void clear_transaction(int txindex); // #clears a tx in memory. (overwrites)

// sign a given inputted transaction with a given private key, and return a hex signed transaction.
// we may want to add such things to 'advanced' section:
// locktime, possibilities for multiple outputs, data, sequence.
LIBDOGECOIN_API int sign_raw_transaction(int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, long double amount, char* privkey);

LIBDOGECOIN_API int sign_indexed_raw_transaction(int txindex, int inputindex, char* incomingrawtx, char* scripthex, int sighashtype, long double amount, char* privkey);

LIBDOGECOIN_API int sign_transaction(int txindex, long double amounts[], char* script_pubkey, char* privkey);

LIBDOGECOIN_API int store_raw_transaction(char* incomingrawtx);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_TRANSACTION_H__
