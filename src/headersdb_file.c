/*

 The MIT License (MIT)

 Copyright (c) 2017 Jonas Schnelli
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

#include <inttypes.h>
#include <sys/stat.h>

#include <dogecoin/headersdb_file.h>
#include <dogecoin/blockchain.h>
#include <dogecoin/common.h>
#include <dogecoin/pow.h>
#include <dogecoin/serialize.h>
#include <dogecoin/utils.h>
#include <dogecoin/validation.h>

static const unsigned char file_hdr_magic[4] = {0xA8, 0xF0, 0x11, 0xC5}; /* header magic */
static const uint32_t current_version = 3; /* 3: added chainwork */

/**
 * "Compare two block headers by their hashes."
 *
 * The function takes two block headers as arguments, and returns 0 if the two headers are identical,
 * and -1 if the first header is less than the second header
 *
 * @param l the first pointer
 * @param r The right-hand side of the comparison.
 *
 * @return Nothing.
 */
int dogecoin_header_compare(const void *l, const void *r)
{
    const dogecoin_blockindex *lm = l;
    const dogecoin_blockindex *lr = r;

    uint8_t *hashA = (uint8_t *)lm->hash;
    uint8_t *hashB = (uint8_t *)lr->hash;

    unsigned int i;
    for (i = 0; i < sizeof(uint256_t); i++) {
        uint8_t iA = hashA[i];
        uint8_t iB = hashB[i];
        if (iA > iB)
            return -1;
        else if (iA < iB)
            return 1;
    }

    return 0;
}

/**
 * The function creates a new dogecoin_headers_db object and initializes it
 *
 * @param chainparams The chainparams struct that contains the genesis block hash.
 * @param inmem_only If true, the database will be in-memory only. If false, it will be on disk.
 *
 * @return Nothing.
 */
dogecoin_headers_db* dogecoin_headers_db_new(const dogecoin_chainparams* chainparams, dogecoin_bool inmem_only) {
    dogecoin_headers_db* db;
    db = dogecoin_calloc(1, sizeof(*db));
    db->read_write_file = !inmem_only;
    db->use_binary_tree = true;
    db->max_hdr_in_mem = 1440;
    db->params = chainparams;
    db->genesis.height = 0;
    db->genesis.prev = NULL;
    memcpy_safe(db->genesis.hash, chainparams->genesisblockhash, DOGECOIN_HASH_LENGTH);
    uint_to_arith(&db->genesis.chainwork, &chainparams->genesisblockchainwork);
    db->chaintip = &db->genesis;
    db->chainbottom = &db->genesis;

    if (db->use_binary_tree) {
        db->tree_root = 0;
    }

    return db;
}

/**
 * @param db The database object.
 *
 * @return Nothing
 */
void dogecoin_headers_db_free(dogecoin_headers_db* db) {

    if (!db)
        return;

    if (db->headers_tree_file)
    {
        fclose(db->headers_tree_file);
        db->headers_tree_file = NULL;
    }

    if (db->tree_root) {
        dogecoin_btree_tdestroy(db->tree_root, dogecoin_free);
        db->tree_root = NULL;
    }

    db->chaintip = NULL;
    db->chainbottom = NULL;

    dogecoin_free(db);
}

/**
 * Loads the headers database from disk
 *
 * @param db the headers database object
 * @param file_path The path to the headers database file. If NULL, the default path is used.
 * @param prompt If true, the user will be prompted to confirm loading the database.
 *
 * @return The return value is a boolean value that indicates whether the database was successfully
 * opened.
 */
