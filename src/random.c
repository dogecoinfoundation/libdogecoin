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

#include <assert.h>
#ifdef HAVE_CONFIG_H
#include "libdogecoin-config.h"
#endif
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#if defined _WIN32
#ifdef _MSC_VER
#include <dogecoin/winunistd.h>
#endif
#else
#include <unistd.h>
#endif
#if defined _WIN32 && ! defined __CYGWIN__
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# if HAVE_BCRYPT_H
#  include <bcrypt.h>
# else
#  define NTSTATUS LONG
typedef void* BCRYPT_ALG_HANDLE;
#  define BCRYPT_USE_SYSTEM_PREFERRED_RNG 0x00000002
#  if HAVE_LIB_BCRYPT
extern NTSTATUS WINAPI BCryptGenRandom(BCRYPT_ALG_HANDLE, UCHAR*, ULONG, ULONG);
#  endif
# endif
# if !HAVE_LIB_BCRYPT
#include <wincrypt.h>
#  ifndef CRYPT_VERIFY_CONTEXT
#   define CRYPT_VERIFY_CONTEXT 0xF0000000
#  endif
# endif
#endif

#if defined _WIN32 && ! defined __CYGWIN__

/* Don't assume that UNICODE is not defined.  */
#undef LoadLibrary
#define LoadLibrary LoadLibraryA
#undef CryptAcquireContext
#define CryptAcquireContext CryptAcquireContextA

#if !HAVE_LIB_BCRYPT
/* Avoid warnings from gcc -Wcast-function-type.  */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#define GetProcAddress \
     (void *)GetProcAddress
#pragma GCC diagnostic pop

/* BCryptGenRandom with the BCRYPT_USE_SYSTEM_PREFERRED_RNG flag works only
   starting with Windows 7.  */
typedef NTSTATUS(WINAPI* BCryptGenRandomFuncType) (BCRYPT_ALG_HANDLE, UCHAR*, ULONG, ULONG);
static BCryptGenRandomFuncType BCryptGenRandomFunc = NULL;
static BOOL initialized = FALSE;

static void
initialize(void)
    {
    HMODULE bcrypt = LoadLibrary("bcrypt.dll");
    if (bcrypt != NULL)
        {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
        BCryptGenRandomFunc =
            (BCryptGenRandomFuncType)GetProcAddress(bcrypt, "BCryptGenRandom"); // TODO: find specific type to cast to when building mingw32
        #pragma GCC diagnostic pop
        }
    initialized = TRUE;
    }

# else
#  define BCryptGenRandomFunc BCryptGenRandom
# endif
#endif

void dogecoin_random_init_internal(void);
dogecoin_bool dogecoin_random_bytes_internal(uint8_t* buf, uint32_t len, const uint8_t update_seed);

static const dogecoin_rnd_mapper default_rnd_mapper = { dogecoin_random_init_internal, dogecoin_random_bytes_internal };
static dogecoin_rnd_mapper current_rnd_mapper = { dogecoin_random_init_internal, dogecoin_random_bytes_internal };

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

dogecoin_bool dogecoin_random_bytes_internal(uint8_t* buf, uint32_t len, const uint8_t update_seed)
    {
    (void)update_seed;
    for (uint32_t i = 0; i < len; i++)
        buf[i] = rand();
    return true;
    }
#else
void dogecoin_random_init_internal(void)
    {
    }
dogecoin_bool dogecoin_random_bytes_internal(uint8_t* buf, uint32_t len, const uint8_t update_seed)
    {
#ifdef WIN32
    (void)update_seed;
    /* BCryptGenRandom, defined in <bcrypt.h>
         <https://docs.microsoft.com/en-us/windows/win32/api/bcrypt/nf-bcrypt-bcryptgenrandom>
         with the BCRYPT_USE_SYSTEM_PREFERRED_RNG flag
         works in Windows 7 and newer.  */
    static int bcrypt_not_working /* = 0 */;
    if (!bcrypt_not_working) {
#if !HAVE_LIB_BCRYPT
        if (!initialized)
            initialize();
#endif
        if (BCryptGenRandomFunc != NULL
            && BCryptGenRandomFunc(NULL, buf, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0 /*STATUS_SUCCESS*/)
            return 1;
        bcrypt_not_working = 1;
        }
#if !HAVE_LIB_BCRYPT
    /* CryptGenRandom, defined in <wincrypt.h>
       <https://docs.microsoft.com/en-us/windows/win32/api/wincrypt/nf-wincrypt-cryptgenrandom>
       works in older releases as well, but is now deprecated.
       CryptAcquireContext, defined in <wincrypt.h>
       <https://docs.microsoft.com/en-us/windows/win32/api/wincrypt/nf-wincrypt-cryptacquirecontexta>  */
    {
    static int crypt_initialized /* = 0 */;
    static HCRYPTPROV provider;
    if (!crypt_initialized) {
        if (CryptAcquireContextA(&provider, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFY_CONTEXT))
            crypt_initialized = 1;
        else
            crypt_initialized = -1;
        }
    if (crypt_initialized >= 0) {
        if (!CryptGenRandom(provider, len, buf)) {
            errno = EIO;
            return -1;
            }
        return 1;
        }
    }
# endif
    errno = ENOSYS;
    return -1;
#else
    (void)update_seed; //unused
    FILE* frand = fopen("/dev/urandom", "r"); // figure out why RANDOM_DEVICE is undeclared here
    if (!frand)
        return false;
    size_t len_read = fread(buf, 1, len, frand);
    assert(len_read == len);
    fclose(frand);
    return true;
#endif
    }
#endif
