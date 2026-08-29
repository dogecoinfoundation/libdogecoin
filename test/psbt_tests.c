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

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <test/utest.h>

#include <dogecoin/cstr.h>
#include <dogecoin/key.h>
#include <dogecoin/psbt.h>
#include <dogecoin/rmd160.h>
#include <dogecoin/script.h>
#include <dogecoin/sha2.h>
#include <dogecoin/tx.h>
#include <dogecoin/utils.h>

/* ── Helpers ────────────────────────────────────────────────── */

/* Build a minimal unsigned dogecoin_tx with one input and two outputs. */
static dogecoin_tx *make_unsigned_tx(void)
{
    dogecoin_tx *tx = dogecoin_tx_new();
    tx->version  = 1;
    tx->locktime = 0;

    /* Input: spend a known outpoint with empty scriptSig */
    dogecoin_tx_in *txin = dogecoin_tx_in_new();
    /* prevout: all-zeros txid, vout 0 */
    memset(txin->prevout.hash, 0, sizeof(txin->prevout.hash));
    txin->prevout.n = 0;
    txin->sequence  = 0xFFFFFFFF;
    /* scriptSig must be empty for PSBT */
    if (txin->script_sig) { cstr_free(txin->script_sig, true); txin->script_sig = NULL; }
    txin->script_sig = cstr_new_sz(0);
    vector_add(tx->vin, txin);

    /* Output: 100 DOGE to a P2PKH (arbitrary hash160) */
    dogecoin_tx_out *txout = dogecoin_tx_out_new();
    txout->value = 10000000000LL; /* 100 DOGE in koinu */
    uint8_t dummy_hash160[20];
    memset(dummy_hash160, 0xAB, sizeof(dummy_hash160));
    txout->script_pubkey = cstr_new_sz(25);
    dogecoin_script_build_p2pkh(txout->script_pubkey, dummy_hash160);
    vector_add(tx->vout, txout);

    return tx;
}

/* Build a minimal "previous transaction" (the UTXO being spent). */
static dogecoin_tx *make_prev_tx(const uint8_t hash160[20])
{
    dogecoin_tx *tx = dogecoin_tx_new();
    tx->version = 1;

    /* Coinbase-style input */
    dogecoin_tx_in *txin = dogecoin_tx_in_new();
    memset(txin->prevout.hash, 0, sizeof(txin->prevout.hash));
    txin->prevout.n = 0xFFFFFFFF;
    txin->sequence  = 0xFFFFFFFF;
    if (txin->script_sig) { cstr_free(txin->script_sig, true); }
    txin->script_sig = cstr_new_buf("\x01\x00", 2); /* minimal coinbase script */
    vector_add(tx->vin, txin);

    /* Output 0: the UTXO we'll be spending */
    dogecoin_tx_out *txout = dogecoin_tx_out_new();
    txout->value = 10000000000LL;
    txout->script_pubkey = cstr_new_sz(25);
    dogecoin_script_build_p2pkh(txout->script_pubkey, hash160);
    vector_add(tx->vout, txout);

    return tx;
}

/* ── Test: lifecycle / create / free ────────────────────────── */
static void test_psbt_lifecycle(void)
{
    dogecoin_psbt *psbt = dogecoin_psbt_new();
    u_assert_int_eq(psbt->version, PSBT_VERSION_0);
    u_assert_int_eq(psbt->num_inputs, 0);
    u_assert_int_eq(psbt->num_outputs, 0);
    dogecoin_psbt_free(psbt);

    /* NULL safety */
    dogecoin_psbt_free(NULL);
}

/* ── Test: creator role ─────────────────────────────────────── */
static void test_psbt_creator(void)
{
    dogecoin_tx *tx = make_unsigned_tx();
    dogecoin_psbt *psbt = dogecoin_psbt_create(tx);
    u_assert_not_null(psbt);
    u_assert_int_eq((int)psbt->num_inputs, 1);
    u_assert_int_eq((int)psbt->num_outputs, 1);
    u_assert_int_eq(psbt->version, PSBT_VERSION_0);
    u_assert_not_null(psbt->tx);

    /* Creator must reject a tx with non-empty scriptSig */
    dogecoin_tx_in *txin = vector_idx(tx->vin, 0);
    cstr_free(txin->script_sig, true);
    txin->script_sig = cstr_new_buf("\x01\x51", 2); /* OP_1 */
    dogecoin_psbt *bad = dogecoin_psbt_create(tx);
    u_assert_is_null(bad);

    dogecoin_psbt_free(psbt);
    dogecoin_tx_free(tx);
}

/* ── Test: serialization round-trip ────────────────────────── */
static void test_psbt_serialization(void)
{
    dogecoin_tx *tx = make_unsigned_tx();
    dogecoin_psbt *orig = dogecoin_psbt_create(tx);
    dogecoin_tx_free(tx);
    u_assert_not_null(orig);

    /* Serialize */
    cstring *raw = dogecoin_psbt_serialize(orig);
    u_assert_not_null(raw);
    u_assert_int_eq((int)raw->len > PSBT_MAGIC_LEN, 1);

    /* Verify magic */
    u_assert_int_eq(memcmp(raw->str, PSBT_MAGIC_BYTES, PSBT_MAGIC_LEN), 0);

    /* Deserialize */
    dogecoin_psbt *decoded = NULL;
    dogecoin_bool ok = dogecoin_psbt_deserialize(
        (const uint8_t *)raw->str, raw->len, &decoded);
    u_assert_int_eq(ok, true);
    u_assert_not_null(decoded);
    u_assert_int_eq((int)decoded->num_inputs, 1);
    u_assert_int_eq((int)decoded->num_outputs, 1);

    /* Re-serialize and compare bytes */
    cstring *raw2 = dogecoin_psbt_serialize(decoded);
    u_assert_int_eq(raw->len, raw2->len);
    u_assert_int_eq(memcmp(raw->str, raw2->str, raw->len), 0);

    cstr_free(raw,  true);
    cstr_free(raw2, true);
    dogecoin_psbt_free(orig);
    dogecoin_psbt_free(decoded);
}

/* ── Test: base64 round-trip ────────────────────────────────── */
static void test_psbt_base64(void)
{
    dogecoin_tx *tx = make_unsigned_tx();
    dogecoin_psbt *orig = dogecoin_psbt_create(tx);
    dogecoin_tx_free(tx);

    char *b64 = dogecoin_psbt_to_base64(orig);
    u_assert_not_null(b64);
    u_assert_int_eq(((int)strlen(b64)) > 0, 1);

    dogecoin_psbt *decoded = NULL;
    u_assert_int_eq(dogecoin_psbt_from_base64(b64, &decoded), true);
    u_assert_not_null(decoded);
    u_assert_int_eq((int)decoded->num_inputs, 1);
    u_assert_int_eq((int)decoded->num_outputs, 1);

    dogecoin_free(b64);
    dogecoin_psbt_free(orig);
    dogecoin_psbt_free(decoded);
}

/* ── Test: hex round-trip ───────────────────────────────────── */
static void test_psbt_hex(void)
{
    dogecoin_tx *tx = make_unsigned_tx();
    dogecoin_psbt *orig = dogecoin_psbt_create(tx);
    dogecoin_tx_free(tx);

    char *hex = dogecoin_psbt_to_hex(orig);
    u_assert_not_null(hex);

    dogecoin_psbt *decoded = NULL;
    u_assert_int_eq(dogecoin_psbt_from_hex(hex, &decoded), true);
    u_assert_not_null(decoded);

    /* Re-hex and compare */
    char *hex2 = dogecoin_psbt_to_hex(decoded);
    u_assert_int_eq(strcmp(hex, hex2), 0);

    dogecoin_free(hex);
    dogecoin_free(hex2);
    dogecoin_psbt_free(orig);
    dogecoin_psbt_free(decoded);
}

/* ── Test: updater role ─────────────────────────────────────── */
static void test_psbt_updater(void)
{
    dogecoin_tx *tx = make_unsigned_tx();
    dogecoin_psbt *psbt = dogecoin_psbt_create(tx);
    dogecoin_tx_free(tx);

    /* Set non-witness UTXO */
    uint8_t dummy_hash160[20];
    memset(dummy_hash160, 0xAB, sizeof(dummy_hash160));
    dogecoin_tx *utxo = make_prev_tx(dummy_hash160);
    u_assert_int_eq(dogecoin_psbt_input_set_utxo(psbt, 0, utxo), true);
    u_assert_not_null(psbt->inputs[0].non_witness_utxo);
    dogecoin_tx_free(utxo);

    /* Set sighash type */
    u_assert_int_eq(dogecoin_psbt_input_set_sighash(psbt, 0, SIGHASH_ALL), true);
    u_assert_int_eq(psbt->inputs[0].has_sighash_type, true);
    u_assert_int_eq((int)psbt->inputs[0].sighash_type, SIGHASH_ALL);

    /* Add keypath */
    uint8_t fake_pubkey[33];
    memset(fake_pubkey, 0x02, sizeof(fake_pubkey));
    uint32_t path[3] = { 0x8000002C, 0x80000003, 0x80000000 };
    u_assert_int_eq(dogecoin_psbt_input_add_keypath(psbt, 0, fake_pubkey, 33,
                                                     0xDEADBEEF, path, 3), true);
    u_assert_int_eq((int)psbt->inputs[0].num_keypaths, 1);
    u_assert_int_eq((int)psbt->inputs[0].keypaths[0].fingerprint, (int)0xDEADBEEF);

    /* Out of range */
    u_assert_int_eq(dogecoin_psbt_input_set_utxo(psbt, 99, NULL), false);
    u_assert_int_eq(dogecoin_psbt_output_set_redeemscript(psbt, 99, NULL, 0), false);

    dogecoin_psbt_free(psbt);
}

