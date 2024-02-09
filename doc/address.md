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
    - [**getHDNodePrivateKeyWIFByPath**](#getHDNodePrivateKeyWIFByPath)
    - [**getHDNodeAndExtKeyByPath**](#getHDNodeAndExtKeyByPath)
    - [**getDerivedHDAddress**](#getderivedhdaddress)
    - [**getDerivedHDAddressByPath**](#getderivedhdaddressbypath)
  - [Advanced Address API](#advanced-address-api)
    - [**generateRandomEnglishMnemonic:**](#generaterandomenglishmnemonic)
    - [**generateRandomEnglishMnemonicTPM:**](#generaterandomenglishmnemonictpm)
    - [**getDerivedHDAddressFromMnemonic:**](#getderivedhdaddressfrommnemonic)
    - [**getDerivedHDAddressFromEncryptedSeed:**](#getderivedhdaddressfromencryptedseed)
    - [**getDerivedHDAddressFromEncryptedMnemonic:**](#getderivedhdaddressfromencryptedmnemonic)
    - [**getDerivedHDAddressFromEncryptedHDNode:**](#getderivedhdaddressfromencryptedhdnode)

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

These functions implement the core functionality of Libdogecoin for address generation and validation, and are described in depth below. You can access them through a C program, by including the `libdogecoin.h` header in the source code and including the `libdogecoin.a` library at compile time.

When using functions from either the Essential or Advanced Address API, include the -lunistring flag during the linking process. This is because the Advanced Address API uses the GNU libunistring library for Unicode string manipulation. To include the -lunistring flag during linking, simply add it to the linker command when building your project:

`gcc -o example example.c -ldogecoin -lunistring`

Ensure that the `libunistring` library is installed on your system before linking.

---

### **generatePrivPubKeypair:**

`int generatePrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)`

This function will populate provided string variables (privkey, pubkey) with freshly generated respective private and public keys, specifically for either mainnet or testnet as specified through the network flag (is_testnet). The function returns 1 on success and 0 on failure.

_C usage:_

```c
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int privkeyLen = PRIVKEYWIFLEN;
  int pubkeyLen = P2PKHLEN;

  char privKey[privkeyLen];
  char pubKey[pubkeyLen];

  dogecoin_ecc_start();
  generatePrivPubKeypair(privKey, pubKey, false); // generating a mainnet pair
  dogecoin_ecc_stop();

  printf("My private key for mainnet is: %s\n", privKey);
  printf("My public key for mainnet is: %s\n", pubKey);
}
```

---

### **generateHDMasterPubKeypair:**

`int generateHDMasterPubKeypair(char* hd_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)`

This function will populate provided string variables (privkey, pubkey) with freshly generated respective private and public keys for a hierarchical deterministic wallet, specifically for either mainnet or testnet as specified through the network flag (is_testnet). The function returns 1 on success and 0 on failure.

_C usage:_

```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int masterPrivkeyLen = HDKEYLEN; // enough cushion
  int pubkeyLen = P2PKHLEN;

  char masterPrivKey[masterPrivkeyLen];
  char masterPubKey[pubkeyLen];

  dogecoin_ecc_start();
  generateHDMasterPubKeypair(masterPrivKey, masterPubKey, false);
  dogecoin_ecc_stop();

  printf("My private key for mainnet is: %s\n", masterPrivKey);
  printf("My public key for mainnet is: %s\n", masterPubKey);
}
```

---

### **generateDerivedHDPubKey:**

`int generateDerivedHDPubkey(const char* hd_privkey_master, char* p2pkh_pubkey)`

This function takes a given HD master private key (hd_privkey_master) and loads it into the provided pointer for the resulting derived public key (p2pkh_pubkey). This private key input should come from the result of generateHDMasterPubKeypair(). The function returns 1 on success and 0 on failure.

_C usage:_

```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int masterPrivkeyLen = HDKEYLEN; // enough cushion
  int pubkeyLen = P2PKHLEN;

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

---

### **verifyPrivPubKeypair**

`int verifyPrivPubKeypair(char* wif_privkey, char* p2pkh_pubkey, bool is_testnet)`

This function requires a previously generated key pair (wif_privkey, p2pkh_pubkey) and the network they were generated for (is_testnet). It then validates that the given public key was indeed derived from the given private key, returning 1 if the keys are associated and 0 if they are not. This could be useful prior to signing, or in some kind of wallet recovery tool to match keys.

_C usage:_

```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int privkeyLen = PRIVKEYWIFLEN;
  int pubkeyLen = P2PKHLEN;

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

---

### **verifyHDMasterPubKeypair**

`int verifyHDMasterPubKeypair(char* hd_privkey_master, char* p2pkh_pubkey_master, bool is_testnet)`

This function validates that a given master private key matches a given master public key. This could be useful prior to signing, or in some kind of wallet recovery tool to match keys. This function requires a previously generated HD master key pair (hd_privkey_master, p2pkh_pubkey_master) and the network they were generated for (is_testnet). It then validates that the given public key was indeed derived from the given master private key, returning 1 if the keys are associated and 0 if they are not. This could be useful prior to signing, or in some kind of wallet recovery tool to match keys.

_C usage:_

```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int masterPrivkeyLen = HDKEYLEN; // enough cushion
  int pubkeyLen = P2PKHLEN;

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

---

### **verifyP2pkhAddress**

`int verifyP2pkhAddress(char* p2pkh_pubkey, uint8_t len)`

This function accepts an existing public key (p2pkh_pubkey) and its length in characters (len) to perform some basic validation to determine if it looks like a valid Dogecoin address. It returns 1 if the address is valid and 0 if it is not. This is useful for wallets that want to check that a recipient address looks legitimate.

_C usage:_

```C
#include "libdogecoin.h"
#include <stdio.h>

int main() {
  int privkeyLen = HDKEYLEN; // enough cushion
  int pubkeyLen = P2PKHLEN;

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

---

### **getHDNodePrivateKeyWIFByPath**

`char* getHDNodePrivateKeyWIFByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey)`

This function derives a hierarchical deterministic child key by way of providing the extended master key, derived_path, outaddress and outprivkey.
It will return the dogecoin_hdnode's private key serialized in WIF format if successful and exits if the proper arguments are not provided.

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
  u_assert_str_eq(getHDNodePrivateKeyWIFByPath(masterkey_main_ext, "m/44'/3'/0'/0/0", extout, true), "QNvtKnf9Qi7jCRiPNsHhvibNo6P5rSHR1zsg3MvaZVomB2J3VnAG");
  u_assert_str_eq(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
  dogecoin_ecc_stop();
  free(extout);
}
```

---

### **getHDNodeAndExtKeyByPath**

`dogecoin_hdnode* getHDNodeAndExtKeyByPath(const char* masterkey, const char* derived_path, char* outaddress, bool outprivkey)`

This function derives a hierarchical deterministic child key by way of providing the extended master key, derived_path, outaddress and outprivkey.  The masterkey can be either a private or public key, but if it is a public key, the outprivkey flag must be set to false and the derived_path must be a public derivation path.
It will return the dogecoin_hdnode if successful and exits if the proper arguments are not provided.

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
  dogecoin_hdnode* hdnode = getHDNodeAndExtKeyByPath(masterkey_main_ext, "m/44'/3'/0'/0/0", extout, true);
  u_assert_str_eq(utils_uint8_to_hex(hdnode->private_key, sizeof hdnode->private_key), "09648faa2fa89d84c7eb3c622e06ed2c1c67df223bc85ee206b30178deea7927");
  dogecoin_privkey_encode_wif((const dogecoin_key*)hdnode->private_key, &dogecoin_chainparams_main, privkeywif_main, &wiflen);
  u_assert_str_eq(privkeywif_main, "QNvtKnf9Qi7jCRiPNsHhvibNo6P5rSHR1zsg3MvaZVomB2J3VnAG");
  u_assert_str_eq(extout, "dgpv5BeiZXttUioRMzXUhD3s2uE9F23EhAwFu9meZeY9G99YS6hJCsQ9u6PRsAG3qfVwB1T7aQTVGLsmpxMiczV1dRDgzpbUxR7utpTRmN41iV7");
  dogecoin_ecc_stop();
  dogecoin_hdnode_free(hdnode);
  free(extout);
}
```

```C
#include "libdogecoin.h"
#include <assert.h>
#include <stdio.h>

int main() {
  char masterkey[HDKEYLEN] = {0};
  char master_public_key[HDKEYLEN] = {0};
  char extkeypath[KEYPATHMAXLEN] = "m/0/0/0/0/0";
  char extpubkey[HDKEYLEN] = {0};
  dogecoin_ecc_start();

  // Generate a master key pair from the seed, and then get the master public key
  getHDRootKeyFromSeed(utils_hex_to_uint8("5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc19a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4"), MAX_SEED_SIZE, false, masterkey);
  getHDPubKey(masterkey, false, master_public_key);

  // Derive an extended normal (non-hardened) public key from the master public key
  getHDNodeAndExtKeyByPath(master_public_key, extkeypath, extpubkey, false);
  dogecoin_ecc_stop();
  printf("%s\n", extpubkey);
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

These functions implement advanced functionality of Libdogecoin for address generation and validation, and are described in depth below. You can access them through a C program, by including the `libdogecoin.h` header in the source code and including the `libdogecoin.a` library at compile time.

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

### **generateRandomEnglishMnemonicTPM**

`dogecoin_bool generateRandomEnglishMnemonicTPM(MNEMONIC mnemonic, const int file_num, const dogecoin_bool overwrite);`

This function generates a random English mnemonic using a TPM (Trusted Platform Module). It stores the generated mnemonic in the provided `mnemonic` buffer. The `file_num` parameter specifies the encrypted storage file number, and the `overwrite` parameter indicates whether to overwrite an existing mnemonic in the encrypted storage. The function returns `TRUE` on success and `FALSE` on failure.

_C usage:_

```C

#include "libdogecoin.h"

int main() {
    MNEMONIC mnemonic;
    int file_num = 0;  // Specify the TPM storage file number
    dogecoin_bool overwrite = TRUE;  // Set to TRUE to overwrite existing mnemonic

    if (generateRandomEnglishMnemonicTPM(mnemonic, file_num, overwrite)) {
        printf("Generated mnemonic: %s\n", mnemonic);
    } else {
        printf("Error generating mnemonic.\n");
        return -1;
    }
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
  int addressLen = P2PKHLEN;

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
  int addressLen = P2PKHLEN;

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
---

### **getDerivedHDAddressFromEncryptedSeed**

`int getDerivedHDAddressFromEncryptedSeed(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const dogecoin_bool is_testnet, const int file_num);`

This function generates a new Dogecoin address from an encrypted seed and a slip44 key path. The function returns 0 on success and -1 on failure.

_C usage:_

```C

#include "libdogecoin.h"
#include <stdio.h>

int main() {
    int addressLen = P2PKHLEN;
    char derived_address[addressLen];

    if (getDerivedHDAddressFromEncryptedSeed(0, 0, BIP44_CHANGE_EXTERNAL, derived_address, false, TEST_FILE) == 0) {
        printf("Derived address: %s\n", derived_address);
    } else {
        printf("Error occurred.\n");
        return -1;
    }
}
```
---

### **getDerivedHDAddressFromEncryptedMnemonic**

`int getDerivedHDAddressFromEncryptedMnemonic(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, const PASS pass, char* p2pkh_pubkey, const bool is_testnet, const int file_num);`

This function generates a new Dogecoin address from an encrypted mnemonic and a slip44 key path. The function returns 0 on success and -1 on failure.

_C usage:_

```C

#include "libdogecoin.h"
#include <stdio.h>

int main() {
    int addressLen = P2PKHLEN;
    char derived_address[addressLen];

    if (getDerivedHDAddressFromEncryptedMnemonic(0, 0, BIP44_CHANGE_EXTERNAL, NULL, derived_address, false, TEST_FILE) == 0) {
        printf("Derived address: %s\n", derived_address);
    } else {
        printf("Error occurred.\n");
        return -1;
    }
}
```
---

### **getDerivedHDAddressFromEncryptedHDNode**

`int getDerivedHDAddressFromEncryptedHDNode(const uint32_t account, const uint32_t index, const CHANGE_LEVEL change_level, char* p2pkh_pubkey, const bool is_testnet, const int file_num);`

This function generates a new Dogecoin address from an encrypted HD node and a slip44 key path. The function returns 0 on success and -1 on failure.

_C usage:_

```C

#include "libdogecoin.h"
#include <stdio.h>

int main() {
    int addressLen = P2PKHLEN;
    char derived_address[addressLen];

    if (getDerivedHDAddressFromEncryptedHDNode(0, 0, BIP44_CHANGE_EXTERNAL, derived_address, false, TEST_FILE) == 0) {
        printf("Derived address: %s\n", derived_address);
    } else {
        printf("Error occurred.\n");
        return -1;
    }
}
```
