/*

 The MIT License (MIT)

 Copyright (c) 2016 libbtc developers
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

#include <dogecoin/memory.h>

#include <stdio.h>
#include <stdlib.h>

void* dogecoin_malloc_internal(size_t size);
void* dogecoin_calloc_internal(size_t count, size_t size);
void* dogecoin_realloc_internal(void *ptr, size_t size);
void dogecoin_free_internal(void* ptr);

static const dogecoin_mem_mapper default_mem_mapper = {dogecoin_malloc_internal, dogecoin_calloc_internal, dogecoin_realloc_internal, dogecoin_free_internal};
static dogecoin_mem_mapper current_mem_mapper = {dogecoin_malloc_internal, dogecoin_calloc_internal, dogecoin_realloc_internal, dogecoin_free_internal};

void dogecoin_mem_set_mapper_default()
{
    current_mem_mapper = default_mem_mapper;
}

void dogecoin_mem_set_mapper(const dogecoin_mem_mapper mapper)
{
    current_mem_mapper = mapper;
}

void* dogecoin_malloc(size_t size)
{
    return current_mem_mapper.dogecoin_malloc(size);
}

void* dogecoin_calloc(size_t count, size_t size)
{
    return current_mem_mapper.dogecoin_calloc(count, size);
}

void* dogecoin_realloc(void *ptr, size_t size)
{
    return current_mem_mapper.dogecoin_realloc(ptr, size);
}

void dogecoin_free(void* ptr)
{
    current_mem_mapper.dogecoin_free(ptr);
}

void* dogecoin_malloc_internal(size_t size)
{
    void* result;

    if ((result = malloc(size))) { /* assignment intentional */
        return (result);
    } else {
        printf("memory overflow: malloc failed in dogecoin_malloc.");
        printf("  Exiting Program.\n");
        exit(-1);
        return (0);
    }
}

void* dogecoin_calloc_internal(size_t count, size_t size)
{
    void* result;

    if ((result = calloc(count, size))) { /* assignment intentional */
        return (result);
    } else {
        printf("memory overflow: calloc failed in dogecoin_calloc.");
        printf("  Exiting Program.\n");
        exit(-1);
        return (0);
    }
}

void* dogecoin_realloc_internal(void *ptr, size_t size)
{
    void* result;

    if ((result = realloc(ptr, size))) { /* assignment intentional */
        return (result);
    } else {
        printf("memory overflow: realloc failed in dogecoin_realloc.");
        printf("  Exiting Program.\n");
        exit(-1);
        return (0);
    }
}

void dogecoin_free_internal(void* ptr)
{
    free(ptr);
}

#ifdef HAVE_MEMSET_S
volatile void *dogecoin_mem_zero(volatile void *dst, size_t len)
{
    memset_s(dst, len, 0, len);
}
#else
volatile void *dogecoin_mem_zero(volatile void *dst, size_t len)
{
    volatile char *buf;
    for (buf = (volatile char *)dst;  len;  buf[--len] = 0);
    return dst;
}
#endif
