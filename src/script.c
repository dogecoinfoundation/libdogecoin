/*

 The MIT License (MIT)

 Copyright 2012 exMULTI, Inc.
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
#include <string.h>

#include <dogecoin/buffer.h>
#include <dogecoin/hash.h>
#include <dogecoin/rmd160.h>
#include <dogecoin/script.h>
#include <dogecoin/serialize.h>


/**
 * @brief This function copies a script without the 
 * OP_CODESEPARATOR opcode.
 * 
 * @param script_in The pointer to the cstring which holds the script to be copied.
 * @param script_out The pointer to the cstring to hold the script to be created.
 * 
 * @return 1 if copied successfully, 0 otherwise.
 */
dogecoin_bool dogecoin_script_copy_without_op_codeseperator(const cstring* script_in, cstring* script_out)
{
    if (script_in->len == 0)
        return false; /* EOF */

    struct const_buffer buf = {script_in->str, script_in->len};
    unsigned char opcode;
    while (buf.len > 0) {
        if (!deser_bytes(&opcode, &buf, 1))
            goto err_out;

        uint32_t data_len = 0;

        if (opcode < OP_PUSHDATA1 && opcode > OP_0) {
            data_len = opcode;
            cstr_append_buf(script_out, &opcode, 1);
        } else if (opcode == OP_PUSHDATA1) {
            uint8_t v8;
            if (!deser_bytes(&v8, &buf, 1))
                goto err_out;
            cstr_append_buf(script_out, &opcode, 1);
            cstr_append_buf(script_out, &v8, 1);
            data_len = v8;
        } else if (opcode == OP_PUSHDATA2) {
            uint16_t v16;
            if (!deser_u16(&v16, &buf))
                goto err_out;
            cstr_append_buf(script_out, &opcode, 1);
            cstr_append_buf(script_out, &v16, 2);
            data_len = v16;
        } else if (opcode == OP_PUSHDATA4) {
            uint32_t v32;
            if (!deser_u32(&v32, &buf))
                goto err_out;
            cstr_append_buf(script_out, &opcode, 1);
            cstr_append_buf(script_out, &v32, 4);
            data_len = v32;
        } else if (opcode == OP_CODESEPARATOR)
            continue;

        if (data_len > 0) {
            assert(data_len < 16777215); //limit max push to 0xFFFFFF
            unsigned char* bufpush = (unsigned char*)dogecoin_calloc(1, data_len);
            if (!deser_bytes(bufpush, &buf, data_len)) {
                dogecoin_free(bufpush);
                goto err_out;
            }
            cstr_append_buf(script_out, bufpush, data_len);
            dogecoin_free(bufpush);
        } else
            cstr_append_buf(script_out, &opcode, 1);
    }

    return true;

err_out:
    return false;
}


/**
 * @brief This function allocates the memory for a new
 * dogecoin_script_op object.
 * 
 * @return A pointer to the new script operation.
 */
dogecoin_script_op* dogecoin_script_op_new()
{
    dogecoin_script_op* script_op;
    script_op = dogecoin_calloc(1, sizeof(dogecoin_script_op));

    return script_op;
}


/**
 * @brief This function frees the pointer to a dogecoin_script_op
 * object and sets the pointer to NULL.
 * 
 * @param script_op The pointer to the script operation to be freed.
 * 
 * @return Nothing.
 */
void dogecoin_script_op_free(dogecoin_script_op* script_op)
{
    if (script_op->data) {
        dogecoin_free(script_op->data);
        script_op->data = NULL;
    }
    script_op->datalen = 0;
    script_op->op = OP_0;
}


/**
 * @brief This function casts data from a channel buffer to
 * a dogecoin_script_op object and then frees it by calling
 * dogecoin_script_op_free().
 * 
 * @param data The pointer to the data that must be cast.
 * 
 * @return Nothing.
 */
void dogecoin_script_op_free_cb(void* data)
{
    dogecoin_script_op* script_op = data;
    dogecoin_script_op_free(script_op);

    dogecoin_free(script_op);
}


/**
 * @brief This function converts a cstring script into a
 * vector of dogecoin_script_op objects.
 * 
 * @param script_in The pointer to the cstring holding the script to be parsed.
 * @param ops_out The pointer to a vector that will store the parsed script operations.
 * 
 * @return 1 if converted successfully, 0 otherwise.
 */
