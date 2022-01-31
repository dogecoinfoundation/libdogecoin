/*

 The MIT License (MIT)

 Copyright (c) 2015 Douglas J. Bakkum
 Copyright (c) 2015 Jonas Schnelli
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

#include <inttypes.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include <dogecoin/base58.h>
#include <dogecoin/serialize.h>
#include <dogecoin/crypto/sha2.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

void dogecoin_tx_in_free(dogecoin_tx_in* tx_in)
{
    if (!tx_in)
        return;

    memset(&tx_in->prevout.hash, 0, 32);
    tx_in->prevout.n = 0;

    if (tx_in->script_sig) {
        cstr_free(tx_in->script_sig, true);
        tx_in->script_sig = NULL;
    }
}

//callback for vector free function
void dogecoin_tx_in_free_cb(void* data)
{
    if (!data)
        return;

    dogecoin_tx_in* tx_in = data;
    dogecoin_tx_in_free(tx_in);

    memset(tx_in, 0, sizeof(*tx_in));
    free(tx_in);
}


dogecoin_tx_in* dogecoin_tx_in_new()
{
    dogecoin_tx_in* tx_in;
    tx_in = calloc(1, sizeof(*tx_in));
    memset(&tx_in->prevout, 0, sizeof(tx_in->prevout));
    tx_in->sequence = UINT32_MAX;
    return tx_in;
}


void dogecoin_tx_out_free(dogecoin_tx_out* tx_out)
{
    if (!tx_out)
        return;
    tx_out->value = 0;

    if (tx_out->script_pubkey) {
        cstr_free(tx_out->script_pubkey, true);
        tx_out->script_pubkey = NULL;
    }
}


void dogecoin_tx_out_free_cb(void* data)
{
    if (!data)
        return;

    dogecoin_tx_out* tx_out = data;
    dogecoin_tx_out_free(tx_out);

    memset(tx_out, 0, sizeof(*tx_out));
    free(tx_out);
}


dogecoin_tx_out* dogecoin_tx_out_new()
{
    dogecoin_tx_out* tx_out;
    tx_out = calloc(1, sizeof(*tx_out));

    return tx_out;
}


void dogecoin_tx_free(dogecoin_tx* tx)
{
    if (tx->vin)
        vector_free(tx->vin, true);

    if (tx->vout)
        vector_free(tx->vout, true);

    free(tx);
}


dogecoin_tx* dogecoin_tx_new()
{
    dogecoin_tx* tx;
    tx = calloc(1, sizeof(*tx));
    tx->vin = vector_new(8, dogecoin_tx_in_free_cb);
    tx->vout = vector_new(8, dogecoin_tx_out_free_cb);
    tx->version = 1;
    tx->locktime = 0;
    return tx;
}


dogecoin_bool dogecoin_tx_in_deserialize(dogecoin_tx_in* tx_in, struct const_buffer* buf)
{
    deser_u256(tx_in->prevout.hash, buf);
    if (!deser_u32(&tx_in->prevout.n, buf))
        return false;
    if (!deser_varstr(&tx_in->script_sig, buf))
        return false;
    if (!deser_u32(&tx_in->sequence, buf))
        return false;
    return true;
}

dogecoin_bool dogecoin_tx_out_deserialize(dogecoin_tx_out* tx_out, struct const_buffer* buf)
{
    if (!deser_s64(&tx_out->value, buf))
        return false;
    if (!deser_varstr(&tx_out->script_pubkey, buf))
        return false;
    return true;
}

int dogecoin_tx_deserialize(const unsigned char* tx_serialized, size_t inlen, dogecoin_tx* tx)
{
    struct const_buffer buf = {tx_serialized, inlen};

    //tx needs to be initialized
    deser_s32(&tx->version, &buf);
    uint32_t vlen;
    if (!deser_varlen(&vlen, &buf))
        return false;

    unsigned int i;
    for (i = 0; i < vlen; i++) {
        dogecoin_tx_in* tx_in = dogecoin_tx_in_new();

        if (!dogecoin_tx_in_deserialize(tx_in, &buf)) {
            free(tx_in);
        }

        vector_add(tx->vin, tx_in);
    }

    if (!deser_varlen(&vlen, &buf))
        return false;
    for (i = 0; i < vlen; i++) {
        dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

        if (!dogecoin_tx_out_deserialize(tx_out, &buf)) {
            free(tx_out);
        }

        vector_add(tx->vout, tx_out);
    }

    if (!deser_u32(&tx->locktime, &buf))
        return false;

    return true;
}

void dogecoin_tx_in_serialize(cstring* s, const dogecoin_tx_in* tx_in)
{
    ser_u256(s, tx_in->prevout.hash);
    ser_u32(s, tx_in->prevout.n);
    ser_varstr(s, tx_in->script_sig);
    ser_u32(s, tx_in->sequence);
}

void dogecoin_tx_out_serialize(cstring* s, const dogecoin_tx_out* tx_out)
{
    ser_s64(s, tx_out->value);
    ser_varstr(s, tx_out->script_pubkey);
}

void dogecoin_tx_serialize(cstring* s, const dogecoin_tx* tx)
{
    ser_s32(s, tx->version);

    ser_varlen(s, tx->vin ? tx->vin->len : 0);

    unsigned int i;
    if (tx->vin) {
        for (i = 0; i < tx->vin->len; i++) {
            dogecoin_tx_in* tx_in;

            tx_in = vector_idx(tx->vin, i);
            dogecoin_tx_in_serialize(s, tx_in);
        }
    }

    ser_varlen(s, tx->vout ? tx->vout->len : 0);

    if (tx->vout) {
        for (i = 0; i < tx->vout->len; i++) {
            dogecoin_tx_out* tx_out;

            tx_out = vector_idx(tx->vout, i);
            dogecoin_tx_out_serialize(s, tx_out);
        }
    }

    ser_u32(s, tx->locktime);
}

void dogecoin_tx_hash(const dogecoin_tx* tx, uint8_t* hashout)
{
    cstring* txser = cstr_new_sz(1024);
    dogecoin_tx_serialize(txser, tx);


    sha256_Raw((const uint8_t*)txser->str, txser->len, hashout);
    sha256_Raw(hashout, 32, hashout);
    cstr_free(txser, true);
}


void dogecoin_tx_in_copy(dogecoin_tx_in* dest, const dogecoin_tx_in* src)
{
    memcpy(&dest->prevout, &src->prevout, sizeof(dest->prevout));
    dest->sequence = src->sequence;

    if (!src->script_sig)
        dest->script_sig = NULL;
    else {
        dest->script_sig = cstr_new_sz(src->script_sig->len);
        cstr_append_buf(dest->script_sig,
                        src->script_sig->str,
                        src->script_sig->len);
    }
}


void dogecoin_tx_out_copy(dogecoin_tx_out* dest, const dogecoin_tx_out* src)
{
    dest->value = src->value;

    if (!src->script_pubkey)
        dest->script_pubkey = NULL;
    else {
        dest->script_pubkey = cstr_new_sz(src->script_pubkey->len);
        cstr_append_buf(dest->script_pubkey,
                        src->script_pubkey->str,
                        src->script_pubkey->len);
    }
}


void dogecoin_tx_copy(dogecoin_tx* dest, const dogecoin_tx* src)
{
    dest->version = src->version;
    dest->locktime = src->locktime;

    if (!src->vin)
        dest->vin = NULL;
    else {
        unsigned int i;

        if (dest->vin)
            vector_free(dest->vin, true);

        dest->vin = vector_new(src->vin->len, dogecoin_tx_in_free_cb);

        for (i = 0; i < src->vin->len; i++) {
            dogecoin_tx_in *tx_in_old, *tx_in_new;

            tx_in_old = vector_idx(src->vin, i);
            tx_in_new = calloc(1, sizeof(*tx_in_new));
            dogecoin_tx_in_copy(tx_in_new, tx_in_old);
            vector_add(dest->vin, tx_in_new);
        }
    }

    if (!src->vout)
        dest->vout = NULL;
    else {
        unsigned int i;

        if (dest->vout)
            vector_free(dest->vout, true);

        dest->vout = vector_new(src->vout->len,
                                dogecoin_tx_out_free_cb);

        for (i = 0; i < src->vout->len; i++) {
            dogecoin_tx_out *tx_out_old, *tx_out_new;

            tx_out_old = vector_idx(src->vout, i);
            tx_out_new = calloc(1, sizeof(*tx_out_new));
            dogecoin_tx_out_copy(tx_out_new, tx_out_old);
            vector_add(dest->vout, tx_out_new);
        }
    }
}

dogecoin_bool dogecoin_tx_sighash(const dogecoin_tx* tx_to, const cstring* fromPubKey, unsigned int in_num, int hashtype, uint8_t* hash)
{
    if (in_num >= tx_to->vin->len)
        return false;

    dogecoin_bool ret = true;

    dogecoin_tx* tx_tmp = dogecoin_tx_new();
    dogecoin_tx_copy(tx_tmp, tx_to);

    cstring* new_script = cstr_new_sz(fromPubKey->len);
    dogecoin_script_copy_without_op_codeseperator(fromPubKey, new_script);

    unsigned int i;
    dogecoin_tx_in* tx_in;
    for (i = 0; i < tx_tmp->vin->len; i++) {
        tx_in = vector_idx(tx_tmp->vin, i);
        cstr_resize(tx_in->script_sig, 0);

        if (i == in_num)
            cstr_append_buf(tx_in->script_sig,
                            new_script->str,
                            new_script->len);
    }
    cstr_free(new_script, true);
    /* Blank out some of the outputs */
    if ((hashtype & 0x1f) == SIGHASH_NONE) {
        /* Wildcard payee */
        if (tx_tmp->vout)
            vector_free(tx_tmp->vout, true);

        tx_tmp->vout = vector_new(1, dogecoin_tx_out_free_cb);

        /* Let the others update at will */
        for (i = 0; i < tx_tmp->vin->len; i++) {
            tx_in = vector_idx(tx_tmp->vin, i);
            if (i != in_num)
                tx_in->sequence = 0;
        }
    }

    else if ((hashtype & 0x1f) == SIGHASH_SINGLE) {
        /* Only lock-in the txout payee at same index as txin */
        unsigned int n_out = in_num;
        if (n_out >= tx_tmp->vout->len) {
            //TODO: set error code
            ret = false;
            goto out;
        }

        vector_resize(tx_tmp->vout, n_out + 1);

        for (i = 0; i < n_out; i++) {
            dogecoin_tx_out* tx_out;

            tx_out = vector_idx(tx_tmp->vout, i);
            tx_out->value = -1;
            if (tx_out->script_pubkey) {
                cstr_free(tx_out->script_pubkey, true);
                tx_out->script_pubkey = NULL;
            }
        }

        /* Let the others update at will */
        for (i = 0; i < tx_tmp->vin->len; i++) {
            tx_in = vector_idx(tx_tmp->vin, i);
            if (i != in_num)
                tx_in->sequence = 0;
        }
    }

    /* Blank out other inputs completely;
     not recommended for open transactions */
    if (hashtype & SIGHASH_ANYONECANPAY) {
        if (in_num > 0)
            vector_remove_range(tx_tmp->vin, 0, in_num);
        vector_resize(tx_tmp->vin, 1);
    }

    cstring* s = cstr_new_sz(512);
    dogecoin_tx_serialize(s, tx_tmp);
    ser_s32(s, hashtype);

    sha256_Raw((const uint8_t*)s->str, s->len, hash);
    sha256_Raw(hash, 32, hash);

    cstr_free(s, true);

