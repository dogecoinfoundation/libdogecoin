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

#include <assert.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <dogecoin/base58.h>
#include <dogecoin/ecc.h>
#include <dogecoin/memory.h>
#include <dogecoin/segwit_addr.h>
#include <dogecoin/serialize.h>
#include <dogecoin/sha2.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

void dogecoin_tx_in_free(dogecoin_tx_in* tx_in)
{
    if (!tx_in)
        return;

    memset(&tx_in->prevout.hash, 0, sizeof(tx_in->prevout.hash));
    tx_in->prevout.n = 0;

    if (tx_in->script_sig) {
        cstr_free(tx_in->script_sig, true);
        tx_in->script_sig = NULL;
    }

    if (tx_in->witness_stack) {
        vector_free(tx_in->witness_stack, true);
        tx_in->witness_stack = NULL;
    }

    memset(tx_in, 0, sizeof(*tx_in));
    dogecoin_free(tx_in);
}

//callback for vector free function
void dogecoin_tx_in_free_cb(void* data)
{
    if (!data)
        return;

    dogecoin_tx_in* tx_in = data;
    dogecoin_tx_in_free(tx_in);
}

void dogecoin_tx_in_witness_stack_free_cb(void* data)
{
    if (!data)
        return;

    cstring* stack_item = data;
    cstr_free(stack_item, true);
}

dogecoin_tx_in* dogecoin_tx_in_new()
{
    dogecoin_tx_in* tx_in;
    tx_in = dogecoin_calloc(1, sizeof(*tx_in));
    memset(&tx_in->prevout, 0, sizeof(tx_in->prevout));
    tx_in->sequence = UINT32_MAX;

    tx_in->witness_stack = vector_new(8, dogecoin_tx_in_witness_stack_free_cb);
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

    memset(tx_out, 0, sizeof(*tx_out));
    dogecoin_free(tx_out);
}


void dogecoin_tx_out_free_cb(void* data)
{
    if (!data)
        return;

    dogecoin_tx_out* tx_out = data;
    dogecoin_tx_out_free(tx_out);
}


dogecoin_tx_out* dogecoin_tx_out_new()
{
    dogecoin_tx_out* tx_out;
    tx_out = dogecoin_calloc(1, sizeof(*tx_out));

    return tx_out;
}


void dogecoin_tx_free(dogecoin_tx* tx)
{
    if (tx->vin)
        vector_free(tx->vin, true);

    if (tx->vout)
        vector_free(tx->vout, true);

    dogecoin_free(tx);
}


