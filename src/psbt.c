/*
 The MIT License (MIT)

 Copyright (c) 2025 bluezr
 Copyright (c) 2025 The Dogecoin Foundation

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

#if defined(HAVE_CONFIG_H)
#include "libdogecoin-config.h"
#endif

#include <assert.h>
#include <string.h>

#include <dogecoin/buffer.h>
#include <dogecoin/cstr.h>
#include <dogecoin/ecc.h>
#include <dogecoin/key.h>
#include <dogecoin/mem.h>
#include <dogecoin/portable_endian.h>
#include <dogecoin/psbt.h>
#include <dogecoin/rmd160.h>
#include <dogecoin/script.h>
#include <dogecoin/serialize.h>
#include <dogecoin/sha2.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

/* ── Internal helpers ─────────────────────────────────────────── */

/* Script-aware data push: emits the correct opcode(s) for length. */
static void ser_script_push(cstring *s, const uint8_t *data, size_t len)
{
    if (len <= 75) {
        uint8_t b = (uint8_t)len;
        ser_bytes(s, &b, 1);
    } else if (len <= 255) {
        uint8_t hdr[2] = { 0x4c /* OP_PUSHDATA1 */, (uint8_t)len };
        ser_bytes(s, hdr, 2);
    } else {
        uint8_t op = 0x4d; /* OP_PUSHDATA2 */
        uint16_t n = htole16((uint16_t)len);
        ser_bytes(s, &op, 1);
        ser_bytes(s, (const uint8_t *)&n, 2);
    }
    ser_bytes(s, data, len);
}

/* HASH160 = RIPEMD160(SHA256(data)).  out must be 20 bytes. */
static void hash160_buf(const uint8_t *data, size_t len, uint8_t out[20])
{
    uint8_t sha[SHA256_DIGEST_LENGTH];
    sha256_raw(data, len, sha);
    rmd160(sha, SHA256_DIGEST_LENGTH, out);
}

/*
 * Parse a bare-multisig redeem script:
 *   OP_m <pub_0> ... <pub_{n-1}> OP_n OP_CHECKMULTISIG
 * Returns true and fills m_out, n_out, pubkeys[0..n-1], pubkey_lens[0..n-1].
 * pubkeys[] point into script->str — do NOT free them.
 */
static dogecoin_bool psbt_parse_multisig(const cstring *script,
                                          uint8_t *m_out, uint8_t *n_out,
                                          const uint8_t *pubkeys[16],
                                          uint8_t pubkey_lens[16])
{
    if (!script || script->len < 3) return false;
    const uint8_t *p   = (const uint8_t *)script->str;
    const uint8_t *end = p + script->len;

    if (*p < 0x51 || *p > 0x60) return false;   /* OP_1..OP_16 */
    uint8_t m = *p - 0x50;
    p++;

    uint8_t n = 0;
    while (p < end && *p == 0x21) {
        uint8_t pklen = *p++;
        if (p + pklen > end || n >= 16) return false;
        pubkeys[n]     = p;
        pubkey_lens[n] = pklen;
        n++;
        p += pklen;
    }
    if (n == 0) return false;

    if (p >= end || *p < 0x51 || *p > 0x60) return false;  /* OP_n */
    if (*p - 0x50 != n) return false;
    p++;

    if (p >= end || *p != 0xae /* OP_CHECKMULTISIG */) return false;
    p++;

    if (p != end || m > n) return false;

    *m_out = m;
    *n_out = n;
    return true;
}

/* Clear per-input signing fields after finalization (BIP174 §Finalizer). */
static void psbt_input_finalized_clear(dogecoin_psbt_input *in)
{
    dogecoin_free(in->partial_sigs);
    in->partial_sigs     = NULL;
    in->num_partial_sigs = 0;

    in->has_sighash_type = false;
    in->sighash_type     = 0;

    if (in->redeem_script) {
        cstr_free(in->redeem_script, true);
        in->redeem_script = NULL;
    }
    for (size_t i = 0; i < in->num_keypaths; i++)
        dogecoin_free(in->keypaths[i].path);
    dogecoin_free(in->keypaths);
    in->keypaths     = NULL;
    in->num_keypaths = 0;
}

static void ser_psbt_kv(cstring *s,
                        const uint8_t *key, size_t klen,
                        const uint8_t *val, size_t vlen)
{
    ser_varlen(s, (uint32_t)klen);
    ser_bytes(s, key, klen);
    ser_varlen(s, (uint32_t)vlen);
    ser_bytes(s, val, vlen);
}

/* Write the map separator (empty key). */
static void ser_psbt_sep(cstring *s)
{
    uint8_t z = 0;
    ser_bytes(s, &z, 1);
}

/* Read one key-value pair.  Returns false on I/O error.
 * Sets *key_len = 0 when the separator (end-of-map) is encountered.
 * On success the caller owns *key and *val (dogecoin_free them). */
static dogecoin_bool deser_psbt_kv(struct const_buffer *buf,
                                    uint8_t **key, size_t *key_len,
                                    uint8_t **val, size_t *val_len)
{
    uint32_t klen, vlen;
    *key = NULL; *val = NULL;
    *key_len = 0; *val_len = 0;

    if (!deser_varlen(&klen, buf)) return false;
    if (klen == 0) return true; /* separator */
    if (klen > buf->len) return false; /* declared key length exceeds remaining input */

    *key = dogecoin_malloc(klen);
    if (!deser_bytes(*key, buf, klen)) { dogecoin_free(*key); *key = NULL; return false; }
    *key_len = klen;

    if (!deser_varlen(&vlen, buf)) { dogecoin_free(*key); *key = NULL; return false; }
    if (vlen > 0) {
        if (vlen > buf->len) { dogecoin_free(*key); *key = NULL; return false; }
        *val = dogecoin_malloc(vlen);
        if (!deser_bytes(*val, buf, vlen)) {
            dogecoin_free(*key); dogecoin_free(*val);
            *key = NULL; *val = NULL; return false;
        }
    }
    *val_len = vlen;
    return true;
}

/* ── Lifecycle ────────────────────────────────────────────────── */

dogecoin_psbt *dogecoin_psbt_new(void)
{
    dogecoin_psbt *p = dogecoin_calloc(1, sizeof(*p));
    p->tx = dogecoin_tx_new();
    return p;
}

static void psbt_input_clear(dogecoin_psbt_input *in)
{
    if (in->non_witness_utxo) { dogecoin_tx_free(in->non_witness_utxo); in->non_witness_utxo = NULL; }
    if (in->redeem_script)    { cstr_free(in->redeem_script, true); in->redeem_script = NULL; }
    if (in->final_script_sig) { cstr_free(in->final_script_sig, true); in->final_script_sig = NULL; }
    for (size_t i = 0; i < in->num_keypaths; i++)
        dogecoin_free(in->keypaths[i].path);
    dogecoin_free(in->keypaths);
    dogecoin_free(in->partial_sigs);
    for (size_t i = 0; i < in->num_unknowns; i++) {
        dogecoin_free(in->unknowns[i].key);
        dogecoin_free(in->unknowns[i].value);
    }
    dogecoin_free(in->unknowns);
}

static void psbt_output_clear(dogecoin_psbt_output *out)
{
    if (out->redeem_script) { cstr_free(out->redeem_script, true); out->redeem_script = NULL; }
    for (size_t i = 0; i < out->num_keypaths; i++)
        dogecoin_free(out->keypaths[i].path);
    dogecoin_free(out->keypaths);
    for (size_t i = 0; i < out->num_unknowns; i++) {
        dogecoin_free(out->unknowns[i].key);
        dogecoin_free(out->unknowns[i].value);
    }
    dogecoin_free(out->unknowns);
}

void dogecoin_psbt_free(dogecoin_psbt *psbt)
{
    if (!psbt) return;
    if (psbt->tx) dogecoin_tx_free(psbt->tx);
    for (size_t i = 0; i < psbt->num_inputs;  i++) psbt_input_clear(&psbt->inputs[i]);
    for (size_t i = 0; i < psbt->num_outputs; i++) psbt_output_clear(&psbt->outputs[i]);
    dogecoin_free(psbt->inputs);
    dogecoin_free(psbt->outputs);
    for (size_t i = 0; i < psbt->num_xpubs; i++)
        dogecoin_free(psbt->xpubs[i].path);
    dogecoin_free(psbt->xpubs);
    for (size_t i = 0; i < psbt->num_unknowns; i++) {
        dogecoin_free(psbt->unknowns[i].key);
        dogecoin_free(psbt->unknowns[i].value);
    }
    dogecoin_free(psbt->unknowns);
    dogecoin_free(psbt);
}

