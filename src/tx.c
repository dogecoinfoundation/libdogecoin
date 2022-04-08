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

#include <dogecoin/crypto/base58.h>
#include <dogecoin/crypto/ecc.h>
#include <dogecoin/crypto/segwit_addr.h>
#include <dogecoin/crypto/sha2.h>
#include <dogecoin/mem.h>
#include <dogecoin/serialize.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>


/**
 * @brief This function frees the memory allocated
 * for a transaction input.
 * 
 * @param tx_in The pointer to the transaction input to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_tx_in_free(dogecoin_tx_in* tx_in)
{
    if (!tx_in)
        return;

    dogecoin_mem_zero(&tx_in->prevout.hash, sizeof(tx_in->prevout.hash));
    tx_in->prevout.n = 0;

    if (tx_in->script_sig) {
        cstr_free(tx_in->script_sig, true);
        tx_in->script_sig = NULL;
    }

    if (tx_in->witness_stack) {
        vector_free(tx_in->witness_stack, true);
        tx_in->witness_stack = NULL;
    }

    dogecoin_mem_zero(tx_in, sizeof(*tx_in));
    dogecoin_free(tx_in);
}


/**
 * @brief This function casts data from a channel buffer to
 * a transaction input object and then frees it by calling
 * dogecoin_tx_in_free().
 * 
 * @param data The pointer to the data to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_tx_in_free_cb(void* data)
{
    if (!data) {
        return;
    }

    dogecoin_tx_in* tx_in = data;
    dogecoin_tx_in_free(tx_in);
}


/**
 * @brief This function casts data from a channel buffer to
 * a cstring object and then frees it by calling cstr_free().
 * 
 * @param data The pointer to the data to be freed
 * 
 * @return Nothing.
 */
void dogecoin_tx_in_witness_stack_free_cb(void* data)
{
    if (!data) {
        return;
    }

    cstring* stack_item = data;
    cstr_free(stack_item, true);
}


/**
 * @brief This function creates a new dogecoin transaction
 * input object and initializes it to all zeroes.
 * 
 * @return A pointer to the new transaction input object.
 */
dogecoin_tx_in* dogecoin_tx_in_new()
{
    dogecoin_tx_in* tx_in;
    tx_in = dogecoin_calloc(1, sizeof(*tx_in));
    dogecoin_mem_zero(&tx_in->prevout, sizeof(tx_in->prevout));
    tx_in->sequence = UINT32_MAX;
    tx_in->witness_stack = vector_new(8, dogecoin_tx_in_witness_stack_free_cb);
    return tx_in;
}


/**
 * @brief This function frees the memory allocated
 * for a transaction output.
 * 
 * @param tx_in The pointer to the transaction output to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_tx_out_free(dogecoin_tx_out* tx_out)
{
    if (!tx_out) {
        return;
    }
    tx_out->value = 0;

    if (tx_out->script_pubkey) {
        cstr_free(tx_out->script_pubkey, true);
        tx_out->script_pubkey = NULL;
    }

    dogecoin_mem_zero(tx_out, sizeof(*tx_out));
    dogecoin_free(tx_out);
}


/**
 * @brief This function casts data from a channel buffer to
 * a transaction output object and then frees it by calling
 * dogecoin_tx_out_free().
 * 
 * @param data The pointer to the data to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_tx_out_free_cb(void* data)
{
    if (!data) {
        return;
    }

    dogecoin_tx_out* tx_out = data;
    dogecoin_tx_out_free(tx_out);
}


/**
 * @brief This function creates a new dogecoin transaction
 * output object and initializes it to all zeroes.
 * 
 * @return A pointer to the new transaction output object.
 */
dogecoin_tx_out* dogecoin_tx_out_new()
{
    dogecoin_tx_out* tx_out;
    tx_out = dogecoin_calloc(1, sizeof(*tx_out));

    return tx_out;
}


/**
 * @brief This function frees the memory allocated
 * for a full transaction.
 * 
 * @param tx_in The pointer to the transaction to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_tx_free(dogecoin_tx* tx)
{
    if (tx->vin) {
        vector_free(tx->vin, true);
        tx->vin = NULL;
    }

    if (tx->vout) {
        vector_free(tx->vout, true);
        tx->vout = NULL;
    }

    dogecoin_free(tx);
}


/**
 * It takes a pointer to a dogecoin_tx_out, copies to a new dogecoin_tx_out
 * converts dogecoin_tx_out->script_pubkey to a p2pkh address, and 
 * frees the copy.
 * 
 * @param txout The data to be copied which contains the script hash we want.
 * @param p2pkh The variable out we want to contain the converted script hash in.
 * 
 * @return int
 */
int dogecoin_script_hash_to_p2pkh(dogecoin_tx_out* txout, char* p2pkh, int is_testnet) {
    if (!txout) return false;

    dogecoin_tx_out* copy = dogecoin_tx_out_new();
    dogecoin_tx_out_copy(copy, txout);
    size_t length = 2;
    uint8_t* stripped_array[txout->script_pubkey->len];
    dogecoin_mem_zero(stripped_array, sizeof(stripped_array));
    // loop through 20 bytes of the script hash while stripping op codes
    // and copy from index 2 to 21 after prefixing with version
    // from chainparams:
    for (; length < copy->script_pubkey->len - 4; length++) {
        switch (copy->script_pubkey->str[length]) {
            case OP_DUP:
                break;
            case (char)OP_HASH160:
                break;
            case (char)OP_EQUALVERIFY:
                break;
            case (char)OP_CHECKSIG:
                break;
            default:
                copy->script_pubkey->str[2] = is_testnet ? 0x1e : 0x71;
                memccpy(stripped_array, &copy->script_pubkey->str[2], 2, 21);
                break;
        }
    }

    unsigned char d1[SHA256_DIGEST_LENGTH], checksum[4], unencoded_address[25];
    // double sha256 stripped array into d1:
    dogecoin_dblhash((const unsigned char *)stripped_array, strlen((const char *)stripped_array), d1);
    // copy check sum (4 bytes) into checksum var:
    memcpy(checksum, d1, 4);
    // copy stripped array into final var before passing to out variable:
    memcpy(unencoded_address, stripped_array, 21);

    // copy checksum to the last 4 bytes of our unencoded_address:
    unencoded_address[21] = checksum[0];
    unencoded_address[22] = checksum[1];
    unencoded_address[23] = checksum[2];
    unencoded_address[24] = checksum[3];

    size_t strsize = 35;
    char script_hash_to_p2pkh[strsize];
    // base 58 encode check our unencoded_address into the script_hash_to_p2pkh:
    if (!dogecoin_base58_encode_check(unencoded_address, 21, script_hash_to_p2pkh, strsize)) {
        return false;
    }
    
    debug_print("doublesha:         %s\n", utils_uint8_to_hex(d1, sizeof(d1)));
    debug_print("checksum:          %s\n", utils_uint8_to_hex(checksum, sizeof(checksum)));
    debug_print("unencoded_address: %s\n", utils_uint8_to_hex(unencoded_address, sizeof(unencoded_address)));
    debug_print("scripthash2p2pkh:  %s\n", script_hash_to_p2pkh);
    
    // copy to out variable p2pkh, free tx_out copy and return true:
    memcpy(p2pkh, script_hash_to_p2pkh, sizeof(script_hash_to_p2pkh));
    dogecoin_tx_out_free(copy);
    return memcmp(p2pkh, script_hash_to_p2pkh, strlen(script_hash_to_p2pkh)) == 0;
}

