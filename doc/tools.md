# Using Libdogecoin Tools

## Overview

If you are looking to just explore the functionality of Libdogecoin without building a complicated project yourself, look no further than the CLI tools provided in this repo. The first tool, `such`, is an interactive CLI application that allows you to perform all Essential address and transaction operations with prompts to guide you through the process. The second tool, `sendtx`, TODO IDK WHAT THIS ONE DOES

This document goes over the usage of these tools along with examples of how to use them.

## The `such` Tool
As stated above, the `such` tool can be used to perform all Libdogecoin address and transaction functions, and even more. It can generate different types of public and private keys, derive and convert keys, and fully build and sign transactions.

### Usage
The `such` tool can be used by simply running the command `./such` in the top level of the Libdogecoin directory, always followed by a `-c` flag that denotes the desired `such` command to run. The options for this command are below:
- generate_private_key
- generate_public_key
- p2pkh
- bip32_extended_master_key
- bip32maintotest
- derive_child_keys
- print_keys
- sign
- comp2der
- transaction

So an example run of `such` could be something like this:
```
./such -c generate_private_key
```
Most of these commands require a flag following them to denote things like existing keys, transaction hex strings, and more: 

| Flag | Name | Required Arg? | Usage |
| -   | -                   | -   |-  |
| -p  | private_key         | yes | generate_public_key -p <private_key> |
| -k  | public_key          | yes | p2pkh -k <public_key> |
| -m  | derived_path        | yes | derive_child_key -p <extended_private_key> -m <derived_path> |
| -t  | designate_testnet   | no  | generate_private_key -t |
| -s  | script_hex          | yes | comp2der -s <compact_signature> |
| -x  | transaction_hex     | yes | sign -x <transaction_hex> -s <pubkey_script> -i <index_of_utxo_to_sign> -h <sig_hash_type> -a <amount_in_utxo> |
| -i  | input_index         | yes | see above |
| -h  | sighash_type        | yes | see above |
| -a  | amount              | yes | see above |

Below is a list of all the commands and the flags that they require. As a reminder, any command that includes the `-t` flag will set the default chain used in internal calculations to _testnet_ rather than _mainnet_. Also included are descriptions of what each function does.

| Command | Required flags | Optional flags | Description |
| -                         | -                      | -    | - |
| generate_private_key      | None                   | -t   | Generates a private key from a secp256k1 context for either mainnet or testnet. |
| generate_public_key       | -p                     | -t   | Generates a public key derived from the private key specified. Include the testnet flag if it was generated from testnet. |
| p2pkh                     | -k                     | -t   | Generates a p2pkh address derived from the public key specified. Include the testnet flag if it was generated from testnet. |
| bip32_extended_master_key | None                   | -t   | Generate an extended master private key from a secp256k1 context for either mainnet or testnet. |
| bip32maintotest           | -p                     | None | Convert a mainnet private key into an equivalent testnet key. |
| derive_child_keys         | -p, -m                 | -t   | Generates a child key derived from the specified private key using the specified derivation path. 
| print_keys                | -p                     | -t   | Print all keys associated with the provided private key.
| sign                      | -x, -s, -i, -h, -a, -p | -t   | See the definition of sign_raw_transaction in the Transaction API.
| comp2der                  | -s                     | None | Convert a compact signature to a DER signature.
| transaction               | None                   | None | Start the interactive transaction app. |


### Examples
Below are some examples on how to use the tool in practice.

##### Generate a new privatekey WIF and HEX encoded:

    ./such -c generate_private_key
    > privatekey WIF: QSPDnjzvrSPAeiM7N2jCkzv2dqsi7fxoHipgpPfz2zdE3ZpYp74j
    > privatekey HEX: 7073fa30281cf89195dca333134368d539e7abad712abb532c9eaf5f3666d9d1

##### Generate the public key, p2pkh, and p2sh-p2pkh address from a WIF encoded private key

    ./such -c generate_public_key -p QSPDnjzvrSPAeiM7N2jCkzv2dqsi7fxoHipgpPfz2zdE3ZpYp74j
    > pubkey: 02cf2c99c2db4b3d72d4289aa23bdaf5f3ccf4867ec8e5f8223ea716a7a3de10bc
    > p2pkh address: D62RKK6AGkzX6fM8RzoVM8fjPx2nzrdvKU
    > p2sh-p2wpkh address: 9zXbecoxo4aDsG8Ng1osUhGN9URrF1P9JZ

