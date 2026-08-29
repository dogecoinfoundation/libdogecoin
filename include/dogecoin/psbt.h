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

#ifndef __LIBDOGECOIN_PSBT_H__
#define __LIBDOGECOIN_PSBT_H__

#include <stdint.h>
#include <stddef.h>

#include <dogecoin/buffer.h>
#include <dogecoin/cstr.h>
#include <dogecoin/dogecoin.h>
#include <dogecoin/key.h>
#include <dogecoin/tx.h>

LIBDOGECOIN_BEGIN_DECL

/* BIP174 magic: "psbt\xff" */
#define PSBT_MAGIC_BYTES "\x70\x73\x62\x74\xff"
#define PSBT_MAGIC_LEN   5

/* ── Global key types ─────────────────────────────────────────── */
#define PSBT_GLOBAL_UNSIGNED_TX  0x00u
#define PSBT_GLOBAL_XPUB         0x01u
#define PSBT_GLOBAL_VERSION      0xFBu  /* BIP370 version field — preserved on round-trip; only v0 semantics implemented */

/* ── Input key types ──────────────────────────────────────────── */
#define PSBT_IN_NON_WITNESS_UTXO  0x00u
#define PSBT_IN_PARTIAL_SIG       0x02u
#define PSBT_IN_SIGHASH_TYPE      0x03u
#define PSBT_IN_REDEEM_SCRIPT     0x04u
#define PSBT_IN_BIP32_DERIVATION  0x06u
#define PSBT_IN_FINAL_SCRIPTSIG   0x07u

/* ── Output key types ─────────────────────────────────────────── */
#define PSBT_OUT_REDEEM_SCRIPT    0x00u
#define PSBT_OUT_BIP32_DERIVATION 0x02u

/* PSBT version constants — only BIP174 v0 semantics are implemented */
#define PSBT_VERSION_0  0x00000000u
#define PSBT_VERSION_2  0x00000002u  /* recognized in PSBT_GLOBAL_VERSION field; not fully implemented */

/* Max per-field sizes */
#define PSBT_MAX_PUBKEY_LEN  33u   /* compressed secp256k1 public key */
#define PSBT_MAX_SIG_LEN     74u   /* DER sig (73 max) + 1 sighash byte */

/* ── Sub-structures ───────────────────────────────────────────── */

/** One partial signature: pubkey → (DER sig + sighash byte) */
typedef struct dogecoin_psbt_partialsig {
    uint8_t pubkey[PSBT_MAX_PUBKEY_LEN];
    size_t  pubkey_len;
    uint8_t sig[PSBT_MAX_SIG_LEN];
    size_t  sig_len;
} dogecoin_psbt_partialsig;

/** BIP32 derivation path attached to a public key */
typedef struct dogecoin_psbt_keypath {
    uint8_t  pubkey[PSBT_MAX_PUBKEY_LEN];
    size_t   pubkey_len;
    uint32_t fingerprint;  /* master key fingerprint (4 bytes) */
    uint32_t *path;        /* derivation path components */
    size_t   path_len;     /* number of path components */
} dogecoin_psbt_keypath;

/** Global xpub entry (BIP174 §3.2) */
typedef struct dogecoin_psbt_xpub {
    uint8_t  xpub[78];    /* BIP32 serialized extended public key */
    uint32_t fingerprint;
    uint32_t *path;
    size_t   path_len;
} dogecoin_psbt_xpub;

/** Unknown / proprietary key-value field */
typedef struct dogecoin_psbt_unknown {
    uint8_t *key;
    size_t   key_len;
    uint8_t *value;
    size_t   value_len;
} dogecoin_psbt_unknown;

