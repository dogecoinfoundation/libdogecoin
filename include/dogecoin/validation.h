/*

 The MIT License (MIT)

 Copyright (c) 2009-2010 Satoshi Nakamoto
 Copyright (c) 2009-2016 The Bitcoin Core developers
 Copyright (c) 2022 The Dogecoin Core developers
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

#ifndef __LIBDOGECOIN_VALIDATION_H__
#define __LIBDOGECOIN_VALIDATION_H__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <dogecoin/block.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/pow.h>
#include <dogecoin/utils.h>

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API uint32_t get_chainid(uint32_t version);
LIBDOGECOIN_API dogecoin_bool is_auxpow(uint32_t version);
LIBDOGECOIN_API dogecoin_bool is_legacy(uint32_t version);
LIBDOGECOIN_API dogecoin_bool check_auxpow(dogecoin_auxpow_block* block, dogecoin_chainparams* params, uint256_t* chainwork);
LIBDOGECOIN_API dogecoin_bool dogecoin_block_header_scrypt_hash(cstring* s, uint256_t* hash);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_VALIDATION_H__
