/*

 The MIT License (MIT)

 Copyright (c) 2016 Jonas Schnelli
 Copyright (c) 2022 bluezr
 Copyright (c) 2022 The Dogecoin Foundation

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

#include <dogecoin/base58.h>
#include <dogecoin/serialize.h>
#include <dogecoin/wallet.h>

#include <logdb/logdb.h>
#include <logdb/logdb_rec.h>

#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define COINBASE_MATURITY 100

static const char *hdkey_key = "hdkey";
static const char *hdmasterkey_key = "mstkey";
static const char *tx_key = "tx";


/* static interface */
static logdb_memmapper dogecoin_wallet_db_mapper = {
    dogecoin_wallet_logdb_append_cb,
    NULL,
    NULL,
    NULL,
    NULL
};
/* ==================== */
/* txes rbtree callback */
/* ==================== */
void dogecoin_rbtree_wtxes_free_key(void* a) {
    /* keys are cstrings that needs to be released by the rbtree */
    cstring *key = (cstring *)a;
    cstr_free(key, true);
}

void dogecoin_rbtree_wtxes_free_value(void *a){
    /* free the wallet transaction */
    dogecoin_wtx *wtx = (dogecoin_wtx *)a;
    dogecoin_wallet_wtx_free(wtx);
}

int dogecoin_rbtree_wtxes_compare(const void* a,const void* b) {
    return cstr_compare((cstring *)a, (cstring *)b);
}


/* ====================== */
/* hdkeys rbtree callback */
/* ====================== */
void dogecoin_rbtree_hdkey_free_key(void* a) {
    /* keys are cstrings that needs to be released by the rbtree */
    cstring *key = (cstring *)a;
    cstr_free(key, true);
}

void dogecoin_rbtree_hdkey_free_value(void *a){
    /* free the hdnode */
    dogecoin_hdnode *node = (dogecoin_hdnode *)a;
    dogecoin_hdnode_free(node);
}

int dogecoin_rbtree_hdkey_compare(const void* a,const void* b) {
    return cstr_compare((cstring *)a, (cstring *)b);
}

/*
 ==========================================================
 WALLET TRANSACTION (WTX) FUNCTIONS
 ==========================================================
*/

dogecoin_wtx* dogecoin_wallet_wtx_new()
{
    dogecoin_wtx* wtx;
    wtx = calloc(1, sizeof(*wtx));
    wtx->height = 0;
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
    free(wtx);
}

void dogecoin_wallet_wtx_serialize(cstring* s, const dogecoin_wtx* wtx)
{
    ser_u32(s, wtx->height);
    dogecoin_tx_serialize(s, wtx->tx);
}

dogecoin_bool dogecoin_wallet_wtx_deserialize(dogecoin_wtx* wtx, struct const_buffer* buf)
{
    deser_u32(&wtx->height, buf);
    return dogecoin_tx_deserialize(buf->p, buf->len, wtx->tx);
}

/*
 ==========================================================
 WALLET OUTPUT (prev wtx + n) FUNCTIONS
 ==========================================================
 */

dogecoin_output* dogecoin_wallet_output_new()
{
    dogecoin_output* output;
    output = calloc(1, sizeof(*output));
    output->i = 0;
    output->wtx = dogecoin_wallet_wtx_new();

    return output;
}

void dogecoin_wallet_output_free(dogecoin_output* output)
{
    dogecoin_wallet_wtx_free(output->wtx);
    free(output);
}

/*
 ==========================================================
 WALLET CORE FUNCTIONS
 ==========================================================
 */
dogecoin_wallet* dogecoin_wallet_new()
{
    dogecoin_wallet* wallet;
    wallet = calloc(1, sizeof(*wallet));
    wallet->db = logdb_new();
    logdb_set_memmapper(wallet->db, &dogecoin_wallet_db_mapper, wallet);
    wallet->masterkey = NULL;
    wallet->chain = &dogecoin_chain_main;
    wallet->spends = vector_new(10, free);

    wallet->wtxes_rbtree = RBTreeCreate(dogecoin_rbtree_wtxes_compare,dogecoin_rbtree_wtxes_free_key,dogecoin_rbtree_wtxes_free_value,NULL,NULL);
    wallet->hdkeys_rbtree = RBTreeCreate(dogecoin_rbtree_hdkey_compare,dogecoin_rbtree_hdkey_free_key,dogecoin_rbtree_hdkey_free_value,NULL,NULL);
    return wallet;
}

