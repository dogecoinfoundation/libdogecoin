# Using Libdogecoin Tools

## Overview

If you are looking to just explore the functionality of Libdogecoin without building a complicated project yourself, look no further than the CLI tools provided in this repo. The first tool, `such`, is an interactive CLI application that allows you to perform all Essential address and transaction operations with prompts to guide you through the process. The second tool, `sendtx`, handles the process of broadcasting a transaction built using Libdogecoin to eventually push it onto the blockchain.

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
| -    | -    | -             |-      |
| -p, --privkey  | private_key         | yes | generate_public_key -p <private_key> |
| -k, --pubkey  | public_key          | yes | p2pkh -k <public_key> |
| -m, --derived_path | derived_path        | yes | derive_child_key -p <extended_private_key> -m <derived_path> |
| -t, --testnet  | designate_testnet   | no  | generate_private_key -t |
| -s  | script_hex          | yes | comp2der -s <compact_signature> |
| -x  | transaction_hex     | yes | sign -x <transaction_hex> -s <pubkey_script> -i <index_of_utxo_to_sign> -h <sig_hash_type> |
| -i  | input_index         | yes | see above |
| -h  | sighash_type        | yes | see above |

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
| sign                      | -x, -s, -i, -h, -p     | -t   | See the definition of sign_raw_transaction in the Transaction API.
| comp2der                  | -s                     | None | Convert a compact signature to a DER signature.
| transaction               | None                   | None | Start the interactive transaction app. [Usage instructions below.]() |

Lastly, to display the version of `such`, simply run the following command, which overrides any previous ones specified:
```
./such -v
```

### Examples
Below are some examples on how to use the `such` tool in practice.

##### Generate a new private key WIF and hex encoded:

    ./such -c generate_private_key
    > privatekey WIF: QSPDnjzvrSPAeiM7N2jCkzv2dqsi7fxoHipgpPfz2zdE3ZpYp74j
    > privatekey HEX: 7073fa30281cf89195dca333134368d539e7abad712abb532c9eaf5f3666d9d1

##### Generate the public key, p2pkh, and p2sh-p2pkh address from a WIF encoded private key

    ./such -c generate_public_key -p QSPDnjzvrSPAeiM7N2jCkzv2dqsi7fxoHipgpPfz2zdE3ZpYp74j
    > pubkey: 02cf2c99c2db4b3d72d4289aa23bdaf5f3ccf4867ec8e5f8223ea716a7a3de10bc
    > p2pkh address: D62RKK6AGkzX6fM8RzoVM8fjPx2nzrdvKU

##### Generate the P2PKH address from a hex encoded compact public key

    ./such -c generate_public_key -pubkey 02cf2c99c2db4b3d72d4289aa23bdaf5f3ccf4867ec8e5f8223ea716a7a3de10bc
    > p2pkh address: D62RKK6AGkzX6fM8RzoVM8fjPx2nzrdvKU

##### Generate new BIP32 master key

    ./such -c bip32_extended_master_key
    > masterkey: dgpv51eADS3spNJh9qLpW8S7B7uZmusTpNE85NgXsYD7eGuVhebMDfEsj6fNR6DHgpSBCmYdAvw9YRSqRWnFxtYn1bM8AdNipwdi9dDXFCY8vkY


##### Print HD node

    ./such -c print_keys -privkey dgpv51eADS3spNJh9qLpW8S7B7uZmusTpNE85NgXsYD7eGuVhebMDfEsj6fNR6DHgpSBCmYdAvw9YRSqRWnFxtYn1bM8AdNipwdi9dDXFCY8vkY
    > ext key:             dgpv51eADS3spNJh9qLpW8S7B7uZmusTpNE85NgXsYD7eGuVhebMDfEsj6fNR6DHgpSBCmYdAvw9YRSqRWnFxtYn1bM8AdNipwdi9dDXFCY8vkY
    > extended pubkey:     dgub8kXBZ7ymNWy2SgzyYN45HyTAEUF6eVFqMyTk2ec6SPxWFhi3dRneNQ51zJadLERvA1ns9uvMGKM9wYKTSnCP9QrSPJMCKjdfSv4qmT3PkP2
    > pubkey hex:          025368ca428b4c4e0c48631c5f8510d704858a52c7264d4ba74f34b2bcee374220
    > privatekey WIF:      QTtXPXYWc4G6WuA6qNRYeQ3TAdsBUUqrLwN1eWVFEvfHdd8M1ed5
    > depth:               0
    > child index:         0
    > p2pkh address:       D79Q3spkucaM2DvLxUZjgV1X4cQcWDLuyt

