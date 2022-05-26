/*
 ---------------------------------------------------------------------------
 Copyright (c) 1998-2008, Brian Gladman, Worcester, UK. All rights reserved.

 LICENSE TERMS

 The redistribution and use of this software (with or without changes)
 is allowed without the payment of fees or royalties provided that:

  1. source code distributions include the above copyright notice, this
     list of conditions and the following disclaimer;

  2. binary distributions include the above copyright notice, this list
     of conditions and the following disclaimer in their documentation;

  3. the name of the copyright holder is not used to endorse products
     built using this software without specific written permission.

 DISCLAIMER

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.

 ---------------------------------------------------------------------------
 Issue 09/09/2006

 This is an AES implementation that uses only 8-bit byte operations on the
 cipher state.
 */

#ifndef __LIBDOGECOIN_CRYPTO_AES_H__
#define __LIBDOGECOIN_CRYPTO_AES_H__

#include <dogecoin/dogecoin.h>

#define AES_BLOCK_SIZE 16

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API int aes256_cbc_encrypt(const unsigned char aes_key[32], const unsigned char iv[AES_BLOCK_SIZE], const unsigned char* data, int size, int pad, unsigned char* out);
LIBDOGECOIN_API int aes256_cbc_decrypt(const unsigned char aes_key[32], const unsigned char iv[AES_BLOCK_SIZE], const unsigned char* data, int size, int pad, unsigned char* out);

LIBDOGECOIN_END_DECL

#endif //__LIBDOGECOIN_CRYPTO_AES_H__