/* ── Creator ──────────────────────────────────────────────────── */

dogecoin_psbt *dogecoin_psbt_create(const dogecoin_tx *tx)
{
    if (!tx || !tx->vin || !tx->vout) return NULL;

    /* BIP174: all inputs must have empty scriptSig */
    for (size_t i = 0; i < tx->vin->len; i++) {
        dogecoin_tx_in *txin = vector_idx(tx->vin, i);
        if (txin->script_sig && txin->script_sig->len > 0) return NULL;
    }

    dogecoin_psbt *psbt = dogecoin_calloc(1, sizeof(*psbt));
    psbt->tx = dogecoin_tx_new();
    dogecoin_tx_copy(psbt->tx, tx);
    psbt->version = PSBT_VERSION_0;

    psbt->num_inputs  = tx->vin->len;
    psbt->num_outputs = tx->vout->len;
    psbt->inputs  = dogecoin_calloc(psbt->num_inputs,  sizeof(*psbt->inputs));
    psbt->outputs = dogecoin_calloc(psbt->num_outputs, sizeof(*psbt->outputs));
    return psbt;
}

/* ── Serialization ────────────────────────────────────────────── */

cstring *dogecoin_psbt_serialize(const dogecoin_psbt *psbt)
{
    if (!psbt || !psbt->tx) return NULL;

    cstring *s = cstr_new_sz(512);

    /* Magic */
    ser_bytes(s, (const uint8_t *)PSBT_MAGIC_BYTES, PSBT_MAGIC_LEN);

    /* ── Global map ── */
    /* 0x00: unsigned transaction */
    {
        uint8_t  key[1] = { PSBT_GLOBAL_UNSIGNED_TX };
        cstring *txbuf   = cstr_new_sz(256);
        dogecoin_tx_serialize(txbuf, psbt->tx);
        ser_psbt_kv(s, key, 1, (const uint8_t *)txbuf->str, txbuf->len);
        cstr_free(txbuf, true);
    }
    /* 0x01: xpubs */
    for (size_t i = 0; i < psbt->num_xpubs; i++) {
        const dogecoin_psbt_xpub *x = &psbt->xpubs[i];
        uint8_t key[1 + 78];
        key[0] = PSBT_GLOBAL_XPUB;
        memcpy(key + 1, x->xpub, 78);
        size_t vlen = 4 + x->path_len * 4;
        uint8_t *val = dogecoin_malloc(vlen);
        uint32_t fp = htole32(x->fingerprint);
        memcpy(val, &fp, 4);
        for (size_t j = 0; j < x->path_len; j++) {
            uint32_t c = htole32(x->path[j]);
            memcpy(val + 4 + j * 4, &c, 4);
        }
        ser_psbt_kv(s, key, 1 + 78, val, vlen);
        dogecoin_free(val);
    }
    /* 0xFB: version (omit for v0; required for v2+) */
    if (psbt->version != PSBT_VERSION_0) {
        uint8_t key[1] = { PSBT_GLOBAL_VERSION };
        uint32_t ver = htole32(psbt->version);
        ser_psbt_kv(s, key, 1, (const uint8_t *)&ver, 4);
    }
    /* global unknowns */
    for (size_t i = 0; i < psbt->num_unknowns; i++) {
        ser_psbt_kv(s, psbt->unknowns[i].key, psbt->unknowns[i].key_len,
                       psbt->unknowns[i].value, psbt->unknowns[i].value_len);
    }
    ser_psbt_sep(s);

    /* ── Per-input maps ── */
    for (size_t i = 0; i < psbt->num_inputs; i++) {
        const dogecoin_psbt_input *in = &psbt->inputs[i];

        /* 0x00: non-witness UTXO */
        if (in->non_witness_utxo) {
            uint8_t  key[1] = { PSBT_IN_NON_WITNESS_UTXO };
            cstring *txbuf   = cstr_new_sz(256);
            dogecoin_tx_serialize(txbuf, in->non_witness_utxo);
            ser_psbt_kv(s, key, 1, (const uint8_t *)txbuf->str, txbuf->len);
            cstr_free(txbuf, true);
        }
        /* 0x02: partial sigs */
        for (size_t j = 0; j < in->num_partial_sigs; j++) {
            const dogecoin_psbt_partialsig *ps = &in->partial_sigs[j];
            uint8_t key[1 + PSBT_MAX_PUBKEY_LEN];
            key[0] = PSBT_IN_PARTIAL_SIG;
            memcpy(key + 1, ps->pubkey, ps->pubkey_len);
            ser_psbt_kv(s, key, 1 + ps->pubkey_len, ps->sig, ps->sig_len);
        }
        /* 0x03: sighash type */
        if (in->has_sighash_type) {
            uint8_t  key[1] = { PSBT_IN_SIGHASH_TYPE };
            uint32_t sh = htole32(in->sighash_type);
            ser_psbt_kv(s, key, 1, (const uint8_t *)&sh, 4);
        }
        /* 0x04: redeem script */
        if (in->redeem_script && in->redeem_script->len > 0) {
            uint8_t key[1] = { PSBT_IN_REDEEM_SCRIPT };
            ser_psbt_kv(s, key, 1, (const uint8_t *)in->redeem_script->str,
                        in->redeem_script->len);
        }
        /* 0x06: BIP32 derivation paths */
        for (size_t j = 0; j < in->num_keypaths; j++) {
            const dogecoin_psbt_keypath *kp = &in->keypaths[j];
            uint8_t key[1 + PSBT_MAX_PUBKEY_LEN];
            key[0] = PSBT_IN_BIP32_DERIVATION;
            memcpy(key + 1, kp->pubkey, kp->pubkey_len);
            size_t vlen = 4 + kp->path_len * 4;
            uint8_t *val = dogecoin_malloc(vlen);
            uint32_t fp = htole32(kp->fingerprint);
            memcpy(val, &fp, 4);
            for (size_t k = 0; k < kp->path_len; k++) {
                uint32_t c = htole32(kp->path[k]);
                memcpy(val + 4 + k * 4, &c, 4);
            }
            ser_psbt_kv(s, key, 1 + kp->pubkey_len, val, vlen);
            dogecoin_free(val);
        }
        /* 0x07: final scriptSig */
        if (in->final_script_sig && in->final_script_sig->len > 0) {
            uint8_t key[1] = { PSBT_IN_FINAL_SCRIPTSIG };
            ser_psbt_kv(s, key, 1, (const uint8_t *)in->final_script_sig->str,
                        in->final_script_sig->len);
        }
        /* unknown fields */
        for (size_t j = 0; j < in->num_unknowns; j++) {
            ser_psbt_kv(s, in->unknowns[j].key, in->unknowns[j].key_len,
                           in->unknowns[j].value, in->unknowns[j].value_len);
        }
        ser_psbt_sep(s);
    }

    /* ── Per-output maps ── */
    for (size_t i = 0; i < psbt->num_outputs; i++) {
        const dogecoin_psbt_output *out = &psbt->outputs[i];

        /* 0x00: redeem script */
        if (out->redeem_script && out->redeem_script->len > 0) {
            uint8_t key[1] = { PSBT_OUT_REDEEM_SCRIPT };
            ser_psbt_kv(s, key, 1, (const uint8_t *)out->redeem_script->str,
                        out->redeem_script->len);
        }
        /* 0x02: BIP32 derivation paths */
        for (size_t j = 0; j < out->num_keypaths; j++) {
            const dogecoin_psbt_keypath *kp = &out->keypaths[j];
            uint8_t key[1 + PSBT_MAX_PUBKEY_LEN];
            key[0] = PSBT_OUT_BIP32_DERIVATION;
            memcpy(key + 1, kp->pubkey, kp->pubkey_len);
            size_t vlen = 4 + kp->path_len * 4;
            uint8_t *val = dogecoin_malloc(vlen);
            uint32_t fp = htole32(kp->fingerprint);
            memcpy(val, &fp, 4);
            for (size_t k = 0; k < kp->path_len; k++) {
                uint32_t c = htole32(kp->path[k]);
                memcpy(val + 4 + k * 4, &c, 4);
            }
            ser_psbt_kv(s, key, 1 + kp->pubkey_len, val, vlen);
            dogecoin_free(val);
        }
        /* unknown fields */
        for (size_t j = 0; j < out->num_unknowns; j++) {
            ser_psbt_kv(s, out->unknowns[j].key, out->unknowns[j].key_len,
                           out->unknowns[j].value, out->unknowns[j].value_len);
        }
        ser_psbt_sep(s);
    }

    return s;
}