/**
 * It takes a p2pkh address and converts it to a compressed public key in
 * hexadecimal format. It then strips the network prefix and checksum and 
 * prepends OP_DUP and OP_HASH160 and appends OP_EQUALVERIFY and OP_CHECKSIG.
 * 
 * @param p2pkh The variable out we want to contain the converted script hash in.
 * 
 * @return int
 */
char* dogecoin_p2pkh_to_script_hash(char* p2pkh) {
    if (!p2pkh) return false;
    size_t len = 25;
    unsigned char dec[len];
    if (!dogecoin_base58_decode_check(p2pkh, dec, 35)) {
        printf("failed base58 decode\n");
        return false;
    }
    char* b58_decode_hex = utils_uint8_to_hex(dec, len - 4);
    char* tmp = dogecoin_calloc(1, 51);
    char opcodes_and_pubkey_length_to_prepend[7], opcodes_to_append[5];
    sprintf(opcodes_and_pubkey_length_to_prepend, "%x%x%x", OP_DUP, OP_HASH160, 20);
    sprintf(opcodes_to_append, "%x%x", OP_EQUALVERIFY, OP_CHECKSIG);
    for (size_t l = 0; l < 4; l += 2) {
        if (l == 2) {
            memccpy(tmp, &b58_decode_hex[l], 3, 48);
            prepend(tmp, opcodes_and_pubkey_length_to_prepend);
            append(tmp, opcodes_to_append);
        }
    }
    return tmp;
}

/**
 * It takes in a private key (WIF encoded) and a flag to indicate the correct network
 * needed in order to select the correct base58 prefix from chainparams and then decodes
 * the private key into a dogecoin_key struct which is then encoded. Then it derives the
 * public hexadecimal key and generates the corresponding p2pkh address. Finally it converts
 * the p2pkh to a script pubkey hash, frees the previous structs from memory and returns the
 * script pubkey hash.
 * 
 * @param p2pkh The variable out we want to contain the converted script hash in.
 * 
 * @return char* The script public key hash.
 */
char* dogecoin_private_key_wif_to_script_hash(char* private_key_wif, int is_testnet) {
    if (!private_key_wif) return false;

    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    size_t sizeout = 100;

    /* private key */
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    dogecoin_privkey_decode_wif(private_key_wif, chain, &key);
    if (!dogecoin_privkey_is_valid(&key)) return false;
    char new_wif_privkey[sizeout];
    dogecoin_privkey_encode_wif(&key, chain, new_wif_privkey, &sizeout);

    /* public key */
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(&key, &pubkey);
    if (!dogecoin_pubkey_is_valid(&pubkey)) return false;

    char new_p2pkh_pubkey[sizeout];
    dogecoin_pubkey_getaddr_p2pkh(&pubkey, chain, new_p2pkh_pubkey);
    char* script_hash = dogecoin_p2pkh_to_script_hash(new_p2pkh_pubkey);
    debug_print("p2pkh_pubkey: %s\n", new_p2pkh_pubkey);
    debug_print("script_hash: %s\n", script_hash);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_cleanse(&pubkey);
    return script_hash;
}

/**
 * @brief This function creates a new dogecoin transaction
 * object and initializes it to all zeroes except for the
 * version, which is set to 1.
 * 
 * @return A pointer to the new transaction object.
 */
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


/**
 * @brief This function takes information from a buffer
 * and deserializes it into a transaction input object.
 * 
 * @param tx_in The pointer to the transaction input to deserialize into.
 * @param buf The pointer to the buffer containing the transaction input information.
 * 
 * @return 1 if deserialized successfully, 0 otherwise.
 */
dogecoin_bool dogecoin_tx_in_deserialize(dogecoin_tx_in* tx_in, struct const_buffer* buf)
{
    deser_u256(tx_in->prevout.hash, buf);
    if (!deser_u32(&tx_in->prevout.n, buf)) {
        return false;
    }
    if (!deser_varstr(&tx_in->script_sig, buf)) {
        return false;
    }
    if (!deser_u32(&tx_in->sequence, buf)) {
        return false;
    }
    return true;
}


/**
 * @brief This function takes information from a buffer
 * and deserializes it into a transaction output object.
 * 
 * @param tx_in The pointer to the transaction output to deserialize into.
 * @param buf The pointer to the buffer containing the transaction output information.
 * 
 * @return 1 if deserialized successfully, 0 otherwise.
 */
dogecoin_bool dogecoin_tx_out_deserialize(dogecoin_tx_out* tx_out, struct const_buffer* buf)
{
    if (!deser_s64(&tx_out->value, buf)) {
        return false;
    }
    if (!deser_varstr(&tx_out->script_pubkey, buf)) {
        return false;
    }
    return true;
}


