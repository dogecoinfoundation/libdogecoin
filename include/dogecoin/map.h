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

#ifndef __LIBDOGECOIN_MAP_H__
#define __LIBDOGECOIN_MAP_H__

#include <inttypes.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#include <dogecoin/dogecoin.h>
#include <dogecoin/uthash.h>

LIBDOGECOIN_BEGIN_DECL

#define typename(x) _Generic((x),        /* Get the name of a type */             \
                                                                                  \
        _Bool: "_Bool",                  unsigned char: "unsigned char",          \
         char: "char",                     signed char: "signed char",            \
    short int: "short int",         unsigned short int: "unsigned short int",     \
          int: "int",                     unsigned int: "unsigned int",           \
     long int: "long int",           unsigned long int: "unsigned long int",      \
long long int: "long long int", unsigned long long int: "unsigned long long int", \
        float: "float",                         double: "double",                 \
  long double: "long double",                   char *: "char *",                 \
       void *: "pointer to void",                int *: "pointer to int",         \
       uint8_t *: "uint8_t *",                 default: "other")

/* hash functions */
typedef struct checks {
    dogecoin_bool negative;
    dogecoin_bool overflow;
} checks;

typedef union base_uint {
    uint8_t u8[32];
    uint32_t u32[8];
} base_uint;

typedef struct hash {
    int index;
    base_uint data;
    struct checks checks;
    UT_hash_handle hh;
} hash;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static hash *hashes = NULL;
#pragma GCC diagnostic pop

// instantiates a new hash
LIBDOGECOIN_API hash* new_hash();
LIBDOGECOIN_API int start_hash();
LIBDOGECOIN_API void add_hash(hash *hash);
LIBDOGECOIN_API hash* zero_hash(int index);
LIBDOGECOIN_API void set_hash(int index, uint8_t* data, char* typename);
LIBDOGECOIN_API hash* find_hash(int index);
LIBDOGECOIN_API char* get_hash_by_index(int index);
LIBDOGECOIN_API void remove_hash(hash *hash);
LIBDOGECOIN_API void remove_all_hashes();

/* map functions */
typedef struct map {
    int index;
    int count;
    hash *hashes;
    UT_hash_handle hh;
} map;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static map *maps = NULL;
#pragma GCC diagnostic pop

// instantiates a new map
LIBDOGECOIN_API map* new_map();
LIBDOGECOIN_API int start_map();
LIBDOGECOIN_API void add_map(map *map_external);
LIBDOGECOIN_API map* find_map(int index);
LIBDOGECOIN_API void remove_map(map *map);
LIBDOGECOIN_API void remove_all_maps();

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_MAP_H__