void dogecoin_wallet_free(dogecoin_wallet *wallet)
{
    if (!wallet)
        return;

    if (wallet->db)
    {
        logdb_free(wallet->db);
        wallet->db = NULL;
    }

    if (wallet->spends)
    {
        vector_free(wallet->spends, true);
        wallet->spends = NULL;
    }

    if (wallet->masterkey)
        free(wallet->masterkey);

    RBTreeDestroy(wallet->wtxes_rbtree);
    RBTreeDestroy(wallet->hdkeys_rbtree);

    free(wallet);
}

void dogecoin_wallet_logdb_append_cb(void* ctx, logdb_bool load_phase, logdb_record *rec)
{
    dogecoin_wallet* wallet = (dogecoin_wallet*)ctx;
    if (load_phase)
    {
        if (wallet->masterkey == NULL && rec->mode == RECORD_TYPE_WRITE && rec->key->len > strlen(hdmasterkey_key) && memcmp(rec->key->str, hdmasterkey_key, strlen(hdmasterkey_key)) == 0)
        {
            wallet->masterkey = dogecoin_hdnode_new();
            dogecoin_hdnode_deserialize(rec->value->str, wallet->chain, wallet->masterkey);
        }
        if (rec->key->len == strlen(hdkey_key)+20 && memcmp(rec->key->str, hdkey_key, strlen(hdkey_key)) == 0)
        {
            dogecoin_hdnode *hdnode = dogecoin_hdnode_new();
            dogecoin_hdnode_deserialize(rec->value->str, wallet->chain, hdnode);

            /* rip out the hash from the record key (avoid re-SHA256) */
            cstring *keyhash160 = cstr_new_buf(rec->key->str+strlen(hdkey_key),20);

            /* add hdnode to the rbtree */
            RBTreeInsert(wallet->hdkeys_rbtree,keyhash160,hdnode);

            if (hdnode->child_num+1 > wallet->next_childindex)
                wallet->next_childindex = hdnode->child_num+1;
        }

        if (rec->key->len == strlen(tx_key)+SHA256_DIGEST_LENGTH && memcmp(rec->key->str, tx_key, strlen(tx_key)) == 0)
        {
            dogecoin_wtx *wtx = dogecoin_wallet_wtx_new();
            struct const_buffer buf = {rec->value->str, rec->value->len};

            /* deserialize transaction */
            dogecoin_wallet_wtx_deserialize(wtx, &buf);

            /* rip out the hash from the record key (avoid re-SHA256) */
            cstring *wtxhash = cstr_new_buf(rec->key->str+strlen(tx_key),SHA256_DIGEST_LENGTH);

            /* add wtx to the rbtree */
            RBTreeInsert(wallet->wtxes_rbtree,wtxhash,wtx);

            /* add to spends */
            dogecoin_wallet_add_to_spent(wallet, wtx);
        }
    }
}

dogecoin_bool dogecoin_wallet_load(dogecoin_wallet *wallet, const char *file_path, enum logdb_error *error)
{
    if (!wallet)
        return false;

    if (!wallet->db)
        return false;

    if (wallet->db->file)
    {
        *error = LOGDB_ERROR_FILE_ALREADY_OPEN;
        return false;
    }

    struct stat buffer;
    dogecoin_bool create = true;
    if (stat(file_path, &buffer) == 0)
        create = false;

    enum logdb_error db_error = 0;
    if (!logdb_load(wallet->db, file_path, create, &db_error))
    {
        *error = db_error;
        return false;
    }

    return true;
}

dogecoin_bool dogecoin_wallet_flush(dogecoin_wallet *wallet)
{
    return logdb_flush(wallet->db);
}

void dogecoin_wallet_set_master_key_copy(dogecoin_wallet *wallet, dogecoin_hdnode *masterkey)
{
    if (!masterkey)
        return;

    if (wallet->masterkey != NULL)
    {
        //changing the master key should not be done,...
        //anyways, we are going to accept that at this point
        //consuming application needs to take care about that
        dogecoin_hdnode_free(wallet->masterkey);
        wallet->masterkey = NULL;
    }
    wallet->masterkey = dogecoin_hdnode_copy(masterkey);

    //serialize and store node
    char str[128];
    dogecoin_hdnode_serialize_private(wallet->masterkey, wallet->chain, str, sizeof(str));

    uint8_t key[strlen(hdmasterkey_key)+SHA256_DIGEST_LENGTH];
    memcpy(key, hdmasterkey_key, strlen(hdmasterkey_key));
    dogecoin_hash(wallet->masterkey->public_key, DOGECOIN_ECKEY_COMPRESSED_LENGTH, key+strlen(hdmasterkey_key));

    struct buffer buf_key = {key, sizeof(key)};
    struct buffer buf_val = {str, strlen(str)};
    logdb_append(wallet->db, &buf_key, &buf_val);
}