dogecoin_bool dogecoin_headers_db_load(dogecoin_headers_db* db, const char *file_path, dogecoin_bool prompt) {

    if (!db->read_write_file) {
        return 1;
    }

    char *file_path_local = (char *)file_path;
    cstring *path_ret = cstr_new_sz(1024);
    if (!file_path)
    {
        dogecoin_get_default_datadir(path_ret);
        char *filename = "/headers.db";
        cstr_append_buf(path_ret, filename, strlen(filename));
        cstr_append_c(path_ret, 0);
        file_path_local = path_ret->str;
    }

    struct stat buffer;
    dogecoin_bool create = true;
    if (stat(file_path_local, &buffer) == 0) {
        create = false; // Set create to false as file already exists

        if (prompt) {
            printf("\nLoad %s? (Enter) or (o)verwrite:", file_path_local);
            char response[MAX_LEN];
            if (!fgets(response, MAX_LEN, stdin)) {
                printf("Error reading input.\n");
                return false;
            }
            if (response[0] == 'o' || response[0] == 'O') {
                printf("Are you sure? (y/n): \n");
                char confirm[MAX_LEN];
                if (!fgets(confirm, MAX_LEN, stdin)) {
                    printf("Error reading input.\n");
                    return false;
                }
                if (confirm[0] == 'y' || confirm[0] == 'Y') {
                    /* No remove() here: "w+b" below truncates, and removing
                       first is a check-then-use an attacker with write access
                       to the directory wins by planting a symlink. */
                    create = true;
                }
            }
        }
    }

    db->headers_tree_file = fopen(file_path_local, create ? "w+b" : "r+b");
    cstr_free(path_ret, true);
    if (create) {
        // write file-header-magic
        fwrite(file_hdr_magic, 4, 1, db->headers_tree_file);
        uint32_t v = htole32(current_version);
        fwrite(&v, sizeof(v), 1, db->headers_tree_file); /* uint32_t, LE */
    } else {
        // check file-header-magic
        uint8_t buf[sizeof(file_hdr_magic)+sizeof(current_version)];
        if ((uint32_t)buffer.st_size < (uint32_t)(sizeof(file_hdr_magic)+sizeof(current_version)) ||
             fread(buf, sizeof(file_hdr_magic)+sizeof(current_version), 1, db->headers_tree_file) != 1 ||
             memcmp(buf, file_hdr_magic, sizeof(file_hdr_magic)))
        {
            fprintf(stderr, "Error reading database file\n");
            fclose(db->headers_tree_file);
            db->headers_tree_file = NULL;
            return false;
        }
        if (le32toh(*(buf+sizeof(file_hdr_magic))) > current_version) {
            fprintf(stderr, "Unsupported file version\n");
            fclose(db->headers_tree_file);
            db->headers_tree_file = NULL;
            return false;
        }
    }
    dogecoin_bool firstblock = true;
    size_t connected_headers_count = 0;
    if (db->headers_tree_file && !create)
    {
        printf("Loading headers from disk...\n");
        while (!feof(db->headers_tree_file))
        {
            if (connected_headers_count % 100000 == 0 && connected_headers_count > 0)
            {
                printf("\r%zu headers loaded", connected_headers_count);
                fflush(stdout);
            }

            uint8_t buf_all[SPV_HEADERS_FILE_REC_LEN];
            if (fread(buf_all, sizeof(buf_all), 1, db->headers_tree_file) == 1) {
                struct const_buffer cbuf_all = {buf_all, sizeof(buf_all)};

                uint256_t hash;
                uint32_t height;
                arith_uint256 chainwork = {0};
                deser_u256(hash, &cbuf_all);
                deser_u32(&height, &cbuf_all);
                deser_u256((uint8_t*)chainwork.pn, &cbuf_all);
                dogecoin_bool connected;
                if (firstblock)
                {
                    dogecoin_blockindex *chainheader = dogecoin_calloc(1, sizeof(dogecoin_blockindex));
                    chainheader->height = height;
                    if (!dogecoin_block_header_deserialize(&chainheader->header, &cbuf_all, db->params, &chainheader->chainwork)) {
                        dogecoin_block_header_free(&chainheader->header);
                        dogecoin_free(chainheader);
                        fprintf(stderr, "\nError: Invalid data found.\n");
                        return -1;
                    }
                    dogecoin_block_header_hash(&chainheader->header, (uint8_t *)&chainheader->hash);
                    chainheader->prev = NULL;
                    db->chaintip = chainheader;
                    if (db->use_binary_tree) {
                        dogecoin_btree_tsearch(chainheader, &db->tree_root, dogecoin_header_compare);
                    }
                    firstblock = false;
                } else {
                    dogecoin_blockindex *pindex = dogecoin_headers_db_connect_hdr(db, &cbuf_all, true, &connected);
                    if (!connected)
                    {
                        printf("\nConnecting header %s failed (at height: %" PRIu32 ") read_write: %d\n", hash_to_string(hash), db->chaintip->height, db->read_write_file);
                        dogecoin_block_header_destroy(&pindex->header);
                        dogecoin_free(pindex);
                    }
                    else {
                        connected_headers_count++;
                    }
                }
                db->chaintip->chainwork = chainwork;
            }
        }
    }
    printf("\nConnected %zu headers, now at height: %" PRIu32 "\n", connected_headers_count, db->chaintip->height);
    return (db->headers_tree_file != NULL);
}

