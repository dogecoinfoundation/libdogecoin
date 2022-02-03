/*
 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
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

#ifndef __LIBDOGECOIN_AES256_CBC_H__
#define __LIBDOGECOIN_AES256_CBC_H__

#include "dogecoin.h"

#define AES_BLOCK_SIZE 16

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API int aes256_cbc_encrypt(const unsigned char aes_key[32], const unsigned char iv[AES_BLOCK_SIZE], const unsigned char* data, int size, int pad, unsigned char* out);
LIBDOGECOIN_API int aes256_cbc_decrypt(const unsigned char aes_key[32], const unsigned char iv[AES_BLOCK_SIZE], const unsigned char* data, int size, int pad, unsigned char* out);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_AES256_CBC_H__