/**
 * @brief This function takes information from a string
 * and deserializes it into a full transaction object.
 * 
 * @param tx_serialized The string containing the transaction information.
 * @param inlen The length of the string to be read.
 * @param tx The pointer to the transaction object to be deserialized into.
 * @param consumed_length The pointer to the total number of characters successfully deserialized.
 * @param allow_witness The flag denoting whether witnesses are allowed for this transaction.
 * 
 * @return 1 if deserialized successfully, 0 otherwise.
 */
int dogecoin_tx_deserialize(const unsigned char* tx_serialized, size_t inlen, dogecoin_tx* tx, size_t* consumed_length, dogecoin_bool allow_witness)
{
    struct const_buffer buf = {tx_serialized, inlen};
    if (consumed_length) {
        *consumed_length = 0;
    }

    //tx needs to be initialized
    deser_s32(&tx->version, &buf);

    uint32_t vlen;
    if (!deser_varlen(&vlen, &buf)) {
        return false;
    }

    uint8_t flags = 0;
    if (vlen == 0 && allow_witness) {
        /* We read a dummy or an empty vin. */
        deser_bytes(&flags, &buf, 1);
        if (flags != 0) {
            // contains witness, deser the vin len
            if (!deser_varlen(&vlen, &buf)) {
                return false;
            }
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
        for (i = 0; i < tx->vin->len; i++) {
            dogecoin_tx_in* tx_in = vector_idx(tx->vin, i);
            if (!deser_varlen(&vlen, &buf))
                return false;
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

    if (!deser_u32(&tx->locktime, &buf)) {
        return false;
    }

    if (consumed_length) {
        *consumed_length = inlen - buf.len;
    }
    return true;
}


/**
 * @brief This function serializes a transaction input.
 * 
 * @param s The pointer to the cstring to serialize the data into.
 * @param tx_in The pointer to the transaction input to serialize.
 * 
 * @return Nothing.
 */
void dogecoin_tx_in_serialize(cstring* s, const dogecoin_tx_in* tx_in)
{
    ser_u256(s, tx_in->prevout.hash);
    ser_u32(s, tx_in->prevout.n);
    ser_varstr(s, tx_in->script_sig);
    ser_u32(s, tx_in->sequence);
}


/**
 * @brief This function serializes a transaction output.
 * 
 * @param s The pointer to the cstring to serialize the data into.
 * @param tx_out The pointer to the transaction output to serialize.
 * 
 * @return Nothing.
 */
void dogecoin_tx_out_serialize(cstring* s, const dogecoin_tx_out* tx_out)
{
    ser_s64(s, tx_out->value);
    ser_varstr(s, tx_out->script_pubkey);
}


/**
 * @brief This function checks if the transaction contains
 * any input with witness data.
 * 
 * @param tx The pointer to the transaction to check.
 * 
 * @return 1 if the transaction has a witness, 0 otherwise.
 */
dogecoin_bool dogecoin_tx_has_witness(const dogecoin_tx* tx)
{
    for (size_t i = 0; i < tx->vin->len; i++) {
        dogecoin_tx_in* tx_in = vector_idx(tx->vin, i);
        if (tx_in->witness_stack != NULL && tx_in->witness_stack->len > 0) {
            return true;
        }
    }
    return false;
}


/**
 * @brief This function serializes a full transaction,
 * serializing witnesses as necessary.
 * 
 * @param s The pointer to the cstring to serialize the data into.
 * @param tx The pointer to the transaction to serialize.
 * @param allow_witness The flag denoting whether witnesses are allowed for this transaction.
 * 
 * @return Nothing.
 */
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
                        cstring* item = vector_idx(tx_in->witness_stack, j);
                        ser_varstr(s, item);
                    }
                }
            }
        }
    }
    ser_u32(s, tx->locktime);
}


/**
 * @brief This function performs a double SHA256 hash
 * on a given transaction.
 * 
 * @param tx The pointer to the transaction to hash.
 * @param hashout The result of the hashing operation.
 * 
 * @return Nothing.
 */
void dogecoin_tx_hash(const dogecoin_tx* tx, uint256 hashout)
{
    cstring* txser = cstr_new_sz(1024);
    dogecoin_tx_serialize(txser, tx, false);
    sha256_raw((const uint8_t*)txser->str, txser->len, hashout);
    sha256_raw(hashout, DOGECOIN_HASH_LENGTH, hashout);
    cstr_free(txser, true);
}


/**
 * @brief This function makes a copy of a given transaction
 * input object.
 * 
 * @param dest The pointer to the copy of the transaction input.
 * @param src The pointer to the original transaction input.
 * 
 * @return Nothing.
 */
void dogecoin_tx_in_copy(dogecoin_tx_in* dest, const dogecoin_tx_in* src)
{
    memcpy(&dest->prevout, &src->prevout, sizeof(dest->prevout));
    dest->sequence = src->sequence;

    if (!src->script_sig) {
        dest->script_sig = NULL;
    } else {
        dest->script_sig = cstr_new_sz(src->script_sig->len);
        cstr_append_buf(dest->script_sig,
                        src->script_sig->str,
                        src->script_sig->len);
    }

    if (!src->witness_stack) {
        dest->witness_stack = NULL;
    } else {
        dest->witness_stack = vector_new(src->witness_stack->len, dogecoin_tx_in_witness_stack_free_cb);
        for (unsigned int i = 0; i < src->witness_stack->len; i++) {
            cstring* witness_item = vector_idx(src->witness_stack, i);
            cstring* item_cpy = cstr_new_cstr(witness_item);
            vector_add(dest->witness_stack, item_cpy);
        }
    }
}


/**
 * @brief This function makes a copy of a given transaction
 * output object.
 * 
 * @param dest The pointer to the copy of the transaction output.
 * @param src The pointer to the original transaction output.
 * 
 * @return Nothing.
 */
void dogecoin_tx_out_copy(dogecoin_tx_out* dest, const dogecoin_tx_out* src)
{
    dest->value = src->value;

    if (!src->script_pubkey) {
        dest->script_pubkey = NULL;
    } else {
        dest->script_pubkey = cstr_new_sz(src->script_pubkey->len);
        cstr_append_buf(dest->script_pubkey,
                        src->script_pubkey->str,
                        src->script_pubkey->len);
    }
}


/**
 * @brief This function makes a copy of a given transaction
 * object.
 * 
 * @param dest The pointer to the copy of the transaction.
 * @param src The pointer to the original transaction.
 * 
 * @return Nothing.
 */