/**
 * Open an existing headers DB file for sequential hash lookup only.
 *
 * Unlike dogecoin_headers_db_load(), this function does NOT read any block
 * records or build the in-memory tree.  It simply opens the file, verifies
 * the 8-byte magic+version header, and leaves the file handle ready for
 * dogecoin_headers_db_get_block_hash_at_height().  Use this for the aux
 * hash-lookup DB in BIP157 genesis filter IBD — no tree needed.
 */
dogecoin_bool dogecoin_headers_db_open_for_scan(dogecoin_headers_db *db, const char *filename)
{
    if (!db || !filename) return false;

    db->headers_tree_file = fopen(filename, "rb");
    if (!db->headers_tree_file) {
        fprintf(stderr, "headers_db: cannot open '%s' for scan: %s\n", filename, strerror(errno));
        return false;
    }

    uint8_t buf[sizeof(file_hdr_magic) + sizeof(current_version)];
    if (fread(buf, sizeof(buf), 1, db->headers_tree_file) != 1 ||
        memcmp(buf, file_hdr_magic, sizeof(file_hdr_magic)) != 0) {
        fprintf(stderr, "headers_db: bad magic in '%s'\n", filename);
        fclose(db->headers_tree_file);
        db->headers_tree_file = NULL;
        return false;
    }
    if (le32toh(*(uint32_t *)(buf + sizeof(file_hdr_magic))) > current_version) {
        fprintf(stderr, "headers_db: unsupported version in '%s'\n", filename);
        fclose(db->headers_tree_file);
        db->headers_tree_file = NULL;
        return false;
    }
    return true;
}

/**
 * The function takes a block index and writes it to the headers database
 *
 * @param db the headers database
 * @param blockindex The block index to write to the database.
 *
 * @return Nothing.
 */
dogecoin_bool dogecoin_headers_db_write(dogecoin_headers_db* db, dogecoin_blockindex *blockindex) {
    cstring *rec = cstr_new_sz(SPV_HEADERS_FILE_REC_LEN); // hash + height + chainwork + header
    ser_u256(rec, blockindex->hash);
    ser_u32(rec, blockindex->height);
    ser_u256(rec, arith_to_uint256(&blockindex->chainwork));
    dogecoin_block_header_serialize(rec, &blockindex->header);
    /* The file is an append-only log, but reads share this handle and leave the
       cursor wherever they stopped. Only a freshly created DB gets "a+b", which
       forces writes to the end; a reopened one gets "r+b", where a write lands
       at the cursor and overwrites live records. Seek explicitly so no reader's
       position can decide where a header goes. */
    fseek(db->headers_tree_file, 0, SEEK_END);
    size_t res = fwrite(rec->str, rec->len, 1, db->headers_tree_file);
    if (!db->batch_write)
        dogecoin_file_commit(db->headers_tree_file);
    cstr_free(rec, true);
    return (res == 1);
}

