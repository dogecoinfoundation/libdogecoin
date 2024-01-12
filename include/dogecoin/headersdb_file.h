/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli
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

#ifndef __LIBDOGECOIN_HEADERSDB_FILE_H__
#define __LIBDOGECOIN_HEADERSDB_FILE_H__

#include <dogecoin/dogecoin.h>

#include <dogecoin/blockchain.h>
#include <dogecoin/buffer.h>
#include <dogecoin/cstr.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/headersdb.h>

LIBDOGECOIN_BEGIN_DECL

/* filebased headers database (including binary tree option for fast access)
*/
typedef struct dogecoin_headers_db_
{
    FILE *headers_tree_file;
    dogecoin_bool read_write_file;
    void *tree_root;
    dogecoin_bool use_binary_tree;
    unsigned int max_hdr_in_mem;
    const dogecoin_chainparams *params;
    dogecoin_blockindex genesis;
    dogecoin_blockindex *chaintip;
    dogecoin_blockindex *chainbottom;
} dogecoin_headers_db;

dogecoin_headers_db *dogecoin_headers_db_new(const dogecoin_chainparams* chainparams, dogecoin_bool inmem_only);
void dogecoin_headers_db_free(dogecoin_headers_db *db);
dogecoin_bool dogecoin_headers_db_load(dogecoin_headers_db* db, const char *filename, dogecoin_bool prompt);
dogecoin_blockindex * dogecoin_headers_db_connect_hdr(dogecoin_headers_db* db, struct const_buffer *buf, dogecoin_bool load_process, dogecoin_bool *connected);
void dogecoin_headers_db_fill_block_locator(dogecoin_headers_db* db, vector *blocklocators);
dogecoin_blockindex * dogecoin_headersdb_find(dogecoin_headers_db* db, uint256 hash);
dogecoin_blockindex * dogecoin_headersdb_getchaintip(dogecoin_headers_db* db);
dogecoin_bool dogecoin_headersdb_disconnect_tip(dogecoin_headers_db* db);
dogecoin_bool dogecoin_headersdb_has_checkpoint_start(dogecoin_headers_db* db);
void dogecoin_headersdb_set_checkpoint_start(dogecoin_headers_db* db, uint256 hash, uint32_t height);

static const dogecoin_headers_db_interface dogecoin_headers_db_interface_file = {
    (void* (*)(const dogecoin_chainparams*, dogecoin_bool))dogecoin_headers_db_new,
    (void (*)(void *))dogecoin_headers_db_free,
    (dogecoin_bool (*)(void *, const char *, dogecoin_bool))dogecoin_headers_db_load,
    (void (*)(void* , vector *))dogecoin_headers_db_fill_block_locator,
    (dogecoin_blockindex *(*)(void* , struct const_buffer *, dogecoin_bool , dogecoin_bool *))dogecoin_headers_db_connect_hdr,
    (dogecoin_blockindex* (*)(void *))dogecoin_headersdb_getchaintip,
    (dogecoin_bool (*)(void *))dogecoin_headersdb_disconnect_tip,
    (dogecoin_bool (*)(void *))dogecoin_headersdb_has_checkpoint_start,
    (void (*)(void *, uint256, uint32_t))dogecoin_headersdb_set_checkpoint_start
};

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_HEADERSDB_FILE_H__
