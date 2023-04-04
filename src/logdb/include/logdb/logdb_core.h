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

#ifndef __LIBLOGDB_LOGDB_H__
#define __LIBLOGDB_LOGDB_H__

#include <logdb/logdb_base.h>
#include <logdb/logdb_rec.h>
#include <dogecoin/sha2.h>
#include <dogecoin/buffer.h>

#include <stdint.h>
#include <stdio.h>

LIBDOGECOIN_BEGIN_DECL

/** error types */
enum logdb_error {
    LOGDB_SUCCESS = 0,
    LOGDB_ERROR_UNKNOWN = 100,
    LOGDB_ERROR_FOPEN_FAILED = 200,
    LOGDB_ERROR_WRONG_FILE_FORMAT = 300,
    LOGDB_ERROR_DATASTREAM_ERROR = 400,
    LOGDB_ERROR_CHECKSUM = 500,
    LOGDB_ERROR_FILE_ALREADY_OPEN = 600
};

typedef struct logdb_memmapper_ logdb_memmapper; /* forward declaration */

/** logdb handle */
typedef struct logdb_log_db {
    FILE *file;
    logdb_memmapper *mem_mapper;
    void *cb_ctx;
    logdb_record *cache_head;
    sha256_context hashctx;
    uint8_t hashlen;
    uint32_t version;
    uint32_t support_flags;
} logdb_log_db;

typedef struct logdb_txn {
    logdb_record *txn_head;
} logdb_txn;

/* function pointer interface for flexible memory mapping functions*/
struct logdb_memmapper_
{
    void (*append_cb)(void*, logdb_bool, logdb_record *);
    void (*init_cb)(logdb_log_db*);
    void (*cleanup_cb)(void*);
    cstring* (*find_cb)(logdb_log_db*, cstring *);
    size_t (*size_cb)(logdb_log_db*);
};

/* DB HANDLING
////////////////////////////////// */
LIBLOGDB_API logdb_log_db* logdb_new();
LIBLOGDB_API logdb_log_db* logdb_rbtree_new();
LIBLOGDB_API void logdb_free(logdb_log_db* db);
LIBLOGDB_API void logdb_set_memmapper(logdb_log_db* db, logdb_memmapper *mapper, void *ctx);
LIBLOGDB_API logdb_bool logdb_load(logdb_log_db* handle, const char *file_path, logdb_bool create, enum logdb_error *error);
LIBLOGDB_API logdb_bool logdb_flush(logdb_log_db* db);
LIBLOGDB_API void logdb_delete(logdb_log_db* db, logdb_txn *txn, cstring *key);
LIBLOGDB_API void logdb_append(logdb_log_db* db, logdb_txn *txn, cstring *key, cstring *value);

LIBLOGDB_API cstring * logdb_find_cache(logdb_log_db* db, cstring *key);
LIBLOGDB_API cstring * logdb_find(logdb_log_db* db, cstring *key);

LIBLOGDB_API size_t logdb_cache_size(logdb_log_db* db);
LIBLOGDB_API size_t logdb_count_keys(logdb_log_db* db);

logdb_bool logdb_write_record(logdb_log_db* db, logdb_record *rec);
logdb_bool logdb_record_deser_from_file(logdb_record* rec, logdb_log_db *db, enum logdb_error *error);
logdb_bool logdb_remove_existing_records(logdb_record *usehead, cstring *key);


/* TRANSACTION HANDLING
////////////////////////////////// */
LIBLOGDB_API logdb_txn* logdb_txn_new();
LIBLOGDB_API void logdb_txn_free(logdb_txn* db);
LIBLOGDB_API void logdb_txn_commit(logdb_log_db* db, logdb_txn *txn);

LIBDOGECOIN_END_DECL

#endif /* __LIBLOGDB_LOGDB_H__ */
