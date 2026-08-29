/*

 The MIT License (MIT)

 Copyright (c) 2024 bluezr
 Copyright (c) 2024 The Dogecoin Foundation

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

#ifndef _WIN32
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <test/utest.h>

/*
 * Create scratch files private to this user. plain cfdb_tmp_open(path, "wb") leaves
 * the mode to the process umask, which can yield a world-writable file another
 * local user could rewrite between creation and read.
 */
static FILE* cfdb_tmp_open(const char* path, const char* mode)
{
#ifdef _WIN32
    return fopen(path, mode);
#else
    int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return NULL;
    }
    return fdopen(fd, mode);
#endif
}


#include <dogecoin/cfheadersdb_file.h>
#include <dogecoin/chainparams.h>
#include <dogecoin/compact_filter.h>
#include <dogecoin/cstr.h>
#include <dogecoin/mem.h>
#include <dogecoin/protocol.h>
#include <dogecoin/utils.h>
#include <dogecoin/vector.h>

/* Relative to the working directory: Windows and Android have no /tmp. */
#define CFDB_TEST_HEADERS "test_cfheaders.dat"
#define CFDB_TEST_FILTERS "test_cfilters.dat"

/* Fill a uint256_t with a deterministic, height-dependent pattern so that a
   record read back at the wrong offset is detectable rather than plausible. */
static void fill_header(uint256_t out, uint32_t height)
{
    unsigned int i;
    for (i = 0; i < 32; i++)
        out[i] = (uint8_t)((height * 7u + i * 13u) & 0xff);
}

struct iter_ctx {
    unsigned int count;
    uint32_t     last_height;
    uint32_t     last_len;
    dogecoin_bool payload_ok;
};

/* Largest single allocation seen while a recording mapper is installed.
   iterate() sizes one allocation directly from a length field read off disk, so
   watching the allocator is the only way to tell "rejected the length" apart
   from "asked for 4 GiB and then failed the read" -- both return false. */
static size_t cfdb_max_alloc = 0;

static void *rec_malloc(size_t size)
{
    if (size > cfdb_max_alloc) cfdb_max_alloc = size;
    return malloc(size);
}
static void *rec_calloc(size_t count, size_t size)
{
    if (count * size > cfdb_max_alloc) cfdb_max_alloc = count * size;
    return calloc(count, size);
}
static void *rec_realloc(void *ptr, size_t size)
{
    if (size > cfdb_max_alloc) cfdb_max_alloc = size;
    return realloc(ptr, size);
}
static void rec_free(void *ptr) { free(ptr); }

static dogecoin_bool count_cb(uint32_t height, const uint256_t block_hash,
                              const uint8_t *filter_data, uint32_t data_len,
                              void *ctx)
{
    struct iter_ctx *c = (struct iter_ctx *)ctx;
    uint256_t expect;
    (void)block_hash;
    c->count++;
    c->last_height = height;
    c->last_len = data_len;
    fill_header(expect, height);
    if (filter_data && data_len >= 4 && memcmp(filter_data, expect, 4) != 0)
        c->payload_ok = false;
    return true;
}

