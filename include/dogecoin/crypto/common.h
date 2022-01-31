// Copyright (c) 2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef __LIBDOGECOIN_CRYPTO_COMMON_H__
#define __LIBDOGECOIN_CRYPTO_COMMON_H__

#if defined(HAVE_CONFIG_H)
#include "libdogecoin-config.h"
#endif

#include "../dogecoin.h"

#include <stdint.h>
#include <string.h>

#include <dogecoin/compat/portable_endian.h>

LIBDOGECOIN_API static inline uint16_t ReadLE16(const unsigned char* ptr)
{
    uint16_t x;
    memcpy((char*)&x, ptr, 2);
    return le16toh(x);
}

LIBDOGECOIN_API static inline uint32_t ReadLE32(const unsigned char* ptr)
{
    uint32_t x;
    memcpy((char*)&x, ptr, 4);
    return le32toh(x);
}

LIBDOGECOIN_API static inline uint64_t ReadLE64(const unsigned char* ptr)
{
    uint64_t x;
    memcpy((char*)&x, ptr, 8);
    return le64toh(x);
}

LIBDOGECOIN_API static inline void WriteLE16(unsigned char* ptr, uint16_t x)
{
    uint16_t v = htole16(x);
    memcpy(ptr, (char*)&v, 2);
}

LIBDOGECOIN_API static inline void WriteLE32(unsigned char* ptr, uint32_t x)
{
    uint32_t v = htole32(x);
    memcpy(ptr, (char*)&v, 4);
}

LIBDOGECOIN_API static inline void WriteLE64(unsigned char* ptr, uint64_t x)
{
    uint64_t v = htole64(x);
    memcpy(ptr, (char*)&v, 8);
}

LIBDOGECOIN_API static inline uint32_t ReadBE32(const unsigned char* ptr)
{
    uint32_t x;
    memcpy((char*)&x, ptr, 4);
    return be32toh(x);
}

LIBDOGECOIN_API static inline uint64_t ReadBE64(const unsigned char* ptr)
{
    uint64_t x;
    memcpy((char*)&x, ptr, 8);
    return be64toh(x);
}

LIBDOGECOIN_API static inline void WriteBE32(unsigned char* ptr, uint32_t x)
{
    uint32_t v = htobe32(x);
    memcpy(ptr, (char*)&v, 4);
}

LIBDOGECOIN_API static inline void WriteBE64(unsigned char* ptr, uint64_t x)
{
    uint64_t v = htobe64(x);
    memcpy(ptr, (char*)&v, 8);
}

#endif // __LIBDOGECOIN_CRYPTO_COMMON_H__
