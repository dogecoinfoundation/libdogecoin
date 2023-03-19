/*
 The MIT License (MIT)
 
 Copyright (c) 2023 bluezr, edtubbs
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

#ifndef __LIBDOGECOIN_SIGN_H__
#define __LIBDOGECOIN_SIGN_H__

#include <dogecoin/dogecoin.h>
#include <dogecoin/eckey.h>

LIBDOGECOIN_BEGIN_DECL

/* double sha256 hash a message */
LIBDOGECOIN_API void hash_message(char* msg, uint256 message_bytes);

/* sign a message with a private key */
LIBDOGECOIN_API char* sign_message(char* privkey, char* msg);

/* verify a message with a address */
LIBDOGECOIN_API int verify_message(char* sig, char* msg, char* address);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_SIGN_H__
