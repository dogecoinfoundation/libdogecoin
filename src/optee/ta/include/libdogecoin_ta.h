/**
 * Copyright (c) 2024 edtubbs
 * Copyright (c) 2024 The Dogecoin Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Copyright (c) 2017, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __LIBDOGECOIN_TA_H__
#define __LIBDOGECOIN_TA_H__

/* UUID of the trusted application */
/* 62d95dc0-7fc2-4cb3-a7f3-c13ae4e633c4 */
#define TA_LIBDOGECOIN_UUID \
		{ 0x62d95dc0, 0x7fc2, 0x4cb3, \
            { 0xa7, 0xf3, 0xc1, 0x3a, 0xe4, 0xe6, 0x33, 0xc4} }
/*
 * TA_LIBDOGECOIN_CMD_READ_RAW - Create and fill a secure storage file
 * param[0] (memref) ID used the identify the persistent object
 * param[1] (memref) Raw data dumped from the persistent object
 * param[2] unused
 * param[3] unused
 */
#define TA_LIBDOGECOIN_CMD_READ_RAW		0

/*
 * TA_LIBDOGECOIN_CMD_WRITE_RAW - Create and fill a secure storage file
 * param[0] (memref) ID used the identify the persistent object
 * param[1] (memref) Raw data to be written in the persistent object
 * param[2] unused
 * param[3] unused
 */
#define TA_LIBDOGECOIN_CMD_WRITE_RAW		1

/*
 * TA_LIBDOGECOIN_CMD_DELETE - Delete a persistent object
 * param[0] (memref) ID used the identify the persistent object
 * param[1] unused
 * param[2] unused
 * param[3] unused
 */
#define TA_LIBDOGECOIN_CMD_DELETE		2

/*
 * TA_LIBDOGECOIN_CMD_GENERATE_SEED - Generate a seed
 * param[0] (memref) Seed
 * param[1] unused
 * param[2] unused
 * param[3] unused
 */
#define TA_LIBDOGECOIN_CMD_GENERATE_SEED              3

/*
 * TA_LIBDOGECOIN_CMD_GENERATE_MNEMONIC - Generate a mnemonic
 * param[0] (memref) Mnemonic
 * param[1] (memref) Managed credentials (shared secret, password, flags)
 * param[2] (memref) Mnemonic input
 * param[3] (memref) Entropy size
 */
#define TA_LIBDOGECOIN_CMD_GENERATE_MNEMONIC          4

/*
 * TA_LIBDOGECOIN_CMD_GENERATE_MASTERKEY - Generate a master key
 * param[0] (memref) Master key
 * param[1] unused
 * param[2] unused
 * param[3] unused
 */
#define TA_LIBDOGECOIN_CMD_GENERATE_MASTERKEY         5

/*
 * TA_LIBDOGECOIN_CMD_SIGN_MESSAGE - Sign a message
 * param[0] (memref) Message to be signed
 * param[1] (memref) Signature
 * param[2] (memref) Key path (account, change level, address index), password (optional)
 * param[3] (value) Auth token, password size
 */
#define TA_LIBDOGECOIN_CMD_SIGN_MESSAGE               6

/*
 * TA_LIBDOGECOIN_CMD_SIGN_TRANSACTION - Sign a transaction
 * param[0] (memref) Raw transaction to be signed
 * param[1] (memref) Signed transaction
 * param[2] (memref) Key path (account, change level, address index), password (optional)
 * param[3] (value) Auth token, password size
 */
#define TA_LIBDOGECOIN_CMD_SIGN_TRANSACTION           7

/*
 * TA_LIBDOGECOIN_CMD_GENERATE_EXTENDED_PUBLIC_KEY - Get an extended public key
 * param[0] (memref) Extended public key
 * param[1] (memref) Key path (account, change level)
 * param[2] (value) Auth token
 * param[3] (memref) Password (optional)
 */
#define TA_LIBDOGECOIN_CMD_GENERATE_EXTENDED_PUBLIC_KEY    8

/*
 * TA_LIBDOGECOIN_CMD_GENERATE_ADDRESS - Get an address
 * param[0] (memref) Address
 * param[1] (memref) Key path (account, change level, address index)
 * param[2] (value) Auth token
 * param[3] (memref) Password (optional)
 */
#define TA_LIBDOGECOIN_CMD_GENERATE_ADDRESS                9

/*
 * TA_LIBDOGECOIN_CMD_DELEGATE_KEY - Delegate a key
 * param[0] (memref) Key
 * param[1] (memref) Account number (part of the key path)
 * param[2] (value) Auth token
 * param[3] (memref) Delegate password, password (optional)
 */
#define TA_LIBDOGECOIN_CMD_DELEGATE_KEY                    10

/*
 * TA_LIBDOGECOIN_CMD_EXPORT_DELEGATED_KEY - Export a delegated key
 * param[0] (memref) Key
 * param[1] (memref) Account number (part of the key path)
 * param[2] (unused)
 * param[3] (memref) Delegate password
 */
#define TA_LIBDOGECOIN_CMD_EXPORT_DELEGATED_KEY            11

#endif /* __LIBDOGECOIN_TA_H__ */
