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

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#else
#include <unistd.h>
#endif

#include <dogecoin/base58.h>
#include <dogecoin/bip44.h>
#include <dogecoin/blockchain.h>
#include <dogecoin/common.h>
#include <dogecoin/koinu.h>
#include <dogecoin/serialize.h>
#include <dogecoin/wallet.h>
#include <dogecoin/utils.h>

#define COINBASE_MATURITY 100

uint8_t WALLET_DB_REC_TYPE_MASTERPUBKEY = 0;
uint8_t WALLET_DB_REC_TYPE_PUBKEYCACHE = 1;
uint8_t WALLET_DB_REC_TYPE_ADDR = 1;
uint8_t WALLET_DB_REC_TYPE_TX = 2;

static const unsigned char file_hdr_magic[4] = {0xA8, 0xF0, 0x11, 0xC5}; /* header magic */
static const unsigned char file_rec_magic[4] = {0xC8, 0xF2, 0x69, 0x1E}; /* record magic */
static const uint32_t current_version = 1;

/* ====================== */
/* compare btree callback */
/* ====================== */
int dogecoin_wallet_addr_compare(const void *l, const void *r)
{
    const dogecoin_wallet_addr *lm = l;
    const dogecoin_wallet_addr *lr = r;

    uint8_t *pubkeyA = (uint8_t *)lm->pubkeyhash;
    uint8_t *pubkeyB = (uint8_t *)lr->pubkeyhash;

    /* byte per byte compare */
    /* TODO: switch to memcmp */
    unsigned int i;
    for (i = 0; i < sizeof(uint160); i++) {
        uint8_t iA = pubkeyA[i];
        uint8_t iB = pubkeyB[i];
        if (iA > iB)
            return -1;
        else if (iA < iB)
            return 1;
    }

    return 0;
}

