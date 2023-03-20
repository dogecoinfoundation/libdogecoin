/*

 The MIT License (MIT)

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

#ifndef __LIBDOGECOIN_ECKEY_H__
#define __LIBDOGECOIN_ECKEY_H__

#include <dogecoin/dogecoin.h>
#include <dogecoin/key.h>
#include <dogecoin/uthash.h>

LIBDOGECOIN_BEGIN_DECL

/* hashmap functions */
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
LIBDOGECOIN_API eckey* new_eckey(dogecoin_bool is_testnet);

LIBDOGECOIN_API eckey* new_eckey_from_privkey(char* key);

// adds eckey structure to hash table
LIBDOGECOIN_API void add_eckey(eckey *key);

// find eckey from the hash table
LIBDOGECOIN_API eckey* find_eckey(int idx);

// remove eckey from the hash table
LIBDOGECOIN_API void remove_eckey(eckey *key);

LIBDOGECOIN_API void dogecoin_key_free(eckey* eckey);

// instantiates and adds key to the hash table
LIBDOGECOIN_API int start_key(dogecoin_bool is_testnet);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_ECKEY_H__
