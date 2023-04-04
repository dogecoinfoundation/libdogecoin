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

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <dogecoin/utils.h>

/**
 * Create a new logdb_llist_db handle
 * 
 * @return A pointer to a logdb_llist_db struct.
 */
logdb_llist_db* logdb_llist_db_new()
{
    logdb_llist_db* handle = calloc(1, sizeof(logdb_llist_db));
    handle->head = NULL;
    return handle;
}

/**
 * It frees the internal database.
 * 
 * @param ctx The context pointer.
 * 
 * @return The logdb_llist_db structure.
 */
void logdb_llist_db_free(void *ctx)
{
    logdb_record *rec;
    logdb_record *prev_rec;

    logdb_llist_db *handle = (logdb_llist_db *)ctx;
    if (!handle)
        return;

    /* free the internal database */
    rec = handle->head;
    while (rec)
    {
        prev_rec = rec->prev;
        logdb_record_free(rec);
        rec = prev_rec;
    }

    dogecoin_mem_zero(handle, sizeof(*handle));
    free(handle);
}

/**
 * Initialize the logdb_llist_db structure
 * 
 * @param db The logdb_log_db object.
 */
void logdb_llistdb_init(logdb_log_db* db)
{
    logdb_llist_db* handle = logdb_llist_db_new();
    db->cb_ctx = handle;
}

/**
 * If the record is an erase record, remove the record from the linked list. Otherwise, copy the record
 * and append it to the linked list
 * 
 * @param ctx The context pointer.
 * @param load_phase This is a boolean value that indicates whether the current record is being loaded
 * during the load phase or not.
 * @param rec the record to be appended
 * 
 * @return Nothing.
 */
void logdb_llistdb_append(void* ctx, logdb_bool load_phase, logdb_record *rec)
{
    logdb_llist_db *handle = (logdb_llist_db *)ctx;
    logdb_record *rec_dup;
    logdb_record *current_db_head;
    UNUSED(load_phase);

    if (rec->mode == RECORD_TYPE_ERASE && handle->head)
    {
        handle->head = logdb_record_rm_desc(handle->head, rec->key);
        return;
    }

    /* internal database:
       copy record and append to internal mem db (linked list)
    */
    rec_dup = logdb_record_copy(rec);
    current_db_head = handle->head;

    /* if the list is NOT empty, link the current head */
    if (current_db_head != NULL)
        current_db_head->next = rec_dup;

    /* link to previous element */
    rec_dup->prev = current_db_head;

    /* set the current head */
    handle->head = rec_dup;

    logdb_record_rm_desc(current_db_head, rec_dup->key);
}

/**
 * Given a key, find the record in the linked list that matches the key
 * 
 * @param db The logdb_log_db object.
 * @param key The key to search for.
 * 
 * @return A pointer to the record.
 */
cstring * logdb_llistdb_find(logdb_log_db* db, cstring *key)
{
    logdb_llist_db *handle = (logdb_llist_db *)db->cb_ctx;
    return logdb_record_find_desc(handle->head, key);
}

/**
 * Return the height of the linked list.
 * 
 * @param db The database handle.
 * 
 * @return The height of the linked list.
 */
size_t logdb_llistdb_size(logdb_log_db* db)
{
    logdb_llist_db *handle = (logdb_llist_db *)db->cb_ctx;
    return logdb_record_height(handle->head);
}