##### Generate the P2PKH address from a hex encoded compact public key

    ./such -c generate_public_key -pubkey 02cf2c99c2db4b3d72d4289aa23bdaf5f3ccf4867ec8e5f8223ea716a7a3de10bc
    > p2pkh address: D62RKK6AGkzX6fM8RzoVM8fjPx2nzrdvKU
    > p2sh-p2wpkh address: 9zXbecoxo4aDsG8Ng1osUhGN9URrF1P9JZ
    > p2wpkh (doge / bech32) address: doge1qpx6wxh9xv780a7uj675vl0c88zd3fg4v26vlsn

##### Generate new BIP32 master key

    ./such -c bip32_extended_master_key
    > masterkey: dgpv51eADS3spNJh9qLpW8S7B7uZmusTpNE85NgXsYD7eGuVhebMDfEsj6fNR6DHgpSBCmYdAvw9YRSqRWnFxtYn1bM8AdNipwdi9dDXFCY8vkY


##### Print HD node

    ./such -c print_keys -privkey dgpv51eADS3spNJh9qLpW8S7B7uZmusTpNE85NgXsYD7eGuVhebMDfEsj6fNR6DHgpSBCmYdAvw9YRSqRWnFxtYn1bM8AdNipwdi9dDXFCY8vkY
    > ext key: dgpv51eADS3spNJh9qLpW8S7B7uZmusTpNE85NgXsYD7eGuVhebMDfEsj6fNR6DHgpSBCmYdAvw9YRSqRWnFxtYn1bM8AdNipwdi9dDXFCY8vkY
    > privatekey WIF: QTtXPXYWc4G6WuA6qNRYeQ3TAdsBUUqrLwN1eWVFEvfHdd8M1ed5
    > depth: 0
    > child index: 0
    > p2pkh address: D79Q3spkucaM2DvLxUZjgV1X4cQcWDLuyt
    > pubkey hex: 025368ca428b4c4e0c48631c5f8510d704858a52c7264d4ba74f34b2bcee374220
    > extended pubkey: dgub8kXBZ7ymNWy2SgzyYN45HyTAEUF6eVFqMyTk2ec6SPxWFhi3dRneNQ51zJadLERvA1ns9uvMGKM9wYKTSnCP9QrSPJMCKjdfSv4qmT3PkP2

##### Derive child key (second child key at level 1 in this case)

    ./such -c derive_child_keys -keypath m/1h -privkey dgpv51eADS3spNJh9qLpW8S7B7uZmusTpNE85NgXsYD7eGuVhebMDfEsj6fNR6DHgpSBCmYdAvw9YRSqRWnFxtYn1bM8AdNipwdi9dDXFCY8vkY
    > ext key: dgpv53gfwGVYiKVgf3hybqGjXuxrW2s2iCArhBURxAWaFszfqfP6wc23KFVyCuGj4fGzAX6oC8QmvhvkWz18v4VcdhzYCxoTR3XQizrVtjMwQHS
    > depth: 1
    > p2pkh address: DFqonEEA56VE8zEGvhXNgjiPT3PaPFNQQu
    > pubkey hex: 023973b755fdaf5b2b7b20ac134c936ec7882b1ce0a3a75857fc490c12cdf4fb4f
    > extended pubkey: dgub8nZhGxRSGUA1wuN8e4themWSxbEfYKCZynFe7GuZ413gPiVoMNZoxYucn8DQ5doeqt1cmZnxZ4Ms9SdsraiSbUkZSYbx1GzpGbrAqmFdSSL


## The `sendtx` Tool

















### The sendtx CLI
----------------
This tools can be used to broadcast a raw transaction to peers retrived from a dns seed or specified by ip/port.
The application will try to connect to max 6 peers, send the transaction two two of them and listens on the remaining ones if the transaction has been relayed back.

##### Send a raw transaction to random peers on mainnet

    ./sendtx <txhex>

##### Send a raw transaction to random peers on testnet and show debug infos

    ./sendtx -d -t <txhex>

##### Send a raw transaction to specific peers on mainnet and show debug infos use a timeout of 5s

    ./sendtx -d -s 5 -i 192.168.1.110:22556,127.0.0.1:22556 <txhex>