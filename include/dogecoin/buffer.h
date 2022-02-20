/*

 The MIT License (MIT)

 Copyright (c) 2012 exMULTI, Inc.
 Copyright (c) 2015 Jonas Schnelli
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

#ifndef __LIBDOGECOIN_BUFFER_H__
#define __LIBDOGECOIN_BUFFER_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

struct buffer {
    void* p;
    size_t len;
};

struct const_buffer {
    const void* p;
    size_t len;
};

LIBDOGECOIN_API int buffer_equal(const void* a, const void* b);
LIBDOGECOIN_API void buffer_free(void* struct_buffer);
LIBDOGECOIN_API struct buffer* buffer_copy(const void* data, size_t data_len);

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_BUFFER_H__