/**
 * The function takes a pointer to a blockindex and checks if the block is in the blockchain. If it is,
 * it returns the pointer to the block. If it isn't, it returns a null pointer
 *
 * @param db the database object
 * @param buf The buffer containing the block header.
 * @param load_process If true, the header will be loaded into the database. If false, it will only be
 * added to the tree.
 * @param connected A pointer to a boolean that will be set to true if the block was successfully
 * connected to the chain.
 *
 * @return A pointer to the blockindex.
 */
dogecoin_blockindex * dogecoin_headers_db_connect_hdr(dogecoin_headers_db* db, struct const_buffer *buf, dogecoin_bool load_process, dogecoin_bool *connected) {
    *connected = false;

    dogecoin_blockindex *blockindex = dogecoin_calloc(1, sizeof(dogecoin_blockindex));
    if (!dogecoin_block_header_deserialize(&blockindex->header, buf, db->params, &blockindex->chainwork))
    {
        fprintf(stderr, "Error deserializing block header\n");
        return blockindex;
    }

    dogecoin_block_header_hash(&blockindex->header, (uint8_t *)&blockindex->hash);

    // Check if the block header is already in the database
    dogecoin_blockindex *block = dogecoin_headersdb_find(db, blockindex->hash);
    if (block) {
        // Block header already in database, return blockindex
        return blockindex;
    }

    dogecoin_blockindex *connect_at = NULL;
    dogecoin_blockindex *fork_from_block = NULL;

    if (memcmp(&blockindex->header.prev_block, db->chaintip->hash, DOGECOIN_HASH_LENGTH) == 0)
    {
        connect_at = db->chaintip;
    }
    else {
        // check if we know the prevblock
        fork_from_block = dogecoin_headersdb_find(db, blockindex->header.prev_block);
        if (fork_from_block) {
            connect_at = fork_from_block;
        }
    }

    if (connect_at != NULL) {
        // Check the proof of work
        if (!is_auxpow(blockindex->header.version)) {
            if (!db->skip_pow) {
                uint256_t hash = {0};
                cstring* s = cstr_new_sz(64);
                dogecoin_block_header_serialize(s, (const dogecoin_block_header*) &blockindex->header);
                dogecoin_block_header_scrypt_hash(s, &hash);
                cstr_free(s, true);
                if (!check_pow(&hash, blockindex->header.bits, db->params, &blockindex->chainwork)) {
                    printf("%s:%d:%s : non-AUX proof of work failed : %s\n", __FILE__, __LINE__, __func__, strerror(errno));
                    return blockindex;
                }
            }
        }

        blockindex->prev = connect_at;
        blockindex->height = connect_at->height+1;

        arith_uint256* added_chainwork = add_arith_uint256(&connect_at->chainwork, &blockindex->chainwork);
        blockindex->chainwork = *added_chainwork;

        // Chain reorganization if necessary
        if (fork_from_block && blockindex->height > db->chaintip->height &&
            (arith_uint256_greater_than(added_chainwork, &db->chaintip->chainwork) ||
             (arith_uint256_equal(added_chainwork, &db->chaintip->chainwork) && blockindex->header.timestamp > db->chaintip->header.timestamp))) {

            // Identify the common ancestor
            dogecoin_blockindex* common_ancestor = db->chaintip;
            dogecoin_blockindex* fork_chain = blockindex;

            // Find the common ancestor by comparing hashes
            while (common_ancestor && fork_chain && memcmp(common_ancestor->hash, fork_chain->hash, DOGECOIN_HASH_LENGTH) != 0) {
                if (common_ancestor->height > fork_chain->height) {
                    common_ancestor = common_ancestor->prev;
                } else {
                    fork_chain = fork_chain->prev;
                }

                // Break the loop if either reaches the start of the chain
                if (!common_ancestor || !fork_chain) {
                    fprintf(stderr, "Unable to find common ancestor.\n");
                    dogecoin_free(added_chainwork);
                    return blockindex;
                }
            }

            // Disconnect blocks from the current chain
            while (memcmp(db->chaintip->hash, common_ancestor->hash, DOGECOIN_HASH_LENGTH) != 0) {
                dogecoin_headersdb_disconnect_tip(db);
            }

            // Connect blocks from the new chain
            dogecoin_blockindex* current_block = blockindex;
            while (current_block && memcmp(current_block->hash, common_ancestor->hash, DOGECOIN_HASH_LENGTH) != 0) {
                // current_block->prev points to the previous block in the chain
                dogecoin_blockindex* prev_block = current_block->prev;

                if (!prev_block) {
                    fprintf(stderr, "Previous block in the chain not found.\n");

                    // Add the current_block to the tree if it's not already part of the main chain
                    if (db->use_binary_tree && current_block != blockindex) {
                        fprintf(stderr, "Adding block to tree.\n");
                        dogecoin_btree_tsearch(current_block, &db->tree_root, dogecoin_header_compare);
                    }

                    // Free the dynamically allocated memory
                    dogecoin_free(added_chainwork);
                    return blockindex;
                }

                // Update the chain tip to the previous block
                db->chaintip = prev_block;
                current_block = prev_block;
            }

            printf("\nChain reorganization: %" PRIu32 " blocks disconnected, %" PRIu32 " blocks connected\n", blockindex->height - common_ancestor->height, blockindex->height - db->chaintip->height);

            // Set the new block as the new chain tip
            db->chaintip = blockindex;
        }
        else if (blockindex->height > db->chaintip->height) {
            db->chaintip = blockindex;
        }

        // Free the dynamically allocated memory
        dogecoin_free(added_chainwork);

        if (!load_process && db->read_write_file)
        {
            if (!dogecoin_headers_db_write(db, blockindex)) {
                fprintf(stderr, "Error writing blockheader to database\n");
            }
        }
        if (db->use_binary_tree) {
            dogecoin_btree_tsearch(blockindex, &db->tree_root, dogecoin_header_compare);
        }

        if (!load_process && db->max_hdr_in_mem > 0) {
            // de-allocate no longer required headers
            // keep them only on-disk
            dogecoin_blockindex *scan_tip = db->chaintip;
            unsigned int i;
            /* scan_tip was dereferenced below before the `scan_tip &&` check
               further down had a chance to run. chaintip is NULL until the
               first header is connected. */
            for (i = 0; scan_tip && i < db->max_hdr_in_mem + 1; i++)
            {
                if (scan_tip->prev) {
                    scan_tip = scan_tip->prev;
                } else {
                    break;
                }

                if (scan_tip && i == db->max_hdr_in_mem && scan_tip != &db->genesis) {
                    if (scan_tip->prev && scan_tip->prev != &db->genesis) {
                        dogecoin_btree_tdelete(scan_tip->prev, &db->tree_root, dogecoin_header_compare);
                        dogecoin_free(scan_tip->prev);
                        scan_tip->prev = NULL;
                        db->chainbottom = scan_tip;
                    }
                }
            }
        }
        *connected = true;
        return blockindex;
    }
    return blockindex;
}

