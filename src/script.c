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

#include <dogecoin/script.h>

#include <assert.h>
#include <string.h>

#include <dogecoin/buffer.h>
#include <dogecoin/crypto/hash.h>
#include <dogecoin/crypto/rmd160.h>
#include <dogecoin/serialize.h>

dogecoin_bool dogecoin_script_copy_without_op_codeseperator(const cstring* script_in, cstring* script_out) {
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
            cstr_append_buf(script_out, &v32, 5);
            data_len = v32;
        } else if (opcode == OP_CODESEPARATOR)
            continue;

        if (data_len > 0) {
            assert(data_len < 16777215); //limit max push to 0xFFFFFF
            unsigned char *bufpush = (unsigned char *)dogecoin_malloc(data_len);
            deser_bytes(bufpush, &buf, data_len);
            cstr_append_buf(script_out, bufpush, data_len);
            dogecoin_free(bufpush);
        } else
            cstr_append_buf(script_out, &opcode, 1);
    }

    return true;

err_out:
    return false;
}

dogecoin_script_op* dogecoin_script_op_new() {
    dogecoin_script_op* script_op;
    script_op = dogecoin_calloc(1, sizeof(dogecoin_script_op));

    return script_op;
}

void dogecoin_script_op_free(dogecoin_script_op* script_op) {
    if (script_op->data) {
        dogecoin_free(script_op->data);
        script_op->data = NULL;
    }
    script_op->datalen = 0;
    script_op->op = OP_0;
}

void dogecoin_script_op_free_cb(void* data) {
    dogecoin_script_op* script_op = data;
    dogecoin_script_op_free(script_op);

    dogecoin_free(script_op);
}

dogecoin_bool dogecoin_script_get_ops(const cstring* script_in, vector* ops_out) {
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
        memcpy(op->data, buf.p, data_len);
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

static inline dogecoin_bool dogecoin_script_is_pushdata(const enum opcodetype op) {
    return (op <= OP_PUSHDATA4);
}

static dogecoin_bool dogecoin_script_is_op(const dogecoin_script_op* op, enum opcodetype opcode) {
    return (op->op == opcode);
}

static dogecoin_bool dogecoin_script_is_op_pubkey(const dogecoin_script_op* op) {
    if (!dogecoin_script_is_pushdata(op->op))
        return false;
    if (op->datalen != DOGECOIN_ECKEY_COMPRESSED_LENGTH && op->datalen != DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH)
        return false;
    if (dogecoin_pubkey_get_length(op->data[0]) != op->datalen) {
        return false;
    }
    return true;
}

static dogecoin_bool dogecoin_script_is_op_pubkeyhash(const dogecoin_script_op* op) {
    if (!dogecoin_script_is_pushdata(op->op))
        return false;
    if (op->datalen != 20)
        return false;
    return true;
}

// OP_PUBKEY, OP_CHECKSIG
dogecoin_bool dogecoin_script_is_pubkey(const vector* ops, vector* data_out) {
    if ((ops->len == 2) &&
            dogecoin_script_is_op(vector_idx(ops, 1), OP_CHECKSIG) &&
            dogecoin_script_is_op_pubkey(vector_idx(ops, 0))) {
        if (data_out) {
            //copy the full pubkey (33 or 65) in case of a non empty vector
            const dogecoin_script_op* op = vector_idx(ops, 0);
            uint8_t* buffer = dogecoin_calloc(1, op->datalen);
            memcpy(buffer, op->data, op->datalen);
            vector_add(data_out, buffer);
        }
        return true;
    }
    return false;
}

// OP_DUP, OP_HASH160, OP_PUBKEYHASH, OP_EQUALVERIFY, OP_CHECKSIG,
dogecoin_bool dogecoin_script_is_pubkeyhash(const vector* ops, vector* data_out) {
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
            memcpy(buffer, op->data, sizeof(uint160));
            vector_add(data_out, buffer);
        }
        return true;
    }
    return false;
}

// OP_HASH160, OP_PUBKEYHASH, OP_EQUAL
dogecoin_bool dogecoin_script_is_scripthash(const vector* ops, vector* data_out) {
    if ((ops->len == 3) &&
            dogecoin_script_is_op(vector_idx(ops, 0), OP_HASH160) &&
            dogecoin_script_is_op_pubkeyhash(vector_idx(ops, 1)) &&
            dogecoin_script_is_op(vector_idx(ops, 2), OP_EQUAL)) {

        if (data_out) {
            //copy the data (hash160) in case of a non empty vector
            const dogecoin_script_op* op = vector_idx(ops, 1);
            uint8_t* buffer = dogecoin_calloc(1, sizeof(uint160));
            memcpy(buffer, op->data, sizeof(uint160));
            vector_add(data_out, buffer);
        }

        return true;
    }
    return false;
}

static dogecoin_bool dogecoin_script_is_op_smallint(const dogecoin_script_op* op) {
    return ((op->op == OP_0) ||
            (op->op >= OP_1 && op->op <= OP_16));
}

