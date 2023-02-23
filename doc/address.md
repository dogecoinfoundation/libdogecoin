# Libdogecoin Address API

## Table of Contents
- [Libdogecoin Address API](#libdogecoin-address-api)
  - [Table of Contents](#table-of-contents)
  - [Introduction](#introduction)
    - [Simple Addresses](#simple-addresses)
    - [HD Addresses](#hd-addresses)
    - [Seed phrases](#seed-phrases)
  - [Simple Address Generation Process](#simple-address-generation-process)
  - [Essential Address API](#essential-address-api)
    - [**generatePrivPubKeypair:**](#generateprivpubkeypair)
    - [**generateHDMasterPubKeypair:**](#generatehdmasterpubkeypair)
    - [**generateDerivedHDPubKey:**](#generatederivedhdpubkey)
    - [**verifyPrivPubKeypair**](#verifyprivpubkeypair)
    - [**verifyHDMasterPubKeypair**](#verifyhdmasterpubkeypair)
    - [**verifyP2pkhAddress**](#verifyp2pkhaddress)
    - [**getDerivedHDAddress**](#getderivedhdaddress)
    - [**getDerivedHDAddressByPath**](#getderivedhdaddressbypath)
  - [Advanced Address API](#advanced-address-api)
    - [**generateRandomEnglishMnemonic:**](#generateRandomEnglishMnemonic)
    - [**getDerivedHDAddressFromMnemonic:**](#getDerivedHDAddressFromMnemonic)

## Introduction

A Dogecoin address is the public key of an asymmetric key pair, and provides a designation on the network to hold Dogecoins on the ledger. By holding the private key for an address on the ledger, you have the ability to sign transactions to move (spend/send) the Dogecoins attributed to the address.

Dogecoin addresses can be created in a number of ways, each with benefits depending on the objective of the address owner(s). A couple of these are listed below:

### Simple Addresses

These are the most common form of Dogecoin address, a single private key with a single public key (address). Most often these are what an exchange would use to retain full custody of a wallet on behalf of their users, or for a simple desktop / mobile wallet they provide a basic approach to address management.

### HD Addresses

HD, or _Hierarchical Deterministic_ addresses, unlike simple addresses are created from a seed key-phrase, usually twelve random unique words rather than a 256-bit private key. The key-phrase is then used (with the addition of a simple, incrementing number) to generate an unlimited supply of public Dogecoin addresses which the holder of the key-phrase can sign.

### Seed phrases

Seed phrases, also known as mnemonic phrases, are a commonly used method to create and backup HD addresses. These phrases consist of 12 or 24 random words, generated from a 128 or 256-bit entropy source, respectively. Seed phrases can then be used to generate an HD address.

When using seed phrases to generate HD addresses, it is essential to ensure the security of the seed phrase. Seed phrases should be treated with extreme caution and must be stored safely in a secure location. There are several steps that you can take to ensure that your seed phrase remains secure:
- Do not type your seed phrase into any device connected to the internet, as it could be compromised by malware or keyloggers.
- Store your seed phrase offline in a secure and tamper-evident location, such as a hardware wallet, paper wallet or encrypted digital storage device.
- Do not share your seed phrase with anyone, as it grants access to all the funds in your wallet.
- Use a strong, unique and memorable passphrase to protect your seed phrase from unauthorized access.

When embedding libdogecoin in your project, it is important to consider the wallet type you are using. HD wallets, which use seed phrases to generate addresses, are generally considered more secure than simple wallets, which use a single private key for all addresses. Hot wallets, which are connected to the internet, are more convenient to use but are also more vulnerable to attacks. Cold wallets, which are kept offline, are more secure but may be less convenient to use for frequent transactions. You should carefully consider your security needs and choose the appropriate wallet type for your project.

## Simple Address Generation Process

The first step to create a simple pay-to-public-key-hash (p2pkh) address is to generate the private key, which is a randomly generated 256-bit (32-byte) unsigned integer. This must be done within a secp256k1 context in order to generate a private key, as this context ensures the key's validity can be mathematically proven by other validators.

This random private key is then encoded into wallet import format (WIF) which makes the private key readable and easier to copy. If viewing this private key, **DO NOT REVEAL IT TO ANYONE ELSE**! This private key grants you the ability to spend any Dogecoin sent to any public key associated with this private key, and sharing it will compromise all funds in your wallet.

The next step in the process is deriving the public key from the private key, using the mathematical properties of the secp256k1 elliptical curve (this is done internally by Libdogecoin). This should be done within the same context that was created at the beginning of the session, because it is expensive to regenerate all the cryptographic information necessary and this extraneous computation should be avoided.

One thing to note is that this public key is **NOT** the same as a Dogecoin address. The final step in the address generation process is to convert the public key (in the form of seemingly random bytes) into a readable, valid Dogecoin address. The steps for achieving this are as follows:
- Pass the public key once through a SHA256 hash function.
- Pass the result of this hash into a RIPEMD-160 hash function.
- Prefix this RIPEMD-160 result with the character associated with the specified blockchain. _('D' for mainnet, 'n' for testnet)_
- Double-hash the current string (prefix + RIPEMD-160 result) with SHA256 and save the result as a 32-byte checksum.
- Append the first 4 bytes of this checksum to the current string.
- Encode the current string with base58 to get the final Dogecoin p2pkh address.

You now have a valid Dogecoin address! Its validity can be checked using the verifyPrivPubKeypair() function, which assesses whether the checksum holds after the address undergoes base58 decoding. Keep in mind, the public key cannot be derived from this p2pkh address because it was passed through irreversible hash functions. However, this address can now be used to receive funds and eventually spend them using the private key generated in the beginning.

These steps can be very confusing, so Libdogecoin does all of them for you when you call `generatePrivPubKeyPair()`, from private key generation to public key derivation to p2pkh address conversion. The process is very similar for HD key pair generation but with a little more complexity involved in generating the raw key pair. For more information on how to call the functions that perform the process listed above, see the Essential API description below. 

## Essential Address API

These functions implement the core functionality of Libdogecoin for address generation and validation, and are described in depth below. You can access them through a C program, by including the `libdogecoin.h` header in the source code and including the `libdogecoin.a` library at compile time. Or, you may implement either set of wrappers if you are more inclined towards a high-level language. For more details about wrapper installation and setup, see [bindings.md](bindings.md).

When using functions from either the Essential or Advanced Address API, include the -lunistring flag during the linking process. This is because the Advanced Address API uses the GNU libunistring library for Unicode string manipulation. To include the -lunistring flag during linking, simply add it to the linker command when building your project:

`gcc -o example example.c -ldogecoin -lunistring`

Ensure that the `libunistring` library is installed on your system before linking.

---
### **generatePrivPubKeypair:**

`int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)`

This function will populate provided string variables (privkey, pubkey) with freshly generated respective private and public keys, specifically for either mainnet or testnet as specified through the network flag (is_testnet). If calling the function from the wrappers, only the is_testnet argument is required. From C, the function returns 1 on success and 0 on failure. The Python and Go wrappers both return a tuple containing the base58-encoded private and public key. _Note: in Python, the keys are returned as bytes objects with UTF-8 encoding._

_C usage:_
```c
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int privkeyLen = 53; 
  int pubkeyLen = 35;

  char privKey[privkeyLen]; 
  char pubKey[pubkeyLen];

  dogecoin_ecc_start();
  generatePrivPubKeypair(privKey, pubKey, false); // generating a mainnet pair
  dogecoin_ecc_stop();

  printf("My private key for mainnet is: %s\n", privKey);
  printf("My public key for mainnet is: %s\n", pubKey);
}
```

_Python usage:_
```py
import libdogecoin as l

l.w_context_start()
priv, pub = l.w_generate_priv_pub_key_pair() # python default is for mainnet
l.w_context_stop()

print(f"My private key for mainnet is: {priv.decode('utf-8')}")
print(f"My public key for mainnet is: {pub.decode('utf-8')}")
```

_Golang usage:_
```go
package main

import (
	"fmt"

	"github.com/jaxlotl/go-libdogecoin"
)

func main() {
	libdogecoin.W_context_start()
	priv, pub := libdogecoin.W_generate_priv_pub_keypair(true) // generating a testnet pair
	libdogecoin.W_context_stop()

	fmt.Printf("My private key for testnet is: %s\n", priv)
	fmt.Printf("My public key for testnet is: %s\n", pub)
}
```

---
### **generateHDMasterPubKeypair:**

`int generateHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)`

This function will populate provided string variables (privkey, pubkey) with freshly generated respective private and public keys for a hierarchical deterministic wallet, specifically for either mainnet or testnet as specified through the network flag (is_testnet). If calling the function from the wrappers, only the is_testnet argument is required. From C, the function returns 1 on success and 0 on failure. The Python and Go wrappers both return a tuple containing the base58-encoded private and public key. _Note: in Python, the keys are returned as bytes objects with UTF-8 encoding._

_C usage:_
```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int masterPrivkeyLen = 200; // enough cushion
  int pubkeyLen = 35;

  char masterPrivKey[masterPrivkeyLen]; 
  char masterPubKey[pubkeyLen];

  dogecoin_ecc_start();
  generateHDMasterPubKeypair(masterPrivKey, masterPubKey, false);
  dogecoin_ecc_stop();

  printf("My private key for mainnet is: %s\n", masterPrivKey);
  printf("My public key for mainnet is: %s\n", masterPubKey);
}
```

_Python usage:_
```py
import libdogecoin as l

l.w_context_start()
mpriv, mpub = l.w_generate_hd_master_pub_key_pair()
l.w_context_stop()

print(f"My private key for mainnet is: {mpriv.decode('utf-8')}")
print(f"My public key for mainnet is: {mpub.decode('utf-8')}")
```

_Golang usage:_
```go
package main

import (
	"fmt"

	"github.com/jaxlotl/go-libdogecoin"
)

func main() {
	libdogecoin.W_context_start()
	mpriv, mpub := libdogecoin.W_generate_hd_master_pub_keypair(true)
	libdogecoin.W_context_stop()

	fmt.Printf("My private key for testnet is: %s\n", mpriv)
	fmt.Printf("My public key for testnet is: %s\n", mpub)
}
```

---
### **generateDerivedHDPubKey:**

`int generateDerivedHDPubkey(const char* wif_privkey_master, char* p2pkh_pubkey)`

This function takes a given HD master private key (wif_privkey_master) and loads it into the provided pointer for the resulting derived public key (p2pkh_pubkey). This private key input should come from the result of generateHDMasterPubKeypair(). If calling the function from the wrappers, only the wif_privkey_master argument is required. From C, the function returns 1 on success and 0 on failure, but using the Python and Go wrappers yields only the base58-encoded derived child public key.

_C usage:_
```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int masterPrivkeyLen = 200; // enough cushion
  int pubkeyLen = 35;

  char masterPrivKey[masterPrivkeyLen]; 
  char masterPubKey[pubkeyLen];
  char childPubKey[pubkeyLen];

  dogecoin_ecc_start();
  generateHDMasterPubKeypair(masterPrivKey, masterPubKey, false);
  generateDerivedHDPubkey(masterPrivKey, childPubKey);
  dogecoin_ecc_stop();

  printf("master private key: %s\n", masterPrivKey);
  printf("master public key: %s\n", masterPubKey);
  printf("derived child key: %s\n", childPubKey);
}
```

_Python usage:_
```py
import libdogecoin as l

l.w_context_start()
mpriv, mpub = l.w_generate_hd_master_pub_key_pair()
child_key = l.w_generate_derived_hd_pub_key(mpriv)
l.w_context_stop()

print(f"My private key for mainnet is: {mpriv.decode('utf-8')}")
print(f"My public key for mainnet is: {mpub.decode('utf-8')}")
print(f"My derived child key for mainnet is: {child_key.decode('utf-8')}")
```

_Golang usage:_
```go
package main

import (
	"fmt"

	"github.com/jaxlotl/go-libdogecoin"
)

func main() {
	libdogecoin.W_context_start()
	mpriv, mpub := libdogecoin.W_generate_hd_master_pub_keypair(false)
	child_key := libdogecoin.W_generate_derived_hd_pub_key(mpriv)
	libdogecoin.W_context_stop()

	fmt.Printf("My private key for mainnet is: %s\n", mpriv)
	fmt.Printf("My public key for mainnet is: %s\n", mpub)
	fmt.Printf("My derived child key for mainnet is: %s\n", child_key)
}
```

---
### **verifyPrivPubKeypair**

`int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)`

This function requires a previously generated key pair (wif_privkey, p2pkh_pubkey) and the network they were generated for (is_testnet). It then validates that the given public key was indeed derived from the given private key, returning 1 if the keys are associated and 0 if they are not. This could be useful prior to signing, or in some kind of wallet recovery tool to match keys.

_C usage:_
```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int privkeyLen = 53; 
  int pubkeyLen = 35;

  char privKey[privkeyLen]; 
  char pubKey[pubkeyLen];

  dogecoin_ecc_start();
  generatePrivPubKeypair(privKey, pubKey, false);

  if (verifyPrivPubKeypair(privKey, pubKey, false)) {
    printf("Keypair is valid.\n");
  }
  else {
    printf("Keypair is invalid.\n");
  }
  dogecoin_ecc_stop();
}
```

_Python usage:_
```py
import libdogecoin as l

l.w_context_start()
priv, pub = l.w_generate_priv_pub_key_pair()

if l.w_verify_priv_pub_keypair(priv, pub): # python default is for mainnet
  print("Keypair is valid.")
else:
  print("Keypair is invalid.")
l.w_context_stop()
```

_Golang usage:_
```go
package main

import (
	"fmt"

	"github.com/jaxlotl/go-libdogecoin"
)

func main() {
	libdogecoin.W_context_start()
	priv, pub := libdogecoin.W_generate_priv_pub_keypair(false)

	if libdogecoin.W_verify_priv_pub_keypair(priv, pub, false) {
		fmt.Println("Keypair is valid.")
	} else {
		fmt.Println("Keypair is invalid.")
	}
	libdogecoin.W_context_stop()
}
```

---
### **verifyHDMasterPubKeypair**

`int verifyHDMasterPubKeypair(char* wif_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)`

This function validates that a given master private key matches a given master public key. This could be useful prior to signing, or in some kind of wallet recovery tool to match keys. This function requires a previously generated HD master key pair (wif_privkey_master, p2pkh_pubkey_master) and the network they were generated for (is_testnet). It then validates that the given public key was indeed derived from the given master private key, returning 1 if the keys are associated and 0 if they are not. This could be useful prior to signing, or in some kind of wallet recovery tool to match keys.

_C usage:_
```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int masterPrivkeyLen = 200; // enough cushion 
  int pubkeyLen = 35;

  char masterPrivKey[masterPrivkeyLen]; 
  char masterPubKey[pubkeyLen];

  dogecoin_ecc_start();
  generateHDMasterPubKeypair(masterPrivKey, masterPubKey, false);

  if (verifyHDMasterPubKeypair(masterPrivKey, masterPubKey, false)) {
    printf("Keypair is valid.\n");
  }
  else {
    printf("Keypair is invalid.\n");
  }
  dogecoin_ecc_stop();
}
```

_Python usage:_
```py
import libdogecoin as l

l.w_context_start()
mpriv, mpub = l.w_generate_hd_master_pub_key_pair(chain_code=1)

if l.w_verify_master_priv_pub_keypair(mpriv, mpub, chain_code=1):
  print("Keypair is valid.")
else:
  print("Keypair is invalid.")
l.w_context_stop()
```

_Golang usage:_
```go
package main

import (
	"fmt"

	"github.com/jaxlotl/go-libdogecoin"
)

func main() {
	libdogecoin.W_context_start()
	priv, pub := libdogecoin.W_generate_hd_master_pub_keypair(false)

	if libdogecoin.W_verify_hd_master_pub_keypair(priv, pub, false) {
		fmt.Println("Keypair is valid.")
	} else {
		fmt.Println("Keypair is invalid.")
	}
	libdogecoin.W_context_stop()
}
```

---
### **verifyP2pkhAddress**

`int verifyP2pkhAddress(char* p2pkh_pubkey, uint8_t len)`

This function accepts an existing public key (p2pkh_pubkey) and its length in characters (len) to perform some basic validation to determine if it looks like a valid Dogecoin address. It returns 1 if the address is valid and 0 if it is not. This is useful for wallets that want to check that a recipient address looks legitimate.

_C usage:_
```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int privkeyLen = 200; // enough cushion
  int pubkeyLen = 35;

  char privKey[privkeyLen]; 
  char pubKey[pubkeyLen];

  dogecoin_ecc_start();
  generatePrivPubKeypair(privKey, pubKey, false);

  if (verifyP2pkhAddress(pubKey, strlen(pubKey))) {
    printf("Address is valid.\n");
  }
  else {
    printf("Address is invalid.\n");
  }
  dogecoin_ecc_stop();
}
```

_Python usage:_
```py
import libdogecoin as l

l.w_context_start()
priv, pub = l.w_generate_priv_pub_key_pair()

if l.w_verify_p2pkh_address(pub):
  print("Address is valid.")
else:
  print("Address is invalid.")
l.w_context_stop()
```

_Golang usage:_
```go
package main

import (
	"fmt"

	"github.com/jaxlotl/go-libdogecoin"
)

func main() {
	libdogecoin.W_context_start()
	_, pub := libdogecoin.W_generate_hd_master_pub_keypair(false)

	if libdogecoin.W_verify_p2pkh_address(pub) {
		fmt.Println("Address is valid.")
	} else {
		fmt.Println("Address is invalid.")
	}
	libdogecoin.W_context_stop()
}
```

---
### **getDerivedHDAddress**

`int getDerivedHDAddress(const char* masterkey, uint32_t account, bool ischange, uint32_t addressindex, char* outaddress, bool outprivkey)`

This function derives a hierarchical deterministic address by way of providing the extended master key, account, ischange and addressindex.
It will return 1 if the function is successful and 0 if not.

_C usage:_
```C
#include "libdogecoin.h"
#include <assert.h>
#include <stdio.h>

int main() {
  size_t extoutsize = 112;
  char* extout = dogecoin_char_vla(extoutsize);
  char* masterkey_main_ext = "dgpv51eADS3spNJh8h13wso3DdDAw3EJRqWvftZyjTNCFEG7gqV6zsZmucmJR6xZfvgfmzUthVC6LNicBeNNDQdLiqjQJjPeZnxG8uW3Q3gCA3e";
  dogecoin_ecc_start();
  int res = getDerivedHDAddress(masterkey_main_ext, 0, false, 0, extout, true);
  u_assert_int_eq(res, true);
  u_assert_str_eq(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
  dogecoin_ecc_stop();
  free(extout);
}
```

---
### **getDerivedHDAddressByPath**

`int getDerivedHDAddressByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey)`

This function derives an extended HD address by custom path in string format (derived_path).
It returns 1 if the address is valid and 0 if it is not.

_C usage:_
```C
#include "libdogecoin.h"
#include <assert.h>
#include <stdio.h>

int main() {
  size_t extoutsize = 112;
  char* extout = dogecoin_char_vla(extoutsize);
  char* masterkey_main_ext = "dgpv51eADS3spNJh8h13wso3DdDAw3EJRqWvftZyjTNCFEG7gqV6zsZmucmJR6xZfvgfmzUthVC6LNicBeNNDQdLiqjQJjPeZnxG8uW3Q3gCA3e";
  dogecoin_ecc_start();
  res = getDerivedHDAddressByPath(masterkey_main_ext, "m/44'/3'/0'/0/0", extout, true);
  u_assert_int_eq(res, true);
  u_assert_str_eq(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
  dogecoin_ecc_stop();
  free(extout);
}
```

## Advanced Address API

These functions implement advanced functionality of Libdogecoin for address generation and validation, and are described in depth below. You can access them through a C program, by including the `libdogecoin.h` header in the source code and including the `libdogecoin.a` library at compile time. Or, you may implement either set of wrappers if you are more inclined towards a high-level language. For more details about wrapper installation and setup, see [bindings.md](bindings.md).

---
### **generateRandomEnglishMnemonic**

`int generateRandomEnglishMnemonic(const ENTROPY_SIZE size, MNEMONIC mnemonic);`

This function generates a random English mnemonic phrase (seed phrase). The function returns 0 on success and -1 on failure.

_C usage:_
```C
#include "libdogecoin.h"
#include <stdio.h>

int main () {
  MNEMONIC seed_phrase;

  dogecoin_ecc_start();
  generateRandomEnglishMnemonic("256", seed_phrase);
  dogecoin_ecc_stop();

  printf("%s\n", seed_phrase);
}
```
_C++ usage:_
```C++
extern "C" {
#include "libdogecoin.h"
}
#include <iostream>
using namespace std;

int main () {
  MNEMONIC seed_phrase;

  dogecoin_ecc_start();
  generateRandomEnglishMnemonic("256", seed_phrase);
  dogecoin_ecc_stop();

  cout << seed_phrase << endl;
}
```
---
### **getDerivedHDAddressFromMnemonic**
`int getDerivedHDAddressFromMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const MNEMONIC mnemonic, const PASS pass, char* p2pkh_pubkey, const bool is_testnet);`

This function generates a new dogecoin address from a mnemonic and a slip44 key path.  The function returns 0 on success and -1 on failure.

_C usage:_
```C
#include "libdogecoin.h"
#include <stdio.h>

int main () {
  int addressLen = 53;

  MNEMONIC seed_phrase;
  char address [addressLen];

  dogecoin_ecc_start();
  generateRandomEnglishMnemonic("256", seed_phrase);
  getDerivedHDAddressFromMnemonic(0, 0, "0", seed_phrase, NULL, address, false);
  dogecoin_ecc_stop();

  printf("%s\n", seed_phrase);
  printf("%s\n", address);
}
```
_C++ usage:_
```C++
extern "C" {
#include "libdogecoin.h"
}
#include <iostream>
using namespace std;

int main () {
  int addressLen = 53;

  MNEMONIC seed_phrase;
  char address [addressLen];

  dogecoin_ecc_start();
  generateRandomEnglishMnemonic("256", seed_phrase);
  getDerivedHDAddressFromMnemonic(0, 0, "0", seed_phrase, NULL, address, false);
  dogecoin_ecc_stop();

  cout << seed_phrase << endl;
  cout << address << endl;
}
```
