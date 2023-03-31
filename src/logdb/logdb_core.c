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

#include <logdb/logdb.h>
#include <logdb/logdb_memdb_llist.h>
#include <logdb/logdb_memdb_rbtree.h>
#include <dogecoin/common.h>
#include <dogecoin/serialize.h>

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* reduce sha256 hash to 8 bytes for checksum */
#define kLOGDB_DEFAULT_HASH_LEN 16
#define kLOGDB_DEFAULT_VERSION 1

static const unsigned char file_hdr_magic[4] = {0xF9, 0xAA, 0x03, 0xBA}; /* header magic */
static const unsigned char record_magic[8] = {0x88, 0x61, 0xAD, 0xFC, 0x5A, 0x11, 0x22, 0xF8}; /* record magic */

/**
 * Create a new logdb_log_db object and initialize it
 * 
 * @return A pointer to a logdb_log_db struct.
 */
logdb_log_db* logdb_new_internal()
{
    logdb_log_db* db;
    db = calloc(1, sizeof(*db));
    db->mem_mapper = NULL;
    db->cache_head = NULL;
    db->hashlen = kLOGDB_DEFAULT_HASH_LEN;
    db->version = kLOGDB_DEFAULT_VERSION;
    db->support_flags = 0; /* reserved for future changes */
    sha256_init(&db->hashctx);

    return db;
}

/**
 * Create a new logdb_log_db object and return it
 * 
 * @return A pointer to a logdb_log_db object.
 */
logdb_log_db* logdb_new()
{
    logdb_log_db* db = logdb_new_internal();

    /* use a linked list for default memory mapping */
    /* Will be slow */
    logdb_set_memmapper(db, &logdb_llistdb_mapper, NULL);

    return db;
}

/**
 * Create a new logdb_log_db object and return it.
 * 
 * @return A pointer to a logdb_log_db object.
 */
logdb_log_db* logdb_rbtree_new()
{
    logdb_log_db* db = logdb_new_internal();

    /* use a red black tree as default memory mapping */
    logdb_set_memmapper(db, &logdb_rbtree_mapper, NULL);
    return db;
}

/**
 * Create a new transaction
 * 
 * @return A pointer to a logdb_txn struct.
 */
logdb_txn* logdb_txn_new()
{
    logdb_txn* txn;
    txn = calloc(1, sizeof(*txn));
    txn->txn_head = NULL;
    return txn;
}

/**
 * Free the list of records that have not been written to disk
 * 
 * @param txn The transaction record.
 */
void logdb_free_txn_records(logdb_txn* txn)
{
    /* free the unwritten records list */
    logdb_record *rec = txn->txn_head;
    while (rec)
    {
        logdb_record *prev_rec = rec->prev;
        logdb_record_free(rec);
        rec = prev_rec;
    }
    txn->txn_head = NULL;
}

/**
 * Free the memory allocated for the logdb_txn structure
 * 
 * @param txn The transaction to free.
 * 
 * @return Nothing.
 */
void logdb_txn_free(logdb_txn* txn)
{
    if (!txn)
        return;

    logdb_free_txn_records(txn);
    free(txn);
}

/**
 * Set the memory mapper for the log database
 * 
 * @param db The logdb_log_db object.
 * @param mapper The memory mapper to use.
 * @param ctx The context pointer that will be passed to the callback functions.
 */
void logdb_set_memmapper(logdb_log_db* db, logdb_memmapper *mapper, void *ctx)
{
    /* allow the previos memory mapper to do a cleanup */
    if (db->mem_mapper && db->mem_mapper->cleanup_cb)
        db->mem_mapper->cleanup_cb(db->cb_ctx);

    db->mem_mapper = mapper;
    if (db->mem_mapper && db->mem_mapper->init_cb)
        db->mem_mapper->init_cb(db);

    if (ctx)
        db->cb_ctx = ctx;
}

/**
 * Free the cache list
 * 
 * @param db The logdb_log_db object.
 */
void logdb_free_cachelist(logdb_log_db* db)
{
    /* free the unwritten records list */
    logdb_record *rec = db->cache_head;
    while (rec)
    {
        logdb_record *prev_rec = rec->prev;
        logdb_record_free(rec);
        rec = prev_rec;
    }
    db->cache_head = NULL;
}

/**
 * Free the logdb_log_db structure
 * 
 * @param db The logdb_log_db object.
 * 
 * @return Nothing.
 */