void dogecoin_tx_copy(dogecoin_tx* dest, const dogecoin_tx* src)
{
    dest->version = src->version;
    dest->locktime = src->locktime;

    if (!src->vin) {
        dest->vin = NULL;
    } else {
        unsigned int i;

        if (dest->vin != NULL) {
            vector_free(dest->vin, true);
        }

        dest->vin = vector_new(src->vin->len, dogecoin_tx_in_free_cb);

        for (i = 0; i < src->vin->len; i++) {
            dogecoin_tx_in *tx_in_old, *tx_in_new;
            tx_in_old = vector_idx(src->vin, i);
            tx_in_new = dogecoin_calloc(1, sizeof(*tx_in_old));
            dogecoin_tx_in_copy(tx_in_new, tx_in_old);
            vector_add(dest->vin, tx_in_new);
        }
    }

    if (!src->vout) {
        dest->vout = NULL;
    } else {
        unsigned int i;

        if (dest->vout)
            vector_free(dest->vout, true);

        dest->vout = vector_new(src->vout->len,
                                dogecoin_tx_out_free_cb);

        /* Copying the tx_out from the source transaction to the destination transaction. */
        for (i = 0; i < src->vout->len; i++) {
            dogecoin_tx_out *tx_out_old, *tx_out_new;

            tx_out_old = vector_idx(src->vout, i);
            tx_out_new = dogecoin_calloc(1, sizeof(dogecoin_tx_out*) * 2 + 1);
            dogecoin_tx_out_copy(tx_out_new, tx_out_old);
            vector_add(dest->vout, tx_out_new);
        }
    }
}


/**
 * @brief This function performs a double SHA256 hash
 * on the input data of a given transaction.
 * 
 * @param tx The pointer to the transaction whose inputs will be hashed.
 * @param hash The result of the hashed inputs.
 * 
 * @return Nothing.
 */
void dogecoin_tx_prevout_hash(const dogecoin_tx* tx, uint256 hash)
{
    cstring* s = cstr_new_sz(512);
    unsigned int i;
    for (i = 0; i < tx->vin->len; i++) {
        dogecoin_tx_in* tx_in = vector_idx(tx->vin, i);
        ser_u256(s, tx_in->prevout.hash);
        ser_u32(s, tx_in->prevout.n);
    }

    dogecoin_hash((const uint8_t*)s->str, s->len, hash);
    cstr_free(s, true);
}


/**
 * @brief This function performs a double SHA256 hash
 * on the sequence numbers of all the transaction's 
 * inputs
 * 
 * @param tx The pointer to the transaction whose inputs will be hashed.
 * @param hash The result of the hashed inputs.
 * 
 * @return Nothing.
 */
void dogecoin_tx_sequence_hash(const dogecoin_tx* tx, uint256 hash)
{
    cstring* s = cstr_new_sz(512);
    unsigned int i;
    for (i = 0; i < tx->vin->len; i++) {
        dogecoin_tx_in* tx_in = vector_idx(tx->vin, i);
        ser_u32(s, tx_in->sequence);
    }

    dogecoin_hash((const uint8_t*)s->str, s->len, hash);
    cstr_free(s, true);
}


/**
 * @brief This function perform a double SHA256 hash
 * on the serialized outputs of a given transaction.
 * 
 * @param tx The pointer to the transaction whose outputs will be hashed.
 * @param hash The result of the hashed outputs.
 * 
 * @return Nothing.
 */
void dogecoin_tx_outputs_hash(const dogecoin_tx* tx, uint256 hash)
{
    if (!tx->vout || !hash) return;
    cstring* s = cstr_new_sz(512);
    size_t i;
    for (i = 0; i < tx->vout->len; i++) {
        dogecoin_tx_out* tx_out = vector_idx(tx->vout, i);
        dogecoin_tx_out_serialize(s, tx_out);
    }

    dogecoin_hash((const uint8_t*)s->str, s->len, hash);
    cstr_free(s, true);
}


/**
 * @brief This function takes an existing transaction and
 * generates a signature hash to lock its inputs from being
 * double-spent.
 * 
 * @param tx_to The pointer to the existing transaction.
 * @param fromPubKey The pointer to the cstring containing the public key of the sender.
 * @param in_num The index of the input being signed.
 * @param hashtype The type of signature hash to perform.
 * @param amount The amount to be sent over this transaction.
 * @param sigversion The signature version.
 * @param hash The generated signature hash.
 * 
 * @return 1 if signature hash is generated successfully, 0 otherwise.
 */