int dogecoin_wtx_compare(const void *l, const void *r)
{
    const dogecoin_wtx *lm = l;
    const dogecoin_wtx *lr = r;

    uint8_t *hashA = (uint8_t *)lm->tx_hash_cache;
    uint8_t *hashB = (uint8_t *)lr->tx_hash_cache;

    /* byte per byte compare */
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

int dogecoin_tx_outpoint_compare(const void *l, const void *r)
{
    const dogecoin_tx_outpoint *lm = l;
    const dogecoin_tx_outpoint *lr = r;

    uint8_t *hashA = (uint8_t *)lm->hash;
    uint8_t *hashB = (uint8_t *)lr->hash;

    /* byte per byte compare */
    unsigned int i;
    for (i = 0; i < sizeof(uint256); i++) {
        uint8_t iA = hashA[i];
        uint8_t iB = hashB[i];
        if (iA > iB)
            return -1;
        else if (iA < iB)
            return 1;
    }
    if (lm->n > lr->n) {
        return -1;
    }
    if (lm->n < lr->n) {
        return 1;
    }
    return 0;
}


/*
 ==========================================================
 WALLET TRANSACTION (WTX) FUNCTIONS
 ==========================================================
*/
dogecoin_wtx* dogecoin_wallet_wtx_new()
{
    dogecoin_wtx* wtx;
    wtx = dogecoin_calloc(1, sizeof(*wtx));
    wtx->height = 0;
    wtx->ignore = false;
    dogecoin_hash_clear(wtx->blockhash);
    dogecoin_hash_clear(wtx->tx_hash_cache);
    wtx->tx = dogecoin_tx_new();

    return wtx;
}

dogecoin_wtx* dogecoin_wallet_wtx_copy(dogecoin_wtx* wtx)
{
    dogecoin_wtx* wtx_copy;
    wtx_copy = dogecoin_wallet_wtx_new();
    dogecoin_tx_copy(wtx_copy->tx, wtx->tx);

    return wtx_copy;
}

void dogecoin_wallet_wtx_free(dogecoin_wtx* wtx)
{
    dogecoin_tx_free(wtx->tx);
    dogecoin_free(wtx);
}

void dogecoin_wallet_wtx_serialize(cstring* s, const dogecoin_wtx* wtx)
{
    ser_u32(s, wtx->height);
    ser_u256(s, wtx->tx_hash_cache);
    dogecoin_tx_serialize(s, wtx->tx);
}

dogecoin_bool dogecoin_wallet_wtx_deserialize(dogecoin_wtx* wtx, struct const_buffer* buf)
{
    deser_u32(&wtx->height, buf);
    deser_u256(wtx->tx_hash_cache, buf);
    return dogecoin_tx_deserialize(buf->p, buf->len, wtx->tx, NULL);
}

void dogecoin_wallet_wtx_cachehash(dogecoin_wtx* wtx) {
    dogecoin_tx_hash(wtx->tx, wtx->tx_hash_cache);
}


/*
 ==========================================================
 WALLET ADDRESS (WALLET_ADDR) FUNCTIONS
 ==========================================================
*/

dogecoin_wallet_addr* dogecoin_wallet_addr_new()
{
    dogecoin_wallet_addr* waddr;
    waddr = dogecoin_calloc(1, sizeof(*waddr));
    dogecoin_mem_zero(waddr->pubkeyhash, sizeof(waddr->pubkeyhash));
    return waddr;
}

void dogecoin_wallet_addr_free(dogecoin_wallet_addr* waddr)
{
    dogecoin_free(waddr);
}

void dogecoin_wallet_addr_serialize(cstring* s, const dogecoin_chainparams *params, const dogecoin_wallet_addr* waddr)
{
    (void)(params);
    ser_bytes(s, waddr->pubkeyhash, sizeof(uint160));
    ser_bytes(s, (unsigned char *)&waddr->type, sizeof(uint8_t));
    ser_u32(s, waddr->childindex);
}

dogecoin_bool dogecoin_wallet_addr_deserialize(dogecoin_wallet_addr* waddr, const dogecoin_chainparams *params, struct const_buffer* buf) {
    (void)(params);
    if (!deser_bytes(&waddr->pubkeyhash, buf, sizeof(uint160))) return false;
    if (!deser_bytes((unsigned char *)&waddr->type, buf, sizeof(uint8_t))) return false;
    if (!deser_u32(&waddr->childindex, buf)) return false;
    return true;
}

/*
 ==========================================================
 WALLET OUTPUT (prev wtx + n) FUNCTIONS
 ==========================================================
 */

dogecoin_output* dogecoin_wallet_output_new()
{
    dogecoin_output* output;
    output = dogecoin_calloc(1, sizeof(*output));
    output->i = 0;
    output->wtx = dogecoin_wallet_wtx_new();

    return output;
}

void dogecoin_wallet_output_free(dogecoin_output* output)
{
    dogecoin_wallet_wtx_free(output->wtx);
    dogecoin_free(output);
}

/*
 ==========================================================
 WALLET CORE FUNCTIONS
 ==========================================================
 */
dogecoin_wallet* dogecoin_wallet_new(const dogecoin_chainparams *params)
{
    dogecoin_wallet* wallet = dogecoin_calloc(1, sizeof(*wallet));
    wallet->masterkey = NULL;
    wallet->chain = params;
    wallet->hdkeys_rbtree = 0;
    wallet->spends = vector_new(10, dogecoin_free);
    wallet->spends_rbtree = 0;
    wallet->vec_wtxes = vector_new(10, dogecoin_free);
    wallet->wtxes_rbtree = 0;
    wallet->waddr_vector = vector_new(10, dogecoin_free);
    wallet->waddr_rbtree = 0;
    return wallet;
}

void dogecoin_wallet_free(dogecoin_wallet* wallet)
{
    if (!wallet)
        return;

    if (wallet->dbfile) {
        fclose(wallet->dbfile);
    }

    if (wallet->masterkey)
        dogecoin_free(wallet->masterkey);

    if (wallet->spends) {
        vector_free(wallet->spends, true);
        wallet->spends = NULL;
    }

    dogecoin_btree_tdestroy(wallet->wtxes_rbtree, dogecoin_free);
    dogecoin_btree_tdestroy(wallet->hdkeys_rbtree, dogecoin_free);
    dogecoin_btree_tdestroy(wallet->spends_rbtree, dogecoin_free);

    if (wallet->waddr_vector) {
        vector_free(wallet->waddr_vector, true);
        wallet->waddr_vector = NULL;
    }

    if (wallet->vec_wtxes) {
        vector_free(wallet->vec_wtxes, true);
        wallet->vec_wtxes = NULL;
    }

    wallet->chain = NULL;
    dogecoin_free(wallet);
}


dogecoin_bool dogecoin_wallet_create(dogecoin_wallet* wallet, const char* file_path, int *error)
{
    if (!wallet)
        return false;

    struct stat buffer;
    if (stat(file_path, &buffer) != 0) {
        *error = 1;
        return false;
    }

    wallet->dbfile = fopen(file_path, "a+b");

    // write file-header-magic
    if (fwrite(file_hdr_magic, 4, 1, wallet->dbfile) != 1) return false;

    // write version
    uint32_t v = htole32(current_version);
    if (fwrite(&v, sizeof(v), 1, wallet->dbfile) != 1) return false;

    // write genesis
    if (fwrite(wallet->chain->genesisblockhash, sizeof(uint256), 1, wallet->dbfile ) != 1) return false;

    dogecoin_file_commit(wallet->dbfile);
    return true;
}


void dogecoin_wallet_add_wtx_intern_move(dogecoin_wallet *wallet, const dogecoin_wtx *wtx) {
    //add to spends
    dogecoin_wallet_add_to_spent(wallet, wtx);

    dogecoin_wtx* checkwtx = dogecoin_btree_tfind(wtx, &wallet->wtxes_rbtree, dogecoin_wtx_compare);
    if (checkwtx) {
        // remove existing wtx
        checkwtx = *(dogecoin_wtx **)checkwtx;
        unsigned int i;
        for (i = 0; i < wallet->vec_wtxes->len; i++) {
            dogecoin_wtx *wtx_vec = vector_idx(wallet->vec_wtxes, i);
            if (wtx_vec == checkwtx) {
                vector_remove_idx(wallet->vec_wtxes, i);
            }
        }
        // we do not really delete transactions
        checkwtx->ignore = true;
        dogecoin_btree_tdelete(checkwtx, &wallet->wtxes_rbtree, dogecoin_wtx_compare);
        dogecoin_wallet_wtx_free(checkwtx);
    }
    dogecoin_btree_tfind(wtx, &wallet->wtxes_rbtree, dogecoin_wtx_compare);
    vector_add(wallet->vec_wtxes, (dogecoin_wtx *)wtx);
}


dogecoin_bool dogecoin_wallet_load(dogecoin_wallet* wallet, const char* file_path, int *error, dogecoin_bool *created)
{
    (void)(error);
    if (!wallet) { return false; }

    struct stat buffer;
    *created = true;
    if (stat(file_path, &buffer) == 0) *created = false;

    wallet->dbfile = fopen(file_path, *created ? "a+b" : "r+b");

    if (*created) {
        if (!dogecoin_wallet_create(wallet, file_path, error)) {
            return false;
        }
    }
    else {
        // check file-header-magic, version and genesis
        uint8_t buf[sizeof(file_hdr_magic)+sizeof(current_version)+sizeof(uint256)];
        if ((uint32_t)buffer.st_size < (uint32_t)(sizeof(buf)) || fread(buf, sizeof(buf), 1, wallet->dbfile) != 1 || memcmp(buf, file_hdr_magic, sizeof(file_hdr_magic)))
        {
            fprintf(stderr, "Wallet file: error reading database file\n");
            return false;
        }
        if (le32toh(*(buf+sizeof(file_hdr_magic))) > current_version) {
            fprintf(stderr, "Wallet file: unsupported file version\n");
            return false;
        }
        if (memcmp(buf+sizeof(file_hdr_magic)+sizeof(current_version), wallet->chain->genesisblockhash, sizeof(uint256)) != 0) {
            fprintf(stderr, "Wallet file: different network\n");
            return false;
        }
        // read
        while (!feof(wallet->dbfile))
        {
            uint8_t buf[sizeof(file_rec_magic)];
            if (fread(buf, sizeof(buf), 1, wallet->dbfile) != 1 ) {
                // no more record, break
                break;
            }
            if (memcmp(buf, file_rec_magic, sizeof(file_rec_magic))) {
                fprintf(stderr, "Wallet file: error reading record file (invalid magic). Wallet file is corrupt\n");
                return false;
            }
            uint32_t reclen = 0;
            if (!deser_varlen_from_file(&reclen, wallet->dbfile)) return false;

            uint8_t rectype;
            if (fread(&rectype, 1, 1, wallet->dbfile) != 1) return false;

            if (rectype == WALLET_DB_REC_TYPE_MASTERPUBKEY) {
                uint32_t len;
                char strbuf[196];
                char strbuf_check[196];
                dogecoin_mem_zero(strbuf, sizeof(strbuf));
                dogecoin_mem_zero(strbuf_check, sizeof(strbuf_check));
                if (!deser_varlen_from_file(&len, wallet->dbfile)) return false;
                if (len > sizeof(strbuf)) { return false; }
                if (fread(strbuf, len, 1, wallet->dbfile) != 1) return false;
                if (!deser_varlen_from_file(&len, wallet->dbfile)) return false;
                if (len > sizeof(strbuf_check)) { return false; }
                if (fread(strbuf_check, len, 1, wallet->dbfile) != 1) return false;

                if (strcmp(strbuf, strbuf_check) != 0) {
                    fprintf(stderr, "Wallet file: xpub check failed, corrupt wallet detected.\n");
                    return false;
                }
                wallet->masterkey = dogecoin_hdnode_new();
                dogecoin_hdnode_deserialize(strbuf, wallet->chain, wallet->masterkey);
            } else if (rectype == WALLET_DB_REC_TYPE_ADDR) {
                dogecoin_wallet_addr *waddr= dogecoin_wallet_addr_new();
                size_t addr_len = 20+1+4;
                unsigned char buf[addr_len];
                struct const_buffer cbuf = {buf, addr_len};
                if (fread(buf, addr_len, 1, wallet->dbfile) != 1) {
                    dogecoin_wallet_addr_free(waddr);
                    return false;
                }

                dogecoin_wallet_addr_deserialize(waddr, wallet->chain, &cbuf);
                // add the node to the binary tree
                dogecoin_btree_tfind(waddr, &wallet->waddr_rbtree, dogecoin_wallet_addr_compare);
                vector_add(wallet->waddr_vector, waddr);
                wallet->next_childindex = waddr->childindex+1;
            } else if (rectype == WALLET_DB_REC_TYPE_TX) {
                unsigned char buf[reclen];
                struct const_buffer cbuf = {buf, reclen};
                if (fread(buf, reclen, 1, wallet->dbfile) != 1) {
                    return false;
                }
                dogecoin_wtx *wtx = dogecoin_wallet_wtx_new();
                dogecoin_wallet_wtx_deserialize(wtx, &cbuf);
                char* coin_amount[21];
                dogecoin_mem_zero(coin_amount, 21);
                int length = wtx->tx->vout->len;
                int j = 0;
                for (; j < length - 1; j++) {
                    dogecoin_tx_out* tx_out = vector_idx(wtx->tx->vout, j);
                    printf("output index:       %d\n", j);
                    printf("script public key:  %s\n", utils_uint8_to_hex((const uint8_t*)tx_out->script_pubkey->str, tx_out->script_pubkey->len));
                    koinu_to_coins_str(tx_out->value, (char*)coin_amount);
                    printf("amount:             %s\n", (char*)coin_amount);
                }
                dogecoin_wallet_add_wtx_intern_move(wallet, wtx); // hands memory management over to the binary tree
            } else {
                fseek(wallet->dbfile , reclen, SEEK_CUR);
            }
        }
    }

    return true;
}

dogecoin_bool dogecoin_wallet_flush(dogecoin_wallet* wallet)
{
    dogecoin_file_commit(wallet->dbfile);
    return true;
}


dogecoin_bool wallet_write_record(dogecoin_wallet *wallet, const cstring* record, uint8_t record_type) {
    // write record magic
    if (fwrite(file_rec_magic, 4, 1, wallet->dbfile) != 1) return false;

    //write record len
    cstring *cstr_len = cstr_new_sz(4);
    ser_varlen(cstr_len, record->len);
    if (fwrite(cstr_len->str, cstr_len->len, 1, wallet->dbfile) != 1) {
        cstr_free(cstr_len, true);
        return false;
    }
    cstr_free(cstr_len, true);

    // write record type & record payload
    if (fwrite(&record_type, 1, 1, wallet->dbfile) != 1 ||
        fwrite(record->str, record->len, 1, wallet->dbfile) != 1) {
        return false;
    }
    return true;
}


void dogecoin_wallet_set_master_key_copy(dogecoin_wallet* wallet, const dogecoin_hdnode* master_xpub)
{
    if (!master_xpub)
        return;

    if (wallet->masterkey != NULL) {
        //changing the master key should not be done,...
        //anyways, we are going to accept that at this point
        //consuming application needs to take care about that
        dogecoin_hdnode_free(wallet->masterkey);
        wallet->masterkey = NULL;
    }
    wallet->masterkey = dogecoin_hdnode_copy(master_xpub);

    cstring* record = cstr_new_sz(256);
    char strbuf[196];
    dogecoin_hdnode_serialize_public(wallet->masterkey, wallet->chain, strbuf, sizeof(strbuf));
    ser_str(record, strbuf, sizeof(strbuf));
    ser_str(record, strbuf, sizeof(strbuf));

    wallet_write_record(wallet, record, WALLET_DB_REC_TYPE_MASTERPUBKEY);

    cstr_free(record, true);

    dogecoin_file_commit(wallet->dbfile);
}


dogecoin_wallet_addr* dogecoin_wallet_next_addr(dogecoin_wallet* wallet)
{
    if (!wallet || !wallet->masterkey)
        return NULL;

    //for now, only m/k is possible
    dogecoin_wallet_addr *waddr = dogecoin_wallet_addr_new();
    dogecoin_hdnode *hdnode = dogecoin_hdnode_copy(wallet->masterkey);
    dogecoin_hdnode *bip44_key = dogecoin_hdnode_new();
    uint32_t index = wallet->next_childindex;

    char keypath[BIP44_KEY_PATH_MAX_LENGTH + 1] = "";

    /* Derive the child private key at the index */
    if (derive_bip44_extended_private_key(hdnode, 0, &index, "0", NULL, false, keypath, bip44_key) == -1) {
        return -1;
    }

    dogecoin_hdnode_get_hash160(bip44_key, waddr->pubkeyhash);
    waddr->childindex = wallet->next_childindex;

    //add it to the binary tree
    // tree manages memory
    dogecoin_btree_tsearch(waddr, &wallet->waddr_rbtree, dogecoin_wallet_addr_compare);
    vector_add(wallet->waddr_vector, waddr);

    //serialize and store node
    cstring* record = cstr_new_sz(256);
    dogecoin_wallet_addr_serialize(record, wallet->chain, waddr);
    if (!wallet_write_record(wallet, record, WALLET_DB_REC_TYPE_ADDR)) {
        fprintf(stderr, "Writing wallet address failed\n");
    }
    cstr_free(record, true);
    dogecoin_file_commit(wallet->dbfile);

    //increase the in-memory counter (cache)
    wallet->next_childindex++;

    return waddr;
}

dogecoin_bool dogecoin_p2pkh_address_to_wallet_pubkeyhash(const char* address_in, dogecoin_wallet_addr* addr, dogecoin_wallet* wallet) {
    if (!address_in || !addr || !wallet || !wallet->masterkey) return false;

    // lookup to see if we have address already:
    vector* addrs = vector_new(1, free);
    dogecoin_wallet_get_addresses(wallet, addrs);
    dogecoin_bool match = false;
    unsigned int i;
    for (i = 0; i < addrs->len; i++) {
        char* watch_addr = vector_idx(addrs, i);
        if (strncmp(watch_addr, address_in, strlen(watch_addr))==0) {
            addr->childindex = i;
            match = true;
        }
    }
    vector_free(addrs, true);

    char script_pubkey[40];
    char* pubkey_hash = dogecoin_malloc(40 + 6 + 4 + 1);
    if (!dogecoin_p2pkh_address_to_pubkey_hash((char*)address_in, pubkey_hash)) return false;
    slice(pubkey_hash, script_pubkey, 6, 46);
    dogecoin_free(pubkey_hash);
    memcpy_safe(addr->pubkeyhash, utils_hex_to_uint8(script_pubkey), 20);

    // if no match add to rbtree, vector and db:
    if (!match) {
        addr->childindex = wallet->next_childindex;
        dogecoin_btree_tsearch(addr, &wallet->waddr_rbtree, dogecoin_wallet_addr_compare);
        vector_add(wallet->waddr_vector, addr);
        cstring* record = cstr_new_sz(256);
        dogecoin_wallet_addr_serialize(record, wallet->chain, addr);
        if (!wallet_write_record(wallet, record, WALLET_DB_REC_TYPE_ADDR)) fprintf(stderr, "Writing wallet address failed\n");
        cstr_free(record, true);
        dogecoin_file_commit(wallet->dbfile);
        wallet->next_childindex++;
    }
    return true;
}

void dogecoin_wallet_get_addresses(dogecoin_wallet* wallet, vector* addr_out)
{
    unsigned int i;
    for (i = 0; i < wallet->waddr_vector->len; i++) {
        dogecoin_wallet_addr *waddr = vector_idx(wallet->waddr_vector, i);
        size_t addrsize = 35;
        char* addr = dogecoin_calloc(1, addrsize);
        dogecoin_p2pkh_addr_from_hash160(waddr->pubkeyhash, wallet->chain, addr, addrsize);
        vector_add(addr_out, addr);
    }
}

dogecoin_wallet_addr* dogecoin_wallet_find_waddr_byaddr(dogecoin_wallet* wallet, const char* search_addr)
{
    if (!wallet || !search_addr)
        return NULL;


    uint8_t hashdata[strlen(search_addr)];
    dogecoin_mem_zero(hashdata, sizeof(uint160));
    int outlen = dogecoin_base58_decode_check(search_addr, hashdata, strlen(search_addr));

    if (outlen > 0 && hashdata[0] == wallet->chain->b58prefix_pubkey_address) {

    } else if (outlen > 0 && hashdata[0] == wallet->chain->b58prefix_script_address) {

    }

    dogecoin_wallet_addr* waddr_search;
    waddr_search = dogecoin_calloc(1, sizeof(*waddr_search));
    memcpy_safe(waddr_search->pubkeyhash, hashdata+1, sizeof(uint160));

    dogecoin_wallet_addr *needle = dogecoin_btree_tfind(waddr_search, &wallet->waddr_rbtree, dogecoin_wallet_addr_compare); /* read */
    if (needle) {
        needle = *(dogecoin_wallet_addr **)needle;
    }
    dogecoin_free(waddr_search);

    return needle;
}

dogecoin_bool dogecoin_wallet_add_wtx(dogecoin_wallet* wallet, dogecoin_wtx* wtx) {

    if (!wallet || !wtx)
        return false;

    dogecoin_wallet_wtx_cachehash(wtx);

    cstring* record = cstr_new_sz(1024);
    dogecoin_wallet_wtx_serialize(record, wtx);

    if (!wallet_write_record(wallet, record, WALLET_DB_REC_TYPE_TX)) {
        printf("Writing wtx record failed\n");
        fprintf(stderr, "Writing wtx record failed\n");
    }
    cstr_free(record, true);
    dogecoin_file_commit(wallet->dbfile);

    return true;
}

dogecoin_bool dogecoin_wallet_add_wtx_move(dogecoin_wallet* wallet, dogecoin_wtx* wtx)
{
    dogecoin_wallet_add_wtx(wallet, wtx);

    //add it to the binary tree
    dogecoin_wallet_add_wtx_intern_move(wallet, wtx); //hands memory management over to the binary tree
    return true;
}

dogecoin_bool dogecoin_wallet_have_key(dogecoin_wallet* wallet, uint160 hash160)
{
    if (!wallet)
        return false;

    dogecoin_wallet_addr waddr_search;
    memcpy_safe(&waddr_search.pubkeyhash, hash160, sizeof(uint160));

    dogecoin_wallet_addr *needle = dogecoin_btree_tfind(&waddr_search, &wallet->waddr_rbtree, dogecoin_wallet_addr_compare); /* read */
    if (needle) {
        needle = *(dogecoin_wallet_addr **)needle;
    }

    return (needle != NULL);
}

int64_t dogecoin_wallet_get_balance(dogecoin_wallet* wallet)
{
    int64_t credit = 0;

    if (!wallet)
        return false;

    unsigned int i;
    for (i = 0; i < wallet->vec_wtxes->len; i++) {
        dogecoin_wtx *wtx = vector_idx(wallet->vec_wtxes, i);
        credit += dogecoin_wallet_wtx_get_available_credit(wallet, wtx);
    }

    return credit;
}

int64_t dogecoin_wallet_wtx_get_credit(dogecoin_wallet* wallet, dogecoin_wtx* wtx)
{
    int64_t credit = 0;

    if (dogecoin_tx_is_coinbase(wtx->tx) &&
        (wallet->bestblockheight < COINBASE_MATURITY || wtx->height > wallet->bestblockheight - COINBASE_MATURITY))
        return credit;

    unsigned int i = 0;
    for (i = 0; i < wtx->tx->vout->len; i++) {
        dogecoin_tx_out* tx_out;
        tx_out = vector_idx(wtx->tx->vout, i);
        if (dogecoin_wallet_txout_is_mine(wallet, tx_out)) {
            credit += tx_out->value;
        }
    }
    return credit;
}

int64_t dogecoin_wallet_wtx_get_available_credit(dogecoin_wallet* wallet, dogecoin_wtx* wtx)
{
    int64_t credit = 0;
    if (!wallet) {
        return credit;
    }

    // Must wait until coinbase is safely deep enough in the chain before valuing it
    if (dogecoin_tx_is_coinbase(wtx->tx) &&
        (wallet->bestblockheight < COINBASE_MATURITY || wtx->height > wallet->bestblockheight - COINBASE_MATURITY)) {
        return credit;
    }

    unsigned int i;
    for (i = 0; i < wtx->tx->vout->len; i++)
    {
        if (!dogecoin_wallet_is_spent(wallet, wtx->tx_hash_cache, i))
        {
            dogecoin_tx_out* tx_out = vector_idx(wtx->tx->vout, i);
            if (dogecoin_wallet_txout_is_mine(wallet, tx_out)) {
                credit += tx_out->value;
            }
        }
    }

    return credit;
}

dogecoin_bool dogecoin_wallet_txout_is_mine(dogecoin_wallet* wallet, dogecoin_tx_out* tx_out)
{
    if (!wallet || !tx_out) return false;

    dogecoin_bool ismine = false;

    vector* vec = vector_new(16, free);
    enum dogecoin_tx_out_type type = dogecoin_script_classify(tx_out->script_pubkey, vec);

    //TODO: Multisig, etc.
    if (type == DOGECOIN_TX_PUBKEYHASH) {
        //TODO: find a better format for vector elements (not a pure pointer)
        uint8_t* hash160 = vector_idx(vec, 0);
        if (dogecoin_wallet_have_key(wallet, hash160)) {
            ismine = true;
        }
    }

    vector_free(vec, true);
    return ismine;
}

dogecoin_bool dogecoin_wallet_is_mine(dogecoin_wallet* wallet, const dogecoin_tx *tx)
{
    if (!wallet || !tx) return false;
    if (tx->vout) {
        unsigned int i;
        for (i = 0; i < tx->vout->len; i++) {
            dogecoin_tx_out* tx_out = vector_idx(tx->vout, i);
            if (tx_out && dogecoin_wallet_txout_is_mine(wallet, tx_out)) {
                return true;
            }
        }
    }
    return false;
}

int64_t dogecoin_wallet_get_debit_txi(dogecoin_wallet *wallet, const dogecoin_tx_in *txin) {
    if (!wallet || !txin) return 0;

    dogecoin_wtx wtx;
    memcpy_safe(wtx.tx_hash_cache, txin->prevout.hash, sizeof(wtx.tx_hash_cache));

    dogecoin_wtx* prevwtx = dogecoin_btree_tfind(&wtx, &wallet->wtxes_rbtree, dogecoin_wtx_compare);
    if (prevwtx) {
        // remove existing wtx
        prevwtx = *(dogecoin_wtx **)prevwtx;

        if (txin->prevout.n < prevwtx->tx->vout->len) {
            dogecoin_tx_out *tx_out = vector_idx(prevwtx->tx->vout, txin->prevout.n);
            if (tx_out && dogecoin_wallet_txout_is_mine(wallet, tx_out)) {
                return tx_out->value;
            }
        }
    }

    return 0;
}

int64_t dogecoin_wallet_get_debit_tx(dogecoin_wallet *wallet, const dogecoin_tx *tx) {
    unsigned int i;
    int64_t debit = 0;
    if (tx->vin) {
        for (i = 0; i < tx->vin->len; i++) {
            dogecoin_tx_in* tx_in= vector_idx(tx->vin, i);
            if (tx_in) {
                debit += dogecoin_wallet_get_debit_txi(wallet, tx_in);
            }
        }
    }
    return debit;
}

dogecoin_bool dogecoin_wallet_is_from_me(dogecoin_wallet *wallet, const dogecoin_tx *tx)
{
    return (dogecoin_wallet_get_debit_tx(wallet, tx) > 0);
}

void dogecoin_wallet_add_to_spent(dogecoin_wallet* wallet, const dogecoin_wtx* wtx) {
    if (!wallet || !wtx)
        return;

    if (dogecoin_tx_is_coinbase(wtx->tx))
        return;

    unsigned int i;
    if (wtx->tx->vin) {
        for (i = 0; i < wtx->tx->vin->len; i++) {
            dogecoin_tx_in* tx_in = vector_idx(wtx->tx->vin, i);

            // form outpoint
            dogecoin_tx_outpoint* outpoint = dogecoin_calloc(1, sizeof(dogecoin_tx_outpoint));
            memcpy_safe(outpoint, &tx_in->prevout, sizeof(dogecoin_tx_outpoint));

            // add to binary tree
            // memory is managed there (will free on tdestroy
            dogecoin_btree_tfind(outpoint, &wallet->spends_rbtree, dogecoin_tx_outpoint_compare);
        }
    }
}

dogecoin_bool dogecoin_wallet_is_spent(dogecoin_wallet* wallet, uint256 hash, uint32_t n)
{
    if (!wallet)
        return false;

    dogecoin_tx_outpoint outpoint;
    memcpy_safe(&outpoint.hash, hash, sizeof(uint256));
    outpoint.n = n;
    dogecoin_tx_outpoint* possible_found = dogecoin_btree_tfind(&outpoint, &wallet->spends_rbtree, dogecoin_tx_outpoint_compare);
    if (possible_found) {
        possible_found = *(dogecoin_tx_outpoint **)possible_found;
    }

    return (possible_found != NULL);
}

dogecoin_wtx * dogecoin_wallet_get_wtx(dogecoin_wallet* wallet, const uint256 hash) {
    dogecoin_wtx find;
    find.tx = NULL;
    dogecoin_hash_set(find.tx_hash_cache, hash);
    dogecoin_wtx* check_wtx = dogecoin_btree_tfind(&find, &wallet->wtxes_rbtree, dogecoin_wtx_compare);
    if (check_wtx) {
        return *(dogecoin_wtx **)check_wtx;
    }
    return NULL;
}

dogecoin_bool dogecoin_wallet_get_unspent(dogecoin_wallet* wallet, vector* unspents)
{
    if (!wallet || !unspents) {
        return false;
    }
    unsigned int i, j;
    for (i = 0; i < wallet->vec_wtxes->len; i++) {
        dogecoin_wtx *wtx = vector_idx(wallet->vec_wtxes, i);
        for (j = 0; j < wtx->tx->vout->len; j++)
        {
            if (!dogecoin_wallet_is_spent(wallet, wtx->tx_hash_cache, j))
            {
                dogecoin_tx_out* tx_out = vector_idx(wtx->tx->vout, j);
                if (dogecoin_wallet_txout_is_mine(wallet, tx_out)) {
                    dogecoin_tx_outpoint *outpoint = dogecoin_calloc(1, sizeof(dogecoin_tx_outpoint));
                    dogecoin_hash_set(outpoint->hash, wtx->tx_hash_cache);
                    outpoint->n = j;
                    vector_add(unspents, outpoint);
                }
            }
        }
    }
    return true;
}

void dogecoin_wallet_check_transaction(void *ctx, dogecoin_tx *tx, unsigned int pos, dogecoin_blockindex *pindex) {
    (void)(pos);
    (void)(pindex);
    dogecoin_wallet *wallet = (dogecoin_wallet *)ctx;
    if (dogecoin_wallet_is_mine(wallet, tx) || dogecoin_wallet_is_from_me(wallet, tx)) {
        dogecoin_wtx* wtx = dogecoin_wallet_wtx_new();
        dogecoin_tx_copy(wtx->tx, tx);
        dogecoin_wallet_add_wtx_move(wallet, wtx);
        printf("\nFound relevant transaction!\n");
        int64_t amount = dogecoin_wallet_get_balance(wallet);
        char* coin_amount[21];
        dogecoin_mem_zero(coin_amount, 21);
        koinu_to_coins_str((uint64_t)amount, (char*)coin_amount);
        printf("Wallet balance:             %s\n", (char*)coin_amount);
    }
}
