/**
 * Copyright (c) 2023 bluezr
 * Copyright (c) 2023 The Dogecoin Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <ctype.h>
#include <inttypes.h>

#include <dogecoin/map.h>

#include <dogecoin/dogecoin.h>
#include <dogecoin/common.h>


/**
 * @brief This function instantiates a new working hash,
 * but does not add it to the hash table.
 * 
 * @return A pointer to the new working hash. 
 */
hash* new_hash() {
    hash* h = (struct hash*)dogecoin_calloc(1, sizeof *h);
    int i = 0;
    for (; i < 8; i++) h->data.u32[i] = 0;
    h->index = HASH_COUNT(hashes) + 1;
    return h;
}

/**
 * @brief This function takes a pointer to an existing working
 * hash object and adds it to the hash table.
 * 
 * @param map The pointer to the working hash.
 * 
 * @return Nothing.
 */
void add_hash(hash *x) {
    hash* hash_local;
    HASH_FIND_INT(hashes, &x->index, hash_local);
    if (hash_local == NULL) {
        HASH_ADD_INT(hashes, index, x);
    } else {
        HASH_REPLACE_INT(hashes, index, x, hash_local);
    }
    dogecoin_free(hash_local);
}

/**
 * @brief This function creates a new map, places it in
 * the hash table, and returns the index of the new map,
 * starting from 1 and incrementing each subsequent call.
 * 
 * @return The index of the new map.
 */
int start_hash() {
    hash* hash = new_hash();
    add_hash(hash);
    return hash->index;
}

/**
 * @brief This function takes an index and returns the working
 * hash associated with that index in the hash table.
 * 
 * @param index The index of the target working hash.
 * 
 * @return The pointer to the working hash associated with
 * the provided index.
 */
hash* find_hash(int index) {
    hash *hash;
    HASH_FIND_INT(hashes, &index, hash);
    return hash;
}

hash* zero_hash(int index) {
    hash* hash = find_hash(index);
    dogecoin_mem_zero(hash->data.u8, sizeof(uint256));
    return hash;
}

void showbits(unsigned int x) {
    int i = 0;
    for (i = (sizeof(int) * 8) - 1; i >= 0; i--) {
       putchar(x & (1u << i) ? '1' : '0');
    }
    printf("\n");
}

void print_hash(int index) {
    hash* hash_local = find_hash(index);
    printf("%s\n", utils_uint8_to_hex(hash_local->data.u8, 32));
}

/**
 * @brief This function counts the number of working
 * hashes currently in the hash table.
 * 
 * @return Nothing. 
 */
void count_hashes() {
    int temp = HASH_COUNT(hashes);
    printf("there are %d hashes\n", temp);
}

char* get_hash_by_index(int index) {
    hash* hash_local = find_hash(index);
    return utils_uint8_to_hex(hash_local->data.u8, 32);
}

/**
 * @brief This function removes the specified working hash
 * from the hash table and frees the hashes in memory.
 * 
 * @param map The pointer to the hash to remove.
 * 
 * @return Nothing.
 */
void remove_hash(hash *hash) {
    HASH_DEL(hashes, hash); /* delete it (hashes advances to next) */
    if (hash->data.u8) dogecoin_free(hash->data.u8);
    dogecoin_free(hash);
}

/**
 * @brief This function removes all working hashes from
 * the hash table.
 * 
 * @return Nothing. 
 */
void remove_all_hashes() {
    struct hash *hash;
    struct hash *tmp;

    HASH_ITER(hh, hashes, hash, tmp) {
        remove_hash(hash);
    }
}

/**
 * @brief This function instantiates a new working map,
 * but does not add it to the hash table.
 * 
 * @return A pointer to the new working map. 
 */
map* new_map() {
    map* m = (struct map*)dogecoin_calloc(1, sizeof *m);
    m->count = 1;
    if (HASH_COUNT(hashes) < 1) start_hash();
    m->hashes = hashes;
    m->index = HASH_COUNT(maps) + 1;
    return m;
}

/**
 * @brief This function creates a new map, places it in
 * the hash table, and returns the index of the new map,
 * starting from 1 and incrementing each subsequent call.
 * 
 * @return The index of the new map.
 */
int start_map() {
    map* map = new_map();
    add_map(map);
    return map->index;
}

/**
 * @brief This function takes a pointer to an existing working
 * map object and adds it to the hash table.
 * 
 * @param map The pointer to the working map.
 * 
 * @return Nothing.
 */
void add_map(map* map_external) {
    map* map_local;
    HASH_FIND_INT(maps, &map_external->index, map_local);
    if (map_local == NULL) {
        HASH_ADD_INT(maps, index, map_external);
    } else {
        HASH_REPLACE_INT(maps, index, map_external, map_local);
    }
    dogecoin_free(map_local);
}

/**
 * @brief This function takes an index and returns the working
 * map associated with that index in the hash table.
 * 
 * @param index The index of the target working map.
 * 
 * @return The pointer to the working map associated with
 * the provided index.
 */
map* find_map(int index) {
    map *map;
    HASH_FIND_INT(maps, &index, map);
    return map;
}

/**
 * @brief This function removes the specified working map
 * from the hash table and frees the maps in memory.
 * 
 * @param map The pointer to the map to remove.
 * 
 * @return Nothing.
 */
void remove_map(map *map) {
    HASH_DEL(maps, map); /* delete it (maps advances to next) */
    dogecoin_free(map);
}

/**
 * @brief This function removes all working maps from
 * the hash table.
 * 
 * @return Nothing. 
 */
void remove_all_maps() {
    struct map *map;
    struct map *tmp;

    HASH_ITER(hh, maps, map, tmp) {
        remove_map(map);
    }
}