dogecoin_bool dogecoin_tx_sighash(const dogecoin_tx* tx_to, const cstring* fromPubKey, unsigned int in_num, int hashtype, const uint64_t amount, const enum dogecoin_sig_version sigversion, uint256 hash)
{
    /* Checking if the input number is greater than the number of inputs. */
    if (in_num >= tx_to->vin->len || !tx_to->vout) {
        return false;
    }

    dogecoin_bool ret = true;

    /* Creating a new transaction object. */
    dogecoin_tx* tx_tmp = dogecoin_tx_new();
    /* Copying the transaction to a temporary variable. */
    dogecoin_tx_copy(tx_tmp, tx_to);

    cstring* s = NULL;

    // segwit
    /* The code below is checking if the signature version is witness version 0. If it is, then it will
    check if the witness program is a pay to public key hash. If it is, then it will check if the
    scriptPubKey is a pay to public key hash. If it is, then it will check if the scriptSig is
    empty. If it is, then it will check if the witness program is empty. If it is, then it will
    check if the scriptPubKey is a pay to script hash. If it is, then it will check if the scriptSig
    is */
    if (sigversion == SIGVERSION_WITNESS_V0) {
        uint256 hash_prevouts;
        /* This code is clearing the hash_prevouts array. */
        dogecoin_hash_clear(hash_prevouts);
        uint256 hash_sequence;
        /* This code is clearing the hash_sequence. */
        dogecoin_hash_clear(hash_sequence);
        uint256 hash_outputs;
        /* This code is clearing the hash_outputs variable. */
        dogecoin_hash_clear(hash_outputs);

        if (!(hashtype & SIGHASH_ANYONECANPAY)) {
            /* This code is calculating the hash of the previous outputs. */
            dogecoin_tx_prevout_hash(tx_tmp, hash_prevouts);
            dogecoin_tx_outputs_hash(tx_tmp, hash_outputs);
        }
        if (!(hashtype & SIGHASH_ANYONECANPAY) && (hashtype & 0x1f) != SIGHASH_SINGLE && (hashtype & 0x1f) != SIGHASH_NONE) {
            /* This code is calculating the hash of the sequence of the transaction. */
            dogecoin_tx_sequence_hash(tx_tmp, hash_sequence);
        }

        if ((hashtype & 0x1f) != SIGHASH_SINGLE && (hashtype & 0x1f) != SIGHASH_NONE) {
            /* This code is calculating the hash of the outputs of the transaction. */
            dogecoin_tx_outputs_hash(tx_tmp, hash_outputs);
        } else if ((hashtype & 0x1f) == SIGHASH_SINGLE && in_num < tx_tmp->vout->len) {
            cstring* s1 = cstr_new_sz(512);
            /* Creating a new transaction output from the transaction output vector. */
            dogecoin_tx_out* tx_out = vector_idx(tx_tmp->vout, in_num);
            /* Serializing the transaction output. */
            dogecoin_tx_out_serialize(s1, tx_out);
            /* Hashing the string s1 using the dogecoin_hash function. */
            dogecoin_hash((const uint8_t*)s1->str, s1->len, hash);
            /* Freeing the memory allocated to the string s1. */
            cstr_free(s1, true);
        }

        s = cstr_new_sz(512);
        /* Serializing the transaction version. */
        ser_u32(s, tx_tmp->version); // Version

        // Input prevouts/nSequence (none/all, depending on flags)
        /* Hashing the previous outputs of the transaction. */
        ser_u256(s, hash_prevouts);
        /* Serializing the hash of the sequence number. */
        ser_u256(s, hash_sequence);

        // The input being signed (replacing the scriptSig with scriptCode + amount)
        // The prevout may already be contained in hashPrevout, and the nSequence
        // may already be contain in hashSequence.
        /* Creating a new transaction input from the transaction input at index in_num in the
        transaction tx_tmp. */
        dogecoin_tx_in* tx_in = vector_idx(tx_tmp->vin, in_num);
        /* Serializing the transaction hash of the previous transaction. */
        ser_u256(s, tx_in->prevout.hash);
        /* The code below is serializing the prevout.n field of the transaction input. */
        ser_u32(s, tx_in->prevout.n);

        /* Serializing the script code. */
        ser_varstr(s, (cstring*)fromPubKey); // script code

        /* Serializing the amount field of the transaction. */
        ser_u64(s, amount);
        /* Serializing the sequence number of the input. */
        ser_u32(s, tx_in->sequence);
        /* Serializing the outputs of the transaction. */
        ser_u256(s, hash_outputs);    // Outputs (none/one/all, depending on flags)
        /* Serializing the transaction. */
        ser_u32(s, tx_tmp->locktime); // Locktime
        /* Writing the hashtype to the stream. */
        ser_s32(s, hashtype);         // Sighash type
    } else {
        // standard (non witness) sighash (SIGVERSION_BASE)
        cstring* new_script = cstr_new_sz(fromPubKey->len);
        dogecoin_script_copy_without_op_codeseperator(fromPubKey, new_script);

        unsigned int i;
        dogecoin_tx_in* tx_in;
        /* Checking if the transaction has any inputs. */
        for (i = 0; i < tx_tmp->vin->len; i++) {
            /* Checking if the transaction has a previous transaction. If it does, it is adding the
            previous transaction to the list of transactions. */
            tx_in = vector_idx(tx_tmp->vin, i);
            /* The code below is removing the scriptSig from the transaction. */
            cstr_resize(tx_in->script_sig, 0);

            /* Checking if the current index is equal to the number of inputs. */
            if (i == in_num) {
                cstr_append_buf(tx_in->script_sig,
                                new_script->str,
                                new_script->len);
            }
        }
        cstr_free(new_script, true);
        /* Blank out some of the outputs */
        if ((hashtype & 0x1f) == SIGHASH_NONE) {
            /* Wildcard payee */
            if (tx_tmp->vout) {
                vector_free(tx_tmp->vout, true);
            }

            tx_tmp->vout = vector_new(1, dogecoin_tx_out_free_cb);

            /* Let the others update at will */
            for (i = 0; i < tx_tmp->vin->len; i++) {
                tx_in = vector_idx(tx_tmp->vin, i);
                if (i != in_num) {
                    tx_in->sequence = 0;
                }
            }
        }

        /**
         * If the SIGHASH_SINGLE flag is set, then only the output at the same index as the input being
         * signed will be signed
         * 
         * @param  hashtype - (1 << 8)
         */
        else if ((hashtype & 0x1f) == SIGHASH_SINGLE) {
            /* Only lock-in the txout payee at same index as txin */
            unsigned int n_out = in_num;
            /* Checking if the number of outputs is greater than the length of the vector of outputs. */
            if (n_out >= tx_tmp->vout->len) {
                //TODO: set error code
                ret = false;
                goto out;
            }

            /* Adding a new output to the transaction. */
            vector_resize(tx_tmp->vout, n_out + 1);

            /* Creating a new array of the same size as the input array. */
            for (i = 0; i < n_out; i++) {
                dogecoin_tx_out* tx_out;

                /* Checking if the transaction has a certain output. */
                tx_out = vector_idx(tx_tmp->vout, i);
                /* The code below is checking if the transaction is a coinbase transaction. If it is,
                then it sets the value to -1. */
                tx_out->value = -1;
                if (tx_out->script_pubkey) {
                    /* Freeing the memory allocated to the script_pubkey field of the transaction
                    output. */
                    cstr_free(tx_out->script_pubkey, true);
                    /* Creating a new output with the given amount and script_pubkey. */
                    tx_out->script_pubkey = NULL;
                }
            }

            /* Let the others update at will */
            /* Checking if the transaction has any inputs. */
            for (i = 0; i < tx_tmp->vin->len; i++) {
                /* This is a simple loop that iterates over the vin vector of the transaction. */
                tx_in = vector_idx(tx_tmp->vin, i);
                /* Checking if the current index is not equal to the number of inputs. */
                if (i != in_num) {
                    /* Setting the sequence number of the input to 0. */
                    tx_in->sequence = 0;
                }
            }
        }

        /* Blank out other inputs completely;
         not recommended for open transactions */
        /* This code is checking if the hashtype is set to SIGHASH_ANYONECANPAY. If it is, then it will
        set the input to be the only input. */
        if (hashtype & SIGHASH_ANYONECANPAY) {
            if (in_num > 0) {
                /* The code below is removing the first in_num elements from the vector tx_tmp->vin. */
                vector_remove_range(tx_tmp->vin, 0, in_num);
            }
            /* Adding a new input to the transaction. */
            vector_resize(tx_tmp->vin, 1);
        }

        /* Creating a string of 512 bytes. */
        s = cstr_new_sz(512);
        /* Serializing the transaction into a string. */
        dogecoin_tx_serialize(s, tx_tmp, false);
        /* Writing the hashtype to the stream. */
        ser_s32(s, hashtype);
    }

    /* Hashing the string s using the dogecoin_hash function. */
    dogecoin_hash((const uint8_t*)s->str, s->len, hash);

    /* Freeing the memory allocated to the string s. */
    cstr_free(s, true);

out:
    /* Freeing the memory allocated to the transaction. */
    dogecoin_tx_free(tx_tmp);

    return ret;
}