/* ── Deserialization ──────────────────────────────────────────── */

dogecoin_bool dogecoin_psbt_deserialize(const uint8_t *data, size_t len, dogecoin_psbt **out)
{
    if (!data || len < PSBT_MAGIC_LEN + 1 || !out) return false;

    /* Verify magic */
    if (memcmp(data, PSBT_MAGIC_BYTES, PSBT_MAGIC_LEN) != 0) return false;

    struct const_buffer buf = { data + PSBT_MAGIC_LEN, len - PSBT_MAGIC_LEN };

    dogecoin_psbt *psbt = dogecoin_calloc(1, sizeof(*psbt));
    psbt->version = PSBT_VERSION_0;

    /* ── Parse global map ── */
    dogecoin_bool got_tx = false;
    dogecoin_bool got_version = false;
    while (true) {
        uint8_t *key = NULL, *val = NULL;
        size_t   klen, vlen;
        if (!deser_psbt_kv(&buf, &key, &klen, &val, &vlen)) goto fail;
        if (klen == 0) break; /* end of global map */

        uint8_t type = key[0];

        if (type == PSBT_GLOBAL_UNSIGNED_TX && klen == 1) {
            if (got_tx || vlen == 0) { dogecoin_free(key); dogecoin_free(val); goto fail; }
            psbt->tx = dogecoin_tx_new();
            size_t consumed;
            /* BIP174 global unsigned tx uses legacy (non-witness) encoding;
             * deserialize in non-witness mode so a 0-input tx is not misread
             * as a SegWit marker byte. */
            if (!dogecoin_tx_deserialize_ex(val, vlen, psbt->tx, &consumed, false)) {
                dogecoin_free(key); dogecoin_free(val); goto fail;
            }
            /* BIP174: the value must be exactly a transaction with no trailing
             * bytes — reject when the stated value length does not match what
             * the tx deserializer consumed. */
            if (consumed != vlen) { dogecoin_free(key); dogecoin_free(val); goto fail; }
            /* BIP174: tx inputs must have empty scriptSigs */
            for (size_t i = 0; i < psbt->tx->vin->len; i++) {
                dogecoin_tx_in *txin = vector_idx(psbt->tx->vin, i);
                if (txin->script_sig && txin->script_sig->len > 0) {
                    dogecoin_free(key); dogecoin_free(val); goto fail;
                }
            }
            got_tx = true;
            psbt->num_inputs  = psbt->tx->vin->len;
            psbt->num_outputs = psbt->tx->vout->len;
            psbt->inputs  = dogecoin_calloc(psbt->num_inputs,  sizeof(*psbt->inputs));
            psbt->outputs = dogecoin_calloc(psbt->num_outputs, sizeof(*psbt->outputs));

        } else if (type == PSBT_GLOBAL_VERSION && klen == 1) {
            /* BIP174 §2: a key must be unique within its map */
            if (got_version || vlen != 4) { dogecoin_free(key); dogecoin_free(val); goto fail; }
            memcpy(&psbt->version, val, 4);
            psbt->version = le32toh(psbt->version);
            got_version = true;

        } else if (type == PSBT_GLOBAL_XPUB && klen == 79) {
            if (vlen < 4) { dogecoin_free(key); dogecoin_free(val); goto fail; }
            /* BIP174 §2: reject a duplicate xpub key (compared on the 78 key bytes) */
            for (size_t k = 0; k < psbt->num_xpubs; k++) {
                if (memcmp(psbt->xpubs[k].xpub, key + 1, 78) == 0) {
                    dogecoin_free(key); dogecoin_free(val); goto fail;
                }
            }
            size_t path_len = (vlen - 4) / 4;
            psbt->xpubs = dogecoin_realloc(psbt->xpubs,
                              (psbt->num_xpubs + 1) * sizeof(*psbt->xpubs));
            dogecoin_psbt_xpub *x = &psbt->xpubs[psbt->num_xpubs++];
            memcpy(x->xpub, key + 1, 78);
            memcpy(&x->fingerprint, val, 4);
            x->fingerprint = le32toh(x->fingerprint);
            x->path_len = path_len;
            x->path = path_len ? dogecoin_malloc(path_len * 4) : NULL;
            for (size_t i = 0; i < path_len; i++) {
                uint32_t c;
                memcpy(&c, val + 4 + i * 4, 4);
                x->path[i] = le32toh(c);
            }
        } else if (type == PSBT_GLOBAL_UNSIGNED_TX || type == PSBT_GLOBAL_XPUB ||
                   type == PSBT_GLOBAL_VERSION) {
            /* Known global key type with a wrong key length — reject. */
            dogecoin_free(key); dogecoin_free(val); goto fail;
        } else {
            /* Duplicate unknown key is invalid (BIP174 §Encoding) */
            for (size_t k = 0; k < psbt->num_unknowns; k++) {
                if (psbt->unknowns[k].key_len == klen &&
                    memcmp(psbt->unknowns[k].key, key, klen) == 0) {
                    dogecoin_free(key); dogecoin_free(val); goto fail;
                }
            }
            psbt->unknowns = dogecoin_realloc(psbt->unknowns,
                                 (psbt->num_unknowns + 1) * sizeof(*psbt->unknowns));
            dogecoin_psbt_unknown *u = &psbt->unknowns[psbt->num_unknowns++];
            u->key = key; key = NULL;
            u->key_len = klen;
            u->value = val; val = NULL;
            u->value_len = vlen;
        }
        dogecoin_free(key);
        dogecoin_free(val);
    }
    if (!got_tx) goto fail;

    /* ── Parse per-input maps ── */
    for (size_t i = 0; i < psbt->num_inputs; i++) {
        dogecoin_psbt_input *in = &psbt->inputs[i];
        while (true) {
            uint8_t *key = NULL, *val = NULL;
            size_t   klen, vlen;
            if (!deser_psbt_kv(&buf, &key, &klen, &val, &vlen)) goto fail;
            if (klen == 0) break;

            uint8_t type = key[0];

            if (type == PSBT_IN_NON_WITNESS_UTXO && klen == 1) {
                if (in->non_witness_utxo || vlen == 0) { dogecoin_free(key); dogecoin_free(val); goto fail; }
                in->non_witness_utxo = dogecoin_tx_new();
                size_t consumed;
                if (!dogecoin_tx_deserialize(val, vlen, in->non_witness_utxo, &consumed)) {
                    dogecoin_free(key); dogecoin_free(val); goto fail;
                }
            } else if (type == PSBT_IN_PARTIAL_SIG && klen == 34) {
                /* key: 0x02 + 33-byte compressed pubkey */
                if (vlen == 0 || vlen > PSBT_MAX_SIG_LEN) {
                    dogecoin_free(key); dogecoin_free(val); goto fail;
                }
                size_t pklen = klen - 1;
                /* BIP174 §2: reject a duplicate partial sig key (same pubkey) */
                for (size_t k = 0; k < in->num_partial_sigs; k++) {
                    if (in->partial_sigs[k].pubkey_len == pklen &&
                        memcmp(in->partial_sigs[k].pubkey, key + 1, pklen) == 0) {
                        dogecoin_free(key); dogecoin_free(val); goto fail;
                    }
                }
                in->partial_sigs = dogecoin_realloc(in->partial_sigs,
                    (in->num_partial_sigs + 1) * sizeof(*in->partial_sigs));
                dogecoin_psbt_partialsig *ps = &in->partial_sigs[in->num_partial_sigs++];
                memcpy(ps->pubkey, key + 1, pklen);
                ps->pubkey_len = pklen;
                memcpy(ps->sig, val, vlen);
                ps->sig_len = vlen;
            } else if (type == PSBT_IN_SIGHASH_TYPE && klen == 1) {
                /* BIP174 §2: a key must be unique within its map */
                if (in->has_sighash_type || vlen != 4) { dogecoin_free(key); dogecoin_free(val); goto fail; }
                uint32_t sh; memcpy(&sh, val, 4); in->sighash_type = le32toh(sh);
                in->has_sighash_type = true;
            } else if (type == PSBT_IN_REDEEM_SCRIPT && klen == 1) {
                if (in->redeem_script) { dogecoin_free(key); dogecoin_free(val); goto fail; }
                in->redeem_script = cstr_new_buf((const char *)val, vlen);
            } else if (type == PSBT_IN_BIP32_DERIVATION && klen == 34) {
                size_t pklen = klen - 1;
                if (vlen < 4) { dogecoin_free(key); dogecoin_free(val); goto fail; }
                /* BIP174 §2: reject a duplicate derivation key (same pubkey) */
                for (size_t k = 0; k < in->num_keypaths; k++) {
                    if (in->keypaths[k].pubkey_len == pklen &&
                        memcmp(in->keypaths[k].pubkey, key + 1, pklen) == 0) {
                        dogecoin_free(key); dogecoin_free(val); goto fail;
                    }
                }
                size_t path_len = (vlen - 4) / 4;
                in->keypaths = dogecoin_realloc(in->keypaths,
                    (in->num_keypaths + 1) * sizeof(*in->keypaths));
                dogecoin_psbt_keypath *kp = &in->keypaths[in->num_keypaths++];
                memcpy(kp->pubkey, key + 1, pklen);
                kp->pubkey_len = pklen;
                memcpy(&kp->fingerprint, val, 4); kp->fingerprint = le32toh(kp->fingerprint);
                kp->path_len = path_len;
                kp->path = path_len ? dogecoin_malloc(path_len * 4) : NULL;
                for (size_t k = 0; k < path_len; k++) {
                    uint32_t c; memcpy(&c, val + 4 + k * 4, 4); kp->path[k] = le32toh(c);
                }
            } else if (type == PSBT_IN_FINAL_SCRIPTSIG && klen == 1) {
                if (in->final_script_sig) { dogecoin_free(key); dogecoin_free(val); goto fail; }
                in->final_script_sig = cstr_new_buf((const char *)val, vlen);
            } else if (type == PSBT_IN_NON_WITNESS_UTXO || type == PSBT_IN_PARTIAL_SIG ||
                       type == PSBT_IN_SIGHASH_TYPE || type == PSBT_IN_REDEEM_SCRIPT ||
                       type == PSBT_IN_BIP32_DERIVATION || type == PSBT_IN_FINAL_SCRIPTSIG) {
                /* A known input key type reached here only because its key length
                 * was wrong for that type (e.g. a partial-sig key with a short
                 * pubkey). BIP174 requires a correctly-formed key, so reject
                 * rather than silently storing it as an unknown. */
                dogecoin_free(key); dogecoin_free(val); goto fail;
            } else {
                for (size_t k = 0; k < in->num_unknowns; k++) {
                    if (in->unknowns[k].key_len == klen &&
                        memcmp(in->unknowns[k].key, key, klen) == 0) {
                        dogecoin_free(key); dogecoin_free(val); goto fail;
                    }
                }
                in->unknowns = dogecoin_realloc(in->unknowns,
                    (in->num_unknowns + 1) * sizeof(*in->unknowns));
                dogecoin_psbt_unknown *u = &in->unknowns[in->num_unknowns++];
                u->key = key; key = NULL; u->key_len = klen;
                u->value = val; val = NULL; u->value_len = vlen;
            }
            dogecoin_free(key);
            dogecoin_free(val);
        }
    }

    /* ── Parse per-output maps ── */
    for (size_t i = 0; i < psbt->num_outputs; i++) {
        dogecoin_psbt_output *out = &psbt->outputs[i];
        while (true) {
            uint8_t *key = NULL, *val = NULL;
            size_t   klen, vlen;
            if (!deser_psbt_kv(&buf, &key, &klen, &val, &vlen)) goto fail;
            if (klen == 0) break;

            uint8_t type = key[0];

            if (type == PSBT_OUT_REDEEM_SCRIPT && klen == 1) {
                if (out->redeem_script) { dogecoin_free(key); dogecoin_free(val); goto fail; }
                out->redeem_script = cstr_new_buf((const char *)val, vlen);
            } else if (type == PSBT_OUT_BIP32_DERIVATION && klen == 34) {
                size_t pklen = klen - 1;
                if (vlen < 4) { dogecoin_free(key); dogecoin_free(val); goto fail; }
                /* BIP174 §2: reject a duplicate derivation key (same pubkey) */
                for (size_t k = 0; k < out->num_keypaths; k++) {
                    if (out->keypaths[k].pubkey_len == pklen &&
                        memcmp(out->keypaths[k].pubkey, key + 1, pklen) == 0) {
                        dogecoin_free(key); dogecoin_free(val); goto fail;
                    }
                }
                size_t path_len = (vlen - 4) / 4;
                out->keypaths = dogecoin_realloc(out->keypaths,
                    (out->num_keypaths + 1) * sizeof(*out->keypaths));
                dogecoin_psbt_keypath *kp = &out->keypaths[out->num_keypaths++];
                memcpy(kp->pubkey, key + 1, pklen);
                kp->pubkey_len = pklen;
                memcpy(&kp->fingerprint, val, 4); kp->fingerprint = le32toh(kp->fingerprint);
                kp->path_len = path_len;
                kp->path = path_len ? dogecoin_malloc(path_len * 4) : NULL;
                for (size_t k = 0; k < path_len; k++) {
                    uint32_t c; memcpy(&c, val + 4 + k * 4, 4); kp->path[k] = le32toh(c);
                }
            } else if (type == PSBT_OUT_REDEEM_SCRIPT || type == PSBT_OUT_BIP32_DERIVATION) {
                /* Known output key type with a wrong key length — reject. */
                dogecoin_free(key); dogecoin_free(val); goto fail;
            } else {
                for (size_t k = 0; k < out->num_unknowns; k++) {
                    if (out->unknowns[k].key_len == klen &&
                        memcmp(out->unknowns[k].key, key, klen) == 0) {
                        dogecoin_free(key); dogecoin_free(val); goto fail;
                    }
                }
                out->unknowns = dogecoin_realloc(out->unknowns,
                    (out->num_unknowns + 1) * sizeof(*out->unknowns));
                dogecoin_psbt_unknown *u = &out->unknowns[out->num_unknowns++];
                u->key = key; key = NULL; u->key_len = klen;
                u->value = val; val = NULL; u->value_len = vlen;
            }
            dogecoin_free(key);
            dogecoin_free(val);
        }
    }

    *out = psbt;
    return true;

fail:
    dogecoin_psbt_free(psbt);
    *out = NULL;
    return false;
}

