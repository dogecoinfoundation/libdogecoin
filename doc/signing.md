# Libdogecoin Signing API

## Table of Contents
- [Libdogecoin Signing API](#libdogecoin-signing-api)
  - [Table of Contents](#table-of-contents)
  - [Abstract](#abstract)
  - [Specification](#specification)
  - [Basic Signing API](#basic-signing-api)
    - [**signmsgwithprivatekey:**](#signmsgwithprivatekey)
    - [**verifymessage:**](#verifymessage)
    - [**signmsgwitheckey:**](#signmsgwitheckey)
    - [**verifymessagewithsig:**](#verifymessagewithsig)
  - [Advanced Signing API](#advanced-signing-api)
    - [**signature:**](#signature)
    - [**signatures:**](#signatures)
    - [**new_signature:**](#new_signature)
    - [**add_signature:**](#add_signature)
    - [**find_signature:**](#find_signature)
    - [**remove_signature:**](#remove_signature)
    - [**start_signature:**](#start_signature)
    - [**free_signature:**](#free_signature)

## Abstract

This document describes the process of message signing within libdogecoin. It aims to meet the standards defined in [BIP-137](https://github.com/bitcoin/bips/blob/master/bip-0137.mediawiki) although the implementation is only applicable to P2PKH addresses.

## Specification

One caveat that differentiates this implementation from others is that if the `recid` is not equal to 0 it will be appended to the end of the signature prior to the base64 encoding and decoding steps as this allows the public key to be retrieved successfully on the client side, therefore a modicum of caution should be expressed when dealing with publicly accessible base64 encoded strings, addresses, et al.

## Basic Signing API

---

### **signmsgwithprivatekey:**

```c
char* signmsgwithprivatekey(char* privkey, char* msg) {
    if (!privkey || !msg) return false;

    // vars for signing
    size_t outlen = 74, outlencmp = 64;
    unsigned char *sig = dogecoin_uchar_vla(outlen),
    *sigcmp = dogecoin_uchar_vla(outlencmp);

    // retrieve double sha256 hash of msg:
    uint256 message_bytes;
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 1);
    memcpy_safe(tmp_msg, msg_magic, msg_magic_len);
    memcpy_safe(tmp_msg + msg_magic_len, msg, msg_len + 1);
    int ret = dogecoin_dblhash((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);
    if (!ret) {
        printf("double sha256 hash failed\n");
        return false;
    }

    dogecoin_key key;
    dogecoin_privkey_init(&key);
    assert(dogecoin_privkey_is_valid(&key) == 0);
    const dogecoin_chainparams* chain = chain_from_b58_prefix(privkey);
    ret = dogecoin_privkey_decode_wif(privkey, chain, &key);
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

    dogecoin_free(sigcmp);
    dogecoin_privkey_cleanse(&key);
    dogecoin_pubkey_cleanse(&pubkey);

    // base64 encode output and free sig:
    char* out = b64_encode(sig, outlen);
    dogecoin_free(sig);
    return out;
}
```

This function signs a message with a private key.

_C usage:_
```c
char* sig = signmsgwithprivatekey("QUtnMFjt3JFk1NfeMe6Dj5u4p25DHZA54FsvEFAiQxcNP4bZkPu2", "This is just a test message");
```

---

### **verifymessage:**

```c
int verifymessage(char* sig, char* msg, char* address) {
    if (!(sig || msg || address)) return false;

    // double sha256 hash message:
    uint256 message_bytes;
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 1);
    memcpy_safe(tmp_msg, msg_magic, msg_magic_len);
    memcpy_safe(tmp_msg + msg_magic_len, msg, msg_len + 1);
    int ret = dogecoin_dblhash((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);
    if (!ret) {
        printf("message_bytes failed\n");
        return false;
    }

	size_t outlen = b64_decoded_size(sig);
    unsigned char *out = dogecoin_uchar_vla(outlen),
    *sigcomp_out = dogecoin_uchar_vla(65);
    ret = b64_decode(sig, out, outlen);
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
    dogecoin_free(sigcomp_out);
    if (!ret) {
        printf("key sign recover failed!\n");
        return false;
    }
    ret = dogecoin_pubkey_verify_sig(&pub_key, message_bytes, out, outlen);
    dogecoin_free(out);
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
    dogecoin_pubkey_cleanse(&pub_key);

    ret = strcmp(p2pkh_address, address)==0;
    dogecoin_free(p2pkh_address);
    return ret;
}
```

This function verifies a signed message using non BIP32 derived address.

_C usage:_
```c
char* address = "DA8aeVkgQWwo78y3VCXtLqoWe8uWRoFuc1";
int ret = verifymessage(sig, msg, address);
if (!ret) {
  return false;
}
```

---

### **signmsgwitheckey:**

```c
signature* signmsgwitheckey(eckey* key, char* msg) {
    if (!key || !msg) return false;

    // retrieve double sha256 hash of msg:
    uint256 message_bytes;
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 1);
    memcpy_safe(tmp_msg, msg_magic, msg_magic_len);
    memcpy_safe(tmp_msg + msg_magic_len, msg, msg_len + 1);
    int ret = dogecoin_dblhash((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);
    if (!ret) {
        printf("message_bytes failed!\n");
        return false;
    }

    // vars for signing
    size_t outlen = 74, outlencmp = 64;
    unsigned char *sig = dogecoin_uchar_vla(outlen),
    *sigcmp = dogecoin_uchar_vla(outlencmp);

    // sign hash
    ret = dogecoin_key_sign_hash(&key->private_key, message_bytes, sig, &outlen);
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
    dogecoin_free(sigcmp);

    // base64 encode output and free sig:
    working_sig->content = b64_encode(sig, outlen);

    dogecoin_free(sig);

    return working_sig;
}
```

This function signs a message but with `eckey` key structure.

_C usage:_
```c
...
int key_id = start_key(false);
eckey* key = find_eckey(key_id);
char* msg = "This is a test message";
signature* sig = signmsgwitheckey(key, msg);
...
```

---

### **verifymessagewithsig:**

```c
char* verifymessagewithsig(signature* sig, char* msg) {
    if (!(sig || msg)) return false;

    char* signature_encoded = sig->content;
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

    // double sha256 hash message:
    uint256 message_bytes;
    size_t msg_magic_len = strlen(msg_magic), msg_len = strlen(msg);
    char* tmp_msg = dogecoin_char_vla(msg_magic_len + msg_len + 1);
    memcpy_safe(tmp_msg, msg_magic, msg_magic_len);
    memcpy_safe(tmp_msg + msg_magic_len, msg, msg_len + 1);
    ret = dogecoin_dblhash((const unsigned char*)tmp_msg, strlen(tmp_msg), message_bytes);
    dogecoin_free(tmp_msg);
    if (!ret) {
        printf("message_bytes failed\n");
        return false;
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
```

This function verifies a signed message structure.

_C usage:_
```c
...
char* address = verifymessagewithsig(sig, msg);
u_assert_str_eq(address, sig->address);
remove_eckey(key);
free_signature(sig);
dogecoin_free(address);
...
```


## Advanced Signing API

---
### **signature:**

```
typedef struct signature {
    int idx;
    char* content;
    char address[35];
    int recid;
    UT_hash_handle hh;
} signature;
```

This structure is the base for the signing API. It includes an index (`int idx`), a char* for holding the base64 encoded signature (`char* content`), an address field for holding the corresponding address (`char address[35]`), the recid which allows public key recovery (`int recid`) and finally the hash handle which is specific to using the hash table (`UT_hash_handle hh`).

_C usage:_
```c
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  dogecoin_ecc_start();
  int key_id2 = start_key();
  eckey* key2 = find_eckey(key_id2);
  char* msg2 = "This is a test message";
  signature* sig2 = signmsgwitheckey(key2, msg2);
  char* address2 = verifymessagewithsig(sig2, msg2);
  u_assert_str_eq(address2, sig2->address);
  dogecoin_free(address2);
  dogecoin_ecc_stop();
}
```

---
### **signatures:**

```
static signature *signatures = NULL;
```

This is an empty collection of signature structures and meant for internal consumption.

_C usage:_
```c
```

---

### **new_signature:**

```c
signature* new_signature() {
    signature* sig = (struct signature*)dogecoin_calloc(1, sizeof *sig);
    sig->recid = 0;
    sig->idx = HASH_COUNT(signatures) + 1;
    return sig;
}
```

This function instantiates a new working signature, but does not add it to the hash table.

_C usage:_
```c
signature* sig = new_signature();
```

---

### **add_signature:**

```c
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
```

This function takes a pointer to an existing working signature object and adds it to the hash table.

_C usage:_
```c
signature* sig = new_signature();
add_signature(sig);
```

---

### **start_signature:**

```c
int start_signature() {
    signature* sig = new_signature();
    int index = sig->idx;
    add_signature(sig);
    return index;
}
```

This function creates a new signature, places it in the hash table, and returns the index of the new signature, starting from 1 and incrementing each subsequent call.

_C usage:_
```c
int sig_id = start_signature();
```

---

### **find_signature:**

```c
signature* find_signature(int idx) {
    signature* sig;
    HASH_FIND_INT(signatures, &idx, sig);
    return sig;
}
```

This function takes an index and returns the working signature associated with that index in the hash table.

_C usage:_
```c
...
int sig_id = start_signature();
signature* sig = find_signature(sig_id);
...
```

---

### **remove_signature:**

```c
void remove_signature(signature* sig) {
    HASH_DEL(signatures, sig); /* delete it (signatures advances to next) */
    sig->recid = 0;
    dogecoin_free(sig);
}
```

This function removes the specified working signature from the hash table and frees the signatures in memory.

_C usage:_
```c
int sig_id = start_signature();
signature* sig = find_signature(sig_id);
remove_signature(sig)
```

---

### **free_signature:**

```c
void free_signature(signature* sig)
{
    dogecoin_free(sig->content);
    dogecoin_free(sig);
}
```

This function frees the memory allocated for an signature.

_C usage:_
```c
...
int key_id2 = start_key();
eckey* key2 = find_eckey(key_id2);
char* msg2 = "This is a test message";
signature* sig2 = signmsgwitheckey(key2, msg2);
char* address2 = verifymessagewithsig(sig2, msg2);
u_assert_str_eq(address2, sig2->address);
remove_eckey(key2);
free_signature(sig2);
dogecoin_free(address2);
...
```