dogecoin_bool dogecoin_script_get_ops(const cstring* script_in, vector* ops_out)
{
    if (script_in->len == 0)
        return false; /* EOF */

    struct const_buffer buf = {script_in->str, script_in->len};
    unsigned char opcode;

    dogecoin_script_op* op = NULL;
    while (buf.len > 0) {
        op = dogecoin_script_op_new();

        if (!deser_bytes(&opcode, &buf, 1))
            goto err_out;

        op->op = opcode;

        uint32_t data_len;

        if (opcode < OP_PUSHDATA1) {
            data_len = opcode;
        } else if (opcode == OP_PUSHDATA1) {
            uint8_t v8;
            if (!deser_bytes(&v8, &buf, 1))
                goto err_out;
            data_len = v8;
        } else if (opcode == OP_PUSHDATA2) {
            uint16_t v16;
            if (!deser_u16(&v16, &buf))
                goto err_out;
            data_len = v16;
        } else if (opcode == OP_PUSHDATA4) {
            uint32_t v32;
            if (!deser_u32(&v32, &buf))
                goto err_out;
            data_len = v32;
        } else {
            vector_add(ops_out, op);
            continue;
        }

        // don't alloc a push buffer if there is no more data available
        if (buf.len == 0 || data_len > buf.len) {
            goto err_out;
        }

        op->data = dogecoin_calloc(1, data_len);
        memcpy_safe(op->data, buf.p, data_len);
        op->datalen = data_len;

        vector_add(ops_out, op);

        if (!deser_skip(&buf, data_len))
            goto err_out;
    }

    return true;
err_out:
    dogecoin_script_op_free_cb(op);
    return false;
}


/**
 * @brief This function checks whether an opcode is a 
 * pushdata opcode.
 * 
 * @param op The opcode to check.
 * 
 * @return 1 if opcode is a pushdata opcode, 0 otherwise. 
 */
static inline dogecoin_bool dogecoin_script_is_pushdata(const enum opcodetype op)
{
    return (op <= OP_PUSHDATA4);
}


/**
 * @brief This function checks whether the opcode of a 
 * given script operation matches the specified opcode.
 * 
 * @param op The pointer to the script operation whose opcode will be checked.
 * @param opcode The reference opcode.
 * 
 * @return 1 if the opcodes match, 0 otherwise.
 */
static dogecoin_bool dogecoin_script_is_op(const dogecoin_script_op* op, enum opcodetype opcode)
{
    return (op->op == opcode);
}


/**
 * @brief This function checks whether a given script 
 * operation pushes a pubkey to the stack. This is 
 * true if the script operation has a pushdata opcode
 * and that its data is the same length as a compressed
 * or uncompressed EC key.
 * 
 * @param op The pointer to the script operation to be checked.
 * 
 * @return 1 if the script resembles the pubkey template, 0 otherwise.
 */
static dogecoin_bool dogecoin_script_is_op_pubkey(const dogecoin_script_op* op)
{
    if (!dogecoin_script_is_pushdata(op->op))
        return false;
    if (op->datalen != DOGECOIN_ECKEY_COMPRESSED_LENGTH && op->datalen != DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH)
        return false;
    if (dogecoin_pubkey_get_length(op->data[0]) != op->datalen) {
        return false;
    }
    return true;
}


/**
 * @brief This function checks whether the given script
 * operation pushes a pubkey hash to the stack. This is
 * true if the script operation has a pushdata opcode 
 * and that its data is 20 bytes in size.
 * 
 * @param op The pointer to the script operation to be checked.
 * 
 * @return 1 if the script resembles the pubkey hash template, 0 otherwise.
 */
static dogecoin_bool dogecoin_script_is_op_pubkeyhash(const dogecoin_script_op* op)
{
    if (!dogecoin_script_is_pushdata(op->op))
        return false;
    if (op->datalen != 20)
        return false;
    return true;
}


/**
 * @brief This function checks whether the script matches
 * the template for a pubkey script, which is composed of
 * a push pubkey operation followed by an OP_CHECKSIG. 
 * The pubkey is then loaded into data_out.
 * 
 * @param ops The pointer to a vector storing script data to be checked.
 * @param data_out The pointer to a vector which will hold the pubkey data.
 * 
 * @return 1 if the script is the correct format, 0 otherwise.
 */
dogecoin_bool dogecoin_script_is_pubkey(const vector* ops, vector* data_out)
{
    if ((ops->len == 2) &&
        dogecoin_script_is_op(vector_idx(ops, 1), OP_CHECKSIG) &&
        dogecoin_script_is_op_pubkey(vector_idx(ops, 0))) {
        if (data_out) {
            //copy the full pubkey (33 or 65) in case of a non empty vector
            const dogecoin_script_op* op = vector_idx(ops, 0);
            uint8_t* buffer = dogecoin_calloc(1, op->datalen);
            memcpy_safe(buffer, op->data, op->datalen);
            vector_add(data_out, buffer);
        }
        return true;
    }
    return false;
}

