/*

 The MIT License (MIT)

 Copyright (c) 2009-2010 Satoshi Nakamoto
 Copyright (c) 2009-2014 The Bitcoin developers
 Copyright (c) 2014-2016 Daniel Kraft
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

#ifndef __LIBDOGECOIN_AUXPOW__
#define __LIBDOGECOIN_AUXPOW__

#include <dogecoin/block.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/hash.h>
#include <dogecoin/vector.h>

LIBDOGECOIN_BEGIN_DECL

#define BLOCK_VERSION_AUXPOW_BIT 0x100

/** Header for merge-mining data in the coinbase.  */
static const unsigned char pch_merged_mining_header[] = { 0xfa, 0xbe, 'm', 'm' };

int get_expected_index(uint32_t nNonce, int nChainId, unsigned h);
uint256* check_merkle_branch(uint256* hash, const vector* merkle_branch, int index);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_AUXPOW__