/**
 * The function iterates through the chain tip and adds the hash of each block to the blocklocators
 * vector_t
 *
 * @param db the database object
 * @param blocklocators a vector_t of block hashes
 */
void dogecoin_headers_db_fill_block_locator(dogecoin_headers_db* db, vector_t *blocklocators)
{
    dogecoin_blockindex *scan_tip = db->chaintip;
    if (scan_tip->height > 0)
    {
        int i = 0;
        for(; i<10;i++)
        {
            //TODO: try to share memory and avoid heap allocation
            uint256_t *hash = dogecoin_calloc(1, sizeof(uint256_t));
            memcpy_safe(hash, scan_tip->hash, sizeof(uint256_t));
            vector_add(blocklocators, (void *)hash);

            if (scan_tip->prev)
                scan_tip = scan_tip->prev;
            else
                break;
        }
    }
}

/**
 * The function takes a hash and returns the blockindex with that hash
 *
 * @param db The headers database.
 * @param hash The hash of the block header to find.
 *
 * @return A pointer to the blockindex.
 */
dogecoin_blockindex * dogecoin_headersdb_find(dogecoin_headers_db* db, uint256_t hash) {
    if (db->use_binary_tree)
    {
        dogecoin_blockindex *blockindex = dogecoin_calloc(1, sizeof(dogecoin_blockindex));
        memcpy_safe(blockindex->hash, hash, sizeof(uint256_t));
        dogecoin_blockindex *blockindex_f = dogecoin_btree_tfind(blockindex, &db->tree_root, dogecoin_header_compare); /* read */
        if (blockindex_f) {
            blockindex_f = *(dogecoin_blockindex **)blockindex_f;
        }
        dogecoin_block_header_destroy(&blockindex->header);
        dogecoin_free(blockindex);
        return blockindex_f;
    }
    return NULL;
}

