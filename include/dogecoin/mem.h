/*

 The MIT License (MIT)

 Copyright (c) 2017 Jonas Schnelli
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

#ifndef __LIBDOGECOIN_MEM_H__
#define __LIBDOGECOIN_MEM_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

typedef int errno_t;
typedef size_t rsize_t;
#define RSIZE_MAX (SIZE_MAX >> 1)
/**
 * It's a struct that has pointers to functions that do the actual memory management.
 */
typedef struct dogecoin_mem_mapper {
    void* (*dogecoin_malloc)(size_t size);
    void* (*dogecoin_calloc)(size_t count, size_t size);
    void* (*dogecoin_realloc)(void* ptr, size_t size);
    void (*dogecoin_free)(void* ptr);
} dogecoin_mem_mapper;

// sets a custom memory mapper
// this function is _not_ thread safe and must be called before anything else
LIBDOGECOIN_API void dogecoin_mem_set_mapper(const dogecoin_mem_mapper mapper);
LIBDOGECOIN_API void dogecoin_mem_set_mapper_default();

LIBDOGECOIN_API void* dogecoin_malloc(size_t size);
LIBDOGECOIN_API void* dogecoin_calloc(size_t count, size_t size);
LIBDOGECOIN_API void* dogecoin_realloc(void* ptr, size_t size);
LIBDOGECOIN_API void dogecoin_free(void* ptr);

LIBDOGECOIN_API errno_t memset_safe(volatile void *v, rsize_t smax, int c, rsize_t n);
LIBDOGECOIN_API void* memcpy_safe(void* destination, const void* source, unsigned int count);
LIBDOGECOIN_API volatile void* dogecoin_mem_zero(volatile void* dst, size_t len);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_MEM_H__
