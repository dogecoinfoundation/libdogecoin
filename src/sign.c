/*

 The MIT License (MIT)

 Copyright (c) 2023 bluezr
 Copyright (c) 2023 The Dogecoin Foundation

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

#include <dogecoin/chainparams.h>
#include <dogecoin/ecc.h>
#include <dogecoin/hash.h>
#include <dogecoin/key.h>
#include <dogecoin/mem.h>
#include <dogecoin/sign.h>
#include <dogecoin/utils.h>


const char* msg_magic = "Dogecoin Signed Message:\n";

/**
 * @brief This function instantiates a new working signature,
 * but does not add it to the hash table.
 *
 * @return A pointer to the new working signature.
 */
signature* new_signature() {
    signature* sig = (struct signature*)dogecoin_calloc(1, sizeof *sig);
    sig->recid = 0;
    sig->idx = HASH_COUNT(signatures) + 1;
    return sig;
}

/**
 * @brief This function takes a pointer to an existing working
 * signature object and adds it to the hash table.
 *
 * @param sig The pointer to the working signature.
 *
 * @return Nothing.
 */
void add_signature(signature *sig) {
    signature* sig_old;
    HASH_FIND_INT(signatures, &sig->idx, sig_old);
    if (sig_old == NULL) {
        HASH_ADD_INT(signatures, idx, sig);
    } else {
        HASH_REPLACE_INT(signatures, idx, sig, sig_old);
    }
    dogecoin_free(sig_old);
}

/**
 * @brief This function takes an index and returns the working
 * signature associated with that index in the hash table.
 *
 * @param idx The index of the target working signature.
 *
 * @return The pointer to the working signature associated with
 * the provided index.
 */
signature* find_signature(int idx) {
    signature* sig;
    HASH_FIND_INT(signatures, &idx, sig);
    return sig;
}

/**
 * @brief This function removes the specified working signature
 * from the hash table and frees the signatures in memory.
 *
 * @param sig The pointer to the signature to remove.
 *
 * @return Nothing.
 */
void remove_signature(signature* sig) {
    HASH_DEL(signatures, sig); /* delete it (signatures advances to next) */
    sig->recid = 0;
    dogecoin_free(sig);
}

/**
 * @brief This function creates a new sig, places it in
 * the hash table, and returns the index of the new sig,
 * starting from 1 and incrementing each subsequent call.
 *
 * @return The index of the new sig.
 */
int start_signature() {
    signature* sig = new_signature();
    int index = sig->idx;
    add_signature(sig);
    return index;
}

/**
 * @brief This function frees the memory allocated
 * for an sig.
 *
 * @param sig The pointer to the sig to be freed.
 *
 * @return Nothing.
 */
void free_signature(signature* sig)
{
    dogecoin_free(sig->der);
    dogecoin_free(sig);
}

/**
 * @brief Sign message with private key
 *
 * @param privkey The key to sign the message with.
 * @param message The message to be signed.
 *
 * @return Base64 encoded signature if successful, False (0) if not
 *
 */
char* signmsgwithprivatekey(char* privkey, char* msg) {
    if (!privkey || !msg) return false;

    // retrieve double sha256 hash of msg:
    uint256 message_bytes;
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 7);
    tmp_msg[0] = msg_magic_len;
    strncpy(tmp_msg + 1, msg_magic, msg_magic_len);
    tmp_msg[msg_magic_len + 1] = msg_len;
    strncpy(tmp_msg + msg_magic_len + 2, msg, msg_len + 1);
    dogecoin_hash_sngl_sha256((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);

    // vars for signing
    size_t outlen = 74, outlencmp = 64;
    unsigned char *sig = dogecoin_uchar_vla(outlen),
    *sigcmp = dogecoin_uchar_vla(outlencmp);

    dogecoin_key key;
    dogecoin_privkey_init(&key);
    assert(dogecoin_privkey_is_valid(&key) == 0);
    const dogecoin_chainparams* chain = chain_from_b58_prefix(privkey);
    int ret = dogecoin_privkey_decode_wif(privkey, chain, &key);
    if (!ret) {
        printf("dogecoin_privkey_decode_wif failed!\n");
        return false;
    }
    ret = dogecoin_privkey_is_valid(&key);
    assert(ret == 1);

    // init pubkey
    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    pubkey.compressed = false;
    ret = dogecoin_pubkey_is_valid(&pubkey);
    assert(ret == 0);
    dogecoin_pubkey_from_key(&key, &pubkey);
    ret = dogecoin_pubkey_is_valid(&pubkey);
    if (!ret) return 0;
    ret = dogecoin_privkey_verify_pubkey(&key, &pubkey);
    if (!ret) return 0;

    // sign hash
    ret = dogecoin_key_sign_hash(&key, message_bytes, sig, &outlen);
    if (!ret) {
        printf("dogecoin_key_sign_hash failed!\n");
        return 0;
    }

    int recid = 0;

    // sign compact for recovery of pubkey and free privkey:
    ret = dogecoin_key_sign_hash_compact_recoverable(&key, message_bytes, sigcmp, &outlencmp, &recid);
    if (!ret) {
        printf("key sign recoverable failed!\n");
        return false;
    }
    ret = dogecoin_key_sign_recover_pubkey((const unsigned char*)sigcmp, message_bytes, recid, &pubkey);
    if (!ret) {
        printf("key sign recover failed!\n");
        return false;
    }
    ret = dogecoin_pubkey_verify_sig(&pubkey, message_bytes, sig, outlen);
    if (!ret) {
        printf("pubkey sig verification failed!\n");
        return false;
    }

    if (recid != 0) {
        char tmp[2];
        snprintf(tmp, 2, "%d", recid);
        size_t i = 0;
        for (i = 0; memcmp(&tmp[i], "\0", 1) != 0; i++) {
            sig[outlen + i] = tmp[i];
        }
        memcpy(&sig[outlen + i], "\0", 1);
        outlen += 2;
    }

    // base64 encode output and free sig:
    char* out = b64_encode(sig, outlen);

    dogecoin_free(sig);
    dogecoin_free(sigcmp);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_cleanse(&pubkey);

    return out;
}