/**
 * @brief This function checks whether the script matches
 * the template for a pubkey hash script, which is composed
 * of an OP_DUP, an OP_HASH160, a push pubkey hash operation, 
 * an OP_EQUALVERIFY, and an OP_CHECKSIG, in that order. The
 * pubkey hash is then loaded into data_out.
 * 
 * @param ops The pointer to a vector storing script data to be checked.
 * @param data_out The pointer to a vector where script data will be copied to.
 * 
 * @return 1 if the script is a valid pubkey hash, 0 otherwise.
 */
dogecoin_bool dogecoin_script_is_pubkeyhash(const vector* ops, vector* data_out)
{
    if ((ops->len == 5) &&
        dogecoin_script_is_op(vector_idx(ops, 0), OP_DUP) &&
        dogecoin_script_is_op(vector_idx(ops, 1), OP_HASH160) &&
        dogecoin_script_is_op_pubkeyhash(vector_idx(ops, 2)) &&
        dogecoin_script_is_op(vector_idx(ops, 3), OP_EQUALVERIFY) &&
        dogecoin_script_is_op(vector_idx(ops, 4), OP_CHECKSIG)) {
        if (data_out) {
            //copy the data (hash160) in case of a non empty vector
            const dogecoin_script_op* op = vector_idx(ops, 2);
            uint8_t* buffer = dogecoin_calloc(1, sizeof(uint160));
            memcpy_safe(buffer, op->data, sizeof(uint160));
            vector_add(data_out, buffer);
        }
        return true;
    }
    return false;
}


/**
 * @brief This function checks whether the script matches
 * the template for a script hash, which is composed of an
 * OP_HASH160, a push pubkey hash operation, and an OP_EQUAL,
 * in that order. The script hash is then loaded into data_out.
 * 
 * @param ops The pointer to the vector storing script data to be checked.
 * @param data_out The pointer to a vector where script data will be copied to.
 * 
 * @return 1 if the script is a script hash, 0 otherwise.
 */
dogecoin_bool dogecoin_script_is_scripthash(const vector* ops, vector* data_out)
{
    if ((ops->len == 3) &&
        dogecoin_script_is_op(vector_idx(ops, 0), OP_HASH160) &&
        dogecoin_script_is_op_pubkeyhash(vector_idx(ops, 1)) &&
        dogecoin_script_is_op(vector_idx(ops, 2), OP_EQUAL)) {
        if (data_out) {
            //copy the data (hash160) in case of a non empty vector
            const dogecoin_script_op* op = vector_idx(ops, 1);
            uint8_t* buffer = dogecoin_calloc(1, sizeof(uint160));
            memcpy_safe(buffer, op->data, sizeof(uint160));
            vector_add(data_out, buffer);
        }

        return true;
    }
    return false;
}


/**
 * @brief The function checks whether the script operation
 * has an opcode label of less than or equal to 16.
 * 
 * @param op The pointer to the operation to be checked.
 * 
 * @return 1 if it has a small int opcode, 0 otherwise.
 */
static dogecoin_bool dogecoin_script_is_op_smallint(const dogecoin_script_op* op)
{
    return ((op->op == OP_0) ||
            (op->op >= OP_1 && op->op <= OP_16));
}


/**
 * @brief This function checks whether a script is a multisig
 * script, which is true if the vector is composed of between 
 * 3 and 19 scripts (inclusive), the first operation has a small 
 * int opcode, the second-to-last operation has a small int 
 * opcode, the last operation is OP_CHECKMULTISIG, and all 
 * operations in between push pubkeys to the stack.
 * 
 * @param ops The pointer to the vector storing the script.
 * 
 * @return 1 if the script is a multisig script, 0 otherwise.
 */
dogecoin_bool dogecoin_script_is_multisig(const vector* ops)
{
    if ((ops->len < 3) || (ops->len > (16 + 3)) ||
        !dogecoin_script_is_op_smallint(vector_idx(ops, 0)) ||
        !dogecoin_script_is_op_smallint(vector_idx(ops, ops->len - 2)) ||
        !dogecoin_script_is_op(vector_idx(ops, ops->len - 1), OP_CHECKMULTISIG))
        return false;

    unsigned int i;
    for (i = 1; i < (ops->len - 2); i++)
        if (!dogecoin_script_is_op_pubkey(vector_idx(ops, i)))
            return false;

    return true;
}


