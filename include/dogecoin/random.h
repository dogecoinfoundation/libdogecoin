/*

 The MIT License (MIT)

 Copyright (c) 2015 Douglas J. Bakkum
 Copyright (c) 2024 bluezr
 Copyright (c) 2024 The Dogecoin Foundation

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

#ifndef __LIBDOGECOIN_RANDOM_H__
#define __LIBDOGECOIN_RANDOM_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#include <dogecoin/chacha20.h>
#include <stdint.h>

typedef struct fast_random_context {
    dogecoin_bool requires_seed;
    chacha20* rng;
    unsigned char bytebuf[64];
    int bytebuf_size;
    uint64_t bitbuf;
    int bitbuf_size;
    void (*random_seed)(struct fast_random_context* this);
    void (*fill_byte_buffer)(struct fast_random_context* this);
    void (*fill_bit_buffer)(struct fast_random_context* this);
    uint256* (*rand256)(struct fast_random_context* this);
    uint64_t (*rand64)(struct fast_random_context* this);
    uint64_t (*randbits)(struct fast_random_context* this, int bits);
    uint32_t (*rand32)(struct fast_random_context* this);
    dogecoin_bool (*randbool)(struct fast_random_context* this);
} fast_random_context;

struct fast_random_context* init_fast_random_context(dogecoin_bool f_deterministic, const uint256* seed);
uint256* rand256(struct fast_random_context* this);
uint64_t rand64(struct fast_random_context* this);
void free_fast_random_context(struct fast_random_context* this);

static const ssize_t NUM_OS_RANDOM_BYTES = 32;
void get_os_rand(unsigned char* ent32);
void random_sanity_check();

typedef struct dogecoin_rnd_mapper_ {
    void (*dogecoin_random_init)(void);
    dogecoin_bool (*dogecoin_random_bytes)(uint8_t* buf, uint32_t len, const uint8_t update_seed);
} dogecoin_rnd_mapper;

// sets a custom random callback mapper
// this function is NOT thread safe and should be called before anything else
LIBDOGECOIN_API void dogecoin_rnd_set_mapper(const dogecoin_rnd_mapper mapper);
LIBDOGECOIN_API void dogecoin_rnd_set_mapper_default();

LIBDOGECOIN_API void dogecoin_random_init(void);
LIBDOGECOIN_API dogecoin_bool dogecoin_random_bytes(uint8_t* buf, uint32_t len, const uint8_t update_seed);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_RANDOM_H__