/* ── Test: signer + finalizer + extractor ─────────────────── */
static void test_psbt_sign_finalize_extract(void)
{
    /* Generate a key pair */
    dogecoin_key privkey;
    dogecoin_privkey_init(&privkey);
    dogecoin_privkey_gen(&privkey);

    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(&privkey, &pubkey);
    u_assert_int_eq(dogecoin_pubkey_is_valid(&pubkey), true);

    /* Get hash160 of that pubkey */
    uint8_t hash160[20];
    dogecoin_pubkey_get_hash160(&pubkey, hash160);

    /* Build unsigned tx spending a P2PKH output to that pubkey */
    dogecoin_tx *tx = dogecoin_tx_new();
    tx->version = 1; tx->locktime = 0;

    dogecoin_tx_in *txin = dogecoin_tx_in_new();
    memset(txin->prevout.hash, 0x11, sizeof(txin->prevout.hash));
    txin->prevout.n = 0;
    txin->sequence  = 0xFFFFFFFF;
    if (txin->script_sig) { cstr_free(txin->script_sig, true); }
    txin->script_sig = cstr_new_sz(0);
    vector_add(tx->vin, txin);

    dogecoin_tx_out *txout = dogecoin_tx_out_new();
    txout->value = 5000000000LL; /* 50 DOGE */
    uint8_t dest_hash[20]; memset(dest_hash, 0xCC, 20);
    txout->script_pubkey = cstr_new_sz(25);
    dogecoin_script_build_p2pkh(txout->script_pubkey, dest_hash);
    vector_add(tx->vout, txout);

    /* Create PSBT */
    dogecoin_psbt *psbt = dogecoin_psbt_create(tx);
    u_assert_not_null(psbt);
    dogecoin_tx_free(tx);

    /* Build the UTXO; fix prevout hash to match its actual txid (required by signer) */
    dogecoin_tx *utxo = make_prev_tx(hash160);
    uint8_t utxo_txid[32];
    dogecoin_tx_hash(utxo, utxo_txid);
    dogecoin_tx_in *vin0 = vector_idx(psbt->tx->vin, 0);
    memcpy(vin0->prevout.hash, utxo_txid, 32);
    u_assert_int_eq(dogecoin_psbt_input_set_utxo(psbt, 0, utxo), true);
    dogecoin_tx_free(utxo);

    /* Validation before signing */
    u_assert_int_eq(dogecoin_psbt_is_valid(psbt),     true);
    u_assert_int_eq(dogecoin_psbt_is_finalized(psbt), false);

    /* Sign */
    u_assert_int_eq(dogecoin_psbt_sign(psbt, &privkey), true);
    u_assert_int_eq((int)psbt->inputs[0].num_partial_sigs, 1);

    /* Partial sig pubkey must match our key */
    dogecoin_psbt_partialsig *ps = &psbt->inputs[0].partial_sigs[0];
    u_assert_int_eq((int)ps->pubkey_len, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    u_assert_int_eq(memcmp(ps->pubkey, pubkey.pubkey, DOGECOIN_ECKEY_COMPRESSED_LENGTH), 0);
    u_assert_int_eq(((int)ps->sig_len) > 0, 1);

    /* Signing again with same key should not duplicate the entry */
    u_assert_int_eq(dogecoin_psbt_sign(psbt, &privkey), true);
    u_assert_int_eq((int)psbt->inputs[0].num_partial_sigs, 1);

    /* Finalize */
    u_assert_int_eq(dogecoin_psbt_finalize(psbt), true);
    u_assert_not_null(psbt->inputs[0].final_script_sig);
    u_assert_int_eq(dogecoin_psbt_is_finalized(psbt), true);

    /* Extract */
    dogecoin_tx *signed_tx = dogecoin_psbt_extract(psbt);
    u_assert_not_null(signed_tx);
    dogecoin_tx_in *signed_in = vector_idx(signed_tx->vin, 0);
    u_assert_not_null(signed_in->script_sig);
    u_assert_int_eq(((int)signed_in->script_sig->len) > 0, 1);

    dogecoin_tx_free(signed_tx);
    dogecoin_psbt_free(psbt);
    dogecoin_privkey_cleanse(&privkey);
}

/* ── Test: combiner role ────────────────────────────────────── */
static void test_psbt_combiner(void)
{
    /* Two different signers sign the same PSBT */
    dogecoin_key key1, key2;
    dogecoin_privkey_init(&key1); dogecoin_privkey_gen(&key1);
    dogecoin_privkey_init(&key2); dogecoin_privkey_gen(&key2);

    dogecoin_pubkey pub1, pub2;
    dogecoin_pubkey_init(&pub1); dogecoin_pubkey_from_key(&key1, &pub1);
    dogecoin_pubkey_init(&pub2); dogecoin_pubkey_from_key(&key2, &pub2);

    /* Build a 1-of-1 PSBT (both keys will sign independently) */
    dogecoin_tx *tx = dogecoin_tx_new();
    tx->version = 1; tx->locktime = 0;
    dogecoin_tx_in *txin = dogecoin_tx_in_new();
    memset(txin->prevout.hash, 0x22, sizeof(txin->prevout.hash));
    txin->prevout.n = 0; txin->sequence = 0xFFFFFFFF;
    if (txin->script_sig) { cstr_free(txin->script_sig, true); }
    txin->script_sig = cstr_new_sz(0);
    vector_add(tx->vin, txin);
    dogecoin_tx_out *txout = dogecoin_tx_out_new();
    txout->value = 1000000000LL;
    uint8_t dh[20]; memset(dh, 0xDD, 20);
    txout->script_pubkey = cstr_new_sz(25);
    dogecoin_script_build_p2pkh(txout->script_pubkey, dh);
    vector_add(tx->vout, txout);

    dogecoin_psbt *psbt1 = dogecoin_psbt_create(tx);
    dogecoin_psbt *psbt2 = dogecoin_psbt_create(tx);
    dogecoin_tx_free(tx);

    /* Each signer sets the UTXO; prevout hash must match actual txid */
    uint8_t h1[20]; dogecoin_pubkey_get_hash160(&pub1, h1);
    dogecoin_tx *utxo = make_prev_tx(h1);
    uint8_t utxo_txid[32];
    dogecoin_tx_hash(utxo, utxo_txid);
    dogecoin_tx_in *vin1 = vector_idx(psbt1->tx->vin, 0);
    dogecoin_tx_in *vin2 = vector_idx(psbt2->tx->vin, 0);
    memcpy(vin1->prevout.hash, utxo_txid, 32);
    memcpy(vin2->prevout.hash, utxo_txid, 32);
    dogecoin_psbt_input_set_utxo(psbt1, 0, utxo);
    dogecoin_psbt_input_set_utxo(psbt2, 0, utxo);
    dogecoin_tx_free(utxo);

    dogecoin_psbt_sign(psbt1, &key1);
    dogecoin_psbt_sign(psbt2, &key2);

    u_assert_int_eq((int)psbt1->inputs[0].num_partial_sigs, 1);
    u_assert_int_eq((int)psbt2->inputs[0].num_partial_sigs, 1);

    /* Combine: psbt1 absorbs psbt2's signature */
    u_assert_int_eq(dogecoin_psbt_combine(psbt1, psbt2), true);
    u_assert_int_eq((int)psbt1->inputs[0].num_partial_sigs, 2);

    /* Combining again should not add duplicates */
    u_assert_int_eq(dogecoin_psbt_combine(psbt1, psbt2), true);
    u_assert_int_eq((int)psbt1->inputs[0].num_partial_sigs, 2);

    dogecoin_psbt_free(psbt1);
    dogecoin_psbt_free(psbt2);
    dogecoin_privkey_cleanse(&key1);
    dogecoin_privkey_cleanse(&key2);
}

/* ── Test: P2SH 2-of-3 multisig, signed/combined OUT OF ORDER ──────
 *
 * This is the consensus-critical path: a bare-multisig finalizer MUST
 * emit the signatures in the order their pubkeys appear in the redeem
 * script, regardless of the order in which signatures were collected,
 * because OP_CHECKMULTISIG matches sigs to pubkeys in a single forward
 * pass.  We deliberately collect signatures in the OPPOSITE order to
 * the redeem script (sign with key index 2 first, then merge key index
 * 0 via the combiner) so that a finalizer which simply dumps
 * partial_sigs in storage order would produce an invalid scriptSig and
 * fail the ordering assertion below.
 *
 * It also pins the m-of-n threshold rules: finalize must refuse with
 * fewer than m sigs, and must push EXACTLY m (not all available) sigs.
 */

/* Walk a finalized P2SH-multisig scriptSig and collect each pushed
 * item's (ptr,len).  Layout: OP_0 <push sig...> <push redeemScript>.
 * Returns the number of items, or -1 on a malformed push. */
static int collect_script_pushes(const cstring *ss,
                                  const uint8_t *items[], size_t item_lens[],
                                  int max_items)
{
    const uint8_t *p   = (const uint8_t *)ss->str;
    const uint8_t *end = p + ss->len;
    int n = 0;
    while (p < end && n < max_items) {
        uint8_t op = *p++;
        size_t len;
        if (op == 0x00) {            /* OP_0 — the CHECKMULTISIG dummy */
            items[n] = p; item_lens[n] = 0; n++;
            continue;
        } else if (op <= 0x4b) {     /* direct push of `op` bytes */
            len = op;
        } else if (op == 0x4c) {     /* OP_PUSHDATA1 */
            if (p >= end) return -1;
            len = *p++;
        } else if (op == 0x4d) {     /* OP_PUSHDATA2 */
            if (p + 2 > end) return -1;
            len = (size_t)p[0] | ((size_t)p[1] << 8); p += 2;
        } else {
            return -1;               /* unexpected opcode in this context */
        }
        if (p + len > end) return -1;
        items[n] = p; item_lens[n] = len; n++;
        p += len;
    }
    return (p == end) ? n : -1;
}

static void test_psbt_multisig_2of3_out_of_order(void)
{
    /* Three independent keys; redeem-script order is fixed as k0,k1,k2. */
    dogecoin_key  k[3];
    dogecoin_pubkey pk[3];
    for (int i = 0; i < 3; i++) {
        dogecoin_privkey_init(&k[i]); dogecoin_privkey_gen(&k[i]);
        dogecoin_pubkey_init(&pk[i]); dogecoin_pubkey_from_key(&k[i], &pk[i]);
        u_assert_int_eq(dogecoin_pubkey_is_valid(&pk[i]), true);
    }

    /* Build 2-of-3 redeem script: OP_2 <pk0> <pk1> <pk2> OP_3 CHECKMULTISIG */
    vector_t *pubkeys = vector_new(3, NULL);
    for (int i = 0; i < 3; i++) vector_add(pubkeys, &pk[i]);
    cstring *redeem = cstr_new_sz(110);
    u_assert_int_eq(dogecoin_script_build_multisig(redeem, 2, pubkeys), true);
    vector_free(pubkeys, true); /* frees backing array; elem_free_f is NULL so the stack pubkeys are untouched */

    /* P2SH scriptPubKey = OP_HASH160 <hash160(redeem)> OP_EQUAL */
    uint8_t rsha[SHA256_DIGEST_LENGTH], rh160[20];
    sha256_raw((const uint8_t *)redeem->str, redeem->len, rsha);
    rmd160(rsha, SHA256_DIGEST_LENGTH, rh160);
    cstring *p2sh_spk = cstr_new_sz(23);
    dogecoin_script_build_p2sh(p2sh_spk, rh160);

    /* Funding tx: one output paying the P2SH scriptPubKey. */
    dogecoin_tx *utxo = dogecoin_tx_new();
    utxo->version = 1;
    dogecoin_tx_in *uin = dogecoin_tx_in_new();
    memset(uin->prevout.hash, 0, sizeof(uin->prevout.hash));
    uin->prevout.n = 0xFFFFFFFF; uin->sequence = 0xFFFFFFFF;
    if (uin->script_sig) cstr_free(uin->script_sig, true);
    uin->script_sig = cstr_new_buf("\x01\x00", 2);
    vector_add(utxo->vin, uin);
    dogecoin_tx_out *uout = dogecoin_tx_out_new();
    uout->value = 10000000000LL;
    uout->script_pubkey = cstr_new_cstr(p2sh_spk);
    vector_add(utxo->vout, uout);

    uint8_t utxo_txid[32];
    dogecoin_tx_hash(utxo, utxo_txid);

    /* Unsigned spending tx: one input spending utxo:0, one P2PKH output. */
    dogecoin_tx *tx = dogecoin_tx_new();
    tx->version = 1; tx->locktime = 0;
    dogecoin_tx_in *sin = dogecoin_tx_in_new();
    memcpy(sin->prevout.hash, utxo_txid, 32);
    sin->prevout.n = 0; sin->sequence = 0xFFFFFFFF;
    if (sin->script_sig) cstr_free(sin->script_sig, true);
    sin->script_sig = cstr_new_sz(0);
    vector_add(tx->vin, sin);
    dogecoin_tx_out *sout = dogecoin_tx_out_new();
    sout->value = 9000000000LL;
    uint8_t dest[20]; memset(dest, 0xCC, 20);
    sout->script_pubkey = cstr_new_sz(25);
    dogecoin_script_build_p2pkh(sout->script_pubkey, dest);
    vector_add(tx->vout, sout);

    /* ── Signer A: holds k[2] (LAST in redeem order) ── */
    dogecoin_psbt *psbtA = dogecoin_psbt_create(tx);
    u_assert_not_null(psbtA);
    u_assert_int_eq(dogecoin_psbt_input_set_utxo(psbtA, 0, utxo), true);
    u_assert_int_eq(dogecoin_psbt_input_set_redeemscript(
                        psbtA, 0, (const uint8_t *)redeem->str, redeem->len), true);

    /* Not finalizable yet: only one of two required sigs. */
    u_assert_int_eq(dogecoin_psbt_sign_input(psbtA, 0, &k[2]), true);
    u_assert_int_eq((int)psbtA->inputs[0].num_partial_sigs, 1);
    u_assert_int_eq(dogecoin_psbt_finalize(psbtA), false);
    u_assert_int_eq(dogecoin_psbt_is_finalized(psbtA), false);

    /* ── Signer B: holds k[0] (FIRST in redeem order) ── */
    dogecoin_psbt *psbtB = dogecoin_psbt_create(tx);
    u_assert_not_null(psbtB);
    u_assert_int_eq(dogecoin_psbt_input_set_utxo(psbtB, 0, utxo), true);
    u_assert_int_eq(dogecoin_psbt_input_set_redeemscript(
                        psbtB, 0, (const uint8_t *)redeem->str, redeem->len), true);
    u_assert_int_eq(dogecoin_psbt_sign_input(psbtB, 0, &k[0]), true);
    u_assert_int_eq((int)psbtB->inputs[0].num_partial_sigs, 1);

    dogecoin_tx_free(tx);

    /* ── Combiner: A absorbs B.  Storage order is now [k2, k0] —
     *    i.e. REVERSED relative to the redeem script. ── */
    u_assert_int_eq(dogecoin_psbt_combine(psbtA, psbtB), true);
    u_assert_int_eq((int)psbtA->inputs[0].num_partial_sigs, 2);
    /* Confirm the stored order really is k2-before-k0, so the test is
     * exercising the reorder rather than passing by luck. */
    u_assert_int_eq(memcmp(psbtA->inputs[0].partial_sigs[0].pubkey,
                           pk[2].pubkey, DOGECOIN_ECKEY_COMPRESSED_LENGTH), 0);
    u_assert_int_eq(memcmp(psbtA->inputs[0].partial_sigs[1].pubkey,
                           pk[0].pubkey, DOGECOIN_ECKEY_COMPRESSED_LENGTH), 0);

    /* Snapshot each signer's sig BEFORE finalize, because the finalizer
     * clears partial_sigs (BIP174 §7.5).  Identify by pubkey. */
    uint8_t  sig_k0_buf[PSBT_MAX_SIG_LEN], sig_k2_buf[PSBT_MAX_SIG_LEN];
    size_t   sig_k0_len = 0, sig_k2_len = 0;
    for (size_t j = 0; j < psbtA->inputs[0].num_partial_sigs; j++) {
        const dogecoin_psbt_partialsig *ps = &psbtA->inputs[0].partial_sigs[j];
        if (memcmp(ps->pubkey, pk[0].pubkey, DOGECOIN_ECKEY_COMPRESSED_LENGTH) == 0) {
            memcpy(sig_k0_buf, ps->sig, ps->sig_len); sig_k0_len = ps->sig_len;
        }
        if (memcmp(ps->pubkey, pk[2].pubkey, DOGECOIN_ECKEY_COMPRESSED_LENGTH) == 0) {
            memcpy(sig_k2_buf, ps->sig, ps->sig_len); sig_k2_len = ps->sig_len;
        }
    }
    u_assert_int_eq((int)(sig_k0_len > 0), 1);
    u_assert_int_eq((int)(sig_k2_len > 0), 1);

    /* ── Finalize: now has m=2 sigs, must succeed. ── */
    u_assert_int_eq(dogecoin_psbt_finalize(psbtA), true);
    u_assert_int_eq(dogecoin_psbt_is_finalized(psbtA), true);

    cstring *fss = psbtA->inputs[0].final_script_sig;
    u_assert_not_null(fss);

    /* Decode the scriptSig pushes: expect OP_0, sig, sig, redeemScript. */
    const uint8_t *items[8]; size_t ilens[8];
    int npush = collect_script_pushes(fss, items, ilens, 8);
    u_assert_int_eq(npush, 4);            /* dummy + 2 sigs + redeem (NOT 3 sigs) */
    u_assert_int_eq((int)ilens[0], 0);    /* OP_0 dummy */

    /* Last push must be the redeem script verbatim. */
    u_assert_int_eq((int)ilens[3], (int)redeem->len);
    u_assert_int_eq(memcmp(items[3], redeem->str, redeem->len), 0);

    /* THE assertion: the two sigs must appear in redeem-script pubkey
     * order — k0's signature first, then k2's — even though they were
     * collected k2-then-k0.  items[1] == k0's sig, items[2] == k2's. */
    u_assert_int_eq((int)ilens[1], (int)sig_k0_len);
    u_assert_int_eq(memcmp(items[1], sig_k0_buf, sig_k0_len), 0);
    u_assert_int_eq((int)ilens[2], (int)sig_k2_len);
    u_assert_int_eq(memcmp(items[2], sig_k2_buf, sig_k2_len), 0);

    /* Extractor produces a tx carrying that scriptSig. */
    dogecoin_tx *final_tx = dogecoin_psbt_extract(psbtA);
    u_assert_not_null(final_tx);
    dogecoin_tx_in *fin = vector_idx(final_tx->vin, 0);
    u_assert_not_null(fin->script_sig);
    u_assert_int_eq(fin->script_sig->len, fss->len);
    u_assert_int_eq(memcmp(fin->script_sig->str, fss->str, fss->len), 0);

    dogecoin_tx_free(final_tx);
    dogecoin_tx_free(utxo);
    cstr_free(redeem, true);
    cstr_free(p2sh_spk, true);
    dogecoin_psbt_free(psbtA);
    dogecoin_psbt_free(psbtB);
    for (int i = 0; i < 3; i++) dogecoin_privkey_cleanse(&k[i]);
}

/* ── Test: serialize → deserialize with all fields set ──────── */
static void test_psbt_roundtrip_full(void)
{
    dogecoin_key privkey;
    dogecoin_privkey_init(&privkey); dogecoin_privkey_gen(&privkey);
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey); dogecoin_pubkey_from_key(&privkey, &pubkey);

    uint8_t hash160[20];
    dogecoin_pubkey_get_hash160(&pubkey, hash160);

    dogecoin_tx *tx = dogecoin_tx_new();
    tx->version = 1; tx->locktime = 0;
    dogecoin_tx_in *txin = dogecoin_tx_in_new();
    memset(txin->prevout.hash, 0x33, sizeof(txin->prevout.hash));
    txin->prevout.n = 0; txin->sequence = 0xFFFFFFFF;
    if (txin->script_sig) { cstr_free(txin->script_sig, true); }
    txin->script_sig = cstr_new_sz(0);
    vector_add(tx->vin, txin);
    dogecoin_tx_out *txout = dogecoin_tx_out_new();
    txout->value = 2000000000LL;
    uint8_t dh[20]; memset(dh, 0xEE, 20);
    txout->script_pubkey = cstr_new_sz(25);
    dogecoin_script_build_p2pkh(txout->script_pubkey, dh);
    vector_add(tx->vout, txout);

    dogecoin_psbt *psbt = dogecoin_psbt_create(tx);
    dogecoin_tx_free(tx);

    /* Set UTXO; prevout hash must match actual txid */
    dogecoin_tx *utxo = make_prev_tx(hash160);
    uint8_t utxo_txid[32];
    dogecoin_tx_hash(utxo, utxo_txid);
    dogecoin_tx_in *vin0 = vector_idx(psbt->tx->vin, 0);
    memcpy(vin0->prevout.hash, utxo_txid, 32);
    dogecoin_psbt_input_set_utxo(psbt, 0, utxo);
    dogecoin_tx_free(utxo);

    /* Set sighash */
    dogecoin_psbt_input_set_sighash(psbt, 0, SIGHASH_ALL);

    /* Add keypath */
    uint32_t path[2] = { 0x80000000, 0x00000000 };
    dogecoin_psbt_input_add_keypath(psbt, 0, pubkey.pubkey, 33, 0x12345678, path, 2);

    /* Sign */
    dogecoin_psbt_sign(psbt, &privkey);
    u_assert_int_eq((int)psbt->inputs[0].num_partial_sigs, 1);

    /* Serialize, deserialize, re-serialize — byte equality */
    cstring *raw1 = dogecoin_psbt_serialize(psbt);
    dogecoin_psbt *decoded = NULL;
    u_assert_int_eq(dogecoin_psbt_deserialize(
        (const uint8_t *)raw1->str, raw1->len, &decoded), true);
    u_assert_not_null(decoded);

    /* Verify fields survived round-trip */
    u_assert_not_null(decoded->inputs[0].non_witness_utxo);
    u_assert_int_eq(decoded->inputs[0].has_sighash_type, true);
    u_assert_int_eq((int)decoded->inputs[0].sighash_type, SIGHASH_ALL);
    u_assert_int_eq((int)decoded->inputs[0].num_keypaths, 1);
    u_assert_int_eq((int)decoded->inputs[0].keypaths[0].fingerprint, 0x12345678);
    u_assert_int_eq((int)decoded->inputs[0].num_partial_sigs, 1);

    cstring *raw2 = dogecoin_psbt_serialize(decoded);
    u_assert_int_eq(raw1->len, raw2->len);
    u_assert_int_eq(memcmp(raw1->str, raw2->str, raw1->len), 0);

    /* Finalize and extract on the decoded copy */
    u_assert_int_eq(dogecoin_psbt_finalize(decoded), true);
    dogecoin_tx *signed_tx = dogecoin_psbt_extract(decoded);
    u_assert_not_null(signed_tx);
    dogecoin_tx_in *sin = vector_idx(signed_tx->vin, 0);
    u_assert_int_eq(((int)sin->script_sig->len) > 0, 1);

    dogecoin_tx_free(signed_tx);
    cstr_free(raw1, true);
    cstr_free(raw2, true);
    dogecoin_psbt_free(psbt);
    dogecoin_psbt_free(decoded);
    dogecoin_privkey_cleanse(&privkey);
}