/* ── Base64 / Hex round-trips ─────────────────────────────────── */

char *dogecoin_psbt_to_base64(const dogecoin_psbt *psbt)
{
    cstring *raw = dogecoin_psbt_serialize(psbt);
    if (!raw) return NULL;
    unsigned int out_len = base64_encoded_size((unsigned int)raw->len) + 1;
    char *b64 = dogecoin_malloc(out_len);
    base64_encode((const unsigned char *)raw->str, (unsigned int)raw->len,
                  (unsigned char *)b64);
    b64[out_len - 1] = '\0';
    cstr_free(raw, true);
    return b64;
}

dogecoin_bool dogecoin_psbt_from_base64(const char *b64, dogecoin_psbt **out)
{
    if (!b64 || !out) return false;
    unsigned int b64len = (unsigned int)strlen(b64);
    unsigned int bin_len = base64_decoded_size(b64len);
    uint8_t *bin = dogecoin_malloc(bin_len + 1); /* +1 for null terminator written by base64_decode */
    unsigned int actual = base64_decode((const unsigned char *)b64, b64len, bin);
    dogecoin_bool ok = dogecoin_psbt_deserialize(bin, actual, out);
    dogecoin_free(bin);
    return ok;
}

char *dogecoin_psbt_to_hex(const dogecoin_psbt *psbt)
{
    static const char digits[] = "0123456789abcdef";
    cstring *raw = dogecoin_psbt_serialize(psbt);
    if (!raw) return NULL;
    size_t n = raw->len;
    char *hex = dogecoin_malloc(n * 2 + 1);
    for (size_t i = 0; i < n; i++) {
        uint8_t b = (uint8_t)raw->str[i];
        hex[i * 2]     = digits[b >> 4];
        hex[i * 2 + 1] = digits[b & 0xF];
    }
    hex[n * 2] = '\0';
    cstr_free(raw, true);
    return hex;
}

