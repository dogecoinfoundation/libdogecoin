/**
 * Copyright (c) 2012-2014 Luke Dashjr
 * Copyright (c) 2013-2014 Pavol Rusnak
 * Copyright (c) 2022 bluezr
 * Copyright (c) 2022 The Dogecoin Foundation
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

#include <stdbool.h>
#include <string.h>
#include <sys/types.h>

#include <dogecoin/crypto/base58.h>
#include <dogecoin/crypto/sha2.h>

static const int8_t b58digits_map[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, -1, -1, -1, -1, -1, -1,
    -1, 9, 10, 11, 12, 13, 14, 15, 16, -1, 17, 18, 19, 20, 21, -1,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, -1, -1, -1, -1, -1,
    -1, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, -1, -1, -1, -1, -1,
};

int dogecoin_base58_decode(void* bin, size_t* binszp, const char* b58) {
    size_t binsz = *binszp;
    if (binsz == 0) return false;
    const unsigned char* b58u = (const unsigned char*)b58;
    unsigned char* binu = bin;
    size_t outisz = (binsz + 3) / 4, i, j = 0, b58sz = strlen(b58);
    uint8_t bytesleft = binsz % 4;
    uint32_t outi[outisz],  c = 0, zeromask = bytesleft ? (0xffffffff << (bytesleft * 8)) : 0;
    uint64_t t = 0;
    unsigned zerocount = 0;
    memset(outi, 0, outisz * sizeof(*outi));
    for (i = 0; i < b58sz && !b58digits_map[b58u[i]]; ++i) ++zerocount; // leading zeros, just count
    for (; i < b58sz; ++i) {
        if (b58u[i] & 0x80) return false; // high-bit set on invalid digit
        if (b58digits_map[b58u[i]] == -1) return false; // invalid base58 digit
        c = (unsigned)b58digits_map[b58u[i]];
        for (j = outisz; --j;) {
            t = ((uint64_t)outi[j]) * 58 + c;
            c = (t & 0x3f00000000) >> 32;
            outi[j] = t & 0xffffffff;
        }
        if (c) return false; // output number too big (carry to the next int32)
        if (outi[0] & zeromask) return false; // output number too big (last int32 filled too far)
    }
    j = 0;
    if (bytesleft) { for (i = bytesleft; i > 0; --i) *(binu++) = (outi[0] >> (8 * (i - 1))) & 0xff; ++j; }
    for (; j < outisz; ++j) { for (i = sizeof(*outi); i > 0; --i) *(binu++) = (outi[j] >> (8 * (i - 1))) & 0xff; }
    binu = bin; // locate the most significant byte
    for (i = 0; i < binsz; ++i) { if (binu[i]) break; }
    // prepend the correct number of null-bytes
    if (zerocount > i) return false; /* result too large */
    *binszp = binsz - i + zerocount;
    return true;
}

int dogecoin_b58check(const void* bin, size_t binsz, const char* base58str) {
    unsigned char buf[32] = {0};
    const uint8_t* binc = bin;
    unsigned i = 0;
    if (binsz < 4) return -4;
    sha256_raw(bin, binsz - 4, buf);
    sha256_raw(buf, 32, buf);
    if (memcmp(&binc[binsz - 4], buf, 4)) return -1;
    // check number of zeros is correct AFTER verifying checksum (to avoid possibility of accessing base58str beyond the end)
    for (i = 0; binc[i] == '\0' && base58str[i] == '1'; ++i) {} // just finding the end of zeros, nothing to do in loop
    if (binc[i] == '\0' || base58str[i] == '1') return -3;
    return binc[0];
}

static const char b58digits_ordered[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

int dogecoin_base58_encode(char* b58, size_t* b58sz, const void* data, size_t binsz) {
    const uint8_t* bin = data;
    int carry = 0;
    ssize_t i, j, high, zcount = 0;
    size_t size = 0;
    while (zcount < (ssize_t)binsz && !bin[zcount]) ++zcount;
    size = (binsz - zcount) * 138 / 100 + 1;
    uint8_t buf[size];
    memset(buf, 0, size);
    for (i = zcount, high = size - 1; i < (ssize_t)binsz; ++i, high = j) {
        for (carry = bin[i], j = size - 1; (j > high) || carry; --j) {
            carry += 256 * buf[j];
            buf[j] = carry % 58;
            carry /= 58;
            if (!j) break;
        }
    }
    for (j = 0; j < (ssize_t)size && !buf[j]; ++j);
    if (*b58sz <= zcount + size - j) { *b58sz = zcount + size - j + 1; return false; }
    if (zcount) memset(b58, '1', zcount);
    for (i = zcount; j < (ssize_t)size; ++i, ++j) b58[i] = b58digits_ordered[buf[j]];
    b58[i] = '\0';
    *b58sz = i + 1;
    return true;
}

int dogecoin_base58_encode_check(const uint8_t* data, int datalen, char* str, int strsize) {
    int ret;
    if (datalen > 128) return 0;
    uint8_t buf[datalen + 32];
    memset(buf, 0, sizeof(buf));
    uint8_t* hash = buf + datalen;
    memcpy(buf, data, datalen);
    sha256_raw(data, datalen, hash);
    sha256_raw(hash, 32, hash);
    size_t res = strsize;
    bool success = dogecoin_base58_encode(str, &res, buf, datalen + 4);
    memset(buf, 0, sizeof(buf));
    return success ? res : 0;
}

int dogecoin_base58_decode_check(const char* str, uint8_t* data, size_t datalen) {
    int ret;
    size_t strl = strlen(str), binsize = strl;
    if (dogecoin_base58_decode(data, &binsize, str) != true) { ret = 0; }
    memmove(data, data + strl - binsize, binsize);
    memset(data + binsize, 0, datalen - binsize);
    if (dogecoin_b58check(data, binsize, str) < 0) { ret = 0; } 
    else { ret = binsize; }
    return ret;
}