/* ── Per-input PSBT data (BIP174 §3.3) ───────────────────────── */
typedef struct dogecoin_psbt_input {
    dogecoin_tx *non_witness_utxo;       /* 0x00: full previous tx */
    cstring     *redeem_script;          /* 0x04: P2SH redeem script */
    cstring     *final_script_sig;       /* 0x07: finalized scriptSig */
    uint32_t     sighash_type;           /* 0x03: 0 = unset, use SIGHASH_ALL */
    dogecoin_bool has_sighash_type;

    dogecoin_psbt_partialsig *partial_sigs;
    size_t                    num_partial_sigs;

    dogecoin_psbt_keypath    *keypaths;
    size_t                    num_keypaths;

    dogecoin_psbt_unknown    *unknowns;
    size_t                    num_unknowns;
} dogecoin_psbt_input;

/* ── Per-output PSBT data (BIP174 §3.4) ──────────────────────── */
typedef struct dogecoin_psbt_output {
    cstring *redeem_script;              /* 0x00: P2SH redeem script */

    dogecoin_psbt_keypath *keypaths;
    size_t                 num_keypaths;

    dogecoin_psbt_unknown *unknowns;
    size_t                 num_unknowns;
} dogecoin_psbt_output;

/* ── Top-level PSBT (BIP174 §3.1) ────────────────────────────── */
typedef struct dogecoin_psbt {
    dogecoin_tx          *tx;           /* global: unsigned transaction */
    uint32_t              version;      /* global: version field (BIP370 §2.1.4); v0 semantics only */

    dogecoin_psbt_xpub   *xpubs;
    size_t                num_xpubs;

    dogecoin_psbt_input  *inputs;
    size_t                num_inputs;

    dogecoin_psbt_output *outputs;
    size_t                num_outputs;

    dogecoin_psbt_unknown *unknowns;
    size_t                 num_unknowns;
} dogecoin_psbt;

/* ── Lifecycle ────────────────────────────────────────────────── */
LIBDOGECOIN_API dogecoin_psbt *dogecoin_psbt_new(void);
LIBDOGECOIN_API void           dogecoin_psbt_free(dogecoin_psbt *psbt);

/* ── Creator role (BIP174 §7.1) ──────────────────────────────── */
/**
 * Wrap an unsigned transaction in a new PSBT.
 * All inputs must have empty scriptSigs; returns NULL otherwise.
 */
LIBDOGECOIN_API dogecoin_psbt *dogecoin_psbt_create(const dogecoin_tx *tx);

/* ── Serialization ────────────────────────────────────────────── */
LIBDOGECOIN_API cstring      *dogecoin_psbt_serialize(const dogecoin_psbt *psbt);
LIBDOGECOIN_API dogecoin_bool  dogecoin_psbt_deserialize(const uint8_t *data, size_t len, dogecoin_psbt **out);
LIBDOGECOIN_API char          *dogecoin_psbt_to_base64(const dogecoin_psbt *psbt);
LIBDOGECOIN_API dogecoin_bool  dogecoin_psbt_from_base64(const char *b64, dogecoin_psbt **out);
LIBDOGECOIN_API char          *dogecoin_psbt_to_hex(const dogecoin_psbt *psbt);
LIBDOGECOIN_API dogecoin_bool  dogecoin_psbt_from_hex(const char *hex, dogecoin_psbt **out);

/* ── Updater role (BIP174 §7.2) ──────────────────────────────── */
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_set_utxo(
    dogecoin_psbt *psbt, size_t idx, const dogecoin_tx *utxo);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_set_redeemscript(
    dogecoin_psbt *psbt, size_t idx, const uint8_t *script, size_t len);
/* finalizer role: install a scriptSig the caller built, for an input whose
   redeem script dogecoin_psbt_finalize_input() cannot classify */
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_set_final_scriptsig(
    dogecoin_psbt *psbt, size_t idx, const uint8_t *script, size_t len);

LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_set_sighash(
    dogecoin_psbt *psbt, size_t idx, uint32_t sighash_type);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_add_keypath(
    dogecoin_psbt *psbt, size_t idx,
    const uint8_t *pubkey, size_t pubkey_len,
    uint32_t fingerprint, const uint32_t *path, size_t path_len);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_output_set_redeemscript(
    dogecoin_psbt *psbt, size_t idx, const uint8_t *script, size_t len);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_output_add_keypath(
    dogecoin_psbt *psbt, size_t idx,
    const uint8_t *pubkey, size_t pubkey_len,
    uint32_t fingerprint, const uint32_t *path, size_t path_len);

/* ── Signer role (BIP174 §7.3) ───────────────────────────────── */
/**
 * Sign all inputs that can be signed with this private key.
 * Adds partial signatures; does not finalize.
 */
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_sign(
    dogecoin_psbt *psbt, const dogecoin_key *privkey);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_sign_input(
    dogecoin_psbt *psbt, size_t idx, const dogecoin_key *privkey);

/* ── Combiner role (BIP174 §7.4) ─────────────────────────────── */
/** Merge src into dst; both must have the same unsigned transaction. */
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_combine(
    dogecoin_psbt *dst, const dogecoin_psbt *src);

/* ── Finalizer role (BIP174 §7.5) ────────────────────────────── */
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_finalize(dogecoin_psbt *psbt);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_finalize_input(
    dogecoin_psbt *psbt, size_t idx);

/* ── Extractor role (BIP174 §7.6) ────────────────────────────── */
/**
 * Extract a fully signed transaction.  Returns NULL if any input
 * lacks a final_script_sig.  Caller owns the returned tx.
 */

/* ── Accessors ────────────────────────────────────────────────
   The struct is opaque to consumers, so these read back what the setters put
   in. A caller-supplied finalizer needs them: building a scriptSig means
   reading the partial signatures and the redeem script.
   Each getter with a buffer reports the length it needs via (len_out) and
   returns false when (out) is NULL or (cap) is too small, so size then fetch. */
LIBDOGECOIN_API size_t dogecoin_psbt_num_inputs(const dogecoin_psbt *psbt);
LIBDOGECOIN_API size_t dogecoin_psbt_num_outputs(const dogecoin_psbt *psbt);
LIBDOGECOIN_API uint32_t dogecoin_psbt_get_version(const dogecoin_psbt *psbt);
LIBDOGECOIN_API size_t dogecoin_psbt_input_num_partial_sigs(const dogecoin_psbt *psbt, size_t idx);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_get_partial_sig(
    const dogecoin_psbt *psbt, size_t idx, size_t n,
    uint8_t *pubkey_out, size_t pubkey_cap, size_t *pubkey_len_out,
    uint8_t *sig_out, size_t sig_cap, size_t *sig_len_out);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_get_redeemscript(
    const dogecoin_psbt *psbt, size_t idx, uint8_t *out, size_t cap, size_t *len_out);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_get_final_scriptsig(
    const dogecoin_psbt *psbt, size_t idx, uint8_t *out, size_t cap, size_t *len_out);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_output_get_redeemscript(
    const dogecoin_psbt *psbt, size_t idx, uint8_t *out, size_t cap, size_t *len_out);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_input_get_sighash(
    const dogecoin_psbt *psbt, size_t idx, uint32_t *sighash_out);

LIBDOGECOIN_API dogecoin_tx *dogecoin_psbt_extract(const dogecoin_psbt *psbt);

/* extractor role: the finalized transaction as broadcastable hex; caller frees
   with dogecoin_free(); NULL if any input lacks a final scriptSig */
LIBDOGECOIN_API char *dogecoin_psbt_extract_hex(const dogecoin_psbt *psbt);

/* ── Validation helpers ───────────────────────────────────────── */
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_is_valid(const dogecoin_psbt *psbt);
LIBDOGECOIN_API dogecoin_bool dogecoin_psbt_is_finalized(const dogecoin_psbt *psbt);

LIBDOGECOIN_END_DECL

#endif /* __LIBDOGECOIN_PSBT_H__ */