/**
 * @brief This function adds another transaction output to
 * an existing transaction.
 * 
 * @param tx The pointer to the transaction which will be updated.
 * @param amount The amount that will be sent to the new destination.
 * @param data The pubkey data to be embedded in the new transaction.
 * @param datalen The length of the pubkey data to be embedded.
 * 
 * @return dogecoin_bool 
 */
dogecoin_bool dogecoin_tx_add_data_out(dogecoin_tx* tx, const int64_t amount, const uint8_t* data, const size_t datalen)
{
    if (datalen > 80) {
        return false;
    }

    /* Creating a new transaction output. */
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    /* Creating a new string of size 1024. */
    tx_out->script_pubkey = cstr_new_sz(1024);
    /* This code is appending the OP_RETURN operation to the script_pubkey of the transaction output. */
    dogecoin_script_append_op(tx_out->script_pubkey, OP_RETURN);
    /* This is adding the data to the script_pubkey. */
    dogecoin_script_append_pushdata(tx_out->script_pubkey, (unsigned char*)data, datalen);

    /* The code below is creating a new transaction output. */
    tx_out->value = amount;

    /* Adding a new output to the transaction. */
    vector_add(tx->vout, tx_out);

    return true;
}


/**
 * @brief This function adds another transaction output 
 * containing the puzzle hash for the miner to solve to
 * an existing transaction.
 * 
 * @param tx The pointer to the transaction which will be updated.
 * @param amount The amount that will be sent in the transaction.
 * @param puzzle The puzzle that the miner must solve in order to spend the coin.
 * @param puzzlelen The length of the puzzle in bytes.
 * 
 * @return 1 if the puzzle was added successfully, 0 otherwise.
 */
dogecoin_bool dogecoin_tx_add_puzzle_out(dogecoin_tx* tx, const int64_t amount, const uint8_t* puzzle, const size_t puzzlelen)
{
    if (puzzlelen > DOGECOIN_HASH_LENGTH) {
        return false;
    }

    /* Creating a new transaction output. */
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    /* Creating a new string of size 1024. */
    tx_out->script_pubkey = cstr_new_sz(1024);
    /* This is adding the OP_HASH256 operation to the script. */
    dogecoin_script_append_op(tx_out->script_pubkey, OP_HASH256);
    /* Appending the puzzle to the script_pubkey. */
    dogecoin_script_append_pushdata(tx_out->script_pubkey, (unsigned char*)puzzle, puzzlelen);
    /* This is adding the OP_EQUAL to the script_pubkey. */
    dogecoin_script_append_op(tx_out->script_pubkey, OP_EQUAL);
    /* Creating a new transaction with the given amount. */
    tx_out->value = amount;

    /* Adding a new output to the transaction. */
    vector_add(tx->vout, tx_out);

    return true;
}


/**
 * @brief This function adds another transaction output
 * containing the destination address to an existing 
 * transaction.
 * 
 * @param tx The pointer to the transaction which will be updated.
 * @param chain The pointer to the chainparams which contain the prefixes for the address types.
 * @param amount The amount that will be sent in the transaction. 
 * @param address The address to send coins to, which can be a P2PKH, P2SH, or P2WPKH address.
 * 
 * @return 1 if the address was added successfully, 0 otherwise. 
 */
dogecoin_bool dogecoin_tx_add_address_out(dogecoin_tx* tx, const dogecoin_chainparams* chain, int64_t amount, const char* address)
{
    const size_t buflen = sizeof(uint8_t) * strlen(address) * 2;
    /* Allocating memory for the buffer. */
    uint8_t* buf = dogecoin_calloc(1, buflen);
    /* Decoding the address into a buffer. */
    int r = dogecoin_base58_decode_check(address, buf, buflen);
    if (r > 0 && buf[0] == chain->b58prefix_pubkey_address) {
        /* Adding a new output to the transaction. */
        dogecoin_tx_add_p2pkh_hash160_out(tx, amount, &buf[1]);
    } else if (r > 0 && buf[0] == chain->b58prefix_script_address) {
        /* Adding a P2SH output to the transaction. */
        dogecoin_tx_add_p2sh_hash160_out(tx, amount, &buf[1]);
    } else {
        // check for bech32
        int version = 0;
        unsigned char programm[40] = {0};
        size_t programmlen = 0;
        /* Checking if the address is a valid segwit address. */
        if(segwit_addr_decode(&version, programm, &programmlen, chain->bech32_hrp, address) == 1) {
            if (programmlen == 20) {
                /* Creating a new transaction output. */
                dogecoin_tx_out* tx_out = dogecoin_tx_out_new();
                /* Creating a new string of size 1024. */
                tx_out->script_pubkey = cstr_new_sz(1024);

                /* This code is building a scriptPubKey for the output. */
                dogecoin_script_build_p2wpkh(tx_out->script_pubkey, (const uint8_t *)programm);

                /* The code below is creating a new transaction output. */
                tx_out->value = amount;
                /* Adding a new output to the transaction. */
                vector_add(tx->vout, tx_out);
            }
        }
        dogecoin_free(buf);
        return false;
    }

    dogecoin_free(buf);
    return true;
}


