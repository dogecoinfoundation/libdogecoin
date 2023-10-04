/*

 The MIT License (MIT)

 Copyright (c) 2023 bluezr, edtubbs
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

void hash_message(char* msg, uint256 message_bytes) {
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 7);
    tmp_msg[0] = msg_magic_len;
    memcpy_safe(tmp_msg + 1, msg_magic, msg_magic_len);
    tmp_msg[msg_magic_len + 1] = msg_len;
    memcpy_safe(tmp_msg + msg_magic_len + 2, msg, msg_len + 1);
    dogecoin_hash((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);
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
char* sign_message(char* privkey, char* msg) {
    if (!privkey || !msg) return false;

    uint256 message_bytes;
    hash_message(msg, message_bytes);

    size_t compact_signature_length = 65;
    unsigned char* compact_signature = dogecoin_uchar_vla(compact_signature_length);

    dogecoin_key key;
    dogecoin_pubkey pubkey;
    if (!init_keypair(privkey, &key, &pubkey)) return false;

    int recid = -1;

    if (!dogecoin_key_sign_hash_compact_recoverable_fcomp(&key, message_bytes, compact_signature, &compact_signature_length, &recid)) return false;
    if (!dogecoin_key_recover_pubkey((const unsigned char*)compact_signature, message_bytes, recid, &pubkey)) return false;

    char p2pkh_address[P2PKHLEN];
    if (!dogecoin_pubkey_getaddr_p2pkh(&pubkey, &dogecoin_chainparams_main, p2pkh_address)) return false;

    unsigned char* base64_encoded_output = dogecoin_uchar_vla(1+(sizeof(char)*base64_encoded_size(compact_signature_length)));
    base64_encode((unsigned char*)compact_signature, compact_signature_length, base64_encoded_output);

    dogecoin_free(compact_signature);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_cleanse(&pubkey);

    return (char*)base64_encoded_output;
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
int verify_message(char* sig, char* msg, char* address) {
    if (!(sig || msg || address)) return false;

    uint256 message_bytes;
    hash_message(msg, message_bytes);

    size_t encoded_length = strlen((const char*)sig);
    unsigned char* decoded_signature = dogecoin_uchar_vla(base64_decoded_size(encoded_length+1)+1);
    base64_decode((unsigned char*)sig, encoded_length, decoded_signature);

    dogecoin_pubkey pub_key;
    dogecoin_pubkey_init(&pub_key);
    pub_key.compressed = false;

    int header = decoded_signature[0] & 0xFF;

    if (header < 27 || header > 42) return false;

    if (header >= 31) {
        pub_key.compressed = true;
        header -= 4;
    }

    int recid = header - 27;
    if (!dogecoin_key_recover_pubkey((const unsigned char*)decoded_signature, message_bytes, recid, &pub_key)) return false;
    if (!dogecoin_pubkey_verify_sigcmp(&pub_key, message_bytes, decoded_signature)) return false;

    char p2pkh_address[P2PKHLEN];
    const dogecoin_chainparams* chain = chain_from_b58_prefix(address);
    if (!dogecoin_pubkey_getaddr_p2pkh(&pub_key, chain, p2pkh_address)) return false;

    dogecoin_free(decoded_signature);
    dogecoin_pubkey_cleanse(&pub_key);
    return strcmp(p2pkh_address, address)==0;
}
