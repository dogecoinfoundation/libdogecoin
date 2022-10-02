/*
 The MIT License (MIT)
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

#include <string.h>

#include <dogecoin/aes.h>
#include <dogecoin/ctaes.h>
#include <dogecoin/mem.h>

size_t aes256_cbc_encrypt(const unsigned char aes_key[32], const unsigned char iv[AES_BLOCK_SIZE], const unsigned char* data, size_t size, int pad, unsigned char* out)
{
    size_t written = 0;
    int padsize = size % AES_BLOCK_SIZE;
    unsigned char mixed[AES_BLOCK_SIZE];

    if (!data || !size || !out)
        return 0;

    if (!pad && padsize != 0)
        return 0;

    AES256_ctx aes_ctx;
    // Set cipher key
    AES256_init(&aes_ctx, aes_key);

    memcpy_safe(mixed, iv, AES_BLOCK_SIZE);

    // Write all but the last block
    while (written + AES_BLOCK_SIZE <= size) {
        for (int i = 0; i != AES_BLOCK_SIZE; i++)
            mixed[i] ^= *data++;
        AES256_encrypt(&aes_ctx, 1, out + written, mixed);
        memcpy_safe(mixed, out + written, AES_BLOCK_SIZE);
        written += AES_BLOCK_SIZE;
    }
    if (pad) {
        // For all that remains, pad each byte with the value of the remaining
        // space. If there is none, pad by a full block.
        for (int i = 0; i != padsize; i++)
            mixed[i] ^= *data++;
        for (int i = padsize; i != AES_BLOCK_SIZE; i++)
            mixed[i] ^= AES_BLOCK_SIZE - padsize;
        AES256_encrypt(&aes_ctx, 1, out + written, mixed);
        written += AES_BLOCK_SIZE;
    }
    return written;
}

size_t aes256_cbc_decrypt(const unsigned char aes_key[32], const unsigned char iv[AES_BLOCK_SIZE], const unsigned char* data, size_t size, int pad, unsigned char* out)
{
    unsigned char padsize = 0;
    size_t written = 0;
    int fail = 0;
    const unsigned char* prev = iv;

    if (!data || !size || !out)
        return 0;

    if (size % AES_BLOCK_SIZE != 0)
        return 0;

    AES256_ctx aes_ctx;
    // Set cipher key
    AES256_init(&aes_ctx, aes_key);

    // Decrypt all data. Padding will be checked in the output.
    while (written != size) {
        AES256_decrypt(&aes_ctx, 1, out, data + written);
        for (int i = 0; i != AES_BLOCK_SIZE; i++)
            *out++ ^= prev[i];
        prev = data + written;
        written += AES_BLOCK_SIZE;
    }

    // When decrypting padding, attempt to run in constant-time
    if (pad) {
        // If used, padding size is the value of the last decrypted byte. For
        // it to be valid, It must be between 1 and AES_BLOCK_SIZE.
        padsize = *--out;
        fail = !padsize | (padsize > AES_BLOCK_SIZE);

        // If not well-formed, treat it as though there's no padding.
        padsize *= !fail;

        // All padding must equal the last byte otherwise it's not well-formed
        for (int i = AES_BLOCK_SIZE; i != 0; i--)
            fail |= ((i > AES_BLOCK_SIZE - padsize) & (*out-- != padsize));

        written -= padsize;
    }
    return written * !fail;
}