dogecoin_hdnode* dogecoin_wallet_next_key_new(dogecoin_wallet *wallet)
{
    if (!wallet && !wallet->masterkey)
        return NULL;

    //for now, only m/k is possible
    dogecoin_hdnode *node = dogecoin_hdnode_copy(wallet->masterkey);
    dogecoin_hdnode_private_ckd(node, wallet->next_childindex);

    //serialize and store node
    char str[128];
    dogecoin_hdnode_serialize_public(node, wallet->chain, str, sizeof(str));

    uint8_t key[strlen(hdkey_key)+20];
    memcpy(key, hdkey_key, strlen(hdkey_key)); //set the key prefix for the kv store
    dogecoin_hdnode_get_hash160(node, key+strlen(hdkey_key)); //append the hash160

    struct buffer buf_key = {key, sizeof(key)};
    struct buffer buf_val = {str, strlen(str)};
    logdb_append(wallet->db, &buf_key, &buf_val);
    logdb_flush(wallet->db);

    //add key to the rbtree
    cstring *hdnodehash = cstr_new_buf(((char *)buf_key.p)+strlen(hdkey_key),20);
    RBTreeInsert(wallet->hdkeys_rbtree,hdnodehash,dogecoin_hdnode_copy(node));

    //increase the in-memory counter (cache)
    wallet->next_childindex++;

    return node;
}

void dogecoin_wallet_get_addresses(dogecoin_wallet *wallet, vector *addr_out)
{
    rb_red_blk_node* hdkey_rbtree_node;

    if (!wallet)
        return;

    while((hdkey_rbtree_node = rbtree_enumerate_next(wallet->hdkeys_rbtree)))
    {
        cstring *key = hdkey_rbtree_node->key;
        uint8_t hash160[21];
        hash160[0] = wallet->chain->b58prefix_pubkey_address;
        memcpy(hash160+1, key->str, 20);

        size_t addrsize = 98;
        char *addr = calloc(1, addrsize);
        dogecoin_base58_encode_check(hash160, 21, addr, addrsize);
        vector_add(addr_out, addr);
    }
}

dogecoin_hdnode * dogecoin_wallet_find_hdnode_byaddr(dogecoin_wallet *wallet, const char *search_addr)
{
    if (!wallet || !search_addr)
        return NULL;

    uint8_t hashdata[strlen(search_addr)];
    memset(hashdata, 0, 20);
    dogecoin_base58_decode_check(search_addr, hashdata, strlen(search_addr));

    cstring keyhash160;
    keyhash160.str = (char *)hashdata+1;
    keyhash160.len = 20;
    rb_red_blk_node* node = RBExactQuery(wallet->hdkeys_rbtree, &keyhash160);
    if (node && node->info)
        return (dogecoin_hdnode *)node->info;
    else
        return NULL;
}

dogecoin_bool dogecoin_wallet_add_wtx(dogecoin_wallet *wallet, dogecoin_wtx *wtx)
{
    if (!wallet || !wtx)
        return false;

    cstring* txser = cstr_new_sz(1024);
    dogecoin_wallet_wtx_serialize(txser, wtx);

    uint8_t key[strlen(tx_key)+SHA256_DIGEST_LENGTH];
    memcpy(key, tx_key, strlen(tx_key));
    dogecoin_hash((const uint8_t*)txser->str, txser->len, key+strlen(tx_key));

    struct buffer buf_key = {key, sizeof(key)};
    struct buffer buf_val = {txser->str, txser->len};
    logdb_append(wallet->db, &buf_key, &buf_val);

    //add to spends
    dogecoin_wallet_add_to_spent(wallet, wtx);

    cstr_free(txser, true);

    return true;
}

dogecoin_bool dogecoin_wallet_have_key(dogecoin_wallet *wallet, uint8_t *hash160)
{
    if (!wallet)
        return false;

    cstring keyhash160;
    keyhash160.str = (char *)hash160;
    keyhash160.len = 20;
    rb_red_blk_node* node = RBExactQuery(wallet->hdkeys_rbtree, &keyhash160);
    if (node && node->info)
        return true;

    return false;
}