/**
 * Get the block index of the current tip of the main chain
 *
 * @param db The headers database.
 *
 * @return The current tip of the blockchain.
 */
dogecoin_blockindex * dogecoin_headersdb_getchaintip(dogecoin_headers_db* db) {
    return db->chaintip;
}

/**
 * If the chaintip is not null, then set the chaintip to the previous block and return true. Otherwise
 * return false
 *
 * @param db The headers database.
 *
 * @return A boolean value.
 */
dogecoin_bool dogecoin_headersdb_disconnect_tip(dogecoin_headers_db* db) {
    if (db->chaintip->prev)
    {
        dogecoin_blockindex *oldtip = db->chaintip;
        db->chaintip = db->chaintip->prev;
        dogecoin_btree_tdelete(oldtip, &db->tree_root, dogecoin_header_compare);
        dogecoin_free(oldtip);
        return true;
    }
    return false;
}

/**
 * "Check if the headers database has a checkpoint start."
 *
 * The function returns a bool value
 *
 * @param db The headers database.
 *
 * @return A boolean value.
 */
dogecoin_bool dogecoin_headersdb_has_checkpoint_start(dogecoin_headers_db* db) {
    return (db->chainbottom->height != 0);
}

/**
 * Find a block hash by height, first in the in-memory prev chain and then by
 * scanning the on-disk header file.  Used by the BIP157 cfheaders batcher to
 * compute a valid stop_hash without needing a height-indexed block index.
 */
