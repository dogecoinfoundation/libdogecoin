/*

 The MIT License (MIT)

 Copyright (c) 2015 Douglas J. Bakkum
 Copyright (c) 2015 Jonas Schnelli
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


#ifndef __LIBDOGECOIN_TX_H__
#define __LIBDOGECOIN_TX_H__

#include "dogecoin.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#include <dogecoin/chain.h>
#include <dogecoin/cstr.h>
#include <dogecoin/hash.h>
#include <dogecoin/script.h>
#include <dogecoin/vector.h>


typedef struct dogecoin_script_ {
    int* data;
    size_t limit;   // Total size of the vector
    size_t current; //Number of vectors in it at present
} dogecoin_script;

typedef struct dogecoin_tx_outpoint_ {
    uint256 hash;
    uint32_t n;
} dogecoin_tx_outpoint;

typedef struct dogecoin_tx_in_ {
    dogecoin_tx_outpoint prevout;
    cstring* script_sig;
    uint32_t sequence;
} dogecoin_tx_in;

typedef struct dogecoin_tx_out_ {
    int64_t value;
    cstring* script_pubkey;
} dogecoin_tx_out;

typedef struct dogecoin_tx_ {
    int32_t version;
    vector* vin;
    vector* vout;
    uint32_t locktime;
} dogecoin_tx;


//!create a new tx input
LIBDOGECOIN_API dogecoin_tx_in* dogecoin_tx_in_new();
LIBDOGECOIN_API void dogecoin_tx_in_free(dogecoin_tx_in* tx_in);
LIBDOGECOIN_API void dogecoin_tx_in_copy(dogecoin_tx_in* dest, const dogecoin_tx_in* src);

//!create a new tx output
LIBDOGECOIN_API dogecoin_tx_out* dogecoin_tx_out_new();
LIBDOGECOIN_API void dogecoin_tx_out_free(dogecoin_tx_out* tx_out);
LIBDOGECOIN_API void dogecoin_tx_out_copy(dogecoin_tx_out* dest, const dogecoin_tx_out* src);

//!create a new tx input
LIBDOGECOIN_API dogecoin_tx* dogecoin_tx_new();
LIBDOGECOIN_API void dogecoin_tx_free(dogecoin_tx* tx);
LIBDOGECOIN_API void dogecoin_tx_copy(dogecoin_tx* dest, const dogecoin_tx* src);

//!deserialize/parse a p2p serialized DOGECOIN transaction
LIBDOGECOIN_API int dogecoin_tx_deserialize(const unsigned char* tx_serialized, size_t inlen, dogecoin_tx* tx);

//!serialize a lbc DOGECOIN data structure into a p2p serialized buffer
LIBDOGECOIN_API void dogecoin_tx_serialize(cstring* s, const dogecoin_tx* tx);

LIBDOGECOIN_API void dogecoin_tx_hash(const dogecoin_tx* tx, uint8_t* hashout);

LIBDOGECOIN_API dogecoin_bool dogecoin_tx_sighash(const dogecoin_tx* tx_to, const cstring* fromPubKey, unsigned int in_num, int hashtype, uint8_t* hash);

LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_address_out(dogecoin_tx* tx, const dogecoin_chain* chain, int64_t amount, const char* address);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_p2sh_hash160_out(dogecoin_tx* tx, int64_t amount, uint8_t* hash160);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_p2pkh_hash160_out(dogecoin_tx* tx, int64_t amount, uint8_t* hash160);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_p2pkh_out(dogecoin_tx* tx, int64_t amount, const dogecoin_pubkey* pubkey);

LIBDOGECOIN_API dogecoin_bool dogecoin_tx_outpoint_is_null(dogecoin_tx_outpoint* tx);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_is_coinbase(dogecoin_tx* tx);
#ifdef __cplusplus
}
#endif

#endif //__LIBDOGECOIN_TX_H__