void logdb_free(logdb_log_db* db)
{
    if (!db)
        return;

    if (db->file)
    {
        fclose(db->file);
        db->file = NULL;
    }

    logdb_free_cachelist(db);

    /* allow the memory mapper to do a cleanup */
    if (db->mem_mapper && db->mem_mapper->cleanup_cb)
        db->mem_mapper->cleanup_cb(db->cb_ctx);

    free(db);
}

/**
 * Loads the log database from a file
 * 
 * @param handle the logdb_log_db handle
 * @param file_path The path to the log file.
 * @param create If true, the file will be created if it doesn't exist. If false, the file will be
 * opened.
 * @param error pointer to a variable that will be set to the error code if an error occurs.
 * 
 * @return Nothing.
 */
logdb_bool logdb_load(logdb_log_db* handle, const char *file_path, logdb_bool create, enum logdb_error *error)
{
    uint32_t v;
    enum logdb_error record_error;

    handle->file = fopen(file_path, create ? "a+b" : "r+b");
    if (handle->file == NULL)
    {
        if (error != NULL)
            *error = LOGDB_ERROR_FOPEN_FAILED;
        return false;
    }

    /* write header magic */
    if (create)
    {
        /* write header magic, version & support flags */
        fwrite(file_hdr_magic, 4, 1, handle->file);
        v = htole32(handle->version);
        fwrite(&v, sizeof(v), 1, handle->file); /* uint32_t, LE */
        v = htole32(handle->support_flags);
        fwrite(&v, sizeof(v), 1, handle->file); /* uint32_t, LE */
    }
    else
    {
        /* read file magic, version, etc. */
        unsigned char buf[4];
        if (fread(buf, 4, 1, handle->file) != 1 || memcmp(buf, file_hdr_magic, 4) != 0)
        {
            if (error != NULL)
                *error = LOGDB_ERROR_WRONG_FILE_FORMAT;
            return false;
        }

        /* read and set version */
        v = 0;
        if (fread(&v, sizeof(v), 1, handle->file) != 1)
        {
            if (error != NULL)
                *error = LOGDB_ERROR_WRONG_FILE_FORMAT;
            return false;
        }
        handle->version = le32toh(v);

        /* read and set support flags */
        if (fread(&v, sizeof(v), 1, handle->file) != 1)
        {
            if (error != NULL)
                *error = LOGDB_ERROR_WRONG_FILE_FORMAT;
            return false;
        }
        handle->support_flags = le32toh(v);

        logdb_record *rec;
        rec = logdb_record_new();
        while (logdb_record_deser_from_file(rec, handle, &record_error))
        {
            if (record_error != LOGDB_SUCCESS)
                break;

            /* if a memory mapping function was provided,
             pass the record together with the context to
             this function.
             */
            if (handle->mem_mapper && handle->mem_mapper->append_cb)
                handle->mem_mapper->append_cb(handle->cb_ctx, true, rec);
        }
        logdb_record_free(rec);

        if (record_error != LOGDB_SUCCESS)
        {
            if (error)
                *error = record_error;
            return false;
        }
    }

    return true;
}

/**
 * Flush the cache list to the file
 * 
 * @param db The database to flush.
 * 
 * @return Nothing.
 */
logdb_bool logdb_flush(logdb_log_db* db)
{
    logdb_record *flush_rec;

    if (!db->file)
        return false;

    flush_rec = db->cache_head;

    /* search deepest non written record */
    while (flush_rec != NULL)
    {
        if (flush_rec->written == true)
        {
            flush_rec = flush_rec->next;
            break;
        }

        if (flush_rec->prev != NULL)
            flush_rec = flush_rec->prev;
        else
            break;
    }

    /* write records */
    while (flush_rec != NULL)
    {
        logdb_write_record(db, flush_rec);
        flush_rec->written = true;
        flush_rec = flush_rec->next;
    }

    /*reset cache list
      no need to longer cache the written records
    */
    logdb_free_cachelist(db);

    return true;
}

/**
 * Given a logdb_log_db and a logdb_txn, and a key, append a delete-mode record to the logdb_log_db
 * 
 * @param db The logdb_log_db object.
 * @param txn The transaction that is being committed.
 * @param key The key to delete.
 * 
 * @return Nothing
 */
void logdb_delete(logdb_log_db* db, logdb_txn *txn, cstring *key)
{
    if (key == NULL)
        return;

    /* A NULL value will result in a delete-mode record */
    logdb_append(db, txn, key, NULL);
}

