/*

 The MIT License (MIT)

 Copyright (c) 2012 exMULTI, Inc.
 Copyright (c) 2022 bluezr
 Copyright (c) 2022-2024 The Dogecoin Foundation

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

#ifndef __LIBDOGECOIN_SERIALIZE_H__
#define __LIBDOGECOIN_SERIALIZE_H__

#include <stdint.h>
#include <stdio.h>

#include <dogecoin/buffer.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/cstr.h>
#include <dogecoin/portable_endian.h>

LIBDOGECOIN_BEGIN_DECL

LIBDOGECOIN_API void ser_bytes(cstring* s, const void* p, size_t len);
LIBDOGECOIN_API void ser_u16(cstring* s, uint16_t v_);
LIBDOGECOIN_API void ser_u32(cstring* s, uint32_t v_);
LIBDOGECOIN_API void ser_u64(cstring* s, uint64_t v_);
LIBDOGECOIN_API void ser_u256(cstring* s, const unsigned char* v_);
LIBDOGECOIN_API void ser_varlen(cstring* s, uint32_t vlen);
LIBDOGECOIN_API void ser_str(cstring* s, const char* s_in, size_t maxlen);
LIBDOGECOIN_API void ser_varstr(cstring* s, cstring* s_in);

LIBDOGECOIN_API void ser_s32(cstring* s, int32_t v_);
LIBDOGECOIN_API void ser_s64(cstring* s, int64_t v_);

LIBDOGECOIN_API int deser_skip(struct const_buffer* buf, size_t len);
LIBDOGECOIN_API int deser_bytes(void* po, struct const_buffer* buf, size_t len);
LIBDOGECOIN_API int deser_u16(uint16_t* vo, struct const_buffer* buf);
LIBDOGECOIN_API int deser_u32(uint32_t* vo, struct const_buffer* buf);
LIBDOGECOIN_API int deser_i32(int32_t* vo, struct const_buffer* buf);
LIBDOGECOIN_API int deser_s32(int32_t* vo, struct const_buffer* buf);
LIBDOGECOIN_API int deser_u64(uint64_t* vo, struct const_buffer* buf);
LIBDOGECOIN_API int deser_u256(uint8_t* vo, struct const_buffer* buf);

LIBDOGECOIN_API int deser_varlen(uint32_t* lo, struct const_buffer* buf);
LIBDOGECOIN_API int deser_varlen_from_file(uint32_t* lo, FILE* file);
LIBDOGECOIN_API int deser_varlen_file(uint32_t* lo, FILE* file, uint8_t* rawdata, size_t* buflen_inout);
LIBDOGECOIN_API int deser_str(char* so, struct const_buffer* buf, size_t maxlen);
LIBDOGECOIN_API int deser_varstr(cstring** so, struct const_buffer* buf);

LIBDOGECOIN_API int deser_s64(int64_t* vo, struct const_buffer* buf);

LIBDOGECOIN_END_DECL

#endif /* __LIBDOGECOIN_SERIALIZE_H__ */