/**
 * @brief This function classifies a script as one of four
 * types: pubkey, pubkey hash, script hash, or multisig.
 * 
 * @param ops The pointer to the vector storing the script.
 * 
 * @return The type of script.
 */
enum dogecoin_tx_out_type dogecoin_script_classify_ops(const vector* ops)
{
    if (dogecoin_script_is_pubkeyhash(ops, NULL))
        return DOGECOIN_TX_PUBKEYHASH;
    if (dogecoin_script_is_scripthash(ops, NULL))
        return DOGECOIN_TX_SCRIPTHASH;
    if (dogecoin_script_is_pubkey(ops, NULL))
        return DOGECOIN_TX_PUBKEY;
    if (dogecoin_script_is_multisig(ops))
        return DOGECOIN_TX_MULTISIG;

    return DOGECOIN_TX_NONSTANDARD;
}


/**
 * @brief This function takes a cstring representation of
 * a script, classifies it as one of the four script types
 * and loads the script data into data_out if it is not a 
 * multisig script.
 * 
 * @param script The pointer to the cstring script to be parsed and classified.
 * @param data_out The pointer to the vector that will contain the parsed scripts.
 * 
 * @return The script type.
 */
enum dogecoin_tx_out_type dogecoin_script_classify(const cstring* script, vector* data_out)
{
    //INFO: could be speed up by not forming a vector
    //      and directly parse the script cstring

    enum dogecoin_tx_out_type tx_out_type = DOGECOIN_TX_NONSTANDARD;
    vector* ops = vector_new(10, dogecoin_script_op_free_cb);
    dogecoin_script_get_ops(script, ops);

    if (dogecoin_script_is_pubkeyhash(ops, data_out))
        tx_out_type = DOGECOIN_TX_PUBKEYHASH;
    if (dogecoin_script_is_scripthash(ops, data_out))
        tx_out_type = DOGECOIN_TX_SCRIPTHASH;
    if (dogecoin_script_is_pubkey(ops, data_out))
        tx_out_type = DOGECOIN_TX_PUBKEY;
    if (dogecoin_script_is_multisig(ops))
        tx_out_type = DOGECOIN_TX_MULTISIG;

    vector_free(ops, true);
    return tx_out_type;
}


/**
 * @brief This function takes an int and translates it to a
 * small int opcode if it is between 0 and 16, inclusive.
 * 
 * @param n The number of the desired small int opcode.
 * 
 * @return The small int opcode.
 */
enum opcodetype dogecoin_encode_op_n(const int n)
{
    assert(n >= 0 && n <= 16);
    if (n == 0)
        return OP_0;
    return (enum opcodetype)(OP_1 + n - 1);
}


/**
 * @brief This function takes an existing script in cstring
 * form and appends another script operation to it. No data
 * is pushed to the stack.
 * 
 * @param script_in The pointer to the cstring containing the script.
 * @param op The opcode to append.
 * 
 * @return Nothing.
 */
void dogecoin_script_append_op(cstring* script_in, enum opcodetype op)
{
    cstr_append_buf(script_in, &op, 1);
}


/**
 * @brief This function takes an existing script in cstring
 * form and appends an operation to push a specified buffer
 * of data to the stack.
 * 
 * @param script_in The pointer to the cstring containing the script.
 * @param data The buffer to be pushed in the push operation.
 * @param datalen The size in bytes of the buffer.
 * 
 * @return Nothing.
 */
void dogecoin_script_append_pushdata(cstring* script_in, const unsigned char* data, const size_t datalen)
{
    if (datalen < OP_PUSHDATA1) {
        cstr_append_buf(script_in, (unsigned char*)&datalen, 1);
    } else if (datalen <= 0xff) {
        dogecoin_script_append_op(script_in, OP_PUSHDATA1);
        cstr_append_buf(script_in, (unsigned char*)&datalen, 1);
    } else if (datalen <= 0xffff) {
        dogecoin_script_append_op(script_in, OP_PUSHDATA2);
        uint16_t v = htole16((uint16_t)datalen);
        cstr_append_buf(script_in, &v, sizeof(v));
    } else {
        dogecoin_script_append_op(script_in, OP_PUSHDATA4);
        uint32_t v = htole32((uint32_t)datalen);
        cstr_append_buf(script_in, &v, sizeof(v));
    }
    cstr_append_buf(script_in, data, datalen);
}

/**
 * @brief This function takes a vector of dogecoin_pubkeys
 * and builds a multisig script which pushes all pubkeys 
 * to the stack and checks that each one is valid.
 * 
 * @param script_in The pointer to the cstring where the new multisig script will be built.
 * @param required_signatures The number of required signatures.
 * @param pubkeys_chars The pointer to the vector of pubkey push operations to combine into the multisig.
 * 
 * @return 1 if the script was built successfully, 0 if more than 16 pubkeys are given.
 */
