/*

 The MIT License (MIT)

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

#ifndef _LIBDOGECOIN_H_
#define _LIBDOGECOIN_H_

#include <stdio.h>
#include <limits.h>
#include <stdint.h>

typedef uint8_t dogecoin_bool; //!serialize, c/c++ save bool

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LIBDOGECOIN_API
#if defined(_WIN32)
#ifdef LIBDOGECOIN_BUILD
#define LIBDOGECOIN_API __declspec(dllexport)
#else
#define LIBDOGECOIN_API
#endif
#elif defined(__GNUC__) && defined(LIBDOGECOIN_BUILD)
#define LIBDOGECOIN_API __attribute__((visibility("default")))
#else
#define LIBDOGECOIN_API
#endif
#endif

#ifdef __cplusplus
}
#endif

#define DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH 65
#define DOGECOIN_ECKEY_COMPRESSED_LENGTH 33
#define DOGECOIN_ECKEY_PKEY_LENGTH 32
#define DOGECOIN_ECKEY_PKEY_LENGTH 32
#define DOGECOIN_HASH_LENGTH 32

#endif //_LIBDOGECOIN_H_