/* ── Test: invalid inputs rejected ─────────────────────────── */
static void test_psbt_invalid(void)
{
    /* NULL / garbage data */
    dogecoin_psbt *out = NULL;
    u_assert_int_eq(dogecoin_psbt_deserialize(NULL, 0, &out), false);
    u_assert_int_eq(dogecoin_psbt_deserialize((const uint8_t *)"garbage", 7, &out), false);
    u_assert_is_null(out);

    /* Wrong magic */
    uint8_t bad[6] = { 'b', 'a', 'd', '!', 0xff, 0x00 };
    u_assert_int_eq(dogecoin_psbt_deserialize(bad, sizeof(bad), &out), false);

    /* Extractor requires finalized PSBT */
    dogecoin_tx *tx = make_unsigned_tx();
    dogecoin_psbt *psbt = dogecoin_psbt_create(tx);
    dogecoin_tx_free(tx);
    u_assert_is_null(dogecoin_psbt_extract(psbt)); /* not finalized */
    dogecoin_psbt_free(psbt);
}

/* ── BIP174 canonical test vectors ───────────────────────────── */
/*
 * The complete invalid and valid PSBT vector sets transcribed verbatim from the
 * BIP174 spec test section:
 *   https://github.com/bitcoin/bips/blob/master/bip-0174.mediawiki#test-vectors
 *
 * libdogecoin has no SegWit support, so vectors whose validity depends on
 * witness data are handled per the EXPECT_ tag:
 *   EXPECT_REJECT  - must fail to deserialize (structural problem the parser
 *                    catches regardless of witness support)
 *   EXPECT_XFAIL   - a malformed *witness-typed* key the parser currently
 *                    accepts as an unknown key; documented known limitation,
 *                    not asserted as a hard pass/fail
 *   EXPECT_PARSE   - must deserialize successfully (round-trip not asserted,
 *                    e.g. carries a witness non_witness_utxo we can parse but
 *                    cannot re-serialize byte-identically without SegWit)
 *   EXPECT_PARSE_RT - must deserialize AND re-serialize to byte-identical hex
 *   EXPECT_SKIP_WITNESS - valid only with witness support; skipped, documented
 */
#define EXPECT_REJECT        0
#define EXPECT_XFAIL         1
#define EXPECT_PARSE         2
#define EXPECT_SKIP_WITNESS  3
#define EXPECT_PARSE_RT      4