void test_cfheadersdb()
{
    unsigned int i;

    /* ---------------------------------------------------------------- */
    /* cfheaders: write, flush, reopen, and read the records back        */
    /* ---------------------------------------------------------------- */
    remove(CFDB_TEST_HEADERS);

    dogecoin_cfheaders_db *db = dogecoin_cfheaders_db_new(&dogecoin_chainparams_main, false);
    u_assert_int_eq(db != NULL, 1);

    dogecoin_compact_filter_state *state = dogecoin_compact_filter_state_new();
    u_assert_int_eq(state != NULL, 1);
    u_assert_int_eq(dogecoin_cfheaders_db_load(db, CFDB_TEST_HEADERS, state), true);

    uint256_t genesis_fh;
    fill_header(genesis_fh, 0);
    u_assert_int_eq(dogecoin_cfheaders_db_write_genesis(db, genesis_fh), true);

    for (i = 1; i <= 16; i++) {
        uint256_t fh;
        fill_header(fh, i);
        u_assert_int_eq(dogecoin_cfheaders_db_write(db, i, fh), true);
    }
    u_assert_int_eq(dogecoin_cfheaders_db_flush(db), true);
    u_assert_uint32_eq(db->tip_height, 16);
    dogecoin_cfheaders_db_free(db);
    dogecoin_compact_filter_state_free(state);

    /* Reopen: the records must come back, in order, with the genesis header. */
    db = dogecoin_cfheaders_db_new(&dogecoin_chainparams_main, false);
    state = dogecoin_compact_filter_state_new();
    u_assert_int_eq(dogecoin_cfheaders_db_load(db, CFDB_TEST_HEADERS, state), true);
    u_assert_uint32_eq(db->tip_height, 16);
    u_assert_int_eq(memcmp(state->genesis_filter_header, genesis_fh, 32), 0);
    u_assert_int_eq(state->filter_headers != NULL, 1);
    u_assert_uint32_eq((uint32_t)state->filter_headers->len, 16);
    for (i = 0; i < 16; i++) {
        uint256_t expect;
        uint256_t *got = (uint256_t *)vector_idx(state->filter_headers, i);
        fill_header(expect, i + 1);
        u_assert_int_eq(memcmp(*got, expect, 32), 0);
    }

    /* reset() truncates back to the file header, leaving no records. */
    u_assert_int_eq(dogecoin_cfheaders_db_reset(db), true);
    dogecoin_cfheaders_db_free(db);
    dogecoin_compact_filter_state_free(state);

    db = dogecoin_cfheaders_db_new(&dogecoin_chainparams_main, false);
    state = dogecoin_compact_filter_state_new();
    u_assert_int_eq(dogecoin_cfheaders_db_load(db, CFDB_TEST_HEADERS, state), true);
    u_assert_uint32_eq((uint32_t)state->filter_headers->len, 0);
    dogecoin_cfheaders_db_free(db);
    dogecoin_compact_filter_state_free(state);

    /* ---------------------------------------------------------------- */
    /* A trailing partial record must be ignored, not misparsed          */
    /* ---------------------------------------------------------------- */
    remove(CFDB_TEST_HEADERS);
    db = dogecoin_cfheaders_db_new(&dogecoin_chainparams_main, false);
    state = dogecoin_compact_filter_state_new();
    u_assert_int_eq(dogecoin_cfheaders_db_load(db, CFDB_TEST_HEADERS, state), true);
    for (i = 1; i <= 4; i++) {
        uint256_t fh;
        fill_header(fh, i);
        u_assert_int_eq(dogecoin_cfheaders_db_write(db, i, fh), true);
    }
    u_assert_int_eq(dogecoin_cfheaders_db_flush(db), true);
    dogecoin_cfheaders_db_free(db);
    dogecoin_compact_filter_state_free(state);

    /* Lop 10 bytes off the last record, as an interrupted write would. Done by
       rewriting a shortened copy rather than truncate()/_chsize(), so the test
       stays portable across the platforms this file already builds on. */
    {
        FILE *f = fopen(CFDB_TEST_HEADERS, "rb");
        uint8_t *buf;
        long size;
        size_t got;
        u_assert_int_eq(f != NULL, 1);
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        u_assert_int_eq(size > 10, 1);
        rewind(f);
        buf = dogecoin_malloc((size_t)size);
        u_assert_int_eq(buf != NULL, 1);
        got = fread(buf, 1, (size_t)size, f);
        fclose(f);
        u_assert_int_eq(got == (size_t)size, 1);

        f = cfdb_tmp_open(CFDB_TEST_HEADERS, "wb");
        u_assert_int_eq(f != NULL, 1);
        u_assert_int_eq(fwrite(buf, 1, (size_t)size - 10, f) == (size_t)size - 10, 1);
        fclose(f);
        dogecoin_free(buf);
    }

    db = dogecoin_cfheaders_db_new(&dogecoin_chainparams_main, false);
    state = dogecoin_compact_filter_state_new();
    u_assert_int_eq(dogecoin_cfheaders_db_load(db, CFDB_TEST_HEADERS, state), true);
    /* Three whole records survive; the truncated fourth is dropped. */
    u_assert_uint32_eq((uint32_t)state->filter_headers->len, 3);
    u_assert_uint32_eq(db->tip_height, 3);
    dogecoin_cfheaders_db_free(db);
    dogecoin_compact_filter_state_free(state);

    /* ---------------------------------------------------------------- */
    /* cfilters: write, iterate, and reject an oversized length field    */
    /* ---------------------------------------------------------------- */
    remove(CFDB_TEST_FILTERS);
    dogecoin_cfilters_db *fdb = dogecoin_cfilters_db_new(&dogecoin_chainparams_main, false);
    u_assert_int_eq(fdb != NULL, 1);
    u_assert_int_eq(dogecoin_cfilters_db_load(fdb, CFDB_TEST_FILTERS), true);

    for (i = 1; i <= 5; i++) {
        uint256_t bh, payload;
        cstring *data;
        fill_header(bh, i + 100);
        fill_header(payload, i);
        data = cstr_new_buf(payload, 32);
        u_assert_int_eq(dogecoin_cfilters_db_write(fdb, i, bh, data), true);
        cstr_free(data, true);
    }
    dogecoin_cfilters_db_free(fdb);

    fdb = dogecoin_cfilters_db_new(&dogecoin_chainparams_main, false);
    u_assert_int_eq(dogecoin_cfilters_db_load(fdb, CFDB_TEST_FILTERS), true);
    {
        struct iter_ctx c;
        c.count = 0; c.last_height = 0; c.last_len = 0; c.payload_ok = true;
        u_assert_int_eq(dogecoin_cfilters_db_iterate(fdb, count_cb, &c), true);
        u_assert_uint32_eq(c.count, 5);
        u_assert_uint32_eq(c.last_height, 5);
        u_assert_uint32_eq(c.last_len, 32);
        u_assert_int_eq(c.payload_ok, true);
    }
    dogecoin_cfilters_db_free(fdb);

    /* Corrupt the first record's data_len to 0xFFFFFFFF. Without a bound this
       asks the allocator for 4 GiB before the short read is ever attempted, so
       iterate() must reject the record on the length alone. The file header is
       CF_HEADERS_FILE_HDR_LEN bytes and data_len sits at offset 36 of the
       record header. */
    {
        FILE *f = fopen(CFDB_TEST_FILTERS, "r+b");
        uint32_t bogus = 0xFFFFFFFFu;
        u_assert_int_eq(f != NULL, 1);
        u_assert_int_eq(fseek(f, CF_HEADERS_FILE_HDR_LEN + 36, SEEK_SET) == 0, 1);
        u_assert_int_eq(fwrite(&bogus, 4, 1, f) == 1, 1);
        fclose(f);
    }

    fdb = dogecoin_cfilters_db_new(&dogecoin_chainparams_main, false);
    u_assert_int_eq(dogecoin_cfilters_db_load(fdb, CFDB_TEST_FILTERS), true);
    {
        struct iter_ctx c;
        dogecoin_mem_mapper rec = {rec_malloc, rec_calloc, rec_realloc, rec_free};
        c.count = 0; c.last_height = 0; c.last_len = 0; c.payload_ok = true;

        cfdb_max_alloc = 0;
        dogecoin_mem_set_mapper(rec);
        u_assert_int_eq(dogecoin_cfilters_db_iterate(fdb, count_cb, &c), false);
        dogecoin_mem_set_mapper_default();

        /* The record is rejected and no callback fires. */
        u_assert_uint32_eq(c.count, 0);
        /* And, the point of the bound: the 0xFFFFFFFF length was never handed
           to the allocator. Asserting only on the false return would pass with
           the bound removed, because the oversized read fails immediately
           after the oversized allocation succeeds under memory overcommit. */
        u_assert_int_eq(cfdb_max_alloc <= (size_t)DOGECOIN_MAX_P2P_MSG_SIZE, 1);
    }
    dogecoin_cfilters_db_free(fdb);

    /* ---------------------------------------------------------------- */
    /* inmem_only performs no file I/O                                   */
    /* ---------------------------------------------------------------- */
    {
        dogecoin_cfheaders_db *mem = dogecoin_cfheaders_db_new(&dogecoin_chainparams_main, true);
        uint256_t fh;
        u_assert_int_eq(mem != NULL, 1);
        u_assert_int_eq(mem->read_write, false);
        u_assert_int_eq(mem->file == NULL, 1);
        fill_header(fh, 1);
        /* Writes are accepted and tracked in RAM without touching the disk. */
        u_assert_int_eq(dogecoin_cfheaders_db_write(mem, 1, fh), true);
        u_assert_int_eq(mem->file == NULL, 1);
        dogecoin_cfheaders_db_free(mem);
    }

    remove(CFDB_TEST_HEADERS);
    remove(CFDB_TEST_FILTERS);
}

