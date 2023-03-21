/*

 The MIT License (MIT)

 Copyright (c) 2023 bluezr, edtubbs, michilumin
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

#ifndef __LIBDOGECOIN_CONSTANTS_H__
#define __LIBDOGECOIN_CONSTANTS_H__

#include "dogecoin.h"

LIBDOGECOIN_BEGIN_DECL

#define MAX_INT32_STRINGLEN 12
#define HD_MASTERKEY_STRINGLEN 112
#define P2PKH_ADDR_STRINGLEN 35
#define WIF_UNCOMPRESSED_PRIVKEY_STRINGLEN 53
#define DERIVED_PATH_STRINGLEN 33
/* NOTE: Path string composed of m/44/3/+32bits_Account+/+bool_ischange+/+32bits_Address + string terminator; for a total of 33 bytes. */

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_CONSTANTS_H__