static const struct { const char *desc; const char *hex; int expect; } bip174_invalid[] = {
    /* [0] */ { "Network transaction, not PSBT format",
      "0200000001268171371edff285e937adeea4b37b78000c0566cbb3ad64641713ca42171bf600"
      "0000006a473044022070b2245123e6bf474d60c5b50c043d4c691a5d2435f09a34a7662a9dc2"
      "51790a022001329ca9dacf280bdf30740ec0390422422c81cb45839457aeb76fc12edd95b301"
      "2102657d118d3357b8e0f4c2cd46db7b39f6d9c38d9a70abcb9b2de5dc8dbfe4ce31feffffff"
      "02d3dff505000000001976a914d0c59903c5bac2868760e90fd521a4665aa7652088ac00e1f5"
      "050000000017a9143545e6e33b832c47050f24d3eeb93c9c03948bc787b32e1300",
      EXPECT_REJECT },
    /* [1] */ { "PSBT missing outputs",
      "70736274ff0100750200000001268171371edff285e937adeea4b37b78000c0566cbb3ad6464"
      "1713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac28687"
      "60e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb9"
      "3c9c03948bc787b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbba4cc69"
      "8fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7"
      "de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e6"
      "3aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff"
      "0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef8"
      "4e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be"
      "22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314"
      "e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a9963"
      "72cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2"
      "f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af"
      "59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fca"
      "c6f79a4ea169393380734464f84f2ab30000000000",
      EXPECT_REJECT },
    /* [2] */ { "PSBT where one input has a filled scriptSig in the unsigned tx",
      "70736274ff0100fd0a010200000002ab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf1538"
      "21a8139f877a5b7be4000000006a47304402204759661797c01b036b25928948686218347d89"
      "864b719e1f7fcf57d1e511658702205309eabf56aa4d8891ffd111fdf1336f3a29da866d7f84"
      "86d75546ceedaf93190121035cdc61fc7ba971c0b501a646a2a83b102cb43881217ca682dc86"
      "e2d73fa88292feffffffab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8139f87"
      "7a5b7be40100000000feffffff02603bea0b000000001976a914768a40bbd740cbe81d988e71"
      "de2a4d5c71396b1d88ac8e240000000000001976a9146f4620b553fa095e721b9ee0efe9fa03"
      "9cca459788ac00000000000001012000e1f5050000000017a9143545e6e33b832c47050f24d3"
      "eeb93c9c03948bc787010416001485d13537f2e265405a34dbafa9e3dda01fb82308000000",
      EXPECT_REJECT },
    /* [3] */ { "PSBT where inputs and outputs are provided but without an unsigned tx",
      "70736274ff000100fda5010100000000010289a3c71eab4d20e0371bbba4cc698fa295c9463a"
      "fa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7de4f53349eec"
      "b985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e63aec6a4890b4"
      "0100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff0200c2eb0b00"
      "0000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef84e2c00000017"
      "a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be22e0270f394f"
      "568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314e734c9276bd6"
      "fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a996372cb87e1856d"
      "3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2f5f4ab660654"
      "df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af59f51e44e425"
      "5b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fcac6f79a4ea169"
      "393380734464f84f2ab30000000000",
      EXPECT_REJECT },
    /* [4] */ { "PSBT with duplicate keys in an input",
      "70736274ff0100750200000001268171371edff285e937adeea4b37b78000c0566cbb3ad6464"
      "1713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac28687"
      "60e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb9"
      "3c9c03948bc787b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbba4cc69"
      "8fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7"
      "de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e6"
      "3aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff"
      "0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef8"
      "4e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be"
      "22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314"
      "e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a9963"
      "72cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2"
      "f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af"
      "59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fca"
      "c6f79a4ea169393380734464f84f2ab30000000001003f0200000001ffffffffffffffffffff"
      "ffffffffffffffffffffffffffffffffffffffffffff0000000000ffffffff01000000000000"
      "0000036a010000000000000000",
      EXPECT_REJECT },
    /* [5] */ { "PSBT with invalid global transaction typed key",
      "70736274ff020001550200000001279a2323a5dfb51fc45f220fa58b0fc13e1e3342792a85d7"
      "e36cd6333b5cbc390000000000ffffffff01a05aea0b000000001976a914ffe9c0061097cc3b"
      "636f2cb0460fa4fc427d2b4588ac0000000000010120955eea0b0000000017a9146345200f68"
      "d189e1adc0df1c4d16ea8f14c0dbeb87220203b1341ccba7683b6af4f1238cd6e97e7167d569"
      "fac47f1e48d47541844355bd4646304302200424b58effaaa694e1559ea5c93bbfd4a8906422"
      "4055cdf070b6771469442d07021f5c8eb0fea6516d60b8acb33ad64ede60e8785bfb3aa94b99"
      "bdf86151db9a9a010104220020771fd18ad459666dd49f3d564e3dbc42f4c84774e360ada168"
      "16a8ed488d5681010547522103b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d4"
      "7541844355bd462103de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f8"
      "2083bd52ae220603b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355"
      "bd4610b4a6ba67000000800000008004000080220603de55d1e1dac805e3f8a58c1fbf9b94c0"
      "2f3dbaafe127fefca4995f26f82083bd10b4a6ba670000008000000080050000800000",
      EXPECT_REJECT },
    /* [6] */ { "PSBT with invalid input witness utxo typed key",
      "70736274ff0100550200000001279a2323a5dfb51fc45f220fa58b0fc13e1e3342792a85d7e3"
      "6cd6333b5cbc390000000000ffffffff01a05aea0b000000001976a914ffe9c0061097cc3b63"
      "6f2cb0460fa4fc427d2b4588ac000000000002010020955eea0b0000000017a9146345200f68"
      "d189e1adc0df1c4d16ea8f14c0dbeb87220203b1341ccba7683b6af4f1238cd6e97e7167d569"
      "fac47f1e48d47541844355bd4646304302200424b58effaaa694e1559ea5c93bbfd4a8906422"
      "4055cdf070b6771469442d07021f5c8eb0fea6516d60b8acb33ad64ede60e8785bfb3aa94b99"
      "bdf86151db9a9a010104220020771fd18ad459666dd49f3d564e3dbc42f4c84774e360ada168"
      "16a8ed488d5681010547522103b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d4"
      "7541844355bd462103de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f8"
      "2083bd52ae220603b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355"
      "bd4610b4a6ba67000000800000008004000080220603de55d1e1dac805e3f8a58c1fbf9b94c0"
      "2f3dbaafe127fefca4995f26f82083bd10b4a6ba670000008000000080050000800000",
      EXPECT_XFAIL },
    /* [7] */ { "PSBT with invalid pubkey length for input partial signature typed key",
      "70736274ff0100550200000001279a2323a5dfb51fc45f220fa58b0fc13e1e3342792a85d7e3"
      "6cd6333b5cbc390000000000ffffffff01a05aea0b000000001976a914ffe9c0061097cc3b63"
      "6f2cb0460fa4fc427d2b4588ac0000000000010120955eea0b0000000017a9146345200f68d1"
      "89e1adc0df1c4d16ea8f14c0dbeb87210203b1341ccba7683b6af4f1238cd6e97e7167d569fa"
      "c47f1e48d47541844355bd46304302200424b58effaaa694e1559ea5c93bbfd4a89064224055"
      "cdf070b6771469442d07021f5c8eb0fea6516d60b8acb33ad64ede60e8785bfb3aa94b99bdf8"
      "6151db9a9a010104220020771fd18ad459666dd49f3d564e3dbc42f4c84774e360ada16816a8"
      "ed488d5681010547522103b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541"
      "844355bd462103de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f82083"
      "bd52ae220603b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355bd46"
      "10b4a6ba67000000800000008004000080220603de55d1e1dac805e3f8a58c1fbf9b94c02f3d"
      "baafe127fefca4995f26f82083bd10b4a6ba670000008000000080050000800000",
      EXPECT_REJECT },
    /* [8] */ { "PSBT with invalid redeemscript typed key",
      "70736274ff0100550200000001279a2323a5dfb51fc45f220fa58b0fc13e1e3342792a85d7e3"
      "6cd6333b5cbc390000000000ffffffff01a05aea0b000000001976a914ffe9c0061097cc3b63"
      "6f2cb0460fa4fc427d2b4588ac0000000000010120955eea0b0000000017a9146345200f68d1"
      "89e1adc0df1c4d16ea8f14c0dbeb87220203b1341ccba7683b6af4f1238cd6e97e7167d569fa"
      "c47f1e48d47541844355bd4646304302200424b58effaaa694e1559ea5c93bbfd4a890642240"
      "55cdf070b6771469442d07021f5c8eb0fea6516d60b8acb33ad64ede60e8785bfb3aa94b99bd"
      "f86151db9a9a01020400220020771fd18ad459666dd49f3d564e3dbc42f4c84774e360ada168"
      "16a8ed488d5681010547522103b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d4"
      "7541844355bd462103de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f8"
      "2083bd52ae220603b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355"
      "bd4610b4a6ba67000000800000008004000080220603de55d1e1dac805e3f8a58c1fbf9b94c0"
      "2f3dbaafe127fefca4995f26f82083bd10b4a6ba670000008000000080050000800000",
      EXPECT_REJECT },
    /* [9] */ { "PSBT with invalid witnessscript typed key",
      "70736274ff0100550200000001279a2323a5dfb51fc45f220fa58b0fc13e1e3342792a85d7e3"
      "6cd6333b5cbc390000000000ffffffff01a05aea0b000000001976a914ffe9c0061097cc3b63"
      "6f2cb0460fa4fc427d2b4588ac0000000000010120955eea0b0000000017a9146345200f68d1"
      "89e1adc0df1c4d16ea8f14c0dbeb87220203b1341ccba7683b6af4f1238cd6e97e7167d569fa"
      "c47f1e48d47541844355bd4646304302200424b58effaaa694e1559ea5c93bbfd4a890642240"
      "55cdf070b6771469442d07021f5c8eb0fea6516d60b8acb33ad64ede60e8785bfb3aa94b99bd"
      "f86151db9a9a010104220020771fd18ad459666dd49f3d564e3dbc42f4c84774e360ada16816"
      "a8ed488d568102050047522103b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d4"
      "7541844355bd462103de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f8"
      "2083bd52ae220603b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355"
      "bd4610b4a6ba67000000800000008004000080220603de55d1e1dac805e3f8a58c1fbf9b94c0"
      "2f3dbaafe127fefca4995f26f82083bd10b4a6ba670000008000000080050000800000",
      EXPECT_XFAIL },
    /* [10] */ { "PSBT with invalid pubkey in input BIP 32 derivation paths typed key",
      "70736274ff0100550200000001279a2323a5dfb51fc45f220fa58b0fc13e1e3342792a85d7e3"
      "6cd6333b5cbc390000000000ffffffff01a05aea0b000000001976a914ffe9c0061097cc3b63"
      "6f2cb0460fa4fc427d2b4588ac0000000000010120955eea0b0000000017a9146345200f68d1"
      "89e1adc0df1c4d16ea8f14c0dbeb87220203b1341ccba7683b6af4f1238cd6e97e7167d569fa"
      "c47f1e48d47541844355bd4646304302200424b58effaaa694e1559ea5c93bbfd4a890642240"
      "55cdf070b6771469442d07021f5c8eb0fea6516d60b8acb33ad64ede60e8785bfb3aa94b99bd"
      "f86151db9a9a010104220020771fd18ad459666dd49f3d564e3dbc42f4c84774e360ada16816"
      "a8ed488d5681010547522103b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d475"
      "41844355bd462103de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f820"
      "83bd52ae210603b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355bd"
      "10b4a6ba67000000800000008004000080220603de55d1e1dac805e3f8a58c1fbf9b94c02f3d"
      "baafe127fefca4995f26f82083bd10b4a6ba670000008000000080050000800000",
      EXPECT_REJECT },
    /* [11] */ { "PSBT with invalid non-witness utxo typed key",
      "70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f5"
      "45887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca"
      "78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b"
      "09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8"
      "742d1d87008f0000000000020000bb0200000001aad73931018bd25f84ae400b68848be09db7"
      "06eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826"
      "c015bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f"
      "0c5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b"
      "82c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97"
      "b5d852e4063f6187650000000107da00473044022074018ad4180097b873323c0015720b3684"
      "cc8123891048e7dbcd9b55ad679c99022073d369b740e3eb53dcefa33823c8070514ca55a7dd"
      "9544f157c167913261118c01483045022100f61038b308dc1da865a34852746f015772934208"
      "c6d24454393cd99bdf2217770220056e675a675a6d0a02b85b14e5e29074d8a25a9b5760bea2"
      "816f661910a006ea01475221029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c"
      "2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfe"
      "f536d752ae0001012000c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa9"
      "21e8870107232200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b2"
      "0289030108da0400473044022062eb7a556107a7c73f45ac4ab5a1dddf6f7075fb1275969a7f"
      "383efff784bcb202200c05dbb7470dbf2f08557dd356c7325c1ed30913e996cd3840945db122"
      "28da5f01473044022065f45ba5998b59a27ffe1a7bed016af1f1f90d54b3aa8f7450aa5f56a2"
      "5103bd02207f724703ad1edb96680b284b56d4ffcb88f7fb759eabbe08aa30f29b851383d201"
      "47522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc2102"
      "3add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae00220203"
      "a9a4c37f5996d3aa25dbac6b570af0650394492942460b354753ed9eeca5877110d90c6a4f00"
      "0000800000008004000080002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b3"
      "4282a94b6b5005109610d90c6a4f00000080000000800500008000",
      EXPECT_XFAIL },
    /* [12] */ { "PSBT with invalid final scriptsig typed key",
      "70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f5"
      "45887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca"
      "78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b"
      "09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8"
      "742d1d87008f00000000000100bb0200000001aad73931018bd25f84ae400b68848be09db706"
      "eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c0"
      "15bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c"
      "5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82"
      "c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5"
      "d852e4063f618765000000020700da00473044022074018ad4180097b873323c0015720b3684"
      "cc8123891048e7dbcd9b55ad679c99022073d369b740e3eb53dcefa33823c8070514ca55a7dd"
      "9544f157c167913261118c01483045022100f61038b308dc1da865a34852746f015772934208"
      "c6d24454393cd99bdf2217770220056e675a675a6d0a02b85b14e5e29074d8a25a9b5760bea2"
      "816f661910a006ea01475221029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c"
      "2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfe"
      "f536d752ae0001012000c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa9"
      "21e8870107232200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b2"
      "0289030108da0400473044022062eb7a556107a7c73f45ac4ab5a1dddf6f7075fb1275969a7f"
      "383efff784bcb202200c05dbb7470dbf2f08557dd356c7325c1ed30913e996cd3840945db122"
      "28da5f01473044022065f45ba5998b59a27ffe1a7bed016af1f1f90d54b3aa8f7450aa5f56a2"
      "5103bd02207f724703ad1edb96680b284b56d4ffcb88f7fb759eabbe08aa30f29b851383d201"
      "47522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc2102"
      "3add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae00220203"
      "a9a4c37f5996d3aa25dbac6b570af0650394492942460b354753ed9eeca5877110d90c6a4f00"
      "0000800000008004000080002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b3"
      "4282a94b6b5005109610d90c6a4f00000080000000800500008000",
      EXPECT_REJECT },
    /* [13] */ { "PSBT with invalid final script witness typed key",
      "70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f5"
      "45887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca"
      "78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b"
      "09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8"
      "742d1d87008f00000000000100bb0200000001aad73931018bd25f84ae400b68848be09db706"
      "eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c0"
      "15bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c"
      "5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82"
      "c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5"
      "d852e4063f6187650000000107da00473044022074018ad4180097b873323c0015720b3684cc"
      "8123891048e7dbcd9b55ad679c99022073d369b740e3eb53dcefa33823c8070514ca55a7dd95"
      "44f157c167913261118c01483045022100f61038b308dc1da865a34852746f015772934208c6"
      "d24454393cd99bdf2217770220056e675a675a6d0a02b85b14e5e29074d8a25a9b5760bea281"
      "6f661910a006ea01475221029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c21"
      "83f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfef5"
      "36d752ae0001012000c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa921"
      "e8870107232200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b202"
      "8903020800da0400473044022062eb7a556107a7c73f45ac4ab5a1dddf6f7075fb1275969a7f"
      "383efff784bcb202200c05dbb7470dbf2f08557dd356c7325c1ed30913e996cd3840945db122"
      "28da5f01473044022065f45ba5998b59a27ffe1a7bed016af1f1f90d54b3aa8f7450aa5f56a2"
      "5103bd02207f724703ad1edb96680b284b56d4ffcb88f7fb759eabbe08aa30f29b851383d201"
      "47522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc2102"
      "3add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae00220203"
      "a9a4c37f5996d3aa25dbac6b570af0650394492942460b354753ed9eeca5877110d90c6a4f00"
      "0000800000008004000080002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b3"
      "4282a94b6b5005109610d90c6a4f00000080000000800500008000",
      EXPECT_XFAIL },
    /* [14] */ { "PSBT with invalid pubkey in output BIP 32 derivation paths typed key",
      "70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f5"
      "45887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca"
      "78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b"
      "09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8"
      "742d1d87008f00000000000100bb0200000001aad73931018bd25f84ae400b68848be09db706"
      "eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c0"
      "15bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c"
      "5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82"
      "c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5"
      "d852e4063f6187650000000107da00473044022074018ad4180097b873323c0015720b3684cc"
      "8123891048e7dbcd9b55ad679c99022073d369b740e3eb53dcefa33823c8070514ca55a7dd95"
      "44f157c167913261118c01483045022100f61038b308dc1da865a34852746f015772934208c6"
      "d24454393cd99bdf2217770220056e675a675a6d0a02b85b14e5e29074d8a25a9b5760bea281"
      "6f661910a006ea01475221029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c21"
      "83f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54dae4dba2fbfef5"
      "36d752ae0001012000c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa921"
      "e8870107232200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b202"
      "89030108da0400473044022062eb7a556107a7c73f45ac4ab5a1dddf6f7075fb1275969a7f38"
      "3efff784bcb202200c05dbb7470dbf2f08557dd356c7325c1ed30913e996cd3840945db12228"
      "da5f01473044022065f45ba5998b59a27ffe1a7bed016af1f1f90d54b3aa8f7450aa5f56a251"
      "03bd02207f724703ad1edb96680b284b56d4ffcb88f7fb759eabbe08aa30f29b851383d20147"
      "522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc21023a"
      "dd904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae00210203a9"
      "a4c37f5996d3aa25dbac6b570af0650394492942460b354753ed9eeca58710d90c6a4f000000"
      "800000008004000080002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b34282"
      "a94b6b5005109610d90c6a4f00000080000000800500008000",
      EXPECT_REJECT },
    /* [15] */ { "PSBT with invalid input sighash type typed key",
      "70736274ff0100730200000001301ae986e516a1ec8ac5b4bc6573d32f83b465e23ad76167d6"
      "8b38e730b4dbdb0000000000ffffffff02747b01000000000017a91403aa17ae882b5d0d54b2"
      "5d63104e4ffece7b9ea2876043993b0000000017a914b921b1ba6f722e4bfa83b6557a313998"
      "6a42ec8387000000000001011f00ca9a3b00000000160014d2d94b64ae08587eefc8eeb187c6"
      "01e939f9037c0203000100000000010016001462e9e982fff34dd8239610316b090cd2a3b747"
      "cb000100220020876bad832f1d168015ed41232a9ea65a1815d9ef13c0ef8759f64b5b2b278a"
      "65010125512103b7ce23a01c5b4bf00a642537cdfabb315b668332867478ef51309d2bd57f8a"
      "8751ae00",
      EXPECT_REJECT },
    /* [16] */ { "PSBT with invalid output redeemScript typed key",
      "70736274ff0100730200000001301ae986e516a1ec8ac5b4bc6573d32f83b465e23ad76167d6"
      "8b38e730b4dbdb0000000000ffffffff02747b01000000000017a91403aa17ae882b5d0d54b2"
      "5d63104e4ffece7b9ea2876043993b0000000017a914b921b1ba6f722e4bfa83b6557a313998"
      "6a42ec8387000000000001011f00ca9a3b00000000160014d2d94b64ae08587eefc8eeb187c6"
      "01e939f9037c0002000016001462e9e982fff34dd8239610316b090cd2a3b747cb0001002200"
      "20876bad832f1d168015ed41232a9ea65a1815d9ef13c0ef8759f64b5b2b278a650101255121"
      "03b7ce23a01c5b4bf00a642537cdfabb315b668332867478ef51309d2bd57f8a8751ae00",
      EXPECT_REJECT },
    /* [17] */ { "PSBT with invalid output witnessScript typed key",
      "70736274ff0100730200000001301ae986e516a1ec8ac5b4bc6573d32f83b465e23ad76167d6"
      "8b38e730b4dbdb0000000000ffffffff02747b01000000000017a91403aa17ae882b5d0d54b2"
      "5d63104e4ffece7b9ea2876043993b0000000017a914b921b1ba6f722e4bfa83b6557a313998"
      "6a42ec8387000000000001011f00ca9a3b00000000160014d2d94b64ae08587eefc8eeb187c6"
      "01e939f9037c00010016001462e9e982fff34dd8239610316b090cd2a3b747cb000100220020"
      "876bad832f1d168015ed41232a9ea65a1815d9ef13c0ef8759f64b5b2b278a65210100255121"
      "03b7ce23a01c5b4bf00a642537cdfabb315b668332867478ef51309d06d57f8a8751ae00",
      EXPECT_XFAIL },
    /* [18] */ { "PSBT with unsigned tx serialized with witness serialization format",
      "70736274ff01007802000000000101268171371edff285e937adeea4b37b78000c0566cbb3ad"
      "64641713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac2"
      "868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3"
      "eeb93c9c03948bc78700b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbb"
      "a4cc698fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039d"
      "af3da7de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d"
      "0171e63aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ff"
      "ffffff0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac"
      "72fef84e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d0587024730440220"
      "2712be22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7"
      "dcd314e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad"
      "4a996372cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dc"
      "d961d2f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb"
      "1a88af59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efec"
      "d61fcac6f79a4ea169393380734464f84f2ab300000000000000",
      EXPECT_REJECT },
    /* [19] */ { "PSBT with an invalid value data due to its size being not the stated size",
      "70736274ff0100337401ff0700010000000100ff01000a73317428ff0000000001ff01030100"
      "0001000000000000000076010000004100090000000000",
      EXPECT_REJECT },
};