dogecoin_bool dogecoin_headers_db_get_block_hash_at_height(dogecoin_headers_db *db, uint32_t target_height, uint256_t hash_out)
{
    if (!db) return false;

    /* Fast path: walk the in-memory prev chain (works for the last max_hdr_in_mem entries) */
    dogecoin_blockindex *bi = db->chaintip;
    while (bi && (uint32_t)bi->height > target_height)
        bi = bi->prev;
    if (bi && (uint32_t)bi->height == target_height) {
        memcpy_safe(hash_out, bi->hash, sizeof(uint256_t));
        return true;
    }

    /* Slow path: scan the on-disk file.
     * Records are written in ascending height order (one getheaders batch at a time).
     * Record layout: hash(32) + height(4 LE) + chainwork(32) + header(80) = 148 bytes.
     *
     * cfheaders batches request heights in ascending order (2000, 4000, ...).
     * Scanning from offset 0 each time would be O(N²) over a 6M-block file.
     * Instead, resume from the last scan position when target_height is beyond
     * what was found before; reset to the beginning only when scanning backwards. */
    if (!db->headers_tree_file) return false;

    long saved_pos = ftell(db->headers_tree_file);

    long start_pos = SPV_HEADERS_FILE_HDR_LEN;
    if (db->scan_resume_height > 0 && target_height >= db->scan_resume_height)
        start_pos = db->scan_resume_pos; /* continue forward from last find */

    fseek(db->headers_tree_file, start_pos, SEEK_SET);

    uint8_t rec[SPV_HEADERS_FILE_REC_LEN];
    dogecoin_bool found = false;
    while (fread(rec, SPV_HEADERS_FILE_REC_LEN, 1, db->headers_tree_file) == 1) {
        uint32_t h;
        memcpy(&h, rec + 32, 4); /* height field is at offset 32 (after the hash) */
        h = le32toh(h);
        if (h == target_height) {
            memcpy_safe(hash_out, rec, sizeof(uint256_t));
            /* Remember this position so the next forward lookup can skip ahead. */
            /* Start of the matched record, not the end of it. Storing the end
               made a repeat lookup of the same height resume past its own
               record -- the test is target_height >= scan_resume_height -- so
               it scanned to EOF and reported not found. The second lookup of
               any height failed, which is what drove the getcfilters stop-hash
               fallback and the oversized request behind it. */
            db->scan_resume_pos    = ftell(db->headers_tree_file) - SPV_HEADERS_FILE_REC_LEN;
            db->scan_resume_height = h;
            found = true;
            break;
        }
    }

    fseek(db->headers_tree_file, saved_pos, SEEK_SET);
    return found;
}

/* Sequential variant used during the cfilter rescan: like
 * dogecoin_headers_db_get_block_hash_at_height but never restores the file
 * position.  The caller (spv_rescan_cb) visits heights in strictly ascending
 * order so the file pointer advances forward only, keeping I/O O(N) instead
 * of O(N²) that the save/restore pattern would produce. */
dogecoin_bool dogecoin_headers_db_get_block_hash_at_height_seq(dogecoin_headers_db *db, uint32_t target_height, uint256_t hash_out)
{
    if (!db) return false;

    /* Fast path: in-memory prev chain — only useful for heights near the tip.
     * Skip if target is more than max_hdr_in_mem below the tip to avoid
     * walking the full 1440-entry chain 6M+ times during a bulk rescan. */
    if (db->chaintip &&
        (uint32_t)db->chaintip->height <= target_height + db->max_hdr_in_mem) {
        dogecoin_blockindex *bi = db->chaintip;
        while (bi && (uint32_t)bi->height > target_height)
            bi = bi->prev;
        if (bi && (uint32_t)bi->height == target_height) {
            memcpy_safe(hash_out, bi->hash, sizeof(uint256_t));
            return true;
        }
    }

    if (!db->headers_tree_file) return false;

    long start_pos = SPV_HEADERS_FILE_HDR_LEN;
    if (db->scan_resume_height > 0 && target_height >= db->scan_resume_height)
        start_pos = db->scan_resume_pos;

    fseek(db->headers_tree_file, start_pos, SEEK_SET);

    uint8_t rec[SPV_HEADERS_FILE_REC_LEN];
    while (fread(rec, SPV_HEADERS_FILE_REC_LEN, 1, db->headers_tree_file) == 1) {
        uint32_t h;
        memcpy(&h, rec + 32, 4);
        h = le32toh(h);
        if (h == target_height) {
            memcpy_safe(hash_out, rec, sizeof(uint256_t));
            /* Start of the matched record, not the end of it. Storing the end
               made a repeat lookup of the same height resume past its own
               record -- the test is target_height >= scan_resume_height -- so
               it scanned to EOF and reported not found. The second lookup of
               any height failed, which is what drove the getcfilters stop-hash
               fallback and the oversized request behind it. */
            db->scan_resume_pos    = ftell(db->headers_tree_file) - SPV_HEADERS_FILE_REC_LEN;
            db->scan_resume_height = h;
            return true;
        }
    }
    return false;
}

