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

#ifndef __LIBDOGECOIN_CRYPTO_ECC_H__
#define __LIBDOGECOIN_CRYPTO_ECC_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

//!init static ecc context
LIBDOGECOIN_API void dogecoin_ecc_start(void);

//!destroys the static ecc context
LIBDOGECOIN_API void dogecoin_ecc_stop(void);

//!get public key from given private key
LIBDOGECOIN_API void dogecoin_ecc_get_pubkey(const uint8_t* private_key, uint8_t* public_key, size_t* public_key_len, dogecoin_bool compressed);

//!ec mul tweak on given private key
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_private_key_tweak_add(uint8_t* private_key, const uint8_t* tweak);

//!ec mul tweak on given public key
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_public_key_tweak_add(uint8_t* public_key_inout, const uint8_t* tweak);

//!verifies a given 32byte key
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_verify_privatekey(const uint8_t* private_key);

//!verifies a given public key (compressed[33] or uncompressed[65] bytes)
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_verify_pubkey(const uint8_t* public_key, dogecoin_bool compressed);

//!create a DER signature (72-74 bytes) with private key
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_sign(const uint8_t* private_key, const uint256 hash, unsigned char* sigder, size_t* outlen);

//!create a compact (64bytes) signature with private key
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_sign_compact(const uint8_t* private_key, const uint256 hash, unsigned char* sigcomp, size_t* outlen);

//!create a compact recoverable (65bytes) signature with private key
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_sign_compact_recoverable(const uint8_t* private_key, const uint256 hash, unsigned char* sigcomprec, size_t* outlen, int* recid);

//!recover a pubkey from a signature and recid
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_recover_pubkey(const unsigned char* sigrec, const uint256 hash, const int recid, uint8_t* public_key, size_t* outlen);

//!converts (and normalized) a compact signature to DER
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_compact_to_der_normalized(unsigned char* sigcomp_in, unsigned char* sigder_out, size_t* sigder_len_out);

//!convert DER signature to compact
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_der_to_compact(unsigned char* sigder_in, size_t sigder_len, unsigned char* sigcomp_out);

//!verify DER signature with public key
LIBDOGECOIN_API dogecoin_bool dogecoin_ecc_verify_sig(const uint8_t* public_key, dogecoin_bool compressed, const uint256 hash, unsigned char* sigder, size_t siglen);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_CRYPTO_ECC_H__