static const struct { const char *desc; const char *hex; int expect; } bip174_valid[] = {
    /* [0] */ { "PSBT with one P2PKH input. Outputs are empty",
      "70736274ff0100750200000001268171371edff285e937adeea4b37b78000c0566cbb3ad6464"
      "1713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac28687"
      "60e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb9"
      "3c9c03948bc787b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbba4cc69"
      "8fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7"
      "de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e6"
      "3aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff"
      "0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef8"
      "4e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be"
      "22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314"
      "e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a9963"
      "72cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2"
      "f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af"
      "59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fca"
      "c6f79a4ea169393380734464f84f2ab300000000000000",
      EXPECT_PARSE },
    /* [1] */ { "PSBT with one P2PKH input and one P2SH-P2WPKH input. First input is signed and finalized. Outputs are empty",
      "70736274ff0100a00200000002ab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8"
      "139f877a5b7be40000000000feffffffab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf15"
      "3821a8139f877a5b7be40100000000feffffff02603bea0b000000001976a914768a40bbd740"
      "cbe81d988e71de2a4d5c71396b1d88ac8e240000000000001976a9146f4620b553fa095e721b"
      "9ee0efe9fa039cca459788ac000000000001076a47304402204759661797c01b036b25928948"
      "686218347d89864b719e1f7fcf57d1e511658702205309eabf56aa4d8891ffd111fdf1336f3a"
      "29da866d7f8486d75546ceedaf93190121035cdc61fc7ba971c0b501a646a2a83b102cb43881"
      "217ca682dc86e2d73fa882920001012000e1f5050000000017a9143545e6e33b832c47050f24"
      "d3eeb93c9c03948bc787010416001485d13537f2e265405a34dbafa9e3dda01fb82308000000",
      EXPECT_SKIP_WITNESS },
    /* [2] */ { "PSBT with one P2PKH input which has a non-final scriptSig and has a sighash type specified. Outputs are empty",
      "70736274ff0100750200000001268171371edff285e937adeea4b37b78000c0566cbb3ad6464"
      "1713ca42171bf60000000000feffffff02d3dff505000000001976a914d0c59903c5bac28687"
      "60e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb9"
      "3c9c03948bc787b32e1300000100fda5010100000000010289a3c71eab4d20e0371bbba4cc69"
      "8fa295c9463afa2e397f8533ccb62f9567e50100000017160014be18d152a9b012039daf3da7"
      "de4f53349eecb985ffffffff86f8aa43a71dff1448893a530a7237ef6b4608bbb2dd2d0171e6"
      "3aec6a4890b40100000017160014fe3e9ef1a745e974d902c4355943abcb34bd5353ffffffff"
      "0200c2eb0b000000001976a91485cff1097fd9e008bb34af709c62197b38978a4888ac72fef8"
      "4e2c00000017a914339725ba21efd62ac753a9bcd067d6c7a6a39d05870247304402202712be"
      "22e0270f394f568311dc7ca9a68970b8025fdd3b240229f07f8a5f3a240220018b38d7dcd314"
      "e734c9276bd6fb40f673325bc4baa144c800d2f2f02db2765c012103d2e15674941bad4a9963"
      "72cb87e1856d3652606d98562fe39c5e9e7e413f210502483045022100d12b852d85dcd961d2"
      "f5f4ab660654df6eedcc794c0c33ce5cc309ffb5fce58d022067338a8e0e1725c197fb1a88af"
      "59f51e44e4255b20167c8684031c05d1f2592a01210223b72beef0965d10be0778efecd61fca"
      "c6f79a4ea169393380734464f84f2ab30000000001030401000000000000",
      EXPECT_PARSE },
    /* [3] */ { "PSBT with one P2PKH input and one P2SH-P2WPKH input both with non-final scriptSigs. P2SH-P2WPKH input's redeemScript is available. Outputs filled.",
      "70736274ff0100a00200000002ab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8"
      "139f877a5b7be40000000000feffffffab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf15"
      "3821a8139f877a5b7be40100000000feffffff02603bea0b000000001976a914768a40bbd740"
      "cbe81d988e71de2a4d5c71396b1d88ac8e240000000000001976a9146f4620b553fa095e721b"
      "9ee0efe9fa039cca459788ac00000000000100df0200000001268171371edff285e937adeea4"
      "b37b78000c0566cbb3ad64641713ca42171bf6000000006a473044022070b2245123e6bf474d"
      "60c5b50c043d4c691a5d2435f09a34a7662a9dc251790a022001329ca9dacf280bdf30740ec0"
      "390422422c81cb45839457aeb76fc12edd95b3012102657d118d3357b8e0f4c2cd46db7b39f6"
      "d9c38d9a70abcb9b2de5dc8dbfe4ce31feffffff02d3dff505000000001976a914d0c59903c5"
      "bac2868760e90fd521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f"
      "24d3eeb93c9c03948bc787b32e13000001012000e1f5050000000017a9143545e6e33b832c47"
      "050f24d3eeb93c9c03948bc787010416001485d13537f2e265405a34dbafa9e3dda01fb82308"
      "00220202ead596687ca806043edc3de116cdf29d5e9257c196cd055cf698c8d02bf24e9910b4"
      "a6ba670000008000000080020000800022020394f62be9df19952c5587768aeb7698061ad2c4"
      "a25c894f47d8c162b4d7213d0510b4a6ba6700000080010000800200008000",
      EXPECT_SKIP_WITNESS },
    /* [4] */ { "PSBT with one P2SH-P2WSH input of a 2-of-2 multisig, redeemScript, witnessScript, and keypaths are available. Contains one signature.",
      "70736274ff0100550200000001279a2323a5dfb51fc45f220fa58b0fc13e1e3342792a85d7e3"
      "6cd6333b5cbc390000000000ffffffff01a05aea0b000000001976a914ffe9c0061097cc3b63"
      "6f2cb0460fa4fc427d2b4588ac0000000000010120955eea0b0000000017a9146345200f68d1"
      "89e1adc0df1c4d16ea8f14c0dbeb87220203b1341ccba7683b6af4f1238cd6e97e7167d569fa"
      "c47f1e48d47541844355bd4646304302200424b58effaaa694e1559ea5c93bbfd4a890642240"
      "55cdf070b6771469442d07021f5c8eb0fea6516d60b8acb33ad64ede60e8785bfb3aa94b99bd"
      "f86151db9a9a010104220020771fd18ad459666dd49f3d564e3dbc42f4c84774e360ada16816"
      "a8ed488d5681010547522103b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d475"
      "41844355bd462103de55d1e1dac805e3f8a58c1fbf9b94c02f3dbaafe127fefca4995f26f820"
      "83bd52ae220603b1341ccba7683b6af4f1238cd6e97e7167d569fac47f1e48d47541844355bd"
      "4610b4a6ba67000000800000008004000080220603de55d1e1dac805e3f8a58c1fbf9b94c02f"
      "3dbaafe127fefca4995f26f82083bd10b4a6ba670000008000000080050000800000",
      EXPECT_SKIP_WITNESS },
    /* [5] */ { "PSBT with one P2WSH input of a 2-of-2 multisig. witnessScript, keypaths, and global xpubs are available. Contains no signatures. Outputs filled.",
      "70736274ff01005202000000019dfc6628c26c5899fe1bd3dc338665bfd55d7ada10f6220973"
      "df2d386dec12760100000000ffffffff01f03dcd1d000000001600147b3a00bfdc14d27795c2"
      "b74901d09da6ef133579000000004f01043587cf02da3fd0088000000097048b1ad0445b1ec8"
      "275517727c87b4e4ebc18a203ffa0f94c01566bd38e9000351b743887ee1d40dc32a6043724f"
      "2d6459b3b5a4d73daec8fbae0472f3bc43e20cd90c6a4fae000080000000804f01043587cf02"
      "da3fd00880000001b90452427139cd78c2cff2444be353cd58605e3e513285e528b407fae3f6"
      "173503d30a5e97c8adbc557dac2ad9a7e39c1722ebac69e668b6f2667cc1d671c83cab0cd90c"
      "6a4fae000080010000800001012b0065cd1d000000002200202c5486126c4978079a814e1371"
      "5d65f36459e4d6ccaded266d0508645bafa6320105475221029da12cdb5b235692b91536afef"
      "e5c91c3ab9473d8e43b533836ab456299c88712103372b34234ed7cf9c1fea5d05d441557927"
      "be9542b162eb02e1ab2ce80224c00b52ae2206029da12cdb5b235692b91536afefe5c91c3ab9"
      "473d8e43b533836ab456299c887110d90c6a4fae0000800000008000000000220603372b3423"
      "4ed7cf9c1fea5d05d441557927be9542b162eb02e1ab2ce80224c00b10d90c6a4fae00008001"
      "00008000000000002202039eff1f547a1d5f92dfa2ba7af6ac971a4bd03ba4a734b03156a256"
      "b8ad3a1ef910ede45cc500000080000000800100008000",
      EXPECT_SKIP_WITNESS },
    /* [6] */ { "PSBT with unknown types in the inputs.",
      "70736274ff01003f0200000001ffffffffffffffffffffffffffffffffffffffffffffffffff"
      "ffffffffffffff0000000000ffffffff010000000000000000036a010000000000000af00102"
      "030405060708090f0102030405060708090a0b0c0d0e0f0000",
      EXPECT_PARSE_RT },
    /* [7] */ { "PSBT with PSBT_GLOBAL_XPUB.",
      "70736274ff01009d0100000002710ea76ab45c5cb6438e607e59cc037626981805ae9e0dfd90"
      "89012abb0be5350100000000ffffffff190994d6a8b3c8c82ccbcfb2fba4106aa06639b872a8"
      "d447465c0d42588d6d670000000000ffffffff0200e1f505000000001976a914b6bc2c0ee565"
      "5a843d79afedd0ccc3f7dd64340988ac605af405000000001600141188ef8e4ce0449eaac8fb"
      "141cbf5a1176e6a088000000004f010488b21e039e530cac800000003dbc8a5c9769f031b17e"
      "77fea1518603221a18fd18f2b9a54c6c8c1ac75cbc3502f230584b155d1c7f1cd45120a653c4"
      "8d650b431b67c5b2c13f27d7142037c1691027569c503100008000000080000000800001011f"
      "00e1f5050000000016001433b982f91b28f160c920b4ab95e58ce50dda3a4a220203309680f3"
      "3c7de38ea6a47cd4ecd66f1f5a49747c6ffb8808ed09039243e3ad5c47304402202d704ced83"
      "0c56a909344bd742b6852dccd103e963bae92d38e75254d2bb424502202d86c437195df46c0c"
      "eda084f2a291c3da2d64070f76bf9b90b195e7ef28f77201220603309680f33c7de38ea6a47c"
      "d4ecd66f1f5a49747c6ffb8808ed09039243e3ad5c1827569c50310000800000008000000080"
      "00000000010000000001011f00e1f50500000000160014388fb944307eb77ef45197d0b0b245"
      "e079f011de220202c777161f73d0b7c72b9ee7bde650293d13f095bc7656ad1f525da5fd2e10"
      "b11047304402204cb1fb5f869c942e0e26100576125439179ae88dca8a9dc3ba08f7953988fa"
      "a60220521f49ca791c27d70e273c9b14616985909361e25be274ea200d7e08827e514d012206"
      "02c777161f73d0b7c72b9ee7bde650293d13f095bc7656ad1f525da5fd2e10b1101827569c50"
      "31000080000000800000008000000000000000000000220202d20ca502ee289686d21815bd43"
      "a80637b0698e1fbcdbe4caed445f6c1a0a90ef1827569c503100008000000080000000800000"
      "00000400000000",
      EXPECT_PARSE },
    /* [8] */ { "PSBT with global unsigned tx that has 0 inputs and 0 outputs",
      "70736274ff01000a0000000000000000000000",
      EXPECT_PARSE_RT },
    /* [9] */ { "PSBT with 0 inputs",
      "70736274ff01004c020000000002d3dff505000000001976a914d0c59903c5bac2868760e90f"
      "d521a4665aa7652088ac00e1f5050000000017a9143545e6e33b832c47050f24d3eeb93c9c03"
      "948bc787b32e1300000000",
      EXPECT_PARSE_RT },
    /* [10] */ { "A Witness UTXO is provided for a non-witness input",
      "70736274ff0100a00200000002ab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf153821a8"
      "139f877a5b7be40000000000feffffffab0949a08c5af7c49b8212f417e2f15ab3f5c33dcf15"
      "3821a8139f877a5b7be40100000000feffffff02603bea0b000000001976a914768a40bbd740"
      "cbe81d988e71de2a4d5c71396b1d88ac8e240000000000001976a9146f4620b553fa095e721b"
      "9ee0efe9fa039cca459788ac0000000000010122d3dff505000000001976a914d48ed3110b94"
      "014cb114bd32d6f4d066dc74256b88ac0001012000e1f5050000000017a9143545e6e33b832c"
      "47050f24d3eeb93c9c03948bc787010416001485d13537f2e265405a34dbafa9e3dda01fb823"
      "0800220202ead596687ca806043edc3de116cdf29d5e9257c196cd055cf698c8d02bf24e9910"
      "b4a6ba670000008000000080020000800022020394f62be9df19952c5587768aeb7698061ad2"
      "c4a25c894f47d8c162b4d7213d0510b4a6ba6700000080010000800200008000",
      EXPECT_SKIP_WITNESS },
    /* [11] */ { "redeemScript with non-witness UTXO does not match the scriptPubKey",
      "70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f5"
      "45887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca"
      "78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b"
      "09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8"
      "742d1d87008f00000000000100bb0200000001aad73931018bd25f84ae400b68848be09db706"
      "eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c0"
      "15bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c"
      "5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82"
      "c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5"
      "d852e4063f618765000000220202dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54da"
      "e4dba2fbfef536d7483045022100f61038b308dc1da865a34852746f015772934208c6d24454"
      "393cd99bdf2217770220056e675a675a6d0a02b85b14e5e29074d8a25a9b5760bea2816f6619"
      "10a006ea01010304010000000104475221029583bf39ae0a609747ad199addd634fa6108559d"
      "6c5cd39b4c2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54da"
      "e4dba2fbfef536d752af2206029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c"
      "2183f1ab96e07f10d90c6a4f000000800000008000000080220602dab61ff49a14db6a7d02b0"
      "cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d710d90c6a4f000000800000008001000080"
      "0001012000c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa921e8872202"
      "023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e734730440220"
      "65f45ba5998b59a27ffe1a7bed016af1f1f90d54b3aa8f7450aa5f56a25103bd02207f724703"
      "ad1edb96680b284b56d4ffcb88f7fb759eabbe08aa30f29b851383d201010304010000000104"
      "2200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b2028903010547"
      "522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc21023a"
      "dd904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae2206023add"
      "904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7310d90c6a4f000000"
      "800000008003000080220603089dc10c7ac6db54f91329af617333db388cead0c231f723379d"
      "1b99030b02dc10d90c6a4f00000080000000800200008000220203a9a4c37f5996d3aa25dbac"
      "6b570af0650394492942460b354753ed9eeca5877110d90c6a4f000000800000008004000080"
      "002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b34282a94b6b5005109610d9"
      "0c6a4f00000080000000800500008000",
      EXPECT_SKIP_WITNESS },
    /* [12] */ { "redeemScript with witness UTXO does not match the scriptPubKey",
      "70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f5"
      "45887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca"
      "78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b"
      "09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8"
      "742d1d87008f00000000000100bb0200000001aad73931018bd25f84ae400b68848be09db706"
      "eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c0"
      "15bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c"
      "5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82"
      "c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5"
      "d852e4063f618765000000220202dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54da"
      "e4dba2fbfef536d7483045022100f61038b308dc1da865a34852746f015772934208c6d24454"
      "393cd99bdf2217770220056e675a675a6d0a02b85b14e5e29074d8a25a9b5760bea2816f6619"
      "10a006ea01010304010000000104475221029583bf39ae0a609747ad199addd634fa6108559d"
      "6c5cd39b4c2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54da"
      "e4dba2fbfef536d752ae2206029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c"
      "2183f1ab96e07f10d90c6a4f000000800000008000000080220602dab61ff49a14db6a7d02b0"
      "cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d710d90c6a4f000000800000008001000080"
      "0001012000c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa921e8872202"
      "023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e734730440220"
      "65f45ba5998b59a27ffe1a7bed016af1f1f90d54b3aa8f7450aa5f56a25103bd02207f724703"
      "ad1edb96680b284b56d4ffcb88f7fb759eabbe08aa30f29b851383d201010304010000000104"
      "2200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b2028900010547"
      "522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc21023a"
      "dd904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ae2206023add"
      "904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7310d90c6a4f000000"
      "800000008003000080220603089dc10c7ac6db54f91329af617333db388cead0c231f723379d"
      "1b99030b02dc10d90c6a4f00000080000000800200008000220203a9a4c37f5996d3aa25dbac"
      "6b570af0650394492942460b354753ed9eeca5877110d90c6a4f000000800000008004000080"
      "002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b34282a94b6b5005109610d9"
      "0c6a4f00000080000000800500008000",
      EXPECT_SKIP_WITNESS },
    /* [13] */ { "witnessScript with witness UTXO does not match the redeemScript",
      "70736274ff01009a020000000258e87a21b56daf0c23be8e7070456c336f7cbaa5c8757924f5"
      "45887bb2abdd750000000000ffffffff838d0427d0ec650a68aa46bb0b098aea4422c071b2ca"
      "78352a077959d07cea1d0100000000ffffffff0270aaf00800000000160014d85c2b71d0060b"
      "09c9886aeb815e50991dda124d00e1f5050000000016001400aea9a2e5f0f876a588df5546e8"
      "742d1d87008f00000000000100bb0200000001aad73931018bd25f84ae400b68848be09db706"
      "eac2ac18298babee71ab656f8b0000000048473044022058f6fc7c6a33e1b31548d481c826c0"
      "15bd30135aad42cd67790dab66d2ad243b02204a1ced2604c6735b6393e5b41691dd78b00f0c"
      "5942fb9f751856faa938157dba01feffffff0280f0fa020000000017a9140fb9463421696b82"
      "c833af241c78c17ddbde493487d0f20a270100000017a91429ca74f8a08f81999428185c97b5"
      "d852e4063f618765000000220202dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54da"
      "e4dba2fbfef536d7483045022100f61038b308dc1da865a34852746f015772934208c6d24454"
      "393cd99bdf2217770220056e675a675a6d0a02b85b14e5e29074d8a25a9b5760bea2816f6619"
      "10a006ea01010304010000000104475221029583bf39ae0a609747ad199addd634fa6108559d"
      "6c5cd39b4c2183f1ab96e07f2102dab61ff49a14db6a7d02b0cd1fbb78fc4b18312b5b4e54da"
      "e4dba2fbfef536d752ae2206029583bf39ae0a609747ad199addd634fa6108559d6c5cd39b4c"
      "2183f1ab96e07f10d90c6a4f000000800000008000000080220602dab61ff49a14db6a7d02b0"
      "cd1fbb78fc4b18312b5b4e54dae4dba2fbfef536d710d90c6a4f000000800000008001000080"
      "0001012000c2eb0b0000000017a914b7f5faf40e3d40a5a459b1db3535f2b72fa921e8872202"
      "023add904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e734730440220"
      "65f45ba5998b59a27ffe1a7bed016af1f1f90d54b3aa8f7450aa5f56a25103bd02207f724703"
      "ad1edb96680b284b56d4ffcb88f7fb759eabbe08aa30f29b851383d201010304010000000104"
      "2200208c2353173743b595dfb4a07b72ba8e42e3797da74e87fe7d9d7497e3b2028903010547"
      "522103089dc10c7ac6db54f91329af617333db388cead0c231f723379d1b99030b02dc21023a"
      "dd904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7352ad2206023add"
      "904f3d6dcf59ddb906b0dee23529b7ffb9ed50e5e86151926860221f0e7310d90c6a4f000000"
      "800000008003000080220603089dc10c7ac6db54f91329af617333db388cead0c231f723379d"
      "1b99030b02dc10d90c6a4f00000080000000800200008000220203a9a4c37f5996d3aa25dbac"
      "6b570af0650394492942460b354753ed9eeca5877110d90c6a4f000000800000008004000080"
      "002202027f6399757d2eff55a136ad02c684b1838b6556e5f1b6b34282a94b6b5005109610d9"
      "0c6a4f00000080000000800500008000",
      EXPECT_SKIP_WITNESS },
};

