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

#ifndef __LIBDOGECOIN_UTILS_H__
#define __LIBDOGECOIN_UTILS_H__

#include <dogecoin/cstr.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/mem.h>

#define TO_UINT8_HEX_BUF_LEN 2048
#define VARINT_LEN 20

#define strlens(s) (s == NULL ? 0 : strlen(s))

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API void utils_clear_buffers(void);
LIBDOGECOIN_API void utils_hex_to_bin(const char* str, unsigned char* out, int inLen, int* outLen);
LIBDOGECOIN_API void utils_bin_to_hex(unsigned char* bin_in, size_t inlen, char* hex_out);
LIBDOGECOIN_API uint8_t* utils_hex_to_uint8(const char* str);
LIBDOGECOIN_API char* utils_uint8_to_hex(const uint8_t* bin, size_t l);
LIBDOGECOIN_API void utils_reverse_hex(char* h, int len);
LIBDOGECOIN_API void utils_uint256_sethex(char* psz, uint8_t* out);
LIBDOGECOIN_API void* safe_malloc(size_t size);
LIBDOGECOIN_API void dogecoin_cheap_random_bytes(uint8_t* buf, uint32_t len);
LIBDOGECOIN_API void dogecoin_get_default_datadir(cstring* path_out);
LIBDOGECOIN_API void dogecoin_file_commit(FILE* file);
LIBDOGECOIN_API void print_image(FILE *fptr);
LIBDOGECOIN_API void print_header(char *filepath);
LIBDOGECOIN_API long double koinu_to_coins(uint64_t koinu);
LIBDOGECOIN_API unsigned long long coins_to_koinu(long double coins);
LIBDOGECOIN_API uint64_t coins_to_koinu_str(char* coins);
LIBDOGECOIN_API uint8_t* bytearray_concatenate(uint8_t* input1, uint8_t* input2);
LIBDOGECOIN_API void print_bits(size_t const size, void const* ptr);
LIBDOGECOIN_API void prepend(char* s, const char* t);
LIBDOGECOIN_API void append(char* s, char* t);
LIBDOGECOIN_API void text_to_hex(char* in, char* out);
LIBDOGECOIN_API const char* get_build();

/* support substitute for GNU only tdestroy */
/* let's hope the node struct is always compatible */

struct dogecoin_btree_node {
    void* key;
    struct dogecoin_btree_node* left;
    struct dogecoin_btree_node* right;
};

static inline void dogecoin_btree_tdestroy(void* root, void (*freekey)(void*))
{
    struct dogecoin_btree_node* r = (struct dogecoin_btree_node*)root;
    if (r == 0) {
        return;
    }
    dogecoin_btree_tdestroy(r->left, freekey);
    dogecoin_btree_tdestroy(r->right, freekey);
    if (freekey) {
        freekey(r->key);
    }
    dogecoin_free(r);
}

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_UTILS_H__