dogecoin_tx* dogecoin_tx_new()
{
    dogecoin_tx* tx;
    tx = dogecoin_calloc(1, sizeof(*tx));
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

int dogecoin_tx_deserialize(const unsigned char* tx_serialized, size_t inlen, dogecoin_tx* tx, size_t* consumed_length, dogecoin_bool allow_witness)
{
    struct const_buffer buf = {tx_serialized, inlen};
    if (consumed_length)
        *consumed_length = 0;

    //tx needs to be initialized
    deser_s32(&tx->version, &buf);

    uint32_t vlen;
    if (!deser_varlen(&vlen, &buf))
        return false;

    uint8_t flags = 0;
    if (vlen == 0 && allow_witness) {
        /* We read a dummy or an empty vin. */
        deser_bytes(&flags, &buf, 1);
        if (flags != 0) {
            // contains witness, deser the vin len
            if (!deser_varlen(&vlen, &buf))
                return false;
        }
    }

    unsigned int i;
    for (i = 0; i < vlen; i++) {
        dogecoin_tx_in* tx_in = dogecoin_tx_in_new();

        if (!dogecoin_tx_in_deserialize(tx_in, &buf)) {
            dogecoin_tx_in_free(tx_in);
            return false;
        } else {
            vector_add(tx->vin, tx_in);
        }
    }

    if (!deser_varlen(&vlen, &buf))
        return false;
    for (i = 0; i < vlen; i++) {
        dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

        if (!dogecoin_tx_out_deserialize(tx_out, &buf)) {
            dogecoin_free(tx_out);
            return false;
        } else {
            vector_add(tx->vout, tx_out);
        }
    }

    if ((flags & 1) && allow_witness) {
        /* The witness flag is present, and we support witnesses. */
        flags ^= 1;
        for (size_t i = 0; i < tx->vin->len; i++) {
            dogecoin_tx_in *tx_in = vector_idx(tx->vin, i);
            uint32_t vlen;
            if (!deser_varlen(&vlen, &buf)) return false;
            for (size_t j = 0; j < vlen; j++) {
                cstring* witness_item = cstr_new_sz(1024);
                if (!deser_varstr(&witness_item, &buf)) {
                    cstr_free(witness_item, true);
                    return false;
                }
                vector_add(tx_in->witness_stack, witness_item); //vector is responsible for freeing the items memory
            }
        }
    }
    if (flags) {
        /* Unknown flag in the serialization */
        return false;
    }

    if (!deser_u32(&tx->locktime, &buf))
        return false;

    if (consumed_length)
        *consumed_length = inlen - buf.len;
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

dogecoin_bool dogecoin_tx_has_witness(const dogecoin_tx *tx)
{
    for (size_t i = 0; i < tx->vin->len; i++) {
        dogecoin_tx_in *tx_in = vector_idx(tx->vin, i);
        if (tx_in->witness_stack != NULL && tx_in->witness_stack->len > 0) {
            return true;
        }
    }
    return false;
}

void dogecoin_tx_serialize(cstring* s, const dogecoin_tx* tx, dogecoin_bool allow_witness)
{
    ser_s32(s, tx->version);
    uint8_t flags = 0;
    // Consistency check
    if (allow_witness) {
        /* Check whether witnesses need to be serialized. */
        if (dogecoin_tx_has_witness(tx)) {
            flags |= 1;
        }
    }
    if (flags) {
        /* Use extended format in case witnesses are to be serialized. */
        uint8_t dummy = 0;
        ser_bytes(s, &dummy, 1);
        ser_bytes(s, &flags, 1);
    }

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

    if (flags & 1) {
        // serialize the witness stack
        if (tx->vin) {
            for (i = 0; i < tx->vin->len; i++) {
                dogecoin_tx_in* tx_in;
                tx_in = vector_idx(tx->vin, i);
                if (tx_in->witness_stack) {
                    ser_varlen(s, tx_in->witness_stack->len);
                    for (unsigned int j = 0; j < tx_in->witness_stack->len; j++) {
                        cstring *item = vector_idx(tx_in->witness_stack, j);
                        ser_varstr(s, item);
                    }
                }
            }
        }
    }

    ser_u32(s, tx->locktime);
}

void dogecoin_tx_hash(const dogecoin_tx* tx, uint256 hashout)
{
    cstring* txser = cstr_new_sz(1024);
    dogecoin_tx_serialize(txser, tx, false);


    sha256_Raw((const uint8_t*)txser->str, txser->len, hashout);
    sha256_Raw(hashout, DOGECOIN_HASH_LENGTH, hashout);
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

    if (!src->witness_stack)
        dest->witness_stack = NULL;
    else {
        dest->witness_stack = vector_new(src->witness_stack->len, dogecoin_tx_in_witness_stack_free_cb);
        for (unsigned int i = 0; i < src->witness_stack->len; i++) {
            cstring *witness_item = vector_idx(src->witness_stack, i);
            cstring *item_cpy = cstr_new_cstr(witness_item);
            vector_add(dest->witness_stack, item_cpy);
        }
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
            tx_in_new = dogecoin_malloc(sizeof(*tx_in_new));
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
            tx_out_new = dogecoin_malloc(sizeof(*tx_out_new));
            dogecoin_tx_out_copy(tx_out_new, tx_out_old);
            vector_add(dest->vout, tx_out_new);
        }
    }
}

void dogecoin_tx_prevout_hash(const dogecoin_tx* tx, uint256 hash) {
    cstring* s = cstr_new_sz(512);
    unsigned int i;
    dogecoin_tx_in* tx_in;
    for (i = 0; i < tx->vin->len; i++) {
        tx_in = vector_idx(tx->vin, i);
        ser_u256(s, tx_in->prevout.hash);
        ser_u32(s, tx_in->prevout.n);
    }

    dogecoin_hash((const uint8_t*)s->str, s->len, hash);
    cstr_free(s, true);
}


void dogecoin_tx_sequence_hash(const dogecoin_tx* tx, uint256 hash) {
    cstring* s = cstr_new_sz(512);
    unsigned int i;
    dogecoin_tx_in* tx_in;
    for (i = 0; i < tx->vin->len; i++) {
        tx_in = vector_idx(tx->vin, i);
        ser_u32(s, tx_in->sequence);
    }

    dogecoin_hash((const uint8_t*)s->str, s->len, hash);
    cstr_free(s, true);
}

void dogecoin_tx_outputs_hash(const dogecoin_tx* tx, uint256 hash) {
    cstring* s = cstr_new_sz(512);
    unsigned int i;
    dogecoin_tx_out* tx_out;
    for (i = 0; i < tx->vout->len; i++) {
        tx_out = vector_idx(tx->vout, i);
        dogecoin_tx_out_serialize(s, tx_out);
    }

    dogecoin_hash((const uint8_t*)s->str, s->len, hash);
    cstr_free(s, true);
}

dogecoin_bool dogecoin_tx_sighash(const dogecoin_tx* tx_to, const cstring* fromPubKey, unsigned int in_num, int hashtype, const uint64_t amount, const enum dogecoin_sig_version sigversion, uint256 hash)
{
    if (in_num >= tx_to->vin->len)
        return false;

    dogecoin_bool ret = true;

    dogecoin_tx* tx_tmp = dogecoin_tx_new();
    dogecoin_tx_copy(tx_tmp, tx_to);

    cstring* s = NULL;

    // segwit
    if (sigversion == SIGVERSION_WITNESS_V0) {
        uint256 hash_prevouts;
        dogecoin_hash_clear(hash_prevouts);
        uint256 hash_sequence;
        dogecoin_hash_clear(hash_sequence);
        uint256 hash_outputs;
        dogecoin_hash_clear(hash_outputs);

        if (!(hashtype & SIGHASH_ANYONECANPAY)) {
            dogecoin_tx_prevout_hash(tx_tmp, hash_prevouts);
        }
        if (!(hashtype & SIGHASH_ANYONECANPAY)) {
            dogecoin_tx_outputs_hash(tx_tmp, hash_outputs);
        }
        if (!(hashtype & SIGHASH_ANYONECANPAY) && (hashtype & 0x1f) != SIGHASH_SINGLE && (hashtype & 0x1f) != SIGHASH_NONE) {
            dogecoin_tx_sequence_hash(tx_tmp, hash_sequence);
        }

        if ((hashtype & 0x1f) != SIGHASH_SINGLE && (hashtype & 0x1f) != SIGHASH_NONE) {
            dogecoin_tx_outputs_hash(tx_tmp, hash_outputs);
        } else if ((hashtype & 0x1f) == SIGHASH_SINGLE && in_num < tx_tmp->vout->len) {
            cstring* s1 = cstr_new_sz(512);
            dogecoin_tx_out* tx_out = vector_idx(tx_tmp->vout, in_num);
            dogecoin_tx_out_serialize(s1, tx_out);
            dogecoin_hash((const uint8_t*)s1->str, s1->len, hash);
            cstr_free(s1, true);
        }

        s = cstr_new_sz(512);
        ser_u32(s, tx_tmp->version); // Version

        // Input prevouts/nSequence (none/all, depending on flags)
        ser_u256(s, hash_prevouts);
        ser_u256(s, hash_sequence);

        // The input being signed (replacing the scriptSig with scriptCode + amount)
        // The prevout may already be contained in hashPrevout, and the nSequence
        // may already be contain in hashSequence.
        dogecoin_tx_in* tx_in = vector_idx(tx_tmp->vin, in_num);
        ser_u256(s, tx_in->prevout.hash);
        ser_u32(s, tx_in->prevout.n);

        ser_varstr(s, (cstring *)fromPubKey); // script code

        ser_u64(s, amount);
        ser_u32(s, tx_in->sequence);
        ser_u256(s, hash_outputs); // Outputs (none/one/all, depending on flags)
        ser_u32(s, tx_tmp->locktime); // Locktime
        ser_s32(s, hashtype); // Sighash type
    }
    else {
        // standard (non witness) sighash (SIGVERSION_BASE)
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

        s = cstr_new_sz(512);
        dogecoin_tx_serialize(s, tx_tmp, false);
        ser_s32(s, hashtype);
    }

    //char str[10000];
    //memset(str, strlen(str), 0);
    //utils_bin_to_hex((unsigned char *)s->str, s->len, str);
    //printf("\n");
    //printf("%s\n", str);

    sha256_Raw((const uint8_t*)s->str, s->len, hash);
    sha256_Raw(hash, DOGECOIN_HASH_LENGTH, hash);

    cstr_free(s, true);

out:
    dogecoin_tx_free(tx_tmp);

    return ret;
}

dogecoin_bool dogecoin_tx_add_data_out(dogecoin_tx* tx, const int64_t amount, const uint8_t *data, const size_t datalen)
{
    if (datalen > 80)
        return false;

    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_append_op(tx_out->script_pubkey , OP_RETURN);
    dogecoin_script_append_pushdata(tx_out->script_pubkey, (unsigned char*)data, datalen);

    tx_out->value = amount;

    vector_add(tx->vout, tx_out);

    return true;
}

dogecoin_bool dogecoin_tx_add_puzzle_out(dogecoin_tx* tx, const int64_t amount, const uint8_t *puzzle, const size_t puzzlelen)
{
    if (puzzlelen > DOGECOIN_HASH_LENGTH)
        return false;

    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_append_op(tx_out->script_pubkey , OP_HASH256);
    dogecoin_script_append_pushdata(tx_out->script_pubkey, (unsigned char*)puzzle, puzzlelen);
    dogecoin_script_append_op(tx_out->script_pubkey , OP_EQUAL);
    tx_out->value = amount;

    vector_add(tx->vout, tx_out);

    return true;
}

dogecoin_bool dogecoin_tx_add_address_out(dogecoin_tx* tx, const dogecoin_chainparams* chain, int64_t amount, const char* address)
{
    const size_t buflen = sizeof(uint8_t) * strlen(address) * 2;
    uint8_t *buf = (uint8_t *)dogecoin_malloc(buflen);
    int r = dogecoin_base58_decode_check(address, buf, buflen);
    if (r > 0 && buf[0] == chain->b58prefix_pubkey_address) {
        dogecoin_tx_add_p2pkh_hash160_out(tx, amount, &buf[1]);
    } else if (r > 0 && buf[0] == chain->b58prefix_script_address) {
        dogecoin_tx_add_p2sh_hash160_out(tx, amount, &buf[1]);
    }
    else {
        // check for bech32
        int version = 0;
        unsigned char programm[40] = {0};
        size_t programmlen = 0;
        if(segwit_addr_decode(&version, programm, &programmlen, chain->bech32_hrp, address) == 1) {
            if (programmlen == 20) {
                dogecoin_tx_out* tx_out = dogecoin_tx_out_new();
                tx_out->script_pubkey = cstr_new_sz(1024);

                dogecoin_script_build_p2wpkh(tx_out->script_pubkey, (const uint8_t *)programm);

                tx_out->value = amount;
                vector_add(tx->vout, tx_out);
            }
        }
        dogecoin_free(buf);
        return false;
    }

    dogecoin_free(buf);
    return true;
}


dogecoin_bool dogecoin_tx_add_p2pkh_hash160_out(dogecoin_tx* tx, int64_t amount, uint160 hash160)
{
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_build_p2pkh(tx_out->script_pubkey, hash160);

    tx_out->value = amount;

    vector_add(tx->vout, tx_out);

    return true;
}

dogecoin_bool dogecoin_tx_add_p2sh_hash160_out(dogecoin_tx* tx, int64_t amount, uint160 hash160)
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
    uint160 hash160;
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
    if (tx->vin->len == 1) {
        dogecoin_tx_in* vin = vector_idx(tx->vin, 0);

        if (dogecoin_hash_is_empty(vin->prevout.hash) && vin->prevout.n == UINT32_MAX)
            return true;
    }
    return false;
}