/**
 * @brief This function adds a new P2PKH output to an
 * existing transaction.
 * 
 * @param tx The pointer to the transaction which will be updated.
 * @param amount The amount that will be sent in the transaction.
 * @param hash160 The hash160 of the sender public key.
 * 
 * @return 1 if the output is added successfully.
 */
dogecoin_bool dogecoin_tx_add_p2pkh_hash160_out(dogecoin_tx* tx, int64_t amount, uint160 hash160)
{
    /* Creating a new transaction output. */
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    /* Creating a new string of size 1024. */
    tx_out->script_pubkey = cstr_new_sz(1024);
    /* Building a scriptPubKey for the output. */
    dogecoin_script_build_p2pkh(tx_out->script_pubkey, hash160);

    /* The code below is creating a new transaction output. */
    tx_out->value = amount;

    /* Adding a new output to the transaction. */
    vector_add(tx->vout, tx_out);

    return true;
}


/**
 * @brief This function adds a new P2SH output to an
 * existing transaction.
 * 
 * @param tx The pointer to the transaction which will be updated.
 * @param amount The amount that will be sent in the transaction.
 * @param hash160 The hash160 of the sender public key.
 * 
 * @return 1 if the output is added successfully.
 */
dogecoin_bool dogecoin_tx_add_p2sh_hash160_out(dogecoin_tx* tx, int64_t amount, uint160 hash160)
{
    /* Creating a new transaction output. */
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();

    /* Creating a new string of size 1024. */
    tx_out->script_pubkey = cstr_new_sz(1024);
    /* Building a scriptPubKey for the output. */
    dogecoin_script_build_p2sh(tx_out->script_pubkey, hash160);

    /* The code below is creating a new transaction output. */
    tx_out->value = amount;

    /* Adding a new output to the transaction. */
    vector_add(tx->vout, tx_out);

    return true;
}


/**
 * @brief This function adds an output to an existing
 * transaction by hashing the given public key and calling
 * dogecoin_tx_add_p2pkh_hash160_out() using this hash.
 * 
 * @param tx The pointer to the transaction which will be updated.
 * @param amount The amount that will be sent in the transaction.
 * @param pubkey The pointer to the public key to be hashed.
 * 
 * @return 1 if the output was added successfully.
 */
dogecoin_bool dogecoin_tx_add_p2pkh_out(dogecoin_tx* tx, int64_t amount, const dogecoin_pubkey* pubkey)
{
    uint160 hash160;
    /* Taking the public key and hashing it to get the hash160. */
    dogecoin_pubkey_get_hash160(pubkey, hash160);
    /* Adding a new output to the transaction. */
    return dogecoin_tx_add_p2pkh_hash160_out(tx, amount, hash160);
}


/**
 * @brief This function checks whether a transaction
 * outpoint is null.
 * 
 * @param tx The pointer to the transaction outpoint to check.
 * 
 * @return 1 if the outpoint is null.
 */
dogecoin_bool dogecoin_tx_outpoint_is_null(dogecoin_tx_outpoint* tx)
{
    (void)(tx);
    return true;
}


/**
 * @brief This function checks whether a transaction was
 * a coinbase transaction, which is true if the transaction
 * has only one input, the previous transaction hash is 
 * empty, and the transaction index is UINT32_MAX. 
 * 
 * @param tx The pointer to the transaction to check.
 * @return dogecoin_bool 
 */
dogecoin_bool dogecoin_tx_is_coinbase(dogecoin_tx* tx)
{
    /* Checking if the transaction has only one input. */
    if (tx->vin->len == 1) {
        /* Checking if the transaction has a coinbase input. */
        dogecoin_tx_in* vin = vector_idx(tx->vin, 0);

        /* Checking if the previous transaction hash is empty and if the previous transaction index is
        UINT32_MAX. */
        if (dogecoin_hash_is_empty(vin->prevout.hash) && vin->prevout.n == UINT32_MAX) {
            return true;
        }
    }
    return false;
}


/**
 * @brief This function converts the result of the signing
 * into a regular string.
 * 
 * @param result The string representation of the signing result.
 * 
 * @return The string representation of the signing result, "UNKNOWN" if result is unrecognized.
 */
const char* dogecoin_tx_sign_result_to_str(const enum dogecoin_tx_sign_result result)
{
    if (result == DOGECOIN_SIGN_OK) {
        return "OK";
    } else if (result == DOGECOIN_SIGN_INVALID_TX_OR_SCRIPT) {
        return "INVALID_TX_OR_SCRIPT";
    } else if (result == DOGECOIN_SIGN_INPUTINDEX_OUT_OF_RANGE) {
        return "INPUTINDEX_OUT_OF_RANGE";
    } else if (result == DOGECOIN_SIGN_INVALID_KEY) {
        return "INVALID_KEY";
    } else if (result == DOGECOIN_SIGN_NO_KEY_MATCH) {
        return "NO_KEY_MATCH";
    } else if (result == DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE) {
        return "SIGN_UNKNOWN_SCRIPT_TYPE";
    } else if (result == DOGECOIN_SIGN_SIGHASH_FAILED) {
        return "SIGHASH_FAILED";
    }
    return "UNKOWN";
}


/**
 * @brief This function signs the inputs of a given
 * transaction using the private key and signature.
 * 
 * @param tx_in_out The pointer to the transaction to be signed.
 * @param script The pointer to the cstring containing the script to be signed.
 * @param amount The amount that will be sent in the transaction.
 * @param privkey The pointer to the private key to be used to sign the transaction.
 * @param inputindex The index of the input in the transaction.
 * @param sighashtype The type of signature hash to use.
 * @param sigcompact_out The signature in compact format.
 * @param sigder_out The DER-encoded signature.
 * @param sigder_len_out The length of the signature in DER format.
 * 
 * @return The code denoting which errors occurred, if any.
 */
