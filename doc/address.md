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

**To Generate:**

* generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)

This function will populate provided private/public char* variables with a freshly
generated private/public key pair, specifically for the network specified by the
third parameter. 

_Python usage:_
```py
import libdogecoin

# generatePrivPubKeypair returns a two-touple of pub/priv key strings
keys = libdogecoin.generatePrivPubKeypair()
priv, pub = keys
print(priv, pub)
```

**To Validate:**

* validatePrivPubKeypair ?  TODO: dogecoin_pubkey_is_valid could be wrapped here

#### The Details

The first step to create a simple address is to generate the private key, this is
a randomly generated 32 byte unsigned integer (256 bit).  

Secondly using the private key, TODO: details 

----

## HD Addresses

HD or _Hierarchical Deterministic_ addresses, unlike Simple Addresses are created 
from a seed key-phrase, usually twelve random unqiue words rather than a 256 bit 
private key. The key-phrase is then used (with the addition of a simple, 
incrementing number), to generate an unlimited supply of public Dogecoin Addresses
which the holder of the key-phrase can sign. 

#### Essential APIs

The essential functions provided by Libdogecoin for working with HD Addresses are 
found in `include/dogecoin/address.h` and are:

** To Generate: **

* generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)

This function 

* generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey) 

** To Validate: **

* TODO

#### The Details

----

## MultiSig Addresses

MultiSig Addresses are TODO
