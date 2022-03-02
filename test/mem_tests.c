/**********************************************************************
* Copyright (c) 2015 Jonas Schnelli                                  *
* Distributed under the MIT software license, see the accompanying   *
* file COPYING or http://www.opensource.org/licenses/mit-license.php.*
**********************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <test/utest.h>

#include <dogecoin/mem.h>

void* test_memory_malloc(size_t size)
{
    (void)(size);
    return NULL;
}

void* test_memory_calloc(size_t count, size_t size)
{
    (void)(count);
    (void)(size);
    return NULL;
}

void* test_memory_realloc(void* ptr, size_t size)
{
    (void)(ptr);
    (void)(size);
    return NULL;
}

void test_memory_free(void* ptr)
{
    free(ptr);
}

void test_memory()
{
    dogecoin_mem_mapper mymapper = {test_memory_malloc, test_memory_calloc, test_memory_realloc, test_memory_free};
    dogecoin_mem_set_mapper(mymapper);
    void* ptr = dogecoin_malloc(32);
    u_assert_int_eq((ptr == NULL), 1);
    ptr = dogecoin_calloc(32, 1);
    u_assert_int_eq((ptr == NULL), 1);
    void* buf = malloc(100);
    void* obuf = buf;
    ptr = dogecoin_realloc(buf, 1000);
    u_assert_int_eq((ptr == NULL), 1);
    free(obuf);
    // switch back to the default memory callback mapper
    dogecoin_mem_set_mapper_default();
}
