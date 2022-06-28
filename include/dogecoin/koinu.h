/*

 The MIT License (MIT)

 Copyright (c) 2022 bluezr & jaxlotl
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

#ifndef __LIBDOGECOIN_KOINU_H__
#define __LIBDOGECOIN_KOINU_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API long double koinu_to_coins(uint64_t koinu);
LIBDOGECOIN_API int koinu_to_coins_str(uint64_t koinu, char* str);
LIBDOGECOIN_API uint64_t coins_to_koinu_str(char* coins);
LIBDOGECOIN_API unsigned long long coins_to_koinu(long double coins);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_KOINU_H__
