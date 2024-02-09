# Libdogecoin Elliptic Curve Key API

## Table of Contents
- [Libdogecoin Elliptic Curve Key API](#libdogecoin-elliptic-curve-key-api)
  - [Table of Contents](#table-of-contents)
  - [Abstract](#abstract)
  - [Specification](#specification)
  - [Primitives](#primitives)
  - [Basic Elliptic Curve Key API](#basic-elliptic-curve-key-api)
    - [**eckey:**](#eckey)
    - [**keys:**](#keys)
    - [**new_eckey:**](#new_eckey)
    - [**new_eckey_from_privkey:**](#new_eckey_from_privkey)
    - [**add_eckey:**](#add_eckey)
    - [**start_key:**](#start_key)
    - [**find_eckey:**](#find_eckey)
    - [**remove_eckey:**](#remove_eckey)
    - [**dogecoin_key_free:**](#dogecoin_key_free)

## Abstract

This document explains the basic elliptic curve key API within libdogecoin.

## Specification

Cited from [https://en.bitcoin.it/wiki/Elliptic_Curve_Digital_Signature_Algorithm](https://en.bitcoin.it/wiki/Elliptic_Curve_Digital_Signature_Algorithm)

### Background on ECDSA Signatures

Elliptic Curve Digital Signature Algorithm or ECDSA is a cryptographic algorithm used by Dogecoin to ensure that funds can only be spent by their rightful owners. It is dependent on the curve order and hash function used. For dogecoin these are Secp256k1 and SHA256(SHA256()) respectively.

A few concepts related to ECDSA:

- `private key`: A secret number, known only to the person that generated it. A private key is essentially a randomly generated number. In Dogecoin, someone with the private key that corresponds to funds on the blockchain can spend the funds. In Dogecoin, a private key is a single unsigned 256 bit integer (32 bytes).
- `public key`: A number that corresponds to a private key, but does not need to be kept secret. A public key can be calculated from a private key, but not vice versa. A public key can be used to determine if a signature is genuine (in other words, produced with the proper key) without requiring the private key to be divulged. In Dogecoin, public keys are either compressed or uncompressed. Compressed public keys are 33 bytes, consisting of a prefix either 0x02 or 0x03, and a 256-bit integer called x. The older uncompressed keys are 65 bytes, consisting of constant prefix (0x04), followed by two 256-bit integers called x and y (2 * 32 bytes). The prefix of a compressed key allows for the y value to be derived from the x value.
- `signature`: A number that proves that a signing operation took place. A signature is mathematically generated from a hash of something to be signed, plus a private key. The signature itself is two numbers known as r and s. With the public key, a mathematical algorithm can be used on the signature to determine that it was originally produced from the hash and the private key, without needing to know the private key. Resulting signatures are either 73, 72, or 71 bytes long (with approximate probabilities of 25%, 50%, and 25%, respectively--although sizes even smaller than that are possible with exponentially decreasing probability).

### Primitives

The ECDSA signing and verification algorithms make use of a few fundamental variables which are used to obtain a signature and the reverse process of getting a message from a signature.

- `r` and `s`: These numbers uniquely represent the signature.
- `z`: The hash of the message we want to sign. Normally we are required to use the left-most N bits of the message hash, where `N` is the length of the hash function used, however, this rule does not apply to dogecoin signatures because the length of the hash function used, SHA256, equals the bit length of the secp256k1 curve (256) so no truncation is necessary.
- `k`: A cryptographicly secure random number which is used as a nonce to calculate the `r` and `s` values.
- `dA` and `QA`: These are the private key number and public key point respectively, used to sign and verify the message. Wallets can derive a copy of these when give an address contained inside the wallet.

### Verification Algorithm

The verification algorithm ensures that the signature pair `r` and `s`, `QA` and `z` are all consistent.

- Verify that both `r` and `s` are between `1` and `n-1`.
- Compute `u1 = z*s-1 mod n` and `u2 = r*s-1 mod n`.
- Compute `(x, y) = u1*G + u2*QA` and ensure it is not equal to the point at infinity. The point at infinity is a special point that results when you add two points whose result would otherwise not lie on the curve, such as two points with the same `X` value but inverted `Y` values.
- If `r = x mod n` then the signature is valid. Otherwise, or if any of the checks fail, then the signature is invalid.

## Basic Elliptic Curve Key API

---
### **eckey:**

```
typedef struct eckey {
    int idx;
    dogecoin_key private_key;
    char private_key_wif[PRIVKEYWIFLEN];
    dogecoin_pubkey public_key;
    char public_key_hex[PUBKEYHEXLEN];
    char address[P2PKHLEN];
    UT_hash_handle hh;
} eckey;
```

---
### **keys:**

```
static eckey *keys = NULL;
```

This is an empty collection of key structures and meant for internal consumption.

---

### **new_eckey:**

```c
eckey* new_eckey(dogecoin_bool is_testnet) {
    eckey* key = (struct eckey*)dogecoin_calloc(1, sizeof *key);
    dogecoin_privkey_init(&key->private_key);
    assert(dogecoin_privkey_is_valid(&key->private_key) == 0);
    dogecoin_privkey_gen(&key->private_key);
    assert(dogecoin_privkey_is_valid(&key->private_key)==1);
    dogecoin_pubkey_init(&key->public_key);
    dogecoin_pubkey_from_key(&key->private_key, &key->public_key);
    assert(dogecoin_pubkey_is_valid(&key->public_key) == 1);
    strcpy(key->public_key_hex, utils_uint8_to_hex((const uint8_t *)&key->public_key, 33));
    uint8_t pkeybase58c[34];
    const dogecoin_chainparams* chain = is_testnet ? &dogecoin_chainparams_test : &dogecoin_chainparams_main;
    pkeybase58c[0] = chain->b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */
    memcpy_safe(&pkeybase58c[1], &key->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    assert(dogecoin_base58_encode_check(pkeybase58c, sizeof(pkeybase58c), key->private_key_wif, sizeof(key->private_key_wif)) != 0);
    if (!dogecoin_pubkey_getaddr_p2pkh(&key->public_key, chain, (char*)&key->address)) return false;
    key->idx = HASH_COUNT(keys) + 1;
    return key;
}
```

This function instantiates a new working key, but does not add it to the hash table.

_C usage:_
```c
eckey* key = new_eckey(false);
```

---

### **new_eckey_from_privkey:**

```c
eckey* new_eckey_from_privkey(char* private_key) {
    eckey* key = (struct eckey*)dogecoin_calloc(1, sizeof *key);
    dogecoin_privkey_init(&key->private_key);
    const dogecoin_chainparams* chain = chain_from_b58_prefix(private_key);
    if (!dogecoin_privkey_decode_wif(private_key, chain, &key->private_key)) return false;
    assert(dogecoin_privkey_is_valid(&key->private_key)==1);
    dogecoin_pubkey_init(&key->public_key);
    dogecoin_pubkey_from_key(&key->private_key, &key->public_key);
    assert(dogecoin_pubkey_is_valid(&key->public_key) == 1);
    strcpy(key->public_key_hex, utils_uint8_to_hex((const uint8_t *)&key->public_key, 33));
    uint8_t pkeybase58c[34];
    pkeybase58c[0] = chain->b58prefix_secret_address;
    pkeybase58c[33] = 1; /* always use compressed keys */
    memcpy_safe(&pkeybase58c[1], &key->private_key, DOGECOIN_ECKEY_PKEY_LENGTH);
    assert(dogecoin_base58_encode_check(pkeybase58c, sizeof(pkeybase58c), key->private_key_wif, sizeof(key->private_key_wif)) != 0);
    if (!dogecoin_pubkey_getaddr_p2pkh(&key->public_key, chain, (char*)&key->address)) return false;
    key->idx = HASH_COUNT(keys) + 1;
    return key;
}
```

This function instantiates a new working key from a `private_key` in WIF format, but does not add it to the hash table.

_C usage:_
```c
char* privkey = "QUtnMFjt3JFk1NfeMe6Dj5u4p25DHZA54FsvEFAiQxcNP4bZkPu2";
eckey* key = new_eckey_from_privkey(privkey);
...

dogecoin_free(key);
```

---

### **add_eckey:**

```c
void add_eckey(eckey *key) {
    eckey* key_old;
    HASH_FIND_INT(keys, &key->idx, key_old);
    if (key_old == NULL) {
        HASH_ADD_INT(keys, idx, key);
    } else {
        HASH_REPLACE_INT(keys, idx, key, key_old);
    }
    dogecoin_free(key_old);
}
```

This function takes a pointer to an existing working eckey object and adds it to the hash table.

_C usage:_
```c
eckey* key = new_eckey(false);
add_eckey(key);
```

---

### **start_key:**

```c
int start_key(dogecoin_bool is_testnet) {
    eckey* key = new_eckey(is_testnet);
    int index = key->idx;
    add_eckey(key);
    return index;
}
```

This function creates a new eckey, places it in the hash table, and returns the index of the new eckey, starting from 1 and incrementing each subsequent call.

_C usage:_
```c
int key_id = start_key(false);
```

---

### **find_eckey:**

```c
eckey* find_eckey(int idx) {
    eckey* key;
    HASH_FIND_INT(keys, &idx, key);
    return key;
}
```

This function takes an index and returns the working eckey associated with that index in the hash table.

_C usage:_
```c
...
int key_id = start_key(false);
eckey* key = find_eckey(key_id);
...
```

---

### **remove_eckey:**

```c
void remove_eckey(eckey* key) {
    HASH_DEL(keys, key); /* delete it (keys advances to next) */
    dogecoin_privkey_cleanse(&key->private_key);
    dogecoin_pubkey_cleanse(&key->public_key);
    dogecoin_key_free(key);
}
```

This function removes the specified working eckey from the hash table and frees the eckey in memory.

_C usage:_
```c
int key_id = start_key(false);
eckey* key = find_eckey(key_id);
remove_eckey(key)
```

---

### **dogecoin_key_free:**

```c
void dogecoin_key_free(eckey* eckey)
{
    dogecoin_free(eckey);
}
```

This function frees the memory allocated for an eckey.

_C usage:_
```c
...
dogecoin_key_free(key);
...
```