out:
    dogecoin_tx_free(tx_tmp);

    return ret;
}


dogecoin_bool dogecoin_tx_add_address_out(dogecoin_tx* tx, const dogecoin_chain* chain, int64_t amount, const char* address)
{
    uint8_t buf[strlen(address) * 2];
    int r = dogecoin_base58_decode_check(address, buf, sizeof(buf));
    if (r <= 0)
        return false;

    if (buf[0] == chain->b58prefix_pubkey_address) {
        dogecoin_tx_add_p2pkh_hash160_out(tx, amount, &buf[1]);
    } else if (buf[0] == chain->b58prefix_script_address) {
        dogecoin_tx_add_p2sh_hash160_out(tx, amount, &buf[1]);
    }

    return true;
}


dogecoin_bool dogecoin_tx_add_p2pkh_hash160_out(dogecoin_tx* tx, int64_t amount, uint8_t* hash160)
{
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_build_p2pkh(tx_out->script_pubkey, hash160);

    tx_out->value = amount;

    vector_add(tx->vout, tx_out);

    return true;
}

dogecoin_bool dogecoin_tx_add_p2sh_hash160_out(dogecoin_tx* tx, int64_t amount, uint8_t* hash160)
{
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_build_p2sh(tx_out->script_pubkey, hash160);

    tx_out->value = amount;

    vector_add(tx->vout, tx_out);

    return true;
}

dogecoin_bool dogecoin_tx_add_p2pkh_out(dogecoin_tx* tx, int64_t amount, const dogecoin_pubkey* pubkey)
{
    uint8_t hash160[20];
    dogecoin_pubkey_get_hash160(pubkey, hash160);
    return dogecoin_tx_add_p2pkh_hash160_out(tx, amount, hash160);
}

dogecoin_bool dogecoin_tx_outpoint_is_null(dogecoin_tx_outpoint* tx)
{
    (void)(tx);
    return true;
}

dogecoin_bool dogecoin_tx_is_coinbase(dogecoin_tx* tx)
{
    if (tx->vin->len == 1)
    {
        dogecoin_tx_in *vin = vector_idx(tx->vin, 0);

        if (dogecoin_hash_is_empty(vin->prevout.hash) && vin->prevout.n == UINT32_MAX)
            return true;
    }
    return false;
}