const char* dogecoin_tx_sign_result_to_str(const enum dogecoin_tx_sign_result result) {
    if (result == DOGECOIN_SIGN_OK) {
        return "OK";
    }
    else if (result == DOGECOIN_SIGN_INVALID_TX_OR_SCRIPT) {
        return "INVALID_TX_OR_SCRIPT";
    }
    else if (result == DOGECOIN_SIGN_INPUTINDEX_OUT_OF_RANGE) {
        return "INPUTINDEX_OUT_OF_RANGE";
    }
    else if (result == DOGECOIN_SIGN_INVALID_KEY) {
        return "INVALID_KEY";
    }
    else if (result == DOGECOIN_SIGN_NO_KEY_MATCH) {
        return "NO_KEY_MATCH";
    }
    else if (result == DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE) {
        return "SIGN_UNKNOWN_SCRIPT_TYPE";
    }
    else if (result == DOGECOIN_SIGN_SIGHASH_FAILED) {
        return "SIGHASH_FAILED";
    }
    return "UNKOWN";
}

enum dogecoin_tx_sign_result dogecoin_tx_sign_input(dogecoin_tx *tx_in_out, const cstring *script, uint64_t amount, const dogecoin_key *privkey, int inputindex, int sighashtype, uint8_t *sigcompact_out, uint8_t *sigder_out, int *sigder_len_out) {
    if (!tx_in_out || !script) {
        return DOGECOIN_SIGN_INVALID_TX_OR_SCRIPT;
    }
    if ((size_t)inputindex >= tx_in_out->vin->len) {
        return DOGECOIN_SIGN_INPUTINDEX_OUT_OF_RANGE;
    }
    if (!dogecoin_privkey_is_valid(privkey)) {
        return DOGECOIN_SIGN_INVALID_KEY;
    }
    // calculate pubkey
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(privkey, &pubkey);
    if (!dogecoin_pubkey_is_valid(&pubkey)) {
        return DOGECOIN_SIGN_INVALID_KEY;
    }
    enum dogecoin_tx_sign_result res = DOGECOIN_SIGN_OK;