enum dogecoin_tx_sign_result dogecoin_tx_sign_input(dogecoin_tx* tx_in_out, const cstring* script, uint64_t amount, const dogecoin_key* privkey, int inputindex, int sighashtype, uint8_t* sigcompact_out, uint8_t* sigder_out, int* sigder_len_out)
{
    /* Checking if the transaction or script is valid. */
    if (!tx_in_out || !script) {
        return DOGECOIN_SIGN_INVALID_TX_OR_SCRIPT;
    }
    /* The code below is checking if the input index is out of range. */
    if ((size_t)inputindex >= tx_in_out->vin->len) {
        return DOGECOIN_SIGN_INPUTINDEX_OUT_OF_RANGE;
    }
    /* Checking if the private key is valid. */
    if (!dogecoin_privkey_is_valid(privkey)) {
        return DOGECOIN_SIGN_INVALID_KEY;
    }
    // calculate pubkey
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(privkey, &pubkey);
    /* Checking if the public key is valid. */
    if (!dogecoin_pubkey_is_valid(&pubkey)) {
        return DOGECOIN_SIGN_INVALID_KEY;
    }
    enum dogecoin_tx_sign_result res = DOGECOIN_SIGN_OK;

    cstring* script_sign = cstr_new_cstr(script); //copy the script because we may modify it
    dogecoin_tx_in* tx_in = vector_idx(tx_in_out->vin, inputindex);
    vector* script_pushes = vector_new(1, free);

    /* This code is checking the script type and then setting the witness_set_scriptsig to the correct
    script. */
    cstring* witness_set_scriptsig = NULL; //required in order to set the P2SH-P2WPKH scriptSig
    /* Classifying the script into a type. */
    enum dogecoin_tx_out_type type = dogecoin_script_classify(script, script_pushes);
    /* The code below is checking the version of the signature. */
    enum dogecoin_sig_version sig_version = SIGVERSION_BASE;
    /* This code is setting the script_pushes vector to the correct value for the type of transaction. */
    if (type == DOGECOIN_TX_SCRIPTHASH) {
        // p2sh script, need the redeem script
        // for now, pretend to be a p2sh-p2wpkh
        vector_free(script_pushes, true);
        script_pushes = vector_new(1, free);
        type = DOGECOIN_TX_WITNESS_V0_PUBKEYHASH;
        uint8_t* hash160 = dogecoin_calloc(1, 20);
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
        /* Checking if the private key matches the script. */
        // check if given private key matches the script
        uint160 hash160;
        dogecoin_pubkey_get_hash160(&pubkey, hash160);
        uint160* hash160_in_script = vector_idx(script_pushes, 0);
        if (memcmp(hash160_in_script, hash160, sizeof(hash160)) != 0) {
            res = DOGECOIN_SIGN_NO_KEY_MATCH; //sign anyways
        }
    } else if (type == DOGECOIN_TX_WITNESS_V0_PUBKEYHASH && script_pushes->len == 1) {
        /* Building a script that will be used to sign the transaction. */
        uint160* hash160_in_script = vector_idx(script_pushes, 0);
        sig_version = SIGVERSION_WITNESS_V0;

        // check if given private key matches the script
        uint160 hash160;
        dogecoin_pubkey_get_hash160(&pubkey, hash160);
        if (memcmp(hash160_in_script, hash160, sizeof(hash160)) != 0) {
            res = DOGECOIN_SIGN_NO_KEY_MATCH; //sign anyways
        }

        cstr_resize(script_sign, 0);
        dogecoin_script_build_p2pkh(script_sign, *hash160_in_script);
    } else {
        // unknown script, however, still try to create a signature (don't apply though)
        res = DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE;
    }
    vector_free(script_pushes, true);

    /* Signing the transaction. */
    uint256 sighash;
    dogecoin_mem_zero(sighash, sizeof(sighash));
    /* Checking if the signature is valid. */
    if (!dogecoin_tx_sighash(tx_in_out, script_sign, inputindex, sighashtype, amount, sig_version, sighash)) {
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
    /* The code below is converting the signature into a DER format, and adding the sighashtype to the end
    of the signature. */
    unsigned char sigder_plus_hashtype[74 + 1];
    size_t sigderlen = 75;
    dogecoin_ecc_compact_to_der_normalized(sig, sigder_plus_hashtype, &sigderlen);
    assert(sigderlen <= 74 && sigderlen >= 70);
    sigder_plus_hashtype[sigderlen] = sighashtype;
    sigderlen += 1; //+hashtype
    /* Copying the signature and hashtype into the sigder_out buffer. */
    if (sigcompact_out) {
        memcpy(sigder_out, sigder_plus_hashtype, sigderlen);
    }
    /* Checking if the sigder_len_out is not null, and if it is not null, it is setting the value of
    sigderlen to the value of sigder_len_out. */
    if (sigder_len_out) {
        *sigder_len_out = sigderlen;
    }

    // apply signature depending on script type
    /* Serializing the signature and the public key. */
    if (type == DOGECOIN_TX_PUBKEYHASH) {
        // apply DER sig
        ser_varlen(tx_in->script_sig, sigderlen);
        ser_bytes(tx_in->script_sig, sigder_plus_hashtype, sigderlen);

        // apply pubkey
        ser_varlen(tx_in->script_sig, pubkey.compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH);
        ser_bytes(tx_in->script_sig, pubkey.pubkey, pubkey.compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH);
    } else if (type == DOGECOIN_TX_WITNESS_V0_PUBKEYHASH) {
        /* This is the code that is used to create the witness stack for a P2WPKH transaction. */
        // signal witness by emtpying script sig (may be already empty)
        cstr_resize(tx_in->script_sig, 0);
        if (witness_set_scriptsig) {
            // append the script sig in case of P2SH-P2WPKH
            cstr_append_cstr(tx_in->script_sig, witness_set_scriptsig);
            cstr_free(witness_set_scriptsig, true);
        }

        // fill witness stack (DER sig, pubkey)
        cstring* witness_item = cstr_new_buf(sigder_plus_hashtype, sigderlen);
        vector_add(tx_in->witness_stack, witness_item);

        witness_item = cstr_new_buf(pubkey.pubkey, pubkey.compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH);
        vector_add(tx_in->witness_stack, witness_item);
    } else {
        // append nothing
        res = DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE;
    }
    return res;
}
