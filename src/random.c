/*

 The MIT License (MIT)

 Copyright (c) 2015 Douglas J. Bakkum
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

#include <dogecoin/random.h>
#ifdef HAVE_CONFIG_H
#  include "libdogecoin-config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#endif

void dogecoin_random_init_internal(void);
dogecoin_bool dogecoin_random_bytes_internal(uint8_t* buf, uint32_t len, const uint8_t update_seed);

static const dogecoin_rnd_mapper default_rnd_mapper = {dogecoin_random_init_internal, dogecoin_random_bytes_internal};
static dogecoin_rnd_mapper current_rnd_mapper = {dogecoin_random_init_internal, dogecoin_random_bytes_internal};

void dogecoin_rnd_set_mapper_default()
{
    current_rnd_mapper = default_rnd_mapper;
}

void dogecoin_rnd_set_mapper(const dogecoin_rnd_mapper mapper)
{
    current_rnd_mapper = mapper;
}

void dogecoin_random_init(void)
{
    current_rnd_mapper.dogecoin_random_init();
}

dogecoin_bool dogecoin_random_bytes(uint8_t* buf, uint32_t len, const uint8_t update_seed)
{
    return current_rnd_mapper.dogecoin_random_bytes(buf, len, update_seed);
}

#ifdef TESTING
void dogecoin_random_init_internal(void)
{
    srand(time(NULL));
}


dogecoin_bool dogecoin_random_bytes_internal(uint8_t* buf, uint32_t len, uint8_t update_seed)
{
    (void)update_seed;
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = rand();
    }

    return true;
}
#else
void dogecoin_random_init_internal(void) {}
dogecoin_bool dogecoin_random_bytes_internal(uint8_t* buf, uint32_t len, const uint8_t update_seed)
{
#ifdef WIN32
    HCRYPTPROV hProvider;
    int ret = CryptAcquireContextW(&hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    assert(ret);
    ret = CryptGenRandom(hProvider, len, buf);
    assert(ret);
    CryptReleaseContext(hProvider, 0);
    return ret;
#else
    (void)update_seed; //unused
    FILE* frand = fopen(RANDOM_DEVICE, "r");
    if (!frand) {
        return false;
    }

    size_t len_read = fread(buf, 1, len, frand);
    assert(len_read == len);
    fclose(frand);
    return true;
#endif
}
#endif
