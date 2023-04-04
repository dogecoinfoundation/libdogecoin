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

#ifndef __LIBLOGDB_RBTREE_H__
#define __LIBLOGDB_RBTREE_H__

#include <dogecoin/cstr.h>
#include <logdb/logdb_base.h>
#include <logdb/logdb_rec.h>
#include <logdb/logdb_core.h>
#include <logdb/red_black_tree.h>

#include <stdint.h>
#include <stddef.h>

LIBDOGECOIN_BEGIN_DECL

typedef struct logdb_rbtree_db_ {
    struct rb_red_blk_tree *tree;
} logdb_rbtree_db;

logdb_rbtree_db* logdb_rbtree_db_new();
void logdb_rbtree_free(void *ctx);
LIBLOGDB_API void logdb_rbtree_init(logdb_log_db* db);
LIBLOGDB_API void logdb_rbtree_append(void* ctx, logdb_bool load_phase, logdb_record *rec);
LIBLOGDB_API cstring * logdb_rbtree_find(logdb_log_db* db, cstring *key);
LIBLOGDB_API size_t logdb_rbtree_size(logdb_log_db* db);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
/* static interface */
static logdb_memmapper logdb_rbtree_mapper = {
    logdb_rbtree_append,
    logdb_rbtree_init,
    logdb_rbtree_free,
    logdb_rbtree_find,
    logdb_rbtree_size
};
#pragma GCC diagnostic pop

LIBDOGECOIN_END_DECL

#endif /* __LIBLOGDB_RBTREE_H__ */