dogecoin_bool dogecoin_psbt_from_hex(const char *hex, dogecoin_psbt **out)
{
    if (!hex || !out) return false;
    size_t hex_len = strlen(hex);
    if (hex_len % 2 != 0) return false;
    size_t bin_len = hex_len / 2;
    uint8_t *bin = dogecoin_malloc(bin_len);
    size_t actual_len = 0;
    utils_hex_to_bin((const char *)hex, bin, hex_len, &actual_len);
    dogecoin_bool ok = dogecoin_psbt_deserialize(bin, actual_len ? actual_len : bin_len, out);
    dogecoin_free(bin);
    return ok;
}

/* ── Updater ──────────────────────────────────────────────────── */

dogecoin_bool dogecoin_psbt_input_set_utxo(dogecoin_psbt *psbt, size_t idx,
                                            const dogecoin_tx *utxo)
{
    if (!psbt || idx >= psbt->num_inputs || !utxo) return false;
    dogecoin_psbt_input *in = &psbt->inputs[idx];
    if (in->non_witness_utxo) dogecoin_tx_free(in->non_witness_utxo);
    in->non_witness_utxo = dogecoin_tx_new();
    dogecoin_tx_copy(in->non_witness_utxo, utxo);
    return true;
}

dogecoin_bool dogecoin_psbt_input_set_redeemscript(dogecoin_psbt *psbt, size_t idx,
                                                    const uint8_t *script, size_t len)
{
    if (!psbt || idx >= psbt->num_inputs) return false;
    dogecoin_psbt_input *in = &psbt->inputs[idx];
    if (in->redeem_script) cstr_free(in->redeem_script, true);
    in->redeem_script = cstr_new_buf((const char *)script, len);
    return true;
}

/* Finalizer role: install a scriptSig the caller built.
 *
 * dogecoin_psbt_finalize_input() classifies the input and builds the scriptSig
 * for the shapes it recognises. BIP174 leaves the finalizer application
 * specific precisely because a redeem script can be anything, and there was no
 * way to complete a PSBT whose script did not classify -- the 0x07 field the
 * serializer already writes could only ever be set by the built-in finalizer.
 *
 * Setting this marks the input finalized, so dogecoin_psbt_extract() will
 * accept it. The caller is responsible for the scriptSig being correct; nothing
 * here can check it without knowing the script semantics. */
dogecoin_bool dogecoin_psbt_input_set_final_scriptsig(dogecoin_psbt *psbt, size_t idx,
                                                      const uint8_t *script, size_t len)
{
    if (!psbt || idx >= psbt->num_inputs) return false;
    if (!script && len) return false;
    dogecoin_psbt_input *in = &psbt->inputs[idx];
    if (in->final_script_sig) cstr_free(in->final_script_sig, true);
    in->final_script_sig = cstr_new_buf((const char *)script, len);
    return in->final_script_sig != NULL;
}

dogecoin_bool dogecoin_psbt_input_set_sighash(dogecoin_psbt *psbt, size_t idx,
                                               uint32_t sighash_type)
{
    if (!psbt || idx >= psbt->num_inputs) return false;
    psbt->inputs[idx].sighash_type     = sighash_type;
    psbt->inputs[idx].has_sighash_type = true;
    return true;
}

dogecoin_bool dogecoin_psbt_input_add_keypath(dogecoin_psbt *psbt, size_t idx,
                                               const uint8_t *pubkey, size_t pubkey_len,
                                               uint32_t fingerprint,
                                               const uint32_t *path, size_t path_len)
{
    if (!psbt || idx >= psbt->num_inputs || !pubkey || pubkey_len > PSBT_MAX_PUBKEY_LEN)
        return false;
    dogecoin_psbt_input *in = &psbt->inputs[idx];
    in->keypaths = dogecoin_realloc(in->keypaths,
        (in->num_keypaths + 1) * sizeof(*in->keypaths));
    dogecoin_psbt_keypath *kp = &in->keypaths[in->num_keypaths++];
    memcpy(kp->pubkey, pubkey, pubkey_len); kp->pubkey_len = pubkey_len;
    kp->fingerprint = fingerprint;
    kp->path_len = path_len;
    kp->path = path_len ? dogecoin_malloc(path_len * 4) : NULL;
    if (path_len) memcpy(kp->path, path, path_len * 4);
    return true;
}

dogecoin_bool dogecoin_psbt_output_set_redeemscript(dogecoin_psbt *psbt, size_t idx,
                                                     const uint8_t *script, size_t len)
{
    if (!psbt || idx >= psbt->num_outputs) return false;
    dogecoin_psbt_output *out = &psbt->outputs[idx];
    if (out->redeem_script) cstr_free(out->redeem_script, true);
    out->redeem_script = cstr_new_buf((const char *)script, len);
    return true;
}

dogecoin_bool dogecoin_psbt_output_add_keypath(dogecoin_psbt *psbt, size_t idx,
                                                const uint8_t *pubkey, size_t pubkey_len,
                                                uint32_t fingerprint,
                                                const uint32_t *path, size_t path_len)
{
    if (!psbt || idx >= psbt->num_outputs || !pubkey || pubkey_len > PSBT_MAX_PUBKEY_LEN)
        return false;
    dogecoin_psbt_output *out = &psbt->outputs[idx];
    out->keypaths = dogecoin_realloc(out->keypaths,
        (out->num_keypaths + 1) * sizeof(*out->keypaths));
    dogecoin_psbt_keypath *kp = &out->keypaths[out->num_keypaths++];
    memcpy(kp->pubkey, pubkey, pubkey_len); kp->pubkey_len = pubkey_len;
    kp->fingerprint = fingerprint;
    kp->path_len = path_len;
    kp->path = path_len ? dogecoin_malloc(path_len * 4) : NULL;
    if (path_len) memcpy(kp->path, path, path_len * 4);
    return true;
}

/* ── Signer ───────────────────────────────────────────────────── */

/*
 * Return the scriptCode for signing input i, performing mandatory BIP174
 * signer checks:
 *  (a) non_witness_utxo.txid must match the input's prevout hash
 *  (b) for P2SH: HASH160(redeem_script) must match the scriptPubKey hash
 *
 * Returns NULL if any check fails or no UTXO is present.
 * Note: signing without a UTXO is intentionally rejected — the "redeem
 * script only" path was an unsafe fallback that let unverified scripts be
 * signed.
 */
static cstring *psbt_get_script_for_input(const dogecoin_psbt *psbt, size_t i)
{
    const dogecoin_psbt_input *in = &psbt->inputs[i];
    if (!in->non_witness_utxo) return NULL;   /* require UTXO for signing */

    dogecoin_tx_in *txin = vector_idx(psbt->tx->vin, i);

    /* (a) txid of provided UTXO must match prevout hash */
    uint256_t utxo_txid;
    dogecoin_tx_hash(in->non_witness_utxo, utxo_txid);
    if (memcmp(utxo_txid, txin->prevout.hash, 32) != 0) return NULL;

    uint32_t vout = txin->prevout.n;
    if (!in->non_witness_utxo->vout || vout >= in->non_witness_utxo->vout->len)
        return NULL;
    dogecoin_tx_out *txout = vector_idx(in->non_witness_utxo->vout, vout);
    if (!txout->script_pubkey) return NULL;

    if (in->redeem_script) {
        /* (b) P2SH: verify HASH160(redeem_script) == scriptPubKey[2..21] */
        const cstring *spk = txout->script_pubkey;
        if (spk->len != 23 ||
            (uint8_t)spk->str[0] != 0xa9 ||
            (uint8_t)spk->str[1] != 0x14 ||
            (uint8_t)spk->str[22] != 0x87) return NULL;  /* not P2SH */
        uint8_t h160[20];
        hash160_buf((const uint8_t *)in->redeem_script->str,
                    in->redeem_script->len, h160);
        if (memcmp(h160, (const uint8_t *)spk->str + 2, 20) != 0) return NULL;
        return in->redeem_script;
    }

    return txout->script_pubkey;
}

