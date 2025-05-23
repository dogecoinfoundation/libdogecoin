/*

 The MIT License (MIT)

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

#ifndef __LIBDOGECOIN_VECTOR_H__
#define __LIBDOGECOIN_VECTOR_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

typedef struct vector_t {
    void** data;  /* array of pointers */
    size_t len;   /* array element count */
    size_t alloc; /* allocated array elements */

    void (*elem_free_f)(void*);
} vector_t;

#define vector_idx(vec, idx) vec->data[idx]

LIBDOGECOIN_API vector_t* vector_new(size_t res, void (*free_f)(void*));
LIBDOGECOIN_API void vector_free(vector_t* vec, dogecoin_bool free_array);

LIBDOGECOIN_API dogecoin_bool vector_add(vector_t* vec, void* data);
LIBDOGECOIN_API dogecoin_bool vector_remove(vector_t* vec, void* data);
LIBDOGECOIN_API void vector_remove_idx(vector_t* vec, size_t idx);
LIBDOGECOIN_API void vector_remove_range(vector_t* vec, size_t idx, size_t len);
LIBDOGECOIN_API dogecoin_bool vector_resize(vector_t* vec, size_t newsz);

LIBDOGECOIN_API ssize_t vector_find(vector_t* vec, void* data);

/* serialization functions */
LIBDOGECOIN_API dogecoin_bool serializeVector(vector_t* vec, char* out, size_t outlen, size_t* written);
LIBDOGECOIN_API dogecoin_bool deserializeVector(vector_t* vec, const char* in, size_t inlen, size_t* read);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_VECTOR_H__
