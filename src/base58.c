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

#include <dogecoin/base58.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/mem.h>
#include <dogecoin/sha2.h>
#include <dogecoin/hash.h>

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

typedef uint64_t b58_maxint_t;
typedef uint32_t b58_almostmaxint_t;
#define b58_almostmaxint_bits (sizeof(b58_almostmaxint_t) * 8)
static const b58_almostmaxint_t b58_almostmaxint_mask = ((((b58_maxint_t)1) << b58_almostmaxint_bits) - 1);

int dogecoin_base58_decode(void* bin, size_t* binszp, const char* b58, size_t b58sz)
{
    size_t binsz = *binszp;
    if (binsz == 0)
        return false;
    const unsigned char* b58u = (const unsigned char*)b58;
    unsigned char* binu = bin;
    size_t outisz = (binsz + sizeof(b58_almostmaxint_t) - 1) / sizeof(b58_almostmaxint_t);
	b58_maxint_t t;
	b58_almostmaxint_t c;
    size_t i, j = 0;
    uint8_t bytesleft = binsz % sizeof(b58_almostmaxint_t);
    b58_almostmaxint_t* outi = dogecoin_uint32_vla(outisz);
    uint32_t zeromask = bytesleft ? (b58_almostmaxint_mask << (bytesleft * 8)) : 0;
    unsigned zerocount = 0;

    dogecoin_mem_zero(outi, outisz * sizeof(*outi));
    for (i = 0; i < b58sz && b58u[i] == '1'; ++i) {
        ++zerocount; // leading zeros, just count
    }

    for (; i < b58sz; ++i) {
        if (b58u[i] & 0x80) {
            free(outi);
            return false; // high-bit set on invalid digit
        }
        if (b58digits_map[b58u[i]] == -1) {
            free(outi);
            return false; // invalid base58 digit
        }
        c = (unsigned)b58digits_map[b58u[i]];
        for (j = outisz; --j;) {
            t = ((b58_maxint_t)outi[j]) * 58 + c;
            c = t >> b58_almostmaxint_bits;
            outi[j] = t & b58_almostmaxint_mask;
        }
        if (c) {
            dogecoin_mem_zero(outi, outisz * sizeof(*outi));
            free(outi);
            return false; // output number too big (carry to the next int32)
        }
        if (outi[0] & zeromask) {
            dogecoin_mem_zero(outi, outisz * sizeof(*outi));
            free(outi);
            return false; // output number too big (last int32 filled too far)
        }
    }
    j = 0;
    if (bytesleft) {
        for (i = bytesleft; i > 0; --i) {
            *(binu++) = (outi[0] >> (8 * (i - 1))) & 0xff;
        }
        ++j;
    }
    for (; j < outisz; ++j) {
        for (i = sizeof(*outi); i > 0; --i) {
            *(binu++) = (outi[j] >> (8 * (i - 1))) & 0xff;
        }
    }
    // count canonical base58 byte count
    binu = bin; // locate the most significant byte
	for (i = 0; i < binsz; ++i) {
		if (binu[i]) break;
		--*binszp;
	}
    // prepend the correct number of null-bytes
    if (zerocount > i) {
        free(outi);
        return false; /* result too large */
    }
	*binszp += zerocount;
    dogecoin_mem_zero(outi, outisz * sizeof(*outi));
    free(outi);
    return true;
}

int dogecoin_b58check(const void* bin, size_t binsz, const char* base58str)
{
    uint256 buf[32];
    dogecoin_mem_zero(buf, 32);
    const uint8_t* binc = bin;
    unsigned i = 0;
    if (binsz < 4) {
        return -4;
    }
    if (!dogecoin_dblhash(bin, binsz - 4, (uint8_t *)buf)) {
        return -2;
    }
    if (memcmp(&binc[binsz - 4], buf, 4)) {
        return -1;
    }
    // check number of zeros is correct AFTER verifying checksum (to avoid possibility of accessing base58str beyond the end)
    for (i = 0; binc[i] == '\0' && base58str[i] == '1'; ++i) {
    } // just finding the end of zeros, nothing to do in loop
    if (binc[i] == '\0' || base58str[i] == '1') {
        return -3;
    }
    return binc[0];
}