dogecoin_bool dogecoin_psbt_sign_input(dogecoin_psbt *psbt, size_t idx,
                                        const dogecoin_key *privkey)
{
    if (!psbt || idx >= psbt->num_inputs || !privkey) return false;
    dogecoin_psbt_input *in = &psbt->inputs[idx];

    /* Already finalized — skip */
    if (in->final_script_sig) return true;

    cstring *script = psbt_get_script_for_input(psbt, idx);
    if (!script) return false;

    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(privkey, &pubkey);
    if (!dogecoin_pubkey_is_valid(&pubkey)) return false;

    int sighash = in->has_sighash_type ? (int)in->sighash_type : SIGHASH_ALL;

    /* Compute sighash */
    uint256_t sighash_bytes;
    dogecoin_mem_zero(sighash_bytes, sizeof(sighash_bytes));
    if (!dogecoin_tx_sighash(psbt->tx, script, idx, sighash, sighash_bytes))
        return false;

    /* Sign */
    uint8_t sigcompact[64];
    size_t  sigcompact_len = 64;
    dogecoin_key_sign_hash_compact(privkey, sighash_bytes, sigcompact, &sigcompact_len);

    /* Convert to DER + append sighash byte */
    unsigned char sigder[74];
    size_t sigder_len = sizeof(sigder);
    dogecoin_ecc_compact_to_der_normalized(sigcompact, sigder, &sigder_len);
    sigder[sigder_len++] = (uint8_t)sighash;

    /* Check for duplicate partial sig for this pubkey */
    size_t pklen = pubkey.compressed ? DOGECOIN_ECKEY_COMPRESSED_LENGTH
                                     : DOGECOIN_ECKEY_UNCOMPRESSED_LENGTH;
    for (size_t j = 0; j < in->num_partial_sigs; j++) {
        if (in->partial_sigs[j].pubkey_len == pklen &&
            memcmp(in->partial_sigs[j].pubkey, pubkey.pubkey, pklen) == 0) {
            /* Update existing entry */
            memcpy(in->partial_sigs[j].sig, sigder, sigder_len);
            in->partial_sigs[j].sig_len = sigder_len;
            return true;
        }
    }

    /* Add new partial sig */
    in->partial_sigs = dogecoin_realloc(in->partial_sigs,
        (in->num_partial_sigs + 1) * sizeof(*in->partial_sigs));
    dogecoin_psbt_partialsig *ps = &in->partial_sigs[in->num_partial_sigs++];
    memcpy(ps->pubkey, pubkey.pubkey, pklen);
    ps->pubkey_len = pklen;
    memcpy(ps->sig, sigder, sigder_len);
    ps->sig_len = sigder_len;
    return true;
}

dogecoin_bool dogecoin_psbt_sign(dogecoin_psbt *psbt, const dogecoin_key *privkey)
{
    if (!psbt || !privkey) return false;
    dogecoin_bool any = false;
    for (size_t i = 0; i < psbt->num_inputs; i++) {
        if (psbt_get_script_for_input(psbt, i))
            any |= dogecoin_psbt_sign_input(psbt, i, privkey);
    }
    return any;
}

/* ── Combiner ─────────────────────────────────────────────────── */

dogecoin_bool dogecoin_psbt_combine(dogecoin_psbt *dst, const dogecoin_psbt *src)
{
    if (!dst || !src) return false;
    if (dst->num_inputs != src->num_inputs) return false;
    if (dst->num_outputs != src->num_outputs) return false;

    /* Verify same unsigned tx via serialised comparison */
    cstring *da = cstr_new_sz(256), *sa = cstr_new_sz(256);
    dogecoin_tx_serialize(da, dst->tx);
    dogecoin_tx_serialize(sa, src->tx);
    dogecoin_bool same = (da->len == sa->len && memcmp(da->str, sa->str, da->len) == 0);
    cstr_free(da, true); cstr_free(sa, true);
    if (!same) return false;

    for (size_t i = 0; i < dst->num_inputs; i++) {
        dogecoin_psbt_input       *di = &dst->inputs[i];
        const dogecoin_psbt_input *si = &src->inputs[i];

        /* Merge non_witness_utxo (conflict if both set and differ) */
        if (di->non_witness_utxo && si->non_witness_utxo) {
            cstring *dn = cstr_new_sz(256), *sn = cstr_new_sz(256);
            dogecoin_tx_serialize(dn, di->non_witness_utxo);
            dogecoin_tx_serialize(sn, si->non_witness_utxo);
            dogecoin_bool eq = (dn->len == sn->len && memcmp(dn->str, sn->str, dn->len) == 0);
            cstr_free(dn, true); cstr_free(sn, true);
            if (!eq) return false;
        } else if (!di->non_witness_utxo && si->non_witness_utxo) {
            di->non_witness_utxo = dogecoin_tx_new();
            dogecoin_tx_copy(di->non_witness_utxo, si->non_witness_utxo);
        }

        /* Merge redeem script (conflict if both set and differ) */
        if (di->redeem_script && si->redeem_script) {
            if (di->redeem_script->len != si->redeem_script->len ||
                memcmp(di->redeem_script->str, si->redeem_script->str, di->redeem_script->len) != 0)
                return false;
        } else if (!di->redeem_script && si->redeem_script) {
            di->redeem_script = cstr_new_cstr(si->redeem_script);
        }

        /* Merge final_script_sig (conflict if both set and differ) */
        if (di->final_script_sig && si->final_script_sig) {
            if (di->final_script_sig->len != si->final_script_sig->len ||
                memcmp(di->final_script_sig->str, si->final_script_sig->str, di->final_script_sig->len) != 0)
                return false;
        } else if (!di->final_script_sig && si->final_script_sig) {
            di->final_script_sig = cstr_new_cstr(si->final_script_sig);
        }

        /* Merge sighash (conflict if both set and differ) */
        if (di->has_sighash_type && si->has_sighash_type) {
            if (di->sighash_type != si->sighash_type) return false;
        } else if (!di->has_sighash_type && si->has_sighash_type) {
            di->sighash_type     = si->sighash_type;
            di->has_sighash_type = true;
        }

        /* Merge partial sigs (no-duplicate by pubkey) */
        for (size_t j = 0; j < si->num_partial_sigs; j++) {
            const dogecoin_psbt_partialsig *sp = &si->partial_sigs[j];
            dogecoin_bool found = false;
            for (size_t k = 0; k < di->num_partial_sigs; k++) {
                if (di->partial_sigs[k].pubkey_len == sp->pubkey_len &&
                    memcmp(di->partial_sigs[k].pubkey, sp->pubkey, sp->pubkey_len) == 0) {
                    found = true; break;
                }
            }
            if (!found) {
                di->partial_sigs = dogecoin_realloc(di->partial_sigs,
                    (di->num_partial_sigs + 1) * sizeof(*di->partial_sigs));
                di->partial_sigs[di->num_partial_sigs++] = *sp;
            }
        }

        /* Merge keypaths */
        for (size_t j = 0; j < si->num_keypaths; j++) {
            const dogecoin_psbt_keypath *sk = &si->keypaths[j];
            dogecoin_bool found = false;
            for (size_t k = 0; k < di->num_keypaths; k++) {
                if (di->keypaths[k].pubkey_len == sk->pubkey_len &&
                    memcmp(di->keypaths[k].pubkey, sk->pubkey, sk->pubkey_len) == 0) {
                    found = true; break;
                }
            }
            if (!found) {
                di->keypaths = dogecoin_realloc(di->keypaths,
                    (di->num_keypaths + 1) * sizeof(*di->keypaths));
                dogecoin_psbt_keypath *nk = &di->keypaths[di->num_keypaths++];
                *nk = *sk;
                nk->path = sk->path_len ? dogecoin_malloc(sk->path_len * 4) : NULL;
                if (sk->path_len) memcpy(nk->path, sk->path, sk->path_len * 4);
            }
        }

        /* Merge per-input unknowns (conflict if same key, different value) */
        for (size_t j = 0; j < si->num_unknowns; j++) {
            const dogecoin_psbt_unknown *su = &si->unknowns[j];
            dogecoin_bool found = false;
            for (size_t k = 0; k < di->num_unknowns; k++) {
                if (di->unknowns[k].key_len == su->key_len &&
                    memcmp(di->unknowns[k].key, su->key, su->key_len) == 0) {
                    if (di->unknowns[k].value_len != su->value_len ||
                        memcmp(di->unknowns[k].value, su->value, su->value_len) != 0)
                        return false;
                    found = true; break;
                }
            }
            if (!found) {
                di->unknowns = dogecoin_realloc(di->unknowns,
                    (di->num_unknowns + 1) * sizeof(*di->unknowns));
                dogecoin_psbt_unknown *nu = &di->unknowns[di->num_unknowns++];
                nu->key_len = su->key_len;
                nu->key = dogecoin_malloc(su->key_len);
                memcpy(nu->key, su->key, su->key_len);
                nu->value_len = su->value_len;
                nu->value = su->value_len ? dogecoin_malloc(su->value_len) : NULL;
                if (su->value_len) memcpy(nu->value, su->value, su->value_len);
            }
        }
    }

    for (size_t i = 0; i < dst->num_outputs; i++) {
        dogecoin_psbt_output       *dout = &dst->outputs[i];
        const dogecoin_psbt_output *sout = &src->outputs[i];
        if (dout->redeem_script && sout->redeem_script) {
            if (dout->redeem_script->len != sout->redeem_script->len ||
                memcmp(dout->redeem_script->str, sout->redeem_script->str, dout->redeem_script->len) != 0)
                return false;
        } else if (!dout->redeem_script && sout->redeem_script) {
            dout->redeem_script = cstr_new_cstr(sout->redeem_script);
        }
        for (size_t j = 0; j < sout->num_keypaths; j++) {
            const dogecoin_psbt_keypath *sk = &sout->keypaths[j];
            dogecoin_bool found = false;
            for (size_t k = 0; k < dout->num_keypaths; k++) {
                if (dout->keypaths[k].pubkey_len == sk->pubkey_len &&
                    memcmp(dout->keypaths[k].pubkey, sk->pubkey, sk->pubkey_len) == 0) {
                    found = true; break;
                }
            }
            if (!found) {
                dout->keypaths = dogecoin_realloc(dout->keypaths,
                    (dout->num_keypaths + 1) * sizeof(*dout->keypaths));
                dogecoin_psbt_keypath *nk = &dout->keypaths[dout->num_keypaths++];
                *nk = *sk;
                nk->path = sk->path_len ? dogecoin_malloc(sk->path_len * 4) : NULL;
                if (sk->path_len) memcpy(nk->path, sk->path, sk->path_len * 4);
            }
        }

        /* Merge per-output unknowns (conflict if same key, different value) */
        for (size_t j = 0; j < sout->num_unknowns; j++) {
            const dogecoin_psbt_unknown *su = &sout->unknowns[j];
            dogecoin_bool found = false;
            for (size_t k = 0; k < dout->num_unknowns; k++) {
                if (dout->unknowns[k].key_len == su->key_len &&
                    memcmp(dout->unknowns[k].key, su->key, su->key_len) == 0) {
                    if (dout->unknowns[k].value_len != su->value_len ||
                        memcmp(dout->unknowns[k].value, su->value, su->value_len) != 0)
                        return false;
                    found = true; break;
                }
            }
            if (!found) {
                dout->unknowns = dogecoin_realloc(dout->unknowns,
                    (dout->num_unknowns + 1) * sizeof(*dout->unknowns));
                dogecoin_psbt_unknown *nu = &dout->unknowns[dout->num_unknowns++];
                nu->key_len = su->key_len;
                nu->key = dogecoin_malloc(su->key_len);
                memcpy(nu->key, su->key, su->key_len);
                nu->value_len = su->value_len;
                nu->value = su->value_len ? dogecoin_malloc(su->value_len) : NULL;
                if (su->value_len) memcpy(nu->value, su->value, su->value_len);
            }
        }
    }

    /* Merge global xpubs */
    for (size_t i = 0; i < src->num_xpubs; i++) {
        const dogecoin_psbt_xpub *sx = &src->xpubs[i];
        dogecoin_bool found = false;
        for (size_t k = 0; k < dst->num_xpubs; k++) {
            if (memcmp(dst->xpubs[k].xpub, sx->xpub, 78) == 0) {
                found = true; break;
            }
        }
        if (!found) {
            dst->xpubs = dogecoin_realloc(dst->xpubs,
                (dst->num_xpubs + 1) * sizeof(*dst->xpubs));
            dogecoin_psbt_xpub *nx = &dst->xpubs[dst->num_xpubs++];
            memcpy(nx->xpub, sx->xpub, 78);
            nx->fingerprint = sx->fingerprint;
            nx->path_len = sx->path_len;
            nx->path = sx->path_len ? dogecoin_malloc(sx->path_len * 4) : NULL;
            if (sx->path_len) memcpy(nx->path, sx->path, sx->path_len * 4);
        }
    }

    /* Merge global unknowns */
    for (size_t i = 0; i < src->num_unknowns; i++) {
        const dogecoin_psbt_unknown *su = &src->unknowns[i];
        dogecoin_bool found = false;
        for (size_t k = 0; k < dst->num_unknowns; k++) {
            if (dst->unknowns[k].key_len == su->key_len &&
                memcmp(dst->unknowns[k].key, su->key, su->key_len) == 0) {
                if (dst->unknowns[k].value_len != su->value_len ||
                    memcmp(dst->unknowns[k].value, su->value, su->value_len) != 0)
                    return false;
                found = true; break;
            }
        }
        if (!found) {
            dst->unknowns = dogecoin_realloc(dst->unknowns,
                (dst->num_unknowns + 1) * sizeof(*dst->unknowns));
            dogecoin_psbt_unknown *nu = &dst->unknowns[dst->num_unknowns++];
            nu->key_len = su->key_len;
            nu->key = dogecoin_malloc(su->key_len);
            memcpy(nu->key, su->key, su->key_len);
            nu->value_len = su->value_len;
            nu->value = su->value_len ? dogecoin_malloc(su->value_len) : NULL;
            if (su->value_len) memcpy(nu->value, su->value, su->value_len);
        }
    }

    return true;
}