/**
 * @brief Verify signed message using non BIP32 encoded address.
 *
 * @param sig
 * @param msg
 * @param address
 *
 * @return P2PKH address if successful, False (0) otherwise
 *
 */
int verifymessage(char* sig, char* msg, char* address) {
    if (!(sig || msg || address)) return false;

    // retrieve double sha256 hash of msg:
    uint256 message_bytes;
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 7);
    tmp_msg[0] = msg_magic_len;
    strncpy(tmp_msg + 1, msg_magic, msg_magic_len);
    tmp_msg[msg_magic_len + 1] = msg_len;
    strncpy(tmp_msg + msg_magic_len + 2, msg, msg_len + 1);
    dogecoin_hash_sngl_sha256((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);

	size_t outlen = b64_decoded_size(sig);
    unsigned char *out = dogecoin_uchar_vla(outlen),
    *sigcomp_out = dogecoin_uchar_vla(65);
    int ret = b64_decode(sig, out, outlen);
	if (!ret) {
        printf("b64_decode failed!\n");
        return false;
    }

    int recid = 0;
    if (strcmp(utils_uint8_to_hex(&out[outlen - 2], 2), "3100")==0) {
        char* tmp = utils_uint8_to_hex(&out[outlen - 2], 1);
        recid = atoi(&tmp[1]);
        outlen -= 2;
    }

    ret = dogecoin_ecc_der_to_compact(out, outlen, sigcomp_out);
    if (!ret) {
        printf("ecc der to compact failed!\n");
        return false;
    }

    // initialize empty pubkey
    dogecoin_pubkey pub_key;
    dogecoin_pubkey_init(&pub_key);
    pub_key.compressed = false;

    // recover pubkey
    ret = dogecoin_key_sign_recover_pubkey((const unsigned char*)sigcomp_out, message_bytes, recid, &pub_key);
    if (!ret) {
        printf("key sign recover failed!\n");
        return false;
    }
    ret = dogecoin_pubkey_verify_sig(&pub_key, message_bytes, out, outlen);
    if (!ret) {
        printf("pubkey sig verification failed!\n");
        return false;
    }

    const dogecoin_chainparams* chain = chain_from_b58_prefix(address);
    // derive p2pkh address from new injected dogecoin_pubkey with known hexadecimal public key:
    char* p2pkh_address = dogecoin_char_vla(34 + 1);
    ret = dogecoin_pubkey_getaddr_p2pkh(&pub_key, chain, p2pkh_address);
    if (!ret) {
        printf("derived address from pubkey failed!\n");
        return false;
    }

    printf("p2pkh_address: %s\n", p2pkh_address);
    printf("address:       %s\n", address);

    ret = strcmp(p2pkh_address, address)==0;

    dogecoin_free(out);
    dogecoin_free(sigcomp_out);
    dogecoin_free(p2pkh_address);
    dogecoin_pubkey_cleanse(&pub_key);
    return ret;
}

/**
 * @brief Sign message with private key
 *
 * @param key The key to sign the message with.
 * @param msg The message to be signed.
 *
 * @return Signature struct with base64 encoded signature and recid if successful, False (0) if not
 *
 */
