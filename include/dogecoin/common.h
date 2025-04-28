// Copyright (c) 2014 The Bitcoin Core developers
// Copyright (c) 2024 bluezr
// Copyright (c) 2024 The Dogecoin Foundation
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __LIBDOGECOIN_COMMON_H__
#define __LIBDOGECOIN_COMMON_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#if defined(HAVE_CONFIG_H)
#include "libdogecoin-config.h"
#endif

#include <stdint.h>
#include <string.h>

#include <dogecoin/portable_endian.h>
#include <dogecoin/utils.h>

LIBDOGECOIN_API static inline uint16_t read_le16(const unsigned char* ptr)
{
    uint16_t x;
    memcpy_safe((char*)&x, ptr, 2);
    return le16toh(x);
}

LIBDOGECOIN_API static inline uint32_t read_le32(const unsigned char* ptr)
{
    uint32_t x;
    memcpy_safe((char*)&x, ptr, 4);
    return le32toh(x);
}

LIBDOGECOIN_API static inline uint64_t read_le64(const unsigned char* ptr)
{
    uint64_t x;
    memcpy_safe((char*)&x, ptr, 8);
    return le64toh(x);
}

LIBDOGECOIN_API static inline void write_le8(unsigned char* ptr, uint8_t x)
{
    uint8_t v = htole8(x);
    memcpy_safe(ptr, (char*)&v, 1);
}

LIBDOGECOIN_API static inline void write_le16(unsigned char* ptr, uint16_t x)
{
    uint16_t v = htole16(x);
    memcpy_safe(ptr, (char*)&v, 2);
}

LIBDOGECOIN_API static inline void write_le32(unsigned char* ptr, uint32_t x)
{
    uint32_t v = htole32(x);
    memcpy_safe(ptr, (char*)&v, 4);
}

LIBDOGECOIN_API static inline void write_le64(unsigned char* ptr, uint64_t x)
{
    uint64_t v = htole64(x);
    memcpy_safe(ptr, (char*)&v, 8);
}

LIBDOGECOIN_API static inline uint32_t read_be32(const unsigned char* ptr)
{
    uint32_t x;
    memcpy_safe((char*)&x, ptr, 4);
    return be32toh(x);
}

LIBDOGECOIN_API static inline uint64_t read_be64(const unsigned char* ptr)
{
    uint64_t x;
    memcpy_safe((char*)&x, ptr, 8);
    return be64toh(x);
}

LIBDOGECOIN_API static inline void write_be32(unsigned char* ptr, uint32_t x)
{
    uint32_t v = htobe32(x);
    memcpy_safe(ptr, (char*)&v, 4);
}

LIBDOGECOIN_API static inline void write_be64(unsigned char* ptr, uint64_t x)
{
    uint64_t v = htobe64(x);
    memcpy_safe(ptr, (char*)&v, 8);
}

/** Return the smallest number n such that (x >> n) == 0 (or 64 if the highest bit in x is set. */
LIBDOGECOIN_API static inline uint64_t count_bits(uint64_t x)
{
#if HAVE_BUILTIN_CLZL
    if (sizeof(unsigned long) >= sizeof(uint64_t)) {
        return x ? 8 * sizeof(unsigned long) - __builtin_clzl(x) : 0;
    }
#endif
#if HAVE_BUILTIN_CLZLL
    if (sizeof(unsigned long long) >= sizeof(uint64_t)) {
        return x ? 8 * sizeof(unsigned long long) - __builtin_clzll(x) : 0;
    }
#endif
    int ret = 0;
    while (x) {
        x >>= 1;
        ++ret;
    }
    return ret;
}

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_COMMON_H__