dogecoin_bool dogecoin_script_build_multisig(cstring* script_in, const unsigned int required_signatures, const vector* pubkeys_chars)
{
    cstr_resize(script_in, 0); //clear script

    if (required_signatures > 16 || pubkeys_chars->len > 16)
        return false;
    enum opcodetype op_req_sig = dogecoin_encode_op_n(required_signatures);
    cstr_append_buf(script_in, &op_req_sig, 1);

    int i;
    for (i = 0; i < (int)pubkeys_chars->len; i++) {
        dogecoin_pubkey* pkey = pubkeys_chars->data[i];
        dogecoin_script_append_pushdata(script_in, pkey->pubkey, (pkey->compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH));
    }

    enum opcodetype op_pub_len = dogecoin_encode_op_n(pubkeys_chars->len);
    cstr_append_buf(script_in, &op_pub_len, 1);

    enum opcodetype op_checkmultisig = OP_CHECKMULTISIG;
    cstr_append_buf(script_in, &op_checkmultisig, 1);

    return true;
}


/**
 * @brief This function builds a pay-to-public-key-hash script
 * which duplicates the value at the top of the stack (a public
 * key), hashes this value, checks that this hash equals the
 * given uint160 object, and verifies the signature. This script
 * is then loaded into the cstring script_in.
 * 
 * @param script_in The pointer to the cstring where the p2pkh script will be built.
 * @param hash160 The hash160 of a public key.
 * 
 * @return 1 if the p2pkh script was built successfully.
 */
dogecoin_bool dogecoin_script_build_p2pkh(cstring* script_in, const uint160 hash160)
{
    cstr_resize(script_in, 0); //clear script

    dogecoin_script_append_op(script_in, OP_DUP);
    dogecoin_script_append_op(script_in, OP_HASH160);


    dogecoin_script_append_pushdata(script_in, (unsigned char*)hash160, sizeof(uint160));
    dogecoin_script_append_op(script_in, OP_EQUALVERIFY);
    dogecoin_script_append_op(script_in, OP_CHECKSIG);

    return true;
}


/**
 * @brief This function builds a pay-to-script-hash script
 * which hashes the value on the top of the stack (script
 * data) and ensures that this equals the uint160 object given. 
 * The script is then loaded into the cstring script_in.
 * 
 * @param script_in The pointer to the cstring where the p2sh script will be built.
 * @param hash160 The hash160 of a public key.
 * 
 * @return 1 if the p2sh script was built successfully.
 */
dogecoin_bool dogecoin_script_build_p2sh(cstring* script_in, const uint160 hash160)
{
    cstr_resize(script_in, 0); //clear script
    dogecoin_script_append_op(script_in, OP_HASH160);
    dogecoin_script_append_pushdata(script_in, (unsigned char*)hash160, sizeof(uint160));
    dogecoin_script_append_op(script_in, OP_EQUAL);

    return true;
}


/**
 * @brief This function takes a script, computes its hash, 
 * and returns it as a uint160 object.
 * 
 * @param script_in The pointer to the cstring which holds the script.
 * @param scripthash The uint160 object that will hold the hash of the script.
 * 
 * @return 1 if the script was hashed correctly, 0 if a null cstring was provided.
 */
dogecoin_bool dogecoin_script_get_scripthash(const cstring* script_in, uint160 scripthash)
{
    if (!script_in) {
        return false;
    }
    uint256 hash;
    dogecoin_hash_sngl_sha256((const unsigned char*)script_in->str, script_in->len, hash);
    rmd160(hash, sizeof(hash), scripthash);

    return true;
}


/**
 * @brief This function takes a script type and translates
 * it into string format.
 * 
 * @param type The type of script.
 * 
 * @return The string representation of the script.
 */
const char* dogecoin_tx_out_type_to_str(const enum dogecoin_tx_out_type type)
{
    if (type == DOGECOIN_TX_PUBKEY) {
        return "TX_PUBKEY";
    } else if (type == DOGECOIN_TX_PUBKEYHASH) {
        return "TX_PUBKEYHASH";
    } else if (type == DOGECOIN_TX_SCRIPTHASH) {
        return "TX_SCRIPTHASH";
    } else if (type == DOGECOIN_TX_MULTISIG) {
        return "TX_MULTISIG";
    } else {
        return "TX_NONSTANDARD";
    }
}