signature* signmsgwitheckey(eckey* key, char* msg) {
    if (!key || !msg) return false;

    // retrieve double sha256 hash of msg:
    uint256 message_bytes;
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 7);
    tmp_msg[0] = msg_magic_len;
    strncpy(tmp_msg + 1, msg_magic, msg_magic_len);
    tmp_msg[msg_magic_len + 1] = msg_len;
    strncpy(tmp_msg + msg_magic_len + 2, msg, msg_len + 1);
    dogecoin_hash_sngl_sha256((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);

    // vars for signing
    size_t outlen = 74, outlencmp = 64;
    unsigned char *sig = dogecoin_uchar_vla(outlen),
    *sigcmp = dogecoin_uchar_vla(outlencmp + 1);

    // sign hash
    int ret = dogecoin_key_sign_hash(&key->private_key, message_bytes, sig, &outlen);
    if (!ret) {
        printf("dogecoin_key_sign_hash failed!\n");
        return 0;
    }

    int recid = 0;

    // sign compact for recovery of pubkey and free privkey:
    ret = dogecoin_key_sign_hash_compact_recoverable(&key->private_key, message_bytes, sigcmp, &outlencmp, &recid);
    if (!ret) {
        printf("key sign recoverable failed!\n");
        return false;
    }
    ret = dogecoin_key_sign_recover_pubkey((const unsigned char*)sigcmp, message_bytes, recid, &key->public_key);
    if (!ret) {
        printf("key sign recover failed!\n");
        return false;
    }
    ret = dogecoin_pubkey_verify_sig(&key->public_key, message_bytes, sig, outlen);
    if (!ret) {
        printf("pubkey sig verification failed!\n");
        return false;
    }

    signature* working_sig = new_signature();
    working_sig->recid = recid;
    if (recid != 0) {
        char tmp[2];
        snprintf(tmp, 2, "%d", recid);
        size_t i = 0;
        for (i = 0; memcmp(&tmp[i], "\0", 1) != 0; i++) {
            sig[outlen + i] = tmp[i];
        }
        memcpy(&sig[outlen + i], "\0", 1);
        outlen += 2;
    }

    const dogecoin_chainparams* chain = chain_from_b58_prefix((const char*)key->private_key_wif);
    // derive p2pkh address from new injected dogecoin_pubkey with known hexadecimal public key:
    ret = dogecoin_pubkey_getaddr_p2pkh(&key->public_key, chain, working_sig->address);
    if (!ret) {
        printf("derived address from pubkey failed!\n");
        return false;
    }

    // base64 encode output and free sig:
    working_sig->der = b64_encode(sig, outlen);
    printf("working_sig->der: %s\n", working_sig->der);

    dogecoin_free(sig);
    dogecoin_free(sigcmp);

    return working_sig;
}

/**
 * @brief Verify signed message
 *
 * @param sig
 * @param msg
 *
 * @return P2PKH address if successful, False (0) otherwise
 *
 */
char* verifymessagewithsig(signature* sig, char* msg) {
    if (!(sig || msg)) return false;

    // double sha256 hash message:
    uint256 message_bytes;
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 7);
    tmp_msg[0] = msg_magic_len;
    strncpy(tmp_msg + 1, msg_magic, msg_magic_len);
    tmp_msg[msg_magic_len + 1] = msg_len;
    strncpy(tmp_msg + msg_magic_len + 2, msg, msg_len + 1);
    dogecoin_hash_sngl_sha256((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);

    char* signature_encoded = sig->der;
	size_t outlen = b64_decoded_size(signature_encoded);
    unsigned char *out = dogecoin_uchar_vla(outlen),
    *sigcomp_out = dogecoin_uchar_vla(65);
    int ret = b64_decode(signature_encoded, out, outlen);
	if (!ret) {
        printf("b64_decode failed!\n");
        return false;
    }

    int recid = sig->recid;
    if (recid != 0) {
        outlen -= 2;
    }

    ret = dogecoin_ecc_der_to_compact(out, outlen, sigcomp_out);
    if (!ret) {
        printf("ecc der to compact failed!\n");
        return false;
    }

    dogecoin_pubkey pubkey;
    dogecoin_pubkey_init(&pubkey);
    pubkey.compressed = false;


    // recover pubkey
    ret = dogecoin_key_sign_recover_pubkey((const unsigned char*)sigcomp_out, message_bytes, recid, &pubkey);
    dogecoin_free(sigcomp_out);
    if (!ret) {
        printf("key sign recover failed!\n");
        return false;
    }

    ret = dogecoin_pubkey_verify_sig(&pubkey, message_bytes, out, outlen);
    if (!ret) {
        printf("pubkey sig verification failed!\n");
        return false;
    }
    dogecoin_free(out);

    char* p2pkh_address = dogecoin_char_vla(34 + 1);
    const dogecoin_chainparams* chain = chain_from_b58_prefix(sig->address);
    ret = dogecoin_pubkey_getaddr_p2pkh(&pubkey, chain, p2pkh_address);
    if (!ret) {
        printf("derived address from pubkey failed!\n");
        return false;
    }
    return p2pkh_address;
}