static const char b58digits_ordered[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

int dogecoin_base58_encode(char* b58, size_t* b58sz, const void* data, size_t binsz)
{
    const uint8_t* bin = data;
    int carry = 0;
    ssize_t i = 0, j = 0, high = 0, zcount = 0;
    size_t size = 0;
    while (zcount < (ssize_t)binsz && !bin[zcount]) {
        ++zcount;
    }
    size = (binsz - zcount) * 138 / 100 + 1;
    uint8_t* buf = dogecoin_uint8_vla(size);
    dogecoin_mem_zero(buf, size);
    for (i = zcount, high = size - 1; i < (ssize_t)binsz; ++i, high = j) {
        for (carry = bin[i], j = size - 1; (j > high) || carry; --j) {
            carry += 256 * buf[j];
            buf[j] = carry % 58;
            carry /= 58;
            if (!j) // Otherwise j wraps to maxint which is > high
                break;
        }
    }
    for (j = 0; j < (ssize_t)size && !buf[j]; ++j)
        ;
    if (*b58sz <= zcount + size - j) {
        *b58sz = zcount + size - j + 1;
        dogecoin_mem_zero(buf, size);
        free(buf);
        return false;
    }
    if (zcount) {
        memset(b58, '1', zcount);
    }
    for (i = zcount; j < (ssize_t)size; ++i, ++j) {
        b58[i] = b58digits_ordered[buf[j]];
    }
    b58[i] = '\0';
    *b58sz = i + 1;
    dogecoin_mem_zero(buf, size);
    free(buf);
    return true;
}

size_t dogecoin_base58_encode_check(const uint8_t* data, size_t datalen, char* str, size_t strsize)
{
    size_t ret;
    if (datalen > 128) {
        return 0;
    }
    size_t buf_size = datalen + sizeof(uint256);
    uint8_t* buf = dogecoin_uint8_vla(buf_size);
    uint8_t* hash = buf + datalen;
    memcpy_safe(buf, data, datalen);
    if (!dogecoin_dblhash(data, datalen, hash)) {
        free(buf);
        return false;
    }
    size_t res = strsize;
    if (dogecoin_base58_encode(str, &res, buf, datalen + 4) != true) {
        ret = 0;
    } else {
        ret = res;
    }
    dogecoin_mem_zero(buf, buf_size);
    free(buf);
    return ret;
}

size_t dogecoin_base58_decode_check(const char* str, uint8_t* data, size_t datalen)
{
    size_t ret, i;
    for (i = 0; str[i] && i < 1024; i++){};
    size_t strl = i;
    /* buffer needs to be at least the strsize, will be used
       for the whole decoding */
    if (strl > 128 || datalen < strl) {
        return 0;
    }
    size_t binsize = strl;
    if (!dogecoin_base58_decode(data, &binsize, str, strl)) {
        return 0;
    }
    memmove(data, data + strl - binsize, binsize);
    dogecoin_mem_zero(data + binsize, datalen - binsize);
    if (dogecoin_b58check(data, binsize, str) < 0) {
        ret = 0;
    } else {
        ret = binsize;
    }
    return ret;
}

dogecoin_bool dogecoin_p2pkh_addr_from_hash160(const uint160 hashin, const dogecoin_chainparams* chain, char *addrout, size_t len) {
    uint8_t hash160[sizeof(uint160)+1];
    hash160[0] = chain->b58prefix_pubkey_address;
    memcpy_safe(hash160 + 1, hashin, sizeof(uint160));

    return (dogecoin_base58_encode_check(hash160, sizeof(uint160)+1, addrout, len) > 0);
}

dogecoin_bool dogecoin_p2sh_addr_from_hash160(const uint160 hashin, const dogecoin_chainparams* chain, char* addrout,
                                    size_t len)
{
    uint8_t hash160[sizeof(uint160) + 1];
    hash160[0] = chain->b58prefix_script_address;
    memcpy_safe(hash160 + 1, hashin, sizeof(uint160));

    return (dogecoin_base58_encode_check(hash160, sizeof(uint160) + 1, addrout, len) > 0);
}