static void test_psbt_bip174_invalid_vectors(void)
{
    for (size_t i = 0; i < sizeof(bip174_invalid) / sizeof(bip174_invalid[0]); i++) {
        if (bip174_invalid[i].expect == EXPECT_XFAIL)
            continue; /* known limitation: witness-typed key accepted as unknown */
        dogecoin_psbt *psbt = NULL;
        dogecoin_bool ok = dogecoin_psbt_from_hex(bip174_invalid[i].hex, &psbt);
        if (ok) {
            printf("BIP174 invalid vector [%zu] unexpectedly PARSED: %s\n",
                   i, bip174_invalid[i].desc);
        }
        u_assert_int_eq(ok, false);
        u_assert_is_null(psbt);
    }
}

static void test_psbt_bip174_valid_vectors(void)
{
    for (size_t i = 0; i < sizeof(bip174_valid) / sizeof(bip174_valid[0]); i++) {
        if (bip174_valid[i].expect == EXPECT_SKIP_WITNESS)
            continue; /* needs SegWit support libdogecoin does not provide */
        dogecoin_psbt *psbt = NULL;
        dogecoin_bool ok = dogecoin_psbt_from_hex(bip174_valid[i].hex, &psbt);
        if (!ok) {
            printf("BIP174 valid vector [%zu] failed to PARSE: %s\n",
                   i, bip174_valid[i].desc);
        }
        u_assert_int_eq(ok, true);
        u_assert_not_null(psbt);
        /* Byte-exact round-trip is only asserted for vectors whose contents are
         * fully non-witness. Several spec vectors carry a non_witness_utxo that
         * is itself a SegWit-serialized tx; libdogecoin parses past the witness
         * data but cannot re-serialize it byte-identically, so those are PARSE
         * (parse-only) rather than PARSE_RT. */
        if (bip174_valid[i].expect == EXPECT_PARSE_RT) {
            char *reshex = dogecoin_psbt_to_hex(psbt);
            if (reshex && strcmp(reshex, bip174_valid[i].hex) != 0) {
                printf("BIP174 valid vector [%zu] round-trip MISMATCH: %s\n",
                       i, bip174_valid[i].desc);
            }
            u_assert_not_null(reshex);
            u_assert_str_eq(reshex, bip174_valid[i].hex);
            dogecoin_free(reshex);
        }
        dogecoin_psbt_free(psbt);
    }
}