/**
 * Set the checkpoint block to the given hash and height
 *
 * @param db The headers database.
 * @param hash The hash of the block that is the checkpoint.
 * @param height The height of the block that this is a checkpoint for.
 * @param chainwork The chainwork of the block that this is a checkpoint for.
 */
void dogecoin_headersdb_set_checkpoint_start(dogecoin_headers_db* db, uint256_t hash, uint32_t height, arith_uint256 chainwork) {
    db->chainbottom = dogecoin_calloc(1, sizeof(dogecoin_blockindex));
    db->chainbottom->height = height;
    memcpy_safe(db->chainbottom->hash, hash, sizeof(uint256_t));
    db->chainbottom->chainwork = chainwork;
    db->chaintip = db->chainbottom;
    /* Add to tree so dogecoin_btree_tdestroy (in dogecoin_headers_db_free) and
       the trim path (dogecoin_btree_tdelete) can properly free this block. */
    if (db->use_binary_tree) {
        dogecoin_btree_tsearch(db->chainbottom, &db->tree_root, dogecoin_header_compare);
    }
}

/* ---------------------------------------------------------------------------
 * Interface trampolines
 *
 * dogecoin_headers_db_interface stores its members with generic (void*)
 * signatures. The concrete functions above take a typed (dogecoin_headers_db*)
 * first argument. Assigning them into the interface by casting the *function
 * pointer* is undefined behaviour (C §6.3.2.3/8): calling a function through a
 * pointer of an incompatible type. UBSan (-fsanitize=function) flags this at
 * every call through the interface.
 *
 * These trampolines carry the exact generic signature the interface declares,
 * cast only the *data pointer* (which is well defined), and forward to the
 * typed function. This removes the undefined behaviour without changing the
 * interface shape or the typed functions themselves.
 * ------------------------------------------------------------------------- */
static void* headers_db_if_init(const dogecoin_chainparams* chainparams, dogecoin_bool inmem_only) {
    return dogecoin_headers_db_new(chainparams, inmem_only);
}
static void headers_db_if_free(void* db) {
    dogecoin_headers_db_free((dogecoin_headers_db*)db);
}
static dogecoin_bool headers_db_if_load(void* db, const char* filename, dogecoin_bool prompt) {
    return dogecoin_headers_db_load((dogecoin_headers_db*)db, filename, prompt);
}
static void headers_db_if_fill_blocklocator_tip(void* db, vector_t* blocklocators) {
    dogecoin_headers_db_fill_block_locator((dogecoin_headers_db*)db, blocklocators);
}
static dogecoin_blockindex* headers_db_if_connect_hdr(void* db, struct const_buffer* buf, dogecoin_bool load_process, dogecoin_bool* connected) {
    return dogecoin_headers_db_connect_hdr((dogecoin_headers_db*)db, buf, load_process, connected);
}
static dogecoin_blockindex* headers_db_if_getchaintip(void* db) {
    return dogecoin_headersdb_getchaintip((dogecoin_headers_db*)db);
}
static dogecoin_bool headers_db_if_disconnect_tip(void* db) {
    return dogecoin_headersdb_disconnect_tip((dogecoin_headers_db*)db);
}
static dogecoin_bool headers_db_if_has_checkpoint_start(void* db) {
    return dogecoin_headersdb_has_checkpoint_start((dogecoin_headers_db*)db);
}
static void headers_db_if_set_checkpoint_start(void* db, uint256_t hash, uint32_t height, arith_uint256 chainwork) {
    dogecoin_headersdb_set_checkpoint_start((dogecoin_headers_db*)db, hash, height, chainwork);
}

const dogecoin_headers_db_interface dogecoin_headers_db_interface_file = {
    headers_db_if_init,
    headers_db_if_free,
    headers_db_if_load,
    headers_db_if_fill_blocklocator_tip,
    headers_db_if_connect_hdr,
    headers_db_if_getchaintip,
    headers_db_if_disconnect_tip,
    headers_db_if_has_checkpoint_start,
    headers_db_if_set_checkpoint_start
};
