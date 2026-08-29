/*

 The MIT License (MIT)

 Copyright (c) 2015 Jonas Schnelli
 Copyright (c) 2023 bluezr
 Copyright (c) 2023-2024 The Dogecoin Foundation

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

#define SPV_HEADERS_FILE_HDR_LEN 8 /* magic(4) + version(4) */
#define SPV_HEADERS_FILE_REC_LEN (32 + 4 + 32 + 80) /* hash + height + chainwork + header */

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
    dogecoin_bool batch_write;   /**< Suppress per-record fdatasync; caller must commit after */
    dogecoin_bool skip_pow;      /**< Skip scrypt PoW verify (for checkpoint-anchored bulk loads) */
    /* Sequential scan state for dogecoin_headers_db_get_block_hash_at_height.
     * cfheaders batches request increasing heights; resuming from the last
     * found record avoids O(N²) re-reads over a 6.2M-block file. */
    long     scan_resume_pos;    /**< File offset after last successful scan record */
    uint32_t scan_resume_height; /**< Block height at scan_resume_pos */
} dogecoin_headers_db;

dogecoin_headers_db *dogecoin_headers_db_new(const dogecoin_chainparams* chainparams, dogecoin_bool inmem_only);
void dogecoin_headers_db_free(dogecoin_headers_db *db);
dogecoin_bool dogecoin_headers_db_load(dogecoin_headers_db* db, const char *filename, dogecoin_bool prompt);
dogecoin_blockindex * dogecoin_headers_db_connect_hdr(dogecoin_headers_db* db, struct const_buffer *buf, dogecoin_bool load_process, dogecoin_bool *connected);
void dogecoin_headers_db_fill_block_locator(dogecoin_headers_db* db, vector_t *blocklocators);
dogecoin_blockindex * dogecoin_headersdb_find(dogecoin_headers_db* db, uint256_t hash);
dogecoin_blockindex * dogecoin_headersdb_getchaintip(dogecoin_headers_db* db);
dogecoin_bool dogecoin_headersdb_disconnect_tip(dogecoin_headers_db* db);
dogecoin_bool dogecoin_headersdb_has_checkpoint_start(dogecoin_headers_db* db);
void dogecoin_headersdb_set_checkpoint_start(dogecoin_headers_db* db, uint256_t hash, uint32_t height, arith_uint256 chainwork);

/**
 * @brief Find the block hash stored at a specific height in the on-disk header file.
 *
 * Scans the headers.db file from its beginning, reading fixed-size records until
 * the one with @p target_height is found.  Falls back to walking the in-memory
 * prev-chain first for blocks near the chain tip (no file I/O needed there).
 *
 * @param db          The headers database (file handle must be open).
 * @param target_height  Block height to look up.
 * @param hash_out    Receives the 32-byte block hash on success.
 * @return true if found and @p hash_out is populated.
 */
dogecoin_bool dogecoin_headers_db_open_for_scan(dogecoin_headers_db *db, const char *filename);
dogecoin_bool dogecoin_headers_db_get_block_hash_at_height(dogecoin_headers_db *db, uint32_t target_height, uint256_t hash_out);
dogecoin_bool dogecoin_headers_db_get_block_hash_at_height_seq(dogecoin_headers_db *db, uint32_t target_height, uint256_t hash_out);

/* Defined in headersdb_file.c using typed trampolines (avoids the
 * function-pointer-cast UB that -fsanitize=function flags). */
extern const dogecoin_headers_db_interface dogecoin_headers_db_interface_file;

LIBDOGECOIN_END_DECL

#endif // __LIBDOGECOIN_HEADERSDB_FILE_H__