##### Derive child key (second child key at level 1 in this case)

    ./such -c derive_child_keys -m m/1h -privkey dgpv51eADS3spNJh9qLpW8S7B7uZmusTpNE85NgXsYD7eGuVhebMDfEsj6fNR6DHgpSBCmYdAvw9YRSqRWnFxtYn1bM8AdNipwdi9dDXFCY8vkY
    > ext key:             dgpv53gfwGVYiKVgf3hybqGjXuxrW2s2iCArhBURxAWaFszfqfP6wc23KFVyCuGj4fGzAX6oC8QmvhvkWz18v4VcdhzYCxoTR3XQizrVtjMwQHS
    > extended pubkey:     dgub8nZhGxRSGUA1wuN8e4themWSxbEfYKCZynFe7GuZ413gPiVoMNZoxYucn8DQ5doeqt1cmZnxZ4Ms9SdsraiSbUkZSYbx1GzpGbrAqmFdSSL
    > pubkey hex:          023973b755fdaf5b2b7b20ac134c936ec7882b1ce0a3a75857fc490c12cdf4fb4f
    > privatekey WIF:      QQUwLsFpWWXsHFLCxjvBMn8Qd4Pgqji5QUXz6zN8vkiKMPvv7mpZ
    > depth:               1
    > child index:         -2147483647
    > p2pkh address:       DFqonEEA56VE8zEGvhXNgjiPT3PaPFNQQu

### Interactive Transaction Building with `such`

When you start the interactive `such` transaction tool with `./such -c transaction`, you will be faced with a menu of options. To choose one of these options to execute, simply type the number of that command and hit enter. 

| Command | Description |
| -       | -           |
| add transaction           | Start building a new transaction. |
| edit transaction by id    | Make changes to a transaction that has already been started. |
| find transaction          | Print out the hex of a transaction that has already been started. |
| sign transaction          | Sign the inputs of a finalized transaction. |
| delete transaction        | Remove an existing transaction from memory. |
| delete all transactions   | Remove all existing transactions from memory. |
| print transactions        | Start building a new transaction. |
| import raw transaction    | Saves the entered transaction hex as a transaction object in memory. |
| broadcast transaction     | Performs the same operation as [`./sendtx`] (#the-sendtx-tool) (`sendtx` recommended) |
| change network            | Specify the network for building transactions. |
| quit                      | Exit the tool. |

Once you choose a command, there will be on-screen prompts to guide your next actions. All of these commands internally call the functions that make up Libdogecoin, so for more information on what happens when these commands are run, please refer to the [Libdogecoin Essential Transaction API](doc/../transaction.md). 



## The `sendtx` Tool

Now that you've built a sendable transaction with Libdogecoin, `sendtx` is here to broadcast that transaction so that it can be published on the blockchain. You can broadcast to peers retrieved from a DNS seed or specify with IP/port. The application will try to connect to a default maximum of 10 peers, send the transaction to two of them, and listen on the remaining ones if the transaction has been relayed back. Alongside Libdogecoin, `sendtx` gives you the capability to publish your own transactions directly to the blockchain without using external services.

### Usage

Similar to `such`, `sendtx` is simple to run and is invoked by simply running the command `./sendtx` in the top level of the Libdogecoin directory, which is then simply followed by the transaction hex to broadcast rather than a command like in `such`. There are still several flags that may be helpful

| Flag | Name | Required Arg? | Usage |
| -   | -                   | -   |-  |
| -t, --testnet  | designate_testnet   | no  | ./sendtx -t <tx_hex_for_testnet> |
| -r, --regtest  | designate_regtest   | no  | ./sendtx -r <tx_hex_for_regtest> |
| -d, --debug    | designate_debug     | no  | ./sendtx -d <tx_hex> |
| -s, --timeout  | timeout_threshold   | yes | ./sendtx -s 10 <tx_hex> |
| -i, --ips      | ip_addresses        | yes | ./sendtx -i 127.0.0.1:22556,192.168.0.1:22556 <tx_hex>|
| -m, --maxnodes | max_connected_nodes | yes | ./sendtx -m 6 <tx_hex> |

Lastly, to display only the version of `sendtx`, simply run the following command:
```
./sendtx -v
```

### Examples
Below are some examples on how to use the `sendtx` tool in practice.

##### Send a raw transaction to random peers on mainnet

    ./sendtx <tx_hex>

##### Send a raw transaction to random peers on testnet and show debug information

    ./sendtx -d -t <tx_hex>

##### Send a raw transaction to specific peers on mainnet and show debug information using a timeout of 5s

    ./sendtx -d -s 5 -i 192.168.1.110:22556,127.0.0.1:22556 <tx_hex>

##### Send a raw transaction to at most 5 random peers on mainnet

    ./sendtx -m 5 <tx_hex>
