/*

 The MIT License (MIT)
 Copyright (c) 2023 bluezr
 Copyright (c) 2023 The Dogecoin Foundation

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

#ifndef __LIBDOGECOIN_DOGECOIN_H__
#define __LIBDOGECOIN_DOGECOIN_H__

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_CONFIG_H)
#include "src/libdogecoin-config.h"
#endif

typedef uint8_t dogecoin_bool; //!serialize, c/c++ save bool

#ifndef __cplusplus
#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif
#endif //__cplusplus

#ifdef __cplusplus
#define LIBDOGECOIN_BEGIN_DECL extern "C" {
#define LIBDOGECOIN_END_DECL }
#else
#define LIBDOGECOIN_BEGIN_DECL /* empty */
#define LIBDOGECOIN_END_DECL   /* empty */
#endif

#ifndef LIBDOGECOIN_API
#if defined(_WIN32)
#ifdef LIBDOGECOIN_BUILD
#define LIBDOGECOIN_API __declspec(dllexport)
#else
#define LIBDOGECOIN_API
#endif
#elif defined(__GNUC__) && defined(LIBDOGECOIN_BUILD)
#define LIBDOGECOIN_API __attribute__((visibility("default")))
#else
#define LIBDOGECOIN_API
#endif
#endif

#if defined(_MSC_VER)
    #define DISABLE_WARNING_PUSH           __pragma(warning( push ))
    #define DISABLE_WARNING_POP            __pragma(warning( pop )) 
    #define DISABLE_WARNING(warningNumber) __pragma(warning( disable : warningNumber ))

    #define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER    DISABLE_WARNING(4100)
    #define DISABLE_WARNING_UNREFERENCED_FUNCTION            DISABLE_WARNING(4505)
    // other warnings you want to deactivate...
    #include <BaseTsd.h>
    typedef SSIZE_T ssize_t;

    //MLUMIN:MSVC - need winsock for msvc 
    #pragma comment(lib, "Ws2_32.lib")
    #pragma comment(lib, "wsock32.lib")
    //MLUMIN:MSVC - need Iphlpapi.lib for __imp_f_nametoindex in msvc
    #pragma comment(lib, "Iphlpapi.lib")
    //MLUMIN:MSVC - need strtok_r redefined as strtok_s in msvc
    #define strtok_r strtok_s


#elif defined(__GNUC__) || defined(__clang__)
    #define DO_PRAGMA(X) _Pragma(#X)
    #define DISABLE_WARNING_PUSH           DO_PRAGMA(GCC diagnostic push)
    #define DISABLE_WARNING_POP            DO_PRAGMA(GCC diagnostic pop) 
    #define DISABLE_WARNING(warningName)   DO_PRAGMA(GCC diagnostic ignored #warningName)
    
    #define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER    DISABLE_WARNING(-Wunused-parameter)
    #define DISABLE_WARNING_UNREFERENCED_FUNCTION            DISABLE_WARNING(-Wunused-function)
   // other warnings you want to deactivate... 
    
#else
    #define DISABLE_WARNING_PUSH
    #define DISABLE_WARNING_POP
    #define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER
    #define DISABLE_WARNING_UNREFERENCED_FUNCTION
    // other warnings you want to deactivate:

#endif

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG 0
#endif
#ifdef ENABLE_DEBUG
#define debug_print(fmt, ...) \
        do { if (ENABLE_DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)
#endif

#define DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH 65
#define DOGECOIN_ECKEY_COMPRESSED_LENGTH 33
#define DOGECOIN_ECKEY_PKEY_LENGTH 32
#define DOGECOIN_HASH_LENGTH 32

#define DOGECOIN_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define DOGECOIN_MAX(a, b) (((a) > (b)) ? (a) : (b))

LIBDOGECOIN_BEGIN_DECL

typedef uint8_t uint256[32];
typedef uint8_t uint160[20];

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_DOGECOIN_H__