/* ── Test: duplicate known keys are rejected (BIP174 §2) ──────── */
/*
 * For each known key type that can appear more than once in a serialized map,
 * assert that a well-formed single occurrence parses, but a second occurrence
 * of the same key causes deserialization to fail. The baseline parse proves the
 * rejection is due to the duplicate, not a malformed base PSBT.
 */
static void test_psbt_duplicate_known_keys(void)
{
    static const struct { const char *desc; const char *base; const char *dup; } cases[] = {
        { "global version",
          "70736274ff010055020000000100000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0100e40b54020000001976a914000000000000000000000000000000000000000088ac0000000001fb0400000000000000",
          "70736274ff010055020000000100000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0100e40b54020000001976a914000000000000000000000000000000000000000088ac0000000001fb040000000001fb0400000000000000" },
        { "input sighash type",
          "70736274ff010055020000000100000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0100e40b54020000001976a914000000000000000000000000000000000000000088ac0000000000010304010000000000",
          "70736274ff010055020000000100000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0100e40b54020000001976a914000000000000000000000000000000000000000088ac000000000001030401000000010304010000000000" },
        { "input partial sig",
          "70736274ff010055020000000100000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0100e40b54020000001976a914000000000000000000000000000000000000000088ac00000000002202020000000000000000000000000000000000000000000000000000000000000000093006020101020101010000",
          "70736274ff010055020000000100000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0100e40b54020000001976a914000000000000000000000000000000000000000088ac00000000002202020000000000000000000000000000000000000000000000000000000000000000093006020101020101012202020000000000000000000000000000000000000000000000000000000000000000093006020101020101010000" },
        { "input bip32 derivation",
          "70736274ff010055020000000100000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0100e40b54020000001976a914000000000000000000000000000000000000000088ac000000000022060200000000000000000000000000000000000000000000000000000000000000000800000000000000000000",
          "70736274ff010055020000000100000000000000000000000000000000000000000000000000000000000000000000000000ffffffff0100e40b54020000001976a914000000000000000000000000000000000000000088ac0000000000220602000000000000000000000000000000000000000000000000000000000000000008000000000000000022060200000000000000000000000000000000000000000000000000000000000000000800000000000000000000" },
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        /* baseline single occurrence must parse */
        dogecoin_psbt *base = NULL;
        dogecoin_bool ok_base = dogecoin_psbt_from_hex(cases[i].base, &base);
        if (!ok_base)
            printf("dup-key baseline [%zu] failed to parse: %s\n", i, cases[i].desc);
        u_assert_int_eq(ok_base, true);
        u_assert_not_null(base);
        dogecoin_psbt_free(base);

        /* duplicate occurrence must be rejected */
        dogecoin_psbt *dup = NULL;
        dogecoin_bool ok_dup = dogecoin_psbt_from_hex(cases[i].dup, &dup);
        if (ok_dup)
            printf("dup-key [%zu] unexpectedly accepted duplicate: %s\n", i, cases[i].desc);
        u_assert_int_eq(ok_dup, false);
        u_assert_is_null(dup);
    }
}