/**
 * Append a record to the end of the linked list
 * 
 * @param db The logdb_log_db object.
 * @param txn The transaction object.
 * @param key The key to be added to the log.
 * @param val the value to be appended to the log
 * 
 * @return Nothing.
 */
void logdb_append(logdb_log_db* db, logdb_txn *txn, cstring *key, cstring *val)
{
    logdb_record *rec;
    logdb_record *current_head;

    if (key == NULL)
        return;
    
    rec = logdb_record_new();
    logdb_record_set(rec, key, val);
    if (txn)
        current_head = txn->txn_head;
    else
        current_head = db->cache_head;

    /* if the list is NOT empty, link the current head */
    if (current_head != NULL)
        current_head->next = rec;

    /* link to previous element */
    rec->prev = current_head;

    /* set the current head */
    if (txn)
        txn->txn_head = rec;
    else
        db->cache_head = rec;

    /* update mem mapped database (only non TXN) */
    if (db->mem_mapper && db->mem_mapper->append_cb &&!txn)
        db->mem_mapper->append_cb(db->cb_ctx, false, rec);
}

/**
 * Given a transaction, write all the records in the transaction to the log
 * 
 * @param db The database to write to.
 * @param txn The transaction object.
 */
void logdb_txn_commit(logdb_log_db* db, logdb_txn *txn)
{
    logdb_record *work_head = txn->txn_head;
    /* search deepest non written record */
    while (work_head != NULL)
    {
        if (work_head->prev != NULL)
            work_head = work_head->prev;
        else
            break;
    }
    /* write records */
    while (work_head != NULL)
    {
        logdb_append(db, NULL, work_head->key, work_head->value);
        work_head = work_head->next;
    }
}

/**
 * Given a logdb_log_db* and a cstring*, return the cstring* of the record in the cache that matches
 * the key
 * 
 * @param db The logdb_log_db object.
 * @param key The key to search for.
 * 
 * @return A pointer to the record.
 */
cstring * logdb_find_cache(logdb_log_db* db, cstring *key)
{
    return logdb_record_find_desc(db->cache_head, key);
}

/**
 * Given a key, find the value in the database
 * 
 * @param db The logdb_log_db object.
 * @param key The key to search for.
 * 
 * @return A pointer to a cstring.
 */
cstring * logdb_find(logdb_log_db* db, cstring *key)
{
    if (db->mem_mapper && db->mem_mapper->find_cb)
        return db->mem_mapper->find_cb(db, key);

    return NULL;
}

/**
 * Return the number of keys in the database.
 * 
 * @param db The logdb_log_db object.
 * 
 * @return The number of keys in the database.
 */
size_t logdb_count_keys(logdb_log_db* db)
{
    if (db->mem_mapper && db->mem_mapper->size_cb)
        return db->mem_mapper->size_cb(db);

    return 0;
}

/**
 * Return the height of the cache.
 * 
 * @param db the logdb_log_db object
 * 
 * @return The height of the cache.
 */
size_t logdb_cache_size(logdb_log_db* db)
{
    return logdb_record_height(db->cache_head);
}

/**
 * Write a record to the log database
 * 
 * @param db the log database
 * @param rec the record to write
 * 
 * @return Nothing.
 */
logdb_bool logdb_write_record(logdb_log_db* db, logdb_record *rec)
{
    sha256_context ctx = db->hashctx;
    sha256_context ctx_final;
    uint8_t hash[SHA256_DIGEST_LENGTH];

    /* serialize record to buffer */
    cstring *serbuf = cstr_new_sz(1024);
    logdb_record_ser(rec, serbuf);

    /* create hash of the body */
    sha256_raw((const uint8_t*)serbuf->str, serbuf->len, hash);

    /* write record header */
    if (fwrite(record_magic, 8, 1, db->file) != 1) {
        cstr_free(serbuf, true);
        return false;
    }
    sha256_write(&ctx, record_magic, 8);

    /* write partial hash as body checksum&indicator (body start) */
    if (fwrite(hash, db->hashlen, 1, db->file) != 1) {
        cstr_free(serbuf, true);
        return false;
    }
    sha256_write(&ctx, hash, db->hashlen);

    /* write the body */
    fwrite(serbuf->str, serbuf->len, 1, db->file);
    sha256_write(&ctx, (uint8_t *)serbuf->str, serbuf->len);

    /* write partial hash as body checksum&indicator (body end) */
    if (fwrite(hash, db->hashlen, 1, db->file) != 1) {
        cstr_free(serbuf, true);
        return false;
    }
    sha256_write(&ctx, hash, db->hashlen);
    
    cstr_free(serbuf, true);

    ctx_final = ctx;
    sha256_finalize(&ctx_final, hash);
    if (fwrite(hash, db->hashlen, 1, db->file) != 1)
        return false;
    db->hashctx = ctx;

    return true;
}

