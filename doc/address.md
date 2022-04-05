# Dogecoin Addresses

A Dogecoin Address is the Public Key of an assymetric key-pair, and provides
a designation on the network to hold Dogecoins on the ledger. By holding the
Private Key for an Address on the Ledger you have the ability to sign 
Transactions to move (spend/send) the Dogecoins attributed to the Address.

Dogecoin Addresses can be created in a number of ways, each with benefits
depending on the objective of the Address owner(s). 

## Simple Addresses

These are the most common form of Dogecoin Address, a single Private Key with
a single Public Key (Address). Most often these are what an exchange would use
to retain full custody of a wallet on behalf of their users, or for a simple
desktop / mobile wallet they provide a basic approach to Address management.

#### Essential APIs

The High level 'essential' API provided by Libdogecoin for working with Simple 
Addresses are found in `include/dogecoin/address.h` and are:

**generatePrivPubKeypair:**

`generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)`

This function will populate provided private/public char* variables with a freshly
generated private/public key pair, specifically for the network specified by the
third parameter. 


_C usage:_
```C
#include <dogecoin/address.h>

int keyLen = 100; 
char privKey[keyLen]; 
char pubKey[keyLen]; 
generatePrivPubKeypair(privKey, pubKey, false);

printf("private key: %s\n", privKey);
printf("public key: %s\n", pubKey);
```

_Python usage:_
```py
import libdogecoin

keys = libdogecoin.generate_priv_pub_key_pair()
priv, pub = keys

print("private key:", priv)
print("public key:", pub)
```

---

**verifyPrivPubKeypair**

`verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)`

This function validates that a given private key matches a given public key. This 
could be useful prior to signing, or in some kind of wallet recovery tool to match
keys.

_C usage:_
```C
int keyLen = 100;
char privKey[keyLen];
char pubKey[keyLen];
generatePrivPubKeypair(privKey, pubKey, false);

if (verifyPrivPubKeypair(privKey, pubKey, false)) {
  printf("Keypair is valid.\n");
}
else {
  printf("Keypair is invalid.\n");
}
```

_Python usage:_
```py
import libdogecoin

keys = libdogecoin.generate_priv_pub_key_pair()
priv, pub = keys

if libdogecoin.verify_priv_pub_keypair(priv, pub):
  print("Keypair is valid.")
else:
  print("Keypair is invalid.")

```

---

**verifyP2pkhAddress**

`verifyP2pkhAddress(char* p2pkh_pubkey, uint8_t len)`

This function accepts a public key and does some basic validation to determine
if it looks like a valid Dogecoin Address. This is useful for wallets that want
to check that a recipient address looks legitimate.

_C usage:_
```C
int keyLen = 100;
char privKey[keyLen];
char pubKey[keyLen];
generatePrivPubKeypair(privKey, pubKey, false);

if (verifyP2pkhAddress(pubKey, strlen(pubKey))) {
  printf("Address is valid.\n");
}
else {
  printf("Address is invalid.\n");
}
```

_Python usage:_
```py
import libdogecoin

keys = libdogecoin.generate_priv_pub_key_pair()
priv, pub = keys

if verify_p2pkh_address(pub):
  print("Address is valid.")
else:
  print("Address is invalid.")
```

---

#### The Details

The first step to create a simple address is to generate the private key, this is
a randomly generated 32 byte unsigned integer (256 bit).  

Secondly using the private key, TODO: details 

----

## HD Addresses

HD or _Hierarchical Deterministic_ addresses, unlike Simple Addresses are created 
from a seed key-phrase, usually twelve random unique words rather than a 256 bit 
private key. The key-phrase is then used (with the addition of a simple, 
incrementing number), to generate an unlimited supply of public Dogecoin Addresses
which the holder of the key-phrase can sign. 

#### Essential APIs

The essential functions provided by Libdogecoin for working with HD Addresses are 
found in `include/dogecoin/address.h` and are:


**generateHDMasterPubKeypair:**

`generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)`

This function will populate provided private/public char* variables with a freshly
generated master key pair for a heirarchical deterministic wallet, specifically 
for the network specified by the third parameter. 

_C usage:_
```C
#include <dogecoin/address.h>

int keyLen = 200; 
char masterPrivKey[keyLen]; 
char masterPubKey[keyLen]; 
generatePrivPubKeypair(masterPrivKey, masterPubKey, false);

printf("master private key: %s\n", masterPrivKey);
printf("master public key: %s\n", masterPubKey);
```

_Python usage:_
```py
import libdogecoin

mkeys = libdogecoin.generate_hd_master_pub_key_pair()
mpriv, mpub = mkeys

print("master private key:", mpriv)
print("master public key:", mpub)
```

---

**generateDerivedHDPubKey:**

`generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey)`

This function will populate the provided public char* variable with a child 
key derived from the provided wif-encoded master private key string. This 
input should come from the result of generateHDMasterPubKeypair(). 

_C usage:_
```C
#include <dogecoin/address.h>

int keyLen = 200; 
char masterPrivKey[keyLen];
char masterPubKey[keyLen];
char childPubKey[keyLen];
generateHDMasterPubKeypair(masterPrivKey, masterPubKey, false);
generateDerivedHDPubkey(masterPrivKey, childPubKey);

printf("master private key: %s\n", masterPrivKey);
printf("master public key: %s\n", masterPubKey);
printf("derived child key: %s\n", childKey);
```

_Python usage:_
```py
import libdogecoin

parent_keys = libdogecoin.generate_hd_master_pub_key_pair()
mpriv, mpub = parent_keys
child_key = libdogecoin.generate_derived_hd_pub_key(mpriv)

print("master private key:", mpriv)
print("master public key:", mpub)
print("derived child key:", child_key)
```

---

**verifyHDMasterPubKeypair**

`verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)`

This function validates that a given master private key matches a given
master public key. This could be useful prior to signing, or in some
kind of wallet recovery tool to match keys.

_C usage:_
```C
int keyLen = 200;
char masterPrivKey[keyLen];
char masterPubKey[keyLen];
generateHDMasterPubKeypair(masterPrivKey, masterPubKey, false);

if (verifyHDMasterPubKeypair(masterPrivKey, masterPubKey, false)) {
  printf("Keypair is valid.\n");
}
else {
  printf("Keypair is invalid.\n");
}
```

_Python usage:_
```py
import libdogecoin

keys = libdogecoin.generate_hd_master_pub_key_pair()
priv, pub = keys

if libdogecoin.verify_master_priv_pub_keypair(priv, pub):
  print("Keypair is valid.")
else:
  print("Keypair is invalid.")

```

#### The Details

TODO

----

## MultiSig Addresses

MultiSig Addresses are TODO