/* ── Finalizer ────────────────────────────────────────────────── */

/*
 * Finalize one P2PKH input: build the scriptSig from the single
 * partial signature and its public key.
 * For P2SH, prepend the redeem script after the sig+pubkey push.
 */
dogecoin_bool dogecoin_psbt_finalize_input(dogecoin_psbt *psbt, size_t idx)
{
    if (!psbt || idx >= psbt->num_inputs) return false;
    dogecoin_psbt_input *in = &psbt->inputs[idx];

    if (in->final_script_sig) return true; /* already finalized */

    /* Resolve the scriptPubKey / scriptCode for this input.
     * For finalization we look directly at the stored UTXO — we need the
     * outer scriptPubKey to classify the spending path. */
    if (!in->non_witness_utxo) return false;
    dogecoin_tx_in *txin = vector_idx(psbt->tx->vin, idx);
    uint32_t vout_idx = txin->prevout.n;
    if (!in->non_witness_utxo->vout ||
        vout_idx >= in->non_witness_utxo->vout->len) return false;
    dogecoin_tx_out *txout = vector_idx(in->non_witness_utxo->vout, vout_idx);
    if (!txout->script_pubkey) return false;

    enum dogecoin_tx_out_type type =
        dogecoin_script_classify(txout->script_pubkey, NULL);

    if (type == DOGECOIN_TX_PUBKEYHASH) {
        /* Standard P2PKH: exactly one partial sig required */
        if (in->num_partial_sigs != 1) return false;
        dogecoin_psbt_partialsig *ps = &in->partial_sigs[0];
        cstring *ss = cstr_new_sz(150);
        ser_script_push(ss, ps->sig, ps->sig_len);
        ser_script_push(ss, ps->pubkey, ps->pubkey_len);
        in->final_script_sig = ss;
        psbt_input_finalized_clear(in);
        return true;
    }

    if (type == DOGECOIN_TX_SCRIPTHASH && in->redeem_script) {
        /* P2SH: classify the redeem script to determine the inner spend path */
        enum dogecoin_tx_out_type inner =
            dogecoin_script_classify(in->redeem_script, NULL);

        if (inner == DOGECOIN_TX_PUBKEYHASH) {
            if (in->num_partial_sigs != 1) return false;
            dogecoin_psbt_partialsig *ps = &in->partial_sigs[0];
            cstring *ss = cstr_new_sz(200);
            ser_script_push(ss, ps->sig, ps->sig_len);
            ser_script_push(ss, ps->pubkey, ps->pubkey_len);
            ser_script_push(ss, (const uint8_t *)in->redeem_script->str,
                            in->redeem_script->len);
            in->final_script_sig = ss;
            psbt_input_finalized_clear(in);
            return true;
        }

        if (inner == DOGECOIN_TX_MULTISIG) {
            uint8_t m, n;
            const uint8_t *pubkeys[16];
            uint8_t pubkey_lens[16];
            if (!psbt_parse_multisig(in->redeem_script, &m, &n,
                                     pubkeys, pubkey_lens)) return false;
            if (in->num_partial_sigs < (size_t)m) return false;

            /* OP_0 <sigs in pubkey order, exactly m> <redeem_script> */
            cstring *ss = cstr_new_sz(300);
            uint8_t op0 = 0x00;
            ser_bytes(ss, &op0, 1); /* OP_0 (CHECKMULTISIG bug dummy) */

            uint8_t pushed = 0;
            for (size_t ki = 0; ki < n && pushed < m; ki++) {
                for (size_t j = 0; j < in->num_partial_sigs; j++) {
                    dogecoin_psbt_partialsig *ps = &in->partial_sigs[j];
                    if (ps->pubkey_len == pubkey_lens[ki] &&
                        memcmp(ps->pubkey, pubkeys[ki], pubkey_lens[ki]) == 0) {
                        ser_script_push(ss, ps->sig, ps->sig_len);
                        pushed++;
                        break;
                    }
                }
            }
            if (pushed < m) { cstr_free(ss, true); return false; }

            ser_script_push(ss, (const uint8_t *)in->redeem_script->str,
                            in->redeem_script->len);
            in->final_script_sig = ss;
            psbt_input_finalized_clear(in);
            return true;
        }
    }

    return false;
}

