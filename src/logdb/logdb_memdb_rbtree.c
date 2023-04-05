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
#include <logdb/logdb_memdb_rbtree.h>
#include <logdb/logdb_rec.h>

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <dogecoin/utils.h>

/**
 * The function is called by the rbtree_free_key function
 * 
 * @param a The key to be freed.
 */
void logdb_rbtree_free_key(void* a) {
    /* key needs no releasing, value contains key */
    UNUSED(a);
}

/**
 * This function is called when a node is deleted from the tree. 
 * 
 * @param a The key to be freed.
 */
void logdb_rbtree_free_value(void *a){
    /* free the record which also frees the key */
    logdb_record *rec = (logdb_record *)a;
    logdb_record_free(rec);
}

/**
 * Given two keys, return a negative number if the first key is less than the second, zero if the two
 * keys are equal, or a positive number if the first key is greater than the second
 * 
 * @param a The first parameter is a pointer to the first key to compare.
 * @param b the key to be compared
 * 
 * @return The return value is the result of the comparison of the two keys.
 */
int logdb_rbtree_IntComp(const void* a,const void* b) {
    return cstr_compare((cstring *)a, (cstring *)b);
}

/**
 * Prints the integer value of the node
 * 
 * @param a The root of the tree to print
 */
void logdb_rbtree_IntPrint(const void* a) {
    printf("%i",*(int*)a);
}

/**
 * Prints the contents of the tree
 * 
 * @param a The root of the tree.
 */
void logdb_rbtree_InfoPrint(void* a) {
    UNUSED(a);
}

/**
 * Creates a new logdb_rbtree_db handle
 * 
 * @return A pointer to a logdb_rbtree_db struct.
 */
logdb_rbtree_db* logdb_rbtree_db_new()
{
    logdb_rbtree_db* handle = calloc(1, sizeof(logdb_rbtree_db));
    handle->tree = RBTreeCreate(logdb_rbtree_IntComp,logdb_rbtree_free_key,logdb_rbtree_free_value,logdb_rbtree_IntPrint,logdb_rbtree_InfoPrint);

    return handle;
}

/**
 * It takes a pointer to a logdb_rbtree_db structure, and frees the memory associated with it
 * 
 * @param ctx The context pointer.
 * 
 * @return Nothing.
 */
void logdb_rbtree_free(void *ctx)
{
    logdb_rbtree_db *handle = (logdb_rbtree_db *)ctx;
    if (!handle)
        return;

    RBTreeDestroy(handle->tree);

    dogecoin_mem_zero(handle, sizeof(*handle));
    free(handle);
}

/**
 * Initialize the logdb_rbtree_db structure
 * 
 * @param db The logdb_log_db object.
 */
void logdb_rbtree_init(logdb_log_db* db)
{
    logdb_rbtree_db* handle = logdb_rbtree_db_new();
    db->cb_ctx = handle;
}

/**
 * This function is called by the logdb_rbtree_db object when it needs to add a record to the tree
 * 
 * @param ctx The context pointer.  This is the pointer to the logdb_rbtree_db structure.
 * @param load_phase This is a boolean value that indicates whether the load phase is
 * @param rec the record to be inserted
 * 
 * @return Nothing.
 */
void logdb_rbtree_append(void* ctx, logdb_bool load_phase, logdb_record *rec)
{
    logdb_record *rec_new;
    /* get the rbtree struct from the context */
    logdb_rbtree_db *handle = (logdb_rbtree_db *)ctx;
    UNUSED(load_phase);
    
    if (!handle)
        return;

    /* remove record if recode mode is ERASE */
    if (rec->mode == RECORD_TYPE_ERASE)
    {
        rb_red_blk_node* node = RBExactQuery(handle->tree, rec->key);
        if (node)
            RBDelete(handle->tree, node);
        return;
    }

    /* copy the record, rbtree does its own mem handling */
    rec_new = logdb_record_copy(rec);

    /* insert the node */
    RBTreeInsert(handle->tree,rec_new->key,rec_new);
}

/**
 * Given a key, find the corresponding value in the logdb
 * 
 * @param db The logdb_log_db object.
 * @param key The key to search for.
 * 
 * @return A pointer to the value of the record.
 */
cstring * logdb_rbtree_find(logdb_log_db* db, cstring *key)
{
    logdb_record *rec_new = 0;
    logdb_rbtree_db* handle = (logdb_rbtree_db *)db->cb_ctx;
    rb_red_blk_node* node = RBExactQuery(handle->tree, key);

    if (node && node->info)
        rec_new = (logdb_record *)node->info;

    if (rec_new)
        return rec_new->value;

    return NULL;
}

/**
 * Return the number of records in the database.
 * 
 * @param db The logdb_log_db object.
 * 
 * @return The number of keys in the tree.
 */
size_t logdb_rbtree_size(logdb_log_db* db)
{
    logdb_rbtree_db* handle = (logdb_rbtree_db *)db->cb_ctx;
    size_t val = rbtree_count(handle->tree);
    return val;
}