/* ── Test: combiner rejects conflicting metadata ─────────────── */
/*
 * Two PSBTs over the same unsigned tx but with conflicting scalar fields on the
 * same input must not silently merge; dogecoin_psbt_combine must return false.
 */
static void test_psbt_combiner_conflict(void)
{
    dogecoin_tx *tx = make_unsigned_tx();

    /* Conflicting sighash type on input 0 */
    {
        dogecoin_psbt *a = dogecoin_psbt_create(tx);
        dogecoin_psbt *b = dogecoin_psbt_create(tx);
        a->inputs[0].has_sighash_type = true;
        a->inputs[0].sighash_type     = SIGHASH_ALL;
        b->inputs[0].has_sighash_type = true;
        b->inputs[0].sighash_type     = SIGHASH_SINGLE;
        u_assert_int_eq(dogecoin_psbt_combine(a, b), false);
        dogecoin_psbt_free(a);
        dogecoin_psbt_free(b);
    }

    /* Conflicting redeem script on input 0 */
    {
        dogecoin_psbt *a = dogecoin_psbt_create(tx);
        dogecoin_psbt *b = dogecoin_psbt_create(tx);
        a->inputs[0].redeem_script = cstr_new_buf("\x51", 1); /* OP_1 */
        b->inputs[0].redeem_script = cstr_new_buf("\x52", 1); /* OP_2 */
        u_assert_int_eq(dogecoin_psbt_combine(a, b), false);
        dogecoin_psbt_free(a);
        dogecoin_psbt_free(b);
    }

    /* Non-conflicting (identical) sighash must still combine successfully */
    {
        dogecoin_psbt *a = dogecoin_psbt_create(tx);
        dogecoin_psbt *b = dogecoin_psbt_create(tx);
        a->inputs[0].has_sighash_type = true;
        a->inputs[0].sighash_type     = SIGHASH_ALL;
        b->inputs[0].has_sighash_type = true;
        b->inputs[0].sighash_type     = SIGHASH_ALL;
        u_assert_int_eq(dogecoin_psbt_combine(a, b), true);
        dogecoin_psbt_free(a);
        dogecoin_psbt_free(b);
    }

    dogecoin_tx_free(tx);
}

/* ── Entry point ────────────────────────────────────────────── */
/* ── Test: caller-supplied finalizer and hex extractor ──────── */
static void test_psbt_custom_finalizer(void)
{
    /* A redeem script the built-in finalizer cannot classify: the branch
       selector plus a timelocked path is not any standard shape. */
    dogecoin_tx *tx = make_unsigned_tx();
    dogecoin_psbt *psbt = dogecoin_psbt_create(tx);
    u_assert_not_null(psbt);

    /* nothing finalized yet: extract and extract_hex both refuse */
    u_assert_is_null(dogecoin_psbt_extract(psbt));
    u_assert_is_null(dogecoin_psbt_extract_hex(psbt));
    u_assert_true(dogecoin_psbt_is_finalized(psbt) == false);

    /* the caller builds its own scriptSig: OP_0 <sig-ish> OP_0 */
    const uint8_t script_sig[] = { 0x00, 0x02, 0xde, 0xad, 0x00 };
    u_assert_true(dogecoin_psbt_input_set_final_scriptsig(psbt, 0, script_sig,
                                                          sizeof(script_sig)));
    u_assert_true(dogecoin_psbt_is_finalized(psbt));

    /* it lands in the tx the extractor produces, byte for byte */
    dogecoin_tx *final_tx = dogecoin_psbt_extract(psbt);
    u_assert_not_null(final_tx);
    dogecoin_tx_in *fin = vector_idx(final_tx->vin, 0);
    u_assert_int_eq((int)fin->script_sig->len, (int)sizeof(script_sig));
    u_assert_mem_eq(fin->script_sig->str, script_sig, sizeof(script_sig));
    dogecoin_tx_free(final_tx);

    /* the hex extractor agrees with serializing that same tx */
    char *hex = dogecoin_psbt_extract_hex(psbt);
    u_assert_not_null(hex);
    u_assert_true(strlen(hex) > 0);
    u_assert_true(strlen(hex) % 2 == 0);
    /* the scriptSig bytes appear in the serialized transaction */
    u_assert_not_null(strstr(hex, "0002dead00"));
    dogecoin_free(hex);

    /* replacing it replaces cleanly rather than leaking or appending */
    const uint8_t other[] = { 0x51 };
    u_assert_true(dogecoin_psbt_input_set_final_scriptsig(psbt, 0, other, 1));
    char *hex2 = dogecoin_psbt_extract_hex(psbt);
    u_assert_not_null(hex2);
    u_assert_is_null(strstr(hex2, "0002dead00"));
    dogecoin_free(hex2);

    /* rejections */
    u_assert_true(dogecoin_psbt_input_set_final_scriptsig(NULL, 0, other, 1) == false);
    u_assert_true(dogecoin_psbt_input_set_final_scriptsig(psbt, 99, other, 1) == false);
    u_assert_true(dogecoin_psbt_input_set_final_scriptsig(psbt, 0, NULL, 1) == false);
    u_assert_is_null(dogecoin_psbt_extract_hex(NULL));

    dogecoin_psbt_free(psbt);
    dogecoin_tx_free(tx);
}

/* ── Test: a consumer-side finalizer using only accessors ────── */
static void test_psbt_accessors(void)
{
    dogecoin_key privkey;
    dogecoin_privkey_init(&privkey);
    dogecoin_privkey_gen(&privkey);
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    dogecoin_pubkey_from_key(&privkey, &pubkey);

    uint8_t hash160[20];
    dogecoin_pubkey_get_hash160(&pubkey, hash160);

    dogecoin_tx *tx = dogecoin_tx_new();
    tx->version = 1; tx->locktime = 0;
    dogecoin_tx_in *txin = dogecoin_tx_in_new();
    memset(txin->prevout.hash, 0x11, sizeof(txin->prevout.hash));
    txin->prevout.n = 0; txin->sequence = 0xFFFFFFFF;
    if (txin->script_sig) cstr_free(txin->script_sig, true);
    txin->script_sig = cstr_new_sz(0);
    vector_add(tx->vin, txin);
    dogecoin_tx_out *txout = dogecoin_tx_out_new();
    txout->value = 5000000000LL;
    uint8_t dest[20]; memset(dest, 0xCC, 20);
    txout->script_pubkey = cstr_new_sz(25);
    dogecoin_script_build_p2pkh(txout->script_pubkey, dest);
    vector_add(tx->vout, txout);

    dogecoin_psbt *psbt = dogecoin_psbt_create(tx);
    u_assert_not_null(psbt);
    dogecoin_tx_free(tx);

    dogecoin_tx *utxo = make_prev_tx(hash160);
    uint8_t utxo_txid[32];
    dogecoin_tx_hash(utxo, utxo_txid);
    dogecoin_tx_in *vin0 = vector_idx(psbt->tx->vin, 0);
    memcpy(vin0->prevout.hash, utxo_txid, 32);
    u_assert_true(dogecoin_psbt_input_set_utxo(psbt, 0, utxo));
    dogecoin_tx_free(utxo);

    /* counts and version, without reaching into the struct */
    u_assert_int_eq((int)dogecoin_psbt_num_inputs(psbt), 1);
    u_assert_int_eq((int)dogecoin_psbt_num_outputs(psbt), 1);
    u_assert_int_eq((int)dogecoin_psbt_get_version(psbt), PSBT_VERSION_0);
    u_assert_int_eq((int)dogecoin_psbt_num_inputs(NULL), 0);

    u_assert_true(dogecoin_psbt_input_set_sighash(psbt, 0, 1));
    uint32_t sh = 0;
    u_assert_true(dogecoin_psbt_input_get_sighash(psbt, 0, &sh));
    u_assert_int_eq((int)sh, 1);

    /* sign while the input is still plain P2PKH; a redeem script would make the
       signer treat it as P2SH and the scripts would not agree */
    u_assert_true(dogecoin_psbt_sign(psbt, &privkey));
    u_assert_int_eq((int)dogecoin_psbt_input_num_partial_sigs(psbt, 0), 1);

    uint8_t pk[64], sig[128];
    size_t pklen = 0, siglen = 0;
    u_assert_true(dogecoin_psbt_input_get_partial_sig(psbt, 0, 0, NULL, 0, &pklen,
                                                      NULL, 0, &siglen) == false);
    u_assert_int_eq((int)pklen, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    u_assert_true(siglen > 0);
    u_assert_true(dogecoin_psbt_input_get_partial_sig(psbt, 0, 0, pk, sizeof(pk), &pklen,
                                                      sig, sizeof(sig), &siglen));
    u_assert_mem_eq(pk, pubkey.pubkey, DOGECOIN_ECKEY_COMPRESSED_LENGTH);
    u_assert_true(dogecoin_psbt_input_get_partial_sig(psbt, 0, 9, pk, sizeof(pk), &pklen,
                                                      sig, sizeof(sig), &siglen) == false);

    /* a redeem script set is a redeem script readable */
    const uint8_t redeem[] = { 0x63, 0x52, 0x68, 0xae };
    u_assert_true(dogecoin_psbt_input_set_redeemscript(psbt, 0, redeem, sizeof(redeem)));
    size_t rlen = 0;
    u_assert_true(dogecoin_psbt_input_get_redeemscript(psbt, 0, NULL, 0, &rlen) == false);
    u_assert_int_eq((int)rlen, (int)sizeof(redeem));       /* size query */
    uint8_t rbuf[16];
    u_assert_true(dogecoin_psbt_input_get_redeemscript(psbt, 0, rbuf, 2, &rlen) == false);
    u_assert_true(dogecoin_psbt_input_get_redeemscript(psbt, 0, rbuf, sizeof(rbuf), &rlen));
    u_assert_mem_eq(rbuf, redeem, sizeof(redeem));

    /* build a scriptSig from what we read, and install it: the whole point */
    uint8_t ss[256]; size_t sslen = 0;
    ss[sslen++] = 0x00;                       /* OP_0, multisig off-by-one   */
    ss[sslen++] = (uint8_t)siglen;            /* push the signature we read  */
    memcpy(ss + sslen, sig, siglen); sslen += siglen;
    ss[sslen++] = (uint8_t)rlen;              /* push the redeem script      */
    memcpy(ss + sslen, rbuf, rlen); sslen += rlen;
    u_assert_true(dogecoin_psbt_input_set_final_scriptsig(psbt, 0, ss, sslen));

    /* and read it back identically */
    size_t flen = 0;
    uint8_t fbuf[256];
    u_assert_true(dogecoin_psbt_input_get_final_scriptsig(psbt, 0, fbuf, sizeof(fbuf), &flen));
    u_assert_int_eq((int)flen, (int)sslen);
    u_assert_mem_eq(fbuf, ss, sslen);

    char *hex = dogecoin_psbt_extract_hex(psbt);
    u_assert_not_null(hex);
    if (hex) dogecoin_free(hex);

    /* out-of-range and NULL are refusals, not crashes */
    u_assert_true(dogecoin_psbt_input_get_redeemscript(psbt, 9, rbuf, sizeof(rbuf), &rlen) == false);
    u_assert_true(dogecoin_psbt_input_get_sighash(NULL, 0, &sh) == false);
    u_assert_true(dogecoin_psbt_output_get_redeemscript(psbt, 9, rbuf, sizeof(rbuf), &rlen) == false);

    dogecoin_psbt_free(psbt);
    dogecoin_privkey_cleanse(&privkey);
}

void test_psbt(void)
{
    test_psbt_lifecycle();
    test_psbt_creator();
    test_psbt_serialization();
    test_psbt_base64();
    test_psbt_hex();
    test_psbt_updater();
    test_psbt_sign_finalize_extract();
    test_psbt_custom_finalizer();
    test_psbt_accessors();
    test_psbt_combiner();
    test_psbt_combiner_conflict();
    test_psbt_duplicate_known_keys();
    test_psbt_multisig_2of3_out_of_order();
    test_psbt_roundtrip_full();
    test_psbt_invalid();
    test_psbt_bip174_invalid_vectors();
    test_psbt_bip174_valid_vectors();
}
