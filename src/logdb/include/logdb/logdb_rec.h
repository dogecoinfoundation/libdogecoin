/*

 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
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

#ifndef __LIBLOGDB_REC_H__
#define __LIBLOGDB_REC_H__

#include <dogecoin/dogecoin.h>

LIBDOGECOIN_BEGIN_DECL

#include <logdb/logdb_base.h>

#include <dogecoin/cstr.h>

#include <stdint.h>
#include <stddef.h>

enum logdb_record_type {
    RECORD_TYPE_WRITE = 0,
    RECORD_TYPE_ERASE = 1
};

typedef struct logdb_record {
    cstring* key;
    cstring* value;
    struct logdb_record* next; /* linked list -> next node (NULL if end) */
    struct logdb_record* prev; /* linked list -> prev node (NULL if end) */
    int written; /* 0 = not written to databse, 1 = written */
    uint8_t mode; /* record mode, 0 = WRITE, 1 = ERASE */
} logdb_record;

/* RECORD HANDLING  */
LIBLOGDB_API logdb_record* logdb_record_new();
LIBLOGDB_API void logdb_record_free(logdb_record* rec);
LIBLOGDB_API void logdb_record_set(logdb_record* rec, cstring *key, cstring *val);
LIBLOGDB_API logdb_record* logdb_record_copy(logdb_record* b_rec);
LIBLOGDB_API void logdb_record_ser(logdb_record* rec, cstring *buf);
LIBLOGDB_API size_t logdb_record_height(logdb_record* head);
LIBLOGDB_API cstring * logdb_record_find_desc(logdb_record* head, cstring *key);
LIBLOGDB_API logdb_record* logdb_record_rm_desc(logdb_record *usehead, cstring *key);

LIBDOGECOIN_END_DECL

#endif /* __LIBLOGDB_REC_H__ */