    cstring *script_sign = cstr_new_cstr(script); //copy the script because we may modify it
    dogecoin_tx_in *tx_in = vector_idx(tx_in_out->vin, inputindex);
    vector *script_pushes = vector_new(1, free);

    cstring *witness_set_scriptsig = NULL; //required in order to set the P2SH-P2WPKH scriptSig
    enum dogecoin_tx_out_type type = dogecoin_script_classify(script, script_pushes);
    enum dogecoin_sig_version sig_version = SIGVERSION_BASE;
    if (type == DOGECOIN_TX_SCRIPTHASH) {
        // p2sh script, need the redeem script
        // for now, pretend to be a p2sh-p2wpkh
        vector_free(script_pushes, true);
        script_pushes = vector_new(1, free);
        type = DOGECOIN_TX_WITNESS_V0_PUBKEYHASH;
        uint8_t *hash160 = dogecoin_calloc(1, 20);
        dogecoin_pubkey_get_hash160(&pubkey, hash160);
        vector_add(script_pushes, hash160);

        // set the script sig
        witness_set_scriptsig = cstr_new_sz(22);
        uint8_t version = 0;
        ser_varlen(witness_set_scriptsig, 22);
        ser_bytes(witness_set_scriptsig, &version, 1);
        ser_varlen(witness_set_scriptsig, 20);
        ser_bytes(witness_set_scriptsig, hash160, 20);
    }
    if (type == DOGECOIN_TX_PUBKEYHASH && script_pushes->len == 1) {
        // check if given private key matches the script
        uint160 hash160;
        dogecoin_pubkey_get_hash160(&pubkey, hash160);
        uint160 *hash160_in_script = vector_idx(script_pushes, 0);
        if (memcmp(hash160_in_script, hash160, sizeof(hash160)) != 0) {
            res = DOGECOIN_SIGN_NO_KEY_MATCH; //sign anyways
        }
    }
    else if (type == DOGECOIN_TX_WITNESS_V0_PUBKEYHASH && script_pushes->len == 1) {
        uint160 *hash160_in_script = vector_idx(script_pushes, 0);
        sig_version = SIGVERSION_WITNESS_V0;

        // check if given private key matches the script
        uint160 hash160;
        dogecoin_pubkey_get_hash160(&pubkey, hash160);
        if (memcmp(hash160_in_script, hash160, sizeof(hash160)) != 0) {
            res = DOGECOIN_SIGN_NO_KEY_MATCH; //sign anyways
        }

        cstr_resize(script_sign, 0);
        dogecoin_script_build_p2pkh(script_sign, *hash160_in_script);
    }
    else {
        // unknown script, however, still try to create a signature (don't apply though)
        res = DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE;
    }
    vector_free(script_pushes, true);

