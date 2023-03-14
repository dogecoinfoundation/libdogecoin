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

#ifndef __LIBDOGECOIN_SIGN_H__
#define __LIBDOGECOIN_SIGN_H__

#include <dogecoin/dogecoin.h>
#include <dogecoin/uthash.h>

LIBDOGECOIN_BEGIN_DECL

typedef struct signature {
    int idx;
    char* content;
    char address[35];
    int recid;
    UT_hash_handle hh;
} signature;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static signature *signatures = NULL;
#pragma GCC diagnostic pop

/* instantiates a new signature */
LIBDOGECOIN_API signature* new_signature();

/* adds a signature structure to hash table */
LIBDOGECOIN_API void add_signature(signature *sig);

/* finds a signature from the hash table */
LIBDOGECOIN_API signature* find_signature(int idx);

/* remove the signature from the hash table */
LIBDOGECOIN_API void remove_signature(signature *sig);

/* instantiates and adds signature to the hash table */
LIBDOGECOIN_API int start_signature();

/* frees signature from memory */
LIBDOGECOIN_API void free_signature(signature* sig);

/* sign a message with a private key */
LIBDOGECOIN_API char* signmsgwithprivatekey(char* privkey, char* msg);

/* sign message with eckey structure */
LIBDOGECOIN_API signature* signmsgwitheckey(eckey* key, char* msg);

/* verify a message with a address */
LIBDOGECOIN_API char* verifymessage(char* sig, char* msg);

/* verify a message with a signature structure */
LIBDOGECOIN_API char* verifymessagewithsig(signature* sig, char* msg);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_SIGN_H__
