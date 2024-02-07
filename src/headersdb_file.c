/*

 The MIT License (MIT)

 Copyright (c) 2017 Jonas Schnelli
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

#include <sys/stat.h>

#include <dogecoin/headersdb_file.h>
#include <dogecoin/blockchain.h>
#include <dogecoin/common.h>
#include <dogecoin/pow.h>
#include <dogecoin/serialize.h>
#include <dogecoin/utils.h>
#include <dogecoin/validation.h>

static const unsigned char file_hdr_magic[4] = {0xA8, 0xF0, 0x11, 0xC5}; /* header magic */
static const uint32_t current_version = 2;

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
    for (i = 0; i < sizeof(uint256); i++) {
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
        dogecoin_btree_tdestroy(db->tree_root, NULL);
        db->tree_root = NULL;
    }

    // Free all blockindex structures starting from chaintip to chainbottom
    if (db->chaintip) {
        dogecoin_blockindex *scan_tip = db->chaintip;
        while (scan_tip->prev) {
            dogecoin_blockindex *prev = scan_tip->prev;
            dogecoin_free(scan_tip);
            scan_tip = prev;
        }
        // If scan_tip is not the genesis block, free it
        if (scan_tip != &db->genesis) {
            dogecoin_free(scan_tip);
        }
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
            printf("\nLoad %s? ", file_path_local);
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
                    remove(file_path_local); // remove the existing file
                    create = true; // Set create to true as a new file will be created
                }
            }
        }
    }

    db->headers_tree_file = fopen(file_path_local, create ? "a+b" : "r+b");
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
            return false;
        }
        if (le32toh(*(buf+sizeof(file_hdr_magic))) > current_version) {
            fprintf(stderr, "Unsupported file version\n");
            return false;
        }
    }
    dogecoin_bool firstblock = true;
    size_t connected_headers_count = 0;
    if (db->headers_tree_file && !create)
    {
        printf("Loading headers from disk, this may take several minutes...\n");
        while (!feof(db->headers_tree_file))
        {
            // print progress
            if (connected_headers_count % 1000 == 0)
            {
                printf("\r%ld headers loaded", connected_headers_count);
                fflush(stdout);
            }

            uint8_t buf_all[32+4+80];
            if (fread(buf_all, sizeof(buf_all), 1, db->headers_tree_file) == 1) {
                struct const_buffer cbuf_all = {buf_all, sizeof(buf_all)};

                //load all

                uint256 hash;
                uint32_t height;
                deser_u256(hash, &cbuf_all);
                deser_u32(&height, &cbuf_all);
                dogecoin_bool connected;
                if (firstblock)
                {
                    dogecoin_blockindex *chainheader = dogecoin_calloc(1, sizeof(dogecoin_blockindex));
                    chainheader->height = height;
                    if (!dogecoin_block_header_deserialize(&chainheader->header, &cbuf_all, db->params)) {
                        dogecoin_block_header_free(&chainheader->header);
                        dogecoin_free(chainheader);
                        fprintf(stderr, "\nError: Invalid data found.\n");
                        return -1;
                    }
                    dogecoin_block_header_hash(&chainheader->header, (uint8_t *)&chainheader->hash);
                    chainheader->prev = NULL;
                    db->chaintip = chainheader;
                    firstblock = false;
                } else {
                    dogecoin_blockindex *pindex = dogecoin_headers_db_connect_hdr(db, &cbuf_all, true, &connected);
                    if (!connected)
                    {
                        printf("\nConnecting header %s failed (at height: %d) read_write: %d\n", hash_to_string(hash), db->chaintip->height, db->read_write_file);
                        dogecoin_free(pindex);
                    }
                    else {
                        connected_headers_count++;
                    }
                }
            }
        }
    }
    printf("\nConnected %ld headers, now at height: %d\n",  connected_headers_count, db->chaintip->height);
    return (db->headers_tree_file != NULL);
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
    cstring *rec = cstr_new_sz(100);
    ser_u256(rec, blockindex->hash);
    ser_u32(rec, blockindex->height);
    dogecoin_block_header_serialize(rec, &blockindex->header);
    size_t res = fwrite(rec->str, rec->len, 1, db->headers_tree_file);
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
    if (!dogecoin_block_header_deserialize(&blockindex->header, buf, db->params))
    {
        dogecoin_free(blockindex);
        fprintf(stderr, "Error deserializing block header\n");
        return NULL;
    }

    dogecoin_block_header_hash(&blockindex->header, (uint8_t *)&blockindex->hash);

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
            uint256 hash = {0};
            cstring* s = cstr_new_sz(64);
            dogecoin_block_header_serialize(s, (const dogecoin_block_header*) &blockindex->header);
            dogecoin_block_header_scrypt_hash(s, &hash);
            cstr_free(s, true);
            if (!check_pow(&hash, blockindex->header.bits, db->params, &blockindex->header.chainwork)) {
                printf("%s:%d:%s : non-AUX proof of work failed : %s\n", __FILE__, __LINE__, __func__, strerror(errno));
                dogecoin_free(blockindex);
                return false;
            }
        }

        blockindex->prev = connect_at;
        blockindex->height = connect_at->height+1;

        arith_uint256* connect_at_chainwork = init_arith_uint256();
        memcpy(connect_at_chainwork, connect_at->header.chainwork, sizeof(connect_at->header.chainwork));
        arith_uint256* blockindex_chainwork = init_arith_uint256();
        memcpy(blockindex_chainwork, blockindex->header.chainwork, sizeof(blockindex->header.chainwork));
        arith_uint256* chaintip_chainwork = init_arith_uint256();
        memcpy(chaintip_chainwork, db->chaintip->header.chainwork, sizeof(db->chaintip->header.chainwork));

        arith_uint256* added_chainwork = add_arith_uint256(connect_at_chainwork, blockindex_chainwork);
        memcpy(blockindex->header.chainwork, (const arith_uint256*) added_chainwork, sizeof(blockindex->header.chainwork));

        // Free the dynamically allocated memory
        dogecoin_free(connect_at_chainwork);
        dogecoin_free(blockindex_chainwork);

        // Chain reorganization if necessary
        if (fork_from_block && blockindex->height > db->chaintip->height && arith_uint256_greater_than(added_chainwork, chaintip_chainwork)) {

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
                    dogecoin_free(blockindex);
                    dogecoin_free(chaintip_chainwork);
                    dogecoin_free(added_chainwork);
                    return NULL;
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
                    dogecoin_free(chaintip_chainwork);
                    dogecoin_free(added_chainwork);
                    return NULL;
                }

                // Update the chain tip to the previous block
                db->chaintip = prev_block;
                current_block = prev_block;
            }

            printf("Chain reorganization: %d blocks disconnected, %d blocks connected\n", blockindex->height - common_ancestor->height, blockindex->height - db->chaintip->height);

            // Set the new block as the new chain tip
            db->chaintip = blockindex;
        }
        else if (blockindex->height > db->chaintip->height) {
            db->chaintip = blockindex;
        }

        // Free the dynamically allocated memory
        dogecoin_free(chaintip_chainwork);
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

        if (db->max_hdr_in_mem > 0) {
            // de-allocate no longer required headers
            // keep them only on-disk
            dogecoin_blockindex *scan_tip = db->chaintip;
            unsigned int i;
            for (i = 0; i < db->max_hdr_in_mem + 1; i++)
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
 * vector
 *
 * @param db the database object
 * @param blocklocators a vector of block hashes
 */
void dogecoin_headers_db_fill_block_locator(dogecoin_headers_db* db, vector *blocklocators)
{
    dogecoin_blockindex *scan_tip = db->chaintip;
    if (scan_tip->height > 0)
    {
        int i = 0;
        for(; i<10;i++)
        {
            //TODO: try to share memory and avoid heap allocation
            uint256 *hash = dogecoin_calloc(1, sizeof(uint256));
            memcpy_safe(hash, scan_tip->hash, sizeof(uint256));
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
dogecoin_blockindex * dogecoin_headersdb_find(dogecoin_headers_db* db, uint256 hash) {
    if (db->use_binary_tree)
    {
        dogecoin_blockindex *blockindex = dogecoin_calloc(1, sizeof(dogecoin_blockindex));
        memcpy_safe(blockindex->hash, hash, sizeof(uint256));
        dogecoin_blockindex *blockindex_f = dogecoin_btree_tfind(blockindex, &db->tree_root, dogecoin_header_compare); /* read */
        if (blockindex_f) {
            blockindex_f = *(dogecoin_blockindex **)blockindex_f;
        }
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
 * Set the checkpoint block to the given hash and height
 *
 * @param db The headers database.
 * @param hash The hash of the block that is the checkpoint.
 * @param height The height of the block that this is a checkpoint for.
 */
void dogecoin_headersdb_set_checkpoint_start(dogecoin_headers_db* db, uint256 hash, uint32_t height) {
    db->chainbottom = dogecoin_calloc(1, sizeof(dogecoin_blockindex));
    db->chainbottom->height = height;
    memcpy_safe(db->chainbottom->hash, hash, sizeof(uint256));
    db->chaintip = db->chainbottom;
}