    uint256 sighash;
    memset(sighash, 0, sizeof(sighash));
    if(!dogecoin_tx_sighash(tx_in_out, script_sign, inputindex, sighashtype, amount, sig_version, sighash)) {
        cstr_free(witness_set_scriptsig, true);
        cstr_free(script_sign, true);
        return DOGECOIN_SIGN_SIGHASH_FAILED;
    }
    cstr_free(script_sign, true);
    // sign compact
    uint8_t sig[64];
    size_t siglen = 0;
    dogecoin_key_sign_hash_compact(privkey, sighash, sig, &siglen);
    assert(siglen == sizeof(sig));
    if (sigcompact_out) {
        memcpy(sigcompact_out, sig, siglen);
    }

    // form normalized DER signature & hashtype
    unsigned char sigder_plus_hashtype[74+1];
    size_t sigderlen = 75;
    dogecoin_ecc_compact_to_der_normalized(sig, sigder_plus_hashtype, &sigderlen);
    assert(sigderlen <= 74 && sigderlen >= 70);
    sigder_plus_hashtype[sigderlen] = sighashtype;
    sigderlen+=1; //+hashtype
    if (sigcompact_out) {
        memcpy(sigder_out, sigder_plus_hashtype, sigderlen);
    }
    if (sigder_len_out) {
        *sigder_len_out = sigderlen;
    }