dogecoin_bool dogecoin_script_is_multisig(const vector* ops) {
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

enum dogecoin_tx_out_type dogecoin_script_classify_ops(const vector* ops) {
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

enum dogecoin_tx_out_type dogecoin_script_classify(const cstring* script, vector* data_out) {
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
    uint8_t version = 0;
    uint8_t witness_program[40] = {0};
    int witness_program_len = 0;
    if (dogecoin_script_is_witnessprogram(script, &version, witness_program, &witness_program_len)) {
        if (version == 0 && witness_program_len == 20) {
            tx_out_type = DOGECOIN_TX_WITNESS_V0_PUBKEYHASH;
            if (data_out) {
                uint8_t *witness_program_cpy = dogecoin_calloc(1, witness_program_len);
                memcpy(witness_program_cpy, witness_program, witness_program_len);
                vector_add(data_out, witness_program_cpy);
            }
        }
        if (version == 0 && witness_program_len == 32) {
            tx_out_type = DOGECOIN_TX_WITNESS_V0_SCRIPTHASH;
            if (data_out) {
                uint8_t *witness_program_cpy = dogecoin_calloc(1, witness_program_len);
                memcpy(witness_program_cpy, witness_program, witness_program_len);
                vector_add(data_out, witness_program_cpy);
            }
        }
    }
    vector_free(ops, true);
    return tx_out_type;
}

enum opcodetype dogecoin_encode_op_n(const int n) {
    assert(n >= 0 && n <= 16);
    if (n == 0)
        return OP_0;
    return (enum opcodetype)(OP_1 + n - 1);
}

void dogecoin_script_append_op(cstring* script_in, enum opcodetype op) {
    cstr_append_buf(script_in, &op, 1);
}

void dogecoin_script_append_pushdata(cstring* script_in, const unsigned char* data, const size_t datalen) {
    if (datalen < OP_PUSHDATA1) {
        cstr_append_buf(script_in, (unsigned char*)&datalen, 1);
    } else if (datalen <= 0xff) {
        dogecoin_script_append_op(script_in, OP_PUSHDATA1);
        cstr_append_buf(script_in, (unsigned char*)&datalen, 1);
    } else if (datalen <= 0xffff) {
        dogecoin_script_append_op(script_in, OP_PUSHDATA2);
        uint16_t v = htole16(datalen);
        cstr_append_buf(script_in, &v, sizeof(v));
    } else {
        dogecoin_script_append_op(script_in, OP_PUSHDATA4);
        uint32_t v = htole32(datalen);
        cstr_append_buf(script_in, &v, sizeof(v));
    }
    cstr_append_buf(script_in, data, datalen);
}

dogecoin_bool dogecoin_script_build_multisig(cstring* script_in, const unsigned int required_signatures, const vector* pubkeys_chars) {
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

dogecoin_bool dogecoin_script_build_p2pkh(cstring* script_in, const uint160 hash160) {
    cstr_resize(script_in, 0); //clear script

    dogecoin_script_append_op(script_in, OP_DUP);
    dogecoin_script_append_op(script_in, OP_HASH160);


    dogecoin_script_append_pushdata(script_in, (unsigned char*)hash160, sizeof(uint160));
    dogecoin_script_append_op(script_in, OP_EQUALVERIFY);
    dogecoin_script_append_op(script_in, OP_CHECKSIG);

    return true;
}

dogecoin_bool dogecoin_script_build_p2wpkh(cstring* script_in, const uint160 hash160) {
    cstr_resize(script_in, 0); //clear script

    dogecoin_script_append_op(script_in, OP_0);
    dogecoin_script_append_pushdata(script_in, (unsigned char*)hash160, sizeof(uint160));

    return true;
}

dogecoin_bool dogecoin_script_build_p2sh(cstring* script_in, const uint160 hash160) {
    cstr_resize(script_in, 0); //clear script
    dogecoin_script_append_op(script_in, OP_HASH160);
    dogecoin_script_append_pushdata(script_in, (unsigned char*)hash160, sizeof(uint160));
    dogecoin_script_append_op(script_in, OP_EQUAL);

    return true;
}

dogecoin_bool dogecoin_script_get_scripthash(const cstring* script_in, uint160 scripthash) {
    if (!script_in) {
        return false;
    }
    uint256 hash;
    dogecoin_hash_sngl_sha256((const unsigned char *)script_in->str, script_in->len, hash);
    rmd160(hash, sizeof(hash), scripthash);

    return true;
}

const char * dogecoin_tx_out_type_to_str(const enum dogecoin_tx_out_type type) {
    if (type == DOGECOIN_TX_PUBKEY) {
        return "TX_PUBKEY";
    }
    else if (type == DOGECOIN_TX_PUBKEYHASH) {
        return "TX_PUBKEYHASH";
    }
    else if (type == DOGECOIN_TX_SCRIPTHASH) {
        return "TX_SCRIPTHASH";
    }
    else if (type == DOGECOIN_TX_MULTISIG) {
        return "TX_MULTISIG";
    }
    else {
        return "TX_NONSTANDARD";
    }
}

static uint8_t dogecoin_decode_op_n(enum opcodetype op) {
    if (op == OP_0) {
        return 0;
    }
    assert(op >= OP_1 && op <= OP_16);
    return (uint8_t)op - (uint8_t)(OP_1 - 1);
}

// A witness program is any valid script that consists of a 1-byte push opcode
// followed by a data push between 2 and 40 bytes.
dogecoin_bool dogecoin_script_is_witnessprogram(const cstring* script, uint8_t* version_out, uint8_t *program_out, int *programm_len_out) {
    if (!version_out || !program_out) {
        return false;
    }
    if (script->len < 4 || script->len > 42) {
        return false;
    }
    if (script->str[0] != OP_0 && (script->str[0] < OP_1 || script->str[0] > OP_16)) {
        return false;
    }
    if ((size_t)(script->str[1] + 2) == script->len) {
        *version_out = dogecoin_decode_op_n((enum opcodetype)script->str[0]);
        if (program_out) {
            assert(script->len - 2 <= 40);
            memcpy(program_out, script->str + 2, script->len - 2);
            *programm_len_out = script->len - 2;
        }
        return true;
    }
    return false;
}