int64_t dogecoin_wallet_get_balance(dogecoin_wallet *wallet)
{
    rb_red_blk_node* hdkey_rbtree_node;
    int64_t credit = 0;

    if (!wallet)
        return false;

    // enumerate over the rbtree, calculate balance
    while((hdkey_rbtree_node = rbtree_enumerate_next(wallet->wtxes_rbtree)))
    {
        dogecoin_wtx *wtx = hdkey_rbtree_node->info;
        credit += dogecoin_wallet_wtx_get_credit(wallet, wtx);
    }

    return credit;
}

int64_t dogecoin_wallet_wtx_get_credit(dogecoin_wallet *wallet, dogecoin_wtx *wtx)
{
    int64_t credit = 0;

    if (dogecoin_tx_is_coinbase(wtx->tx) &&
        ( wallet->bestblockheight < COINBASE_MATURITY || wtx->height > wallet->bestblockheight - COINBASE_MATURITY )
        )
        return credit;

    uint256 hash;
    dogecoin_tx_hash(wtx->tx, hash);
    unsigned int i = 0;
    if (wtx->tx->vout) {
        for (i = 0; i < wtx->tx->vout->len; i++) {
            dogecoin_tx_out* tx_out;
            tx_out = vector_idx(wtx->tx->vout, i);

            if (!dogecoin_wallet_is_spent(wallet, hash, i))
            {
                if (dogecoin_wallet_txout_is_mine(wallet, tx_out))
                    credit += tx_out->value;
            }
        }
    }
    return credit;
}

dogecoin_bool dogecoin_wallet_txout_is_mine(dogecoin_wallet *wallet, dogecoin_tx_out *tx_out)
{
    dogecoin_bool ismine = false;

    vector *vec = vector_new(16, free);
    enum dogecoin_tx_out_type type2 = dogecoin_script_classify(tx_out->script_pubkey, vec);

    //TODO: Multisig, etc.
    if (type2 == DOGECOIN_TX_PUBKEYHASH)
    {
        //TODO: find a better format for vector elements (not a pure pointer)
        uint8_t *hash160 = vector_idx(vec, 0);
        if (dogecoin_wallet_have_key(wallet, hash160))
            ismine = true;
    }

    vector_free(vec, true);

    return ismine;
}

void dogecoin_wallet_add_to_spent(dogecoin_wallet *wallet, dogecoin_wtx *wtx)
{
    if (!wallet || !wtx)
        return;

    if (dogecoin_tx_is_coinbase(wtx->tx))
        return;
    
    unsigned int i = 0;
    if (wtx->tx->vin) {
        for (i = 0; i < wtx->tx->vin->len; i++) {
            dogecoin_tx_in* tx_in = vector_idx(wtx->tx->vin, i);

            //add to spends
            dogecoin_tx_outpoint *outpoint = calloc(1, sizeof(dogecoin_tx_outpoint));
            memcpy(outpoint, &tx_in->prevout, sizeof(dogecoin_tx_outpoint));
            vector_add(wallet->spends, outpoint);
        }
    }
}

dogecoin_bool dogecoin_wallet_is_spent(dogecoin_wallet *wallet, uint256 hash, uint32_t n)
{
    if (!wallet)
        return false;

    unsigned int i = 0;
    for (i = wallet->spends->len ; i > 0; i--)
    {
        dogecoin_tx_outpoint *outpoint = vector_idx(wallet->spends, i-1);
        if (memcmp(outpoint->hash, hash, SHA256_DIGEST_LENGTH) == 0 && n == outpoint->n)
            return true;
    }
    return false;
}

dogecoin_bool dogecoin_wallet_get_unspent(dogecoin_wallet *wallet, vector *unspents)
{
    rb_red_blk_node* hdkey_rbtree_node;

    if (!wallet)
        return false;

    while((hdkey_rbtree_node = rbtree_enumerate_next(wallet->wtxes_rbtree)))
    {
        dogecoin_wtx *wtx = hdkey_rbtree_node->info;
        cstring *key = hdkey_rbtree_node->key;
        uint8_t *hash = (uint8_t *)key->str;

        unsigned int i = 0;
        if (wtx->tx->vout) {
            for (i = 0; i < wtx->tx->vout->len; i++) {
                dogecoin_tx_out* tx_out;
                tx_out = vector_idx(wtx->tx->vout, i);

                if (!dogecoin_wallet_is_spent(wallet, hash, i))
                {
                    if (dogecoin_wallet_txout_is_mine(wallet, tx_out))
                    {
                        dogecoin_output *output = dogecoin_wallet_output_new();
                        dogecoin_wallet_wtx_free(output->wtx);
                        output->wtx = dogecoin_wallet_wtx_copy(wtx);
                        output->i = i;
                        vector_add(unspents, output);
                    }
                }
            }
        }
    }

    return true;
}