dogecoin_bool dogecoin_psbt_finalize(dogecoin_psbt *psbt)
{
    if (!psbt) return false;
    dogecoin_bool all = true;
    for (size_t i = 0; i < psbt->num_inputs; i++) {
        if (!dogecoin_psbt_finalize_input(psbt, i)) all = false;
    }
    return all;
}

/* ── Extractor ────────────────────────────────────────────────── */

dogecoin_tx *dogecoin_psbt_extract(const dogecoin_psbt *psbt)
{
    if (!psbt || !psbt->tx) return NULL;

    /* All inputs must have a final scriptSig */
    for (size_t i = 0; i < psbt->num_inputs; i++) {
        if (!psbt->inputs[i].final_script_sig) return NULL;
    }

    dogecoin_tx *tx = dogecoin_tx_new();
    dogecoin_tx_copy(tx, psbt->tx);

    for (size_t i = 0; i < psbt->num_inputs; i++) {
        dogecoin_tx_in  *txin = vector_idx(tx->vin, i);
        const cstring   *fss  = psbt->inputs[i].final_script_sig;
        if (txin->script_sig) cstr_free(txin->script_sig, true);
        txin->script_sig = cstr_new_cstr(fss);
    }
    return tx;
}

/* Extractor role: the finalized transaction as broadcastable hex.
 *
 * dogecoin_psbt_extract() hands back a dogecoin_tx, and tx.h is not installed,
 * so a consumer could produce the transaction and had no way to serialize it.
 * dogecoin_tx_deserialize() is public with no inverse.
 *
 * Caller frees with dogecoin_free(). Returns NULL if any input is unfinalized. */
char *dogecoin_psbt_extract_hex(const dogecoin_psbt *psbt)
{
    dogecoin_tx *tx = dogecoin_psbt_extract(psbt);
    if (!tx) return NULL;

    cstring *ser = cstr_new_sz(1024);
    if (!ser) { dogecoin_tx_free(tx); return NULL; }
    dogecoin_tx_serialize(ser, tx);
    dogecoin_tx_free(tx);

    char *hex = (char *)dogecoin_malloc(ser->len * 2 + 1);
    if (!hex) { cstr_free(ser, true); return NULL; }
    utils_bin_to_hex((unsigned char *)ser->str, ser->len, hex);
    cstr_free(ser, true);
    return hex;
}

/* ── Accessors ────────────────────────────────────────────────── */
/*
 * The struct is not in the installed header set, so a consumer holding a
 * dogecoin_psbt could set fields and never read one back. That is survivable
 * while the library finalizes for you; it is not once the caller supplies its
 * own scriptSig, because building one means reading the partial signatures and
 * the redeem script the signers left behind.
 *
 * Buffer-and-length throughout so no internal type reaches the signature. Each
 * getter reports the length it needs when the buffer is too small, so a caller
 * can size then fetch.
 */

size_t dogecoin_psbt_num_inputs(const dogecoin_psbt *psbt)
{
    return psbt ? psbt->num_inputs : 0;
}

size_t dogecoin_psbt_num_outputs(const dogecoin_psbt *psbt)
{
    return psbt ? psbt->num_outputs : 0;
}

uint32_t dogecoin_psbt_get_version(const dogecoin_psbt *psbt)
{
    return psbt ? psbt->version : 0;
}

size_t dogecoin_psbt_input_num_partial_sigs(const dogecoin_psbt *psbt, size_t idx)
{
    if (!psbt || idx >= psbt->num_inputs) return 0;
    return psbt->inputs[idx].num_partial_sigs;
}

dogecoin_bool dogecoin_psbt_input_get_partial_sig(const dogecoin_psbt *psbt, size_t idx,
                                                  size_t n,
                                                  uint8_t *pubkey_out, size_t pubkey_cap,
                                                  size_t *pubkey_len_out,
                                                  uint8_t *sig_out, size_t sig_cap,
                                                  size_t *sig_len_out)
{
    if (!psbt || idx >= psbt->num_inputs) return false;
    const dogecoin_psbt_input *in = &psbt->inputs[idx];
    if (n >= in->num_partial_sigs) return false;
    const dogecoin_psbt_partialsig *ps = &in->partial_sigs[n];

    if (pubkey_len_out) *pubkey_len_out = ps->pubkey_len;
    if (sig_len_out)    *sig_len_out    = ps->sig_len;
    if (!pubkey_out || !sig_out) return false;                 /* size query */
    if (pubkey_cap < ps->pubkey_len || sig_cap < ps->sig_len) return false;

    memcpy(pubkey_out, ps->pubkey, ps->pubkey_len);
    memcpy(sig_out, ps->sig, ps->sig_len);
    return true;
}

/* shared by the three cstring-valued getters below */
static dogecoin_bool psbt_copy_cstring(const cstring *src, uint8_t *out, size_t cap,
                                       size_t *len_out)
{
    if (!src) { if (len_out) *len_out = 0; return false; }
    if (len_out) *len_out = src->len;
    if (!out) return false;                                    /* size query */
    if (cap < src->len) return false;
    memcpy(out, src->str, src->len);
    return true;
}

dogecoin_bool dogecoin_psbt_input_get_redeemscript(const dogecoin_psbt *psbt, size_t idx,
                                                   uint8_t *out, size_t cap, size_t *len_out)
{
    if (!psbt || idx >= psbt->num_inputs) return false;
    return psbt_copy_cstring(psbt->inputs[idx].redeem_script, out, cap, len_out);
}

dogecoin_bool dogecoin_psbt_input_get_final_scriptsig(const dogecoin_psbt *psbt, size_t idx,
                                                      uint8_t *out, size_t cap, size_t *len_out)
{
    if (!psbt || idx >= psbt->num_inputs) return false;
    return psbt_copy_cstring(psbt->inputs[idx].final_script_sig, out, cap, len_out);
}

dogecoin_bool dogecoin_psbt_output_get_redeemscript(const dogecoin_psbt *psbt, size_t idx,
                                                    uint8_t *out, size_t cap, size_t *len_out)
{
    if (!psbt || idx >= psbt->num_outputs) return false;
    return psbt_copy_cstring(psbt->outputs[idx].redeem_script, out, cap, len_out);
}

dogecoin_bool dogecoin_psbt_input_get_sighash(const dogecoin_psbt *psbt, size_t idx,
                                              uint32_t *sighash_out)
{
    if (!psbt || idx >= psbt->num_inputs) return false;
    const dogecoin_psbt_input *in = &psbt->inputs[idx];
    if (!in->has_sighash_type) return false;
    if (sighash_out) *sighash_out = in->sighash_type;
    return true;
}

/* ── Validation ───────────────────────────────────────────────── */

dogecoin_bool dogecoin_psbt_is_valid(const dogecoin_psbt *psbt)
{
    if (!psbt || !psbt->tx) return false;
    if (psbt->num_inputs  != psbt->tx->vin->len)  return false;
    if (psbt->num_outputs != psbt->tx->vout->len) return false;
    /* All tx inputs must have empty scriptSig in the unsigned tx */
    for (size_t i = 0; i < psbt->tx->vin->len; i++) {
        dogecoin_tx_in *txin = vector_idx(psbt->tx->vin, i);
        if (txin->script_sig && txin->script_sig->len > 0) return false;
    }
    return true;
}

dogecoin_bool dogecoin_psbt_is_finalized(const dogecoin_psbt *psbt)
{
    if (!dogecoin_psbt_is_valid(psbt)) return false;
    for (size_t i = 0; i < psbt->num_inputs; i++) {
        if (!psbt->inputs[i].final_script_sig) return false;
    }
    return true;
}
