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
#include <dogecoin/sha2.h>
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
    uint8_t* stripped_array = dogecoin_uint8_vla(txout->script_pubkey->len);
    dogecoin_mem_zero(stripped_array, txout->script_pubkey->len * sizeof(stripped_array[0]));
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
    free(stripped_array);
    // copy checksum to the last 4 bytes of our unencoded_address:
    unencoded_address[21] = checksum[0];
    unencoded_address[22] = checksum[1];
    unencoded_address[23] = checksum[2];
    unencoded_address[24] = checksum[3];

    char script_hash_to_p2pkh[35];
    // base 58 encode check our unencoded_address into the script_hash_to_p2pkh:
    if (!dogecoin_base58_encode_check(unencoded_address, 21, script_hash_to_p2pkh, sizeof(script_hash_to_p2pkh))) {
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
 
    // strlen(p2pkh) + 1 = 35
    unsigned char dec[35]; //problem is here, it works if its char**

    // MLUMIN: MSVC
    size_t decoded_length = dogecoin_base58_decode_check(p2pkh, (uint8_t*)&dec, sizeof(dec) / sizeof(dec[0]));
    if (decoded_length==0)
    {
        printf("failed base58 decode\n");
        return false;
    }
    
    //decoded bytes = [1-byte versionbits][20-byte hash][4-byte checksum]
    char* b58_decode_hex =utils_uint8_to_hex((const uint8_t*)dec, decoded_length - 4);
    
    //2* (3-byte header + 20-byte hash + 2-byte footer) + 1-byte null terminator
    char* tmp = dogecoin_malloc(40 + 6 + 4 + 1);

    //concatenate the fields
    sprintf(tmp, "%02x%02x%02x%.40s%02x%02x", OP_DUP, OP_HASH160, 20, &b58_decode_hex[2], OP_EQUALVERIFY, OP_CHECKSIG);
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
char* dogecoin_private_key_wif_to_script_hash(char* private_key_wif) {
    if (!private_key_wif) {
        return false;
    }

    const dogecoin_chainparams* chain = (private_key_wif[0] == 'c') ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;

    /* private key */
    dogecoin_key key;
    dogecoin_privkey_init(&key);
    dogecoin_privkey_decode_wif(private_key_wif, chain, &key);
    if (!dogecoin_privkey_is_valid(&key)) {
        debug_print("private key is not valid!\nchain: %s\n", chain->chainname);
        return false;
    }

    char new_wif_privkey[53];
    size_t sizeout = sizeof(new_wif_privkey);
    dogecoin_privkey_encode_wif(&key, chain, new_wif_privkey, &sizeout);

    /* public key */
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(&key, &pubkey);
    if (!dogecoin_pubkey_is_valid(&pubkey)) {
        debug_print("pubkey is not valid!\nchain: %s\n", chain->chainname);
        return false;
    }

    char* new_p2pkh_pubkey = dogecoin_char_vla(sizeout);
    dogecoin_pubkey_getaddr_p2pkh(&pubkey, chain, new_p2pkh_pubkey);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_cleanse(&pubkey);
    char* script_hash=dogecoin_p2pkh_to_script_hash(new_p2pkh_pubkey);
    free(new_p2pkh_pubkey);
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
 * 
 * @return 1 if deserialized successfully, 0 otherwise.
 */
int dogecoin_tx_deserialize(const unsigned char* tx_serialized, size_t inlen, dogecoin_tx* tx, size_t* consumed_length)
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
 * @brief This function serializes a full transaction.
 * 
 * @param s The pointer to the cstring to serialize the data into.
 * @param tx The pointer to the transaction to serialize.
 * 
 * @return Nothing.
 */
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
    dogecoin_tx_serialize(txser, tx);
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
    memcpy_safe(&dest->prevout, &src->prevout, sizeof(dest->prevout));
    dest->sequence = src->sequence;

    if (!src->script_sig) {
        dest->script_sig = NULL;
    } else {
        dest->script_sig = cstr_new_sz(src->script_sig->len);
        cstr_append_buf(dest->script_sig,
                        src->script_sig->str,
                        src->script_sig->len);
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
            tx_in_new = dogecoin_malloc(sizeof(*tx_in_new));
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

        for (i = 0; i < src->vout->len; i++) {
            dogecoin_tx_out *tx_out_old, *tx_out_new;
            tx_out_old = vector_idx(src->vout, i);
            tx_out_new = dogecoin_malloc(sizeof(*tx_out_new));
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
    size_t i, len = tx->vout->len;
    for (i = 0; i < len; i++) {
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
 * @param hash The generated signature hash.
 * 
 * @return 1 if signature hash is generated successfully, 0 otherwise.
 */
dogecoin_bool dogecoin_tx_sighash(const dogecoin_tx* tx_to, const cstring* fromPubKey, size_t in_num, int hashtype, uint256 hash)
{
    if (in_num >= tx_to->vin->len || !tx_to->vout) {
        return false;
    }

    dogecoin_bool ret = true;

    dogecoin_tx* tx_tmp = dogecoin_tx_new();
    dogecoin_tx_copy(tx_tmp, tx_to);
    
    // standard sighash (SIGVERSION_BASE)
    cstring* new_script = cstr_new_sz(fromPubKey->len);
    dogecoin_script_copy_without_op_codeseperator(fromPubKey, new_script);

    size_t i;
    dogecoin_tx_in* tx_in;
    for (i = 0; i < tx_tmp->vin->len; i++) {
        tx_in = vector_idx(tx_tmp->vin, i);
        cstr_resize(tx_in->script_sig, 0);
        if (i == in_num) {
            cstr_append_buf(tx_in->script_sig,
                            new_script->str,
                            new_script->len);
        }
    }
    cstr_free(new_script, true);
    if ((hashtype & 0x1f) == SIGHASH_NONE) {
        if (tx_tmp->vout) {
            vector_free(tx_tmp->vout, true);
        }

        tx_tmp->vout = vector_new(1, dogecoin_tx_out_free_cb);

        for (i = 0; i < tx_tmp->vin->len; i++) {
            tx_in = vector_idx(tx_tmp->vin, i);
            if (i != in_num) {
                tx_in->sequence = 0;
            }
        }
    } else if ((hashtype & 0x1f) == SIGHASH_SINGLE) {
        size_t n_out = in_num;
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

        for (i = 0; i < tx_tmp->vin->len; i++) {
            tx_in = vector_idx(tx_tmp->vin, i);
            if (i != in_num) {
                tx_in->sequence = 0;
            }
        }
    }

    if (hashtype & SIGHASH_ANYONECANPAY) {
        if (in_num > 0) {
            vector_remove_range(tx_tmp->vin, 0, in_num);
        }
        vector_resize(tx_tmp->vin, 1);
    }

    cstring* s = cstr_new_sz(512);
    dogecoin_tx_serialize(s, tx_tmp);
    ser_s32(s, hashtype);
    dogecoin_hash((const uint8_t*)s->str, s->len, hash);
    cstr_free(s, true);

out:
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

    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();
    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_append_op(tx_out->script_pubkey, OP_RETURN);
    dogecoin_script_append_pushdata(tx_out->script_pubkey, (unsigned char*)data, datalen);
    tx_out->value = amount;
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

    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();
    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_append_op(tx_out->script_pubkey, OP_HASH256);
    dogecoin_script_append_pushdata(tx_out->script_pubkey, (unsigned char*)puzzle, puzzlelen);
    dogecoin_script_append_op(tx_out->script_pubkey, OP_EQUAL);
    tx_out->value = amount;
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
    uint8_t* buf = dogecoin_calloc(1, buflen);
    size_t r = dogecoin_base58_decode_check(address, buf, buflen);
    if (r > 0 && buf[0] == chain->b58prefix_pubkey_address) {
        dogecoin_tx_add_p2pkh_hash160_out(tx, amount, &buf[1]);
    } else if (r > 0 && buf[0] == chain->b58prefix_script_address) {
        dogecoin_tx_add_p2sh_hash160_out(tx, amount, &buf[1]);
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
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();
    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_build_p2pkh(tx_out->script_pubkey, hash160);
    tx_out->value = amount;
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
    dogecoin_tx_out* tx_out = dogecoin_tx_out_new();
    tx_out->script_pubkey = cstr_new_sz(1024);
    dogecoin_script_build_p2sh(tx_out->script_pubkey, hash160);
    tx_out->value = amount;
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
    dogecoin_pubkey_get_hash160(pubkey, hash160);
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
    if (tx->vin->len == 1) {
        dogecoin_tx_in* vin = vector_idx(tx->vin, 0);
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
 * @param privkey The pointer to the private key to be used to sign the transaction.
 * @param inputindex The index of the input in the transaction.
 * @param sighashtype The type of signature hash to use.
 * @param sigcompact_out The signature in compact format.
 * @param sigder_out The DER-encoded signature.
 * @param sigder_len_out The length of the signature in DER format.
 * 
 * @return The code denoting which errors occurred, if any.
 */
enum dogecoin_tx_sign_result dogecoin_tx_sign_input(dogecoin_tx* tx_in_out, const cstring* script, const dogecoin_key* privkey, size_t inputindex, int sighashtype, uint8_t* sigcompact_out, uint8_t* sigder_out, size_t* sigder_len_out)
{
    if (!tx_in_out || !script) {
        return DOGECOIN_SIGN_INVALID_TX_OR_SCRIPT;
    }

    if (inputindex >= tx_in_out->vin->len) {
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

    cstring* script_sign = cstr_new_cstr(script); //copy the script because we may modify it
    dogecoin_tx_in* tx_in = vector_idx(tx_in_out->vin, inputindex);
    vector* script_pushes = vector_new(1, free);

    enum dogecoin_tx_out_type type = dogecoin_script_classify(script, script_pushes);
    if (type == DOGECOIN_TX_PUBKEYHASH && script_pushes->len == 1) {
        // check if given private key matches the script
        uint160 hash160;
        dogecoin_pubkey_get_hash160(&pubkey, hash160);
        uint160* hash160_in_script = vector_idx(script_pushes, 0);
        if (memcmp(hash160_in_script, hash160, sizeof(hash160)) != 0) {
            res = DOGECOIN_SIGN_NO_KEY_MATCH; //sign anyways
        }
    } else {
        // unknown script, however, still try to create a signature (don't apply though)
        res = DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE;
    }
    vector_free(script_pushes, true);

    uint256 sighash;
    dogecoin_mem_zero(sighash, sizeof(sighash));
    if (!dogecoin_tx_sighash(tx_in_out, script_sign, inputindex, sighashtype, sighash)) {
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
        memcpy_safe(sigcompact_out, sig, siglen);
    }

    // form normalized DER signature & hashtype
    unsigned char sigder_plus_hashtype[74 + 1];
    size_t sigderlen = 75;
    dogecoin_ecc_compact_to_der_normalized(sig, sigder_plus_hashtype, &sigderlen);
    assert(sigderlen <= 74 && sigderlen >= 70);
    sigder_plus_hashtype[sigderlen] = sighashtype;
    sigderlen += 1; //+hashtype
    if (sigcompact_out) {
        memcpy_safe(sigder_out, sigder_plus_hashtype, sigderlen);
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
    } else {
        // append nothing
        res = DOGECOIN_SIGN_UNKNOWN_SCRIPT_TYPE;
    }
    return res;
}
