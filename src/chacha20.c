// Copyright (c) 2017 The Bitcoin Core developers
// Copyright (c) 2024 bluezr
// Copyright (c) 2024 The Dogecoin Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Based on the public domain implementation 'merged' by D. J. Bernstein
// See https://cr.yp.to/chacha.html.

#include <dogecoin/common.h>
#include <dogecoin/chacha20.h>

#include <string.h>

static inline uint32_t rotl32(uint32_t v, int c) { return (v << c) | (v >> (32 - c)); }

#define QUARTERROUND(a,b,c,d) \
  a += b; d = rotl32(d ^ a, 16); \
  c += d; b = rotl32(b ^ c, 12); \
  a += b; d = rotl32(d ^ a, 8); \
  c += d; b = rotl32(b ^ c, 7);

static const unsigned char sigma[] = "expand 32-byte k";
static const unsigned char tau[] = "expand 16-byte k";

struct chacha20* chacha20_new() {
    struct chacha20* this;
    this = dogecoin_calloc(1, sizeof(*this));
    dogecoin_mem_zero(this->input, 16);
    this->setkey = chacha20_set_key;
    this->setiv = chacha20_set_iv;
    this->seek = chacha20_seek;
    this->output = chacha20_output;
    return this;
}

struct chacha20* chacha20_init(const unsigned char* key, size_t keylen) {
    struct chacha20* this;
    this = dogecoin_calloc(1, sizeof(*this));
    dogecoin_mem_zero(this->input, 16);
    this->setkey = chacha20_set_key;
    this->setiv = chacha20_set_iv;
    this->seek = chacha20_seek;
    this->output = chacha20_output;
    this->setkey(this, key, keylen);
    return this;
}

void chacha20_set_key(struct chacha20* this, const unsigned char* k, size_t keylen) {
    const unsigned char *constants;

    this->input[4] = read_le32(k + 0);
    this->input[5] = read_le32(k + 4);
    this->input[6] = read_le32(k + 8);
    this->input[7] = read_le32(k + 12);
    if (keylen == 32) { /* recommended */
        k += 16;
        constants = sigma;
    } else { /* keylen == 16 */
        constants = tau;
    }
    this->input[8] = read_le32(k + 0);
    this->input[9] = read_le32(k + 4);
    this->input[10] = read_le32(k + 8);
    this->input[11] = read_le32(k + 12);
    this->input[0] = read_le32(constants + 0);
    this->input[1] = read_le32(constants + 4);
    this->input[2] = read_le32(constants + 8);
    this->input[3] = read_le32(constants + 12);
    this->input[12] = 0;
    this->input[13] = 0;
    this->input[14] = 0;
    this->input[15] = 0;
}

void chacha20_set_iv(struct chacha20* this, uint64_t iv) {
    this->input[14] = iv;
    this->input[15] = iv >> 32;
}

void chacha20_seek(struct chacha20* this, uint64_t pos) {
    this->input[12] = pos;
    this->input[13] = pos >> 32;
}

void chacha20_output(struct chacha20* this, unsigned char* c, size_t bytes) {
    uint32_t x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, x14, x15;
    uint32_t j0, j1, j2, j3, j4, j5, j6, j7, j8, j9, j10, j11, j12, j13, j14, j15;
    unsigned char *ctarget = NULL;
    unsigned char tmp[64];
    unsigned int i;

    if (!bytes) return;

    j0 = this->input[0];
    j1 = this->input[1];
    j2 = this->input[2];
    j3 = this->input[3];
    j4 = this->input[4];
    j5 = this->input[5];
    j6 = this->input[6];
    j7 = this->input[7];
    j8 = this->input[8];
    j9 = this->input[9];
    j10 = this->input[10];
    j11 = this->input[11];
    j12 = this->input[12];
    j13 = this->input[13];
    j14 = this->input[14];
    j15 = this->input[15];

    for (;;) {
        if (bytes < 64) {
            ctarget = c;
            c = tmp;
        }
        x0 = j0;
        x1 = j1;
        x2 = j2;
        x3 = j3;
        x4 = j4;
        x5 = j5;
        x6 = j6;
        x7 = j7;
        x8 = j8;
        x9 = j9;
        x10 = j10;
        x11 = j11;
        x12 = j12;
        x13 = j13;
        x14 = j14;
        x15 = j15;
        for (i = 20;i > 0;i -= 2) {
            QUARTERROUND( x0, x4, x8,x12)
            QUARTERROUND( x1, x5, x9,x13)
            QUARTERROUND( x2, x6,x10,x14)
            QUARTERROUND( x3, x7,x11,x15)
            QUARTERROUND( x0, x5,x10,x15)
            QUARTERROUND( x1, x6,x11,x12)
            QUARTERROUND( x2, x7, x8,x13)
            QUARTERROUND( x3, x4, x9,x14)
        }
        x0 += j0;
        x1 += j1;
        x2 += j2;
        x3 += j3;
        x4 += j4;
        x5 += j5;
        x6 += j6;
        x7 += j7;
        x8 += j8;
        x9 += j9;
        x10 += j10;
        x11 += j11;
        x12 += j12;
        x13 += j13;
        x14 += j14;
        x15 += j15;

        ++j12;
        if (!j12) ++j13;

        write_le32(c + 0, x0);
        write_le32(c + 4, x1);
        write_le32(c + 8, x2);
        write_le32(c + 12, x3);
        write_le32(c + 16, x4);
        write_le32(c + 20, x5);
        write_le32(c + 24, x6);
        write_le32(c + 28, x7);
        write_le32(c + 32, x8);
        write_le32(c + 36, x9);
        write_le32(c + 40, x10);
        write_le32(c + 44, x11);
        write_le32(c + 48, x12);
        write_le32(c + 52, x13);
        write_le32(c + 56, x14);
        write_le32(c + 60, x15);

        if (bytes <= 64) {
            if (bytes < 64) {
                for (i = 0;i < bytes;++i) ctarget[i] = c[i];
            }
            this->input[12] = j12;
            this->input[13] = j13;
            return;
        }
        bytes -= 64;
        c += 64;
    }
}

void chacha20_free(struct chacha20* this) {
    dogecoin_free(this);
}