/**
 * Reads a record from the file, and returns a pointer to the record
 * 
 * @param rec the record to be written
 * @param db The logdb_log_db object that we are using to read the file.
 * @param error pointer to an enum logdb_error.
 * 
 * @return Nothing.
 */
logdb_bool logdb_record_deser_from_file(logdb_record* rec, logdb_log_db *db, enum logdb_error *error)
{
    uint32_t len = 0;
    sha256_context ctx = db->hashctx; /* prepare a copy of context that allows rollback */
    sha256_context ctx_final;
    uint8_t magic_buf[8];
    uint8_t hashcheck[SHA256_DIGEST_LENGTH];
    unsigned char check[SHA256_DIGEST_LENGTH];

    /* prepate a buffer for the varint data (max 4 bytes) */
    size_t buflen = sizeof(uint32_t);
    uint8_t readbuf[sizeof(uint32_t)];

    *error = LOGDB_SUCCESS;

    /* read record magic */
    if (fread(magic_buf, 8, 1, db->file) != 1)
    {
        /* very likely end of file reached */
        return false;
    }
    sha256_write(&ctx, magic_buf, 8);

    /* read start hash/magic per record */
    if (fread(hashcheck, db->hashlen, 1, db->file) != 1)
    {
        *error = LOGDB_ERROR_DATASTREAM_ERROR;
        return false;
    }
    sha256_write(&ctx, hashcheck, db->hashlen);

    /* read record mode (write / delete) */
    if (fread(&rec->mode, 1, 1, db->file) != 1)
    {
        *error = LOGDB_ERROR_DATASTREAM_ERROR;
        return false;
    }

    sha256_write(&ctx, (const uint8_t *)&rec->mode, 1);

    /* key */
    if (!deser_varlen_file(&len, db->file, readbuf, &buflen))
    {
        *error = LOGDB_ERROR_DATASTREAM_ERROR;
        return false;
    }

    sha256_write(&ctx, readbuf, buflen);

    cstr_resize(rec->key, len);
    if (fread(rec->key->str, 1, len, db->file) != len)
    {
        *error = LOGDB_ERROR_DATASTREAM_ERROR;
        return false;
    }

    sha256_write(&ctx, (const uint8_t *)rec->key->str, len);

    if (rec->mode == RECORD_TYPE_WRITE)
    {
        /* read value (not for delete mode) */
        buflen = sizeof(uint32_t);
        if (!deser_varlen_file(&len, db->file, readbuf, &buflen))
        {
            *error = LOGDB_ERROR_DATASTREAM_ERROR;
            return false;
        }

        sha256_write(&ctx, readbuf, buflen);

        cstr_resize(rec->value, len);
        if (fread(rec->value->str, 1, len, db->file) != len)
        {
            *error = LOGDB_ERROR_DATASTREAM_ERROR;
            return false;
        }

        sha256_write(&ctx, (const uint8_t *)rec->value->str, len);
    }

    /* read start hash/magic per record */
    if (fread(hashcheck, db->hashlen, 1, db->file) != 1)
    {
        /* very likely end of file reached */
        *error = LOGDB_ERROR_DATASTREAM_ERROR;
        return false;
    }
    sha256_write(&ctx, hashcheck, db->hashlen);

    /* generate final checksum in a context copy */
    ctx_final = ctx;
    sha256_finalize(&ctx_final, hashcheck);

    /* read checksum from file, compare */
    if (fread(check, 1, db->hashlen, db->file) != db->hashlen)
    {
        *error = LOGDB_ERROR_DATASTREAM_ERROR;
        return false;
    }

    if (memcmp(hashcheck,check,(size_t)db->hashlen) != 0)
    {
        *error = LOGDB_ERROR_CHECKSUM;
        return false;
    }

    /* mark record as written because we have
      just loaded it from disk */
    rec->written = true;

    /* update sha256 context */
    db->hashctx = ctx;
    return true;
}
