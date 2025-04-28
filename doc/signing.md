# Libdogecoin Signing API

## Table of Contents
- [Libdogecoin Signing API](#libdogecoin-signing-api)
  - [Table of Contents](#table-of-contents)
  - [Abstract](#abstract)
  - [Basic Signing API](#basic-signing-api)
    - [**sign_message:**](#sign_message)
    - [**verify_message:**](#verify_message)

## Abstract

This document describes the process of message signing within libdogecoin. It aims to meet the standards defined in [BIP-137](https://github.com/bitcoin/bips/blob/master/bip-0137.mediawiki) although the implementation is only applicable to P2PKH addresses.

## Basic Signing API

---

### **sign_message:**

```c
char* sign_message(char* privkey, char* msg) {
    if (!privkey || !msg) return false;

    uint256_t message_bytes;
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
```

This function signs a message with a private key.

_C usage:_
```c
char* sig = sign_message("QUtnMFjt3JFk1NfeMe6Dj5u4p25DHZA54FsvEFAiQxcNP4bZkPu2", "This is just a test message");
```

---

### **verify_message:**

```c
int verify_message(char* sig, char* msg, char* address) {
    if (!(sig || msg || address)) return false;

    uint256_t message_bytes;
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
```

This function verifies a signed message using a P2PKH address.

_C usage:_
```c
char* address = "DA8aeVkgQWwo78y3VCXtLqoWe8uWRoFuc1";
int ret = verify_message(sig, msg, address);
if (!ret) {
  return false;
}
```