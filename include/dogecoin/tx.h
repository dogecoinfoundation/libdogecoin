/*

 The MIT License (MIT)

 Copyright (c) 2015 Douglas J. Bakkum
 Copyright (c) 2015 Jonas Schnelli
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

#ifndef __LIBDOGECOIN_TX_H__
#define __LIBDOGECOIN_TX_H__

#include <dogecoin/buffer.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/cstr.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/hash.h>
#include <dogecoin/script.h>
#include <dogecoin/vector.h>

LIBDOGECOIN_BEGIN_DECL

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

//!p2pkh utilities
LIBDOGECOIN_API int dogecoin_tx_out_pubkey_hash_to_p2pkh_address(dogecoin_tx_out* txout, char* p2pkh, int is_mainnet);
LIBDOGECOIN_API dogecoin_bool dogecoin_pubkey_hash_to_p2pkh_address(char* script_pubkey_hex, size_t script_pubkey_hex_length, char* p2pkh, const dogecoin_chainparams* chain);
LIBDOGECOIN_API dogecoin_bool dogecoin_p2pkh_address_to_pubkey_hash(char* p2pkh, char* scripthash);
LIBDOGECOIN_API char* dogecoin_address_to_pubkey_hash(char* p2pkh);
LIBDOGECOIN_API char* dogecoin_private_key_wif_to_pubkey_hash(char* private_key_wif);

//!create a new tx input
LIBDOGECOIN_API dogecoin_tx_in* dogecoin_tx_in_new();
LIBDOGECOIN_API void dogecoin_tx_in_free(dogecoin_tx_in* tx_in);
LIBDOGECOIN_API void dogecoin_tx_in_copy(dogecoin_tx_in* dest, const dogecoin_tx_in* src);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_in_deserialize(dogecoin_tx_in* tx_in, struct const_buffer* buf);
LIBDOGECOIN_API void dogecoin_tx_in_serialize(cstring* s, const dogecoin_tx_in* tx_in);

//!create a new tx output
LIBDOGECOIN_API dogecoin_tx_out* dogecoin_tx_out_new();
LIBDOGECOIN_API void dogecoin_tx_out_free(dogecoin_tx_out* tx_out);
LIBDOGECOIN_API void dogecoin_tx_out_copy(dogecoin_tx_out* dest, const dogecoin_tx_out* src);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_out_deserialize(dogecoin_tx_out* tx_out, struct const_buffer* buf);
LIBDOGECOIN_API void dogecoin_tx_out_serialize(cstring* s, const dogecoin_tx_out* tx_out);

//!create a new tx input
LIBDOGECOIN_API dogecoin_tx* dogecoin_tx_new();
LIBDOGECOIN_API void dogecoin_tx_free(dogecoin_tx* tx);
LIBDOGECOIN_API void dogecoin_tx_copy(dogecoin_tx* dest, const dogecoin_tx* src);

//!deserialize/parse a p2p serialized dogecoin transaction
LIBDOGECOIN_API int dogecoin_tx_deserialize(const unsigned char* tx_serialized, size_t inlen, dogecoin_tx* tx, size_t* consumed_length);

//!serialize a dogecoin data structure into a p2p serialized buffer
LIBDOGECOIN_API void dogecoin_tx_serialize(cstring* s, const dogecoin_tx* tx);

LIBDOGECOIN_API void dogecoin_tx_hash(const dogecoin_tx* tx, uint256 hashout);

LIBDOGECOIN_API dogecoin_bool dogecoin_tx_sighash(const dogecoin_tx* tx_to, const cstring* fromPubKey, size_t in_num, int hashtype, uint256 hash);

LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_address_out(dogecoin_tx* tx, const dogecoin_chainparams* chain, int64_t amount, const char* address);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_p2sh_hash160_out(dogecoin_tx* tx, int64_t amount, uint160 hash160);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_p2pkh_hash160_out(dogecoin_tx* tx, int64_t amount, uint160 hash160);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_p2pkh_out(dogecoin_tx* tx, int64_t amount, const dogecoin_pubkey* pubkey);

LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_data_out(dogecoin_tx* tx, const int64_t amount, const uint8_t* data, const size_t datalen);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_add_puzzle_out(dogecoin_tx* tx, const int64_t amount, const uint8_t* puzzle, const size_t puzzlelen);

LIBDOGECOIN_API dogecoin_bool dogecoin_tx_outpoint_is_null(dogecoin_tx_outpoint* tx);
LIBDOGECOIN_API dogecoin_bool dogecoin_tx_is_coinbase(dogecoin_tx* tx);

enum dogecoin_tx_sign_result {
    DOGECOIN_SIGN_UNKNOWN = 0,
    DOGECOIN_SIGN_INVALID_KEY = -2,
    DOGECOIN_SIGN_NO_KEY_MATCH = -3, // if the key found in the script doesn't match the given key, will sign anyways
    DOGECOIN_SIGN_SIGHASH_FAILED = -4,
    DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE = -5,
    DOGECOIN_SIGN_INVALID_TX_OR_SCRIPT = -6,
    DOGECOIN_SIGN_INPUTINDEX_OUT_OF_RANGE = -7,
    DOGECOIN_SIGN_OK = 1,
};
const char* dogecoin_tx_sign_result_to_str(const enum dogecoin_tx_sign_result result);
enum dogecoin_tx_sign_result dogecoin_tx_sign_input(dogecoin_tx* tx_in_out, const cstring* script, const dogecoin_key* privkey, size_t inputindex, int sighashtype, uint8_t* sigcompact_out, uint8_t* sigder_out, size_t* sigder_len);

//!wrapper to get the address from a pubkey hash
LIBDOGECOIN_API int getAddrFromPubkeyHash(const char pubkey_hash[PUBKEYHASHLEN], const dogecoin_bool is_testnet, char p2pkh_address[P2PKHLEN]);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_TX_H__