/* A cfheaders batch is only appended if it continues the chain we hold.
   Without this, two getcfheaders in flight -- which the 33s CF timeout causes
   by re-sending getcfcheckpt without cancelling the first -- both append, and
   the same range lands twice. Measured against a peer: a resume from disk tip
   6348646 took a 743-hash batch to 6349389 and then the same range again to
   6350137, 743 past the chain tip, after which every cfilter from 6349390 up
   failed validation. */
void test_cfheaders_batch_extends_tip()
{
    extern dogecoin_bool dogecoin_cfheaders_batch_extends_tip(
        const dogecoin_compact_filter_state *cfstate, const uint8_t *prev_filter_header);

    uint8_t tip[32], other[32];
    memset(tip, 0xAB, sizeof(tip));
    memset(other, 0xCD, sizeof(other));

    dogecoin_compact_filter_state *st = dogecoin_compact_filter_state_new();
    u_assert_true(st != NULL);

    /* Empty chain: the first batch establishes the anchor, so anything goes. */
    u_assert_true(dogecoin_cfheaders_batch_extends_tip(st, tip));
    u_assert_true(dogecoin_cfheaders_batch_extends_tip(st, other));

    /* One header held, tip hash known. */
    uint256_t *fh = dogecoin_calloc(1, sizeof(uint256_t));
    memcpy(fh, tip, 32);
    vector_add(st->filter_headers, fh);
    memcpy(st->cfheaders_tip_hash, tip, 32);

    /* Continues the chain. */
    u_assert_true(dogecoin_cfheaders_batch_extends_tip(st, tip));
    /* A duplicate of an already-applied batch, or a spliced chain, does not. */
    u_assert_true(dogecoin_cfheaders_batch_extends_tip(st, other) == false);

    /* Defensive: no state and no header are both refusals, not crashes. */
    u_assert_true(dogecoin_cfheaders_batch_extends_tip(NULL, tip) == false);
    u_assert_true(dogecoin_cfheaders_batch_extends_tip(st, NULL) == false);

    dogecoin_compact_filter_state_free(st);
}