    // apply signature depending on script type
    if (type == DOGECOIN_TX_PUBKEYHASH) {
        // apply DER sig
        ser_varlen(tx_in->script_sig, sigderlen);
        ser_bytes(tx_in->script_sig, sigder_plus_hashtype, sigderlen);

        // apply pubkey
        ser_varlen(tx_in->script_sig, pubkey.compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH);
        ser_bytes(tx_in->script_sig, pubkey.pubkey, pubkey.compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH);
    }
    else if (type == DOGECOIN_TX_WITNESS_V0_PUBKEYHASH) {
        // signal witness by emtpying script sig (may be already empty)
        cstr_resize(tx_in->script_sig, 0);
        if (witness_set_scriptsig) {
            // apend the script sig in case of P2SH-P2WPKH
            cstr_append_cstr(tx_in->script_sig, witness_set_scriptsig);
            cstr_free(witness_set_scriptsig, true);
        }

        // fill witness stack (DER sig, pubkey)
        cstring* witness_item = cstr_new_buf(sigder_plus_hashtype, sigderlen);
        vector_add(tx_in->witness_stack, witness_item);

        witness_item = cstr_new_buf(pubkey.pubkey, pubkey.compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH);
        vector_add(tx_in->witness_stack, witness_item);
    }
    else {
        // append nothing
        res = DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE;
    }

    return res;
}
