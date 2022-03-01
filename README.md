# Libdogecoin, a clean C library of Dogecoin building blocks.

[![CI](https://github.com/dogecoinfoundation/libdogecoin/actions/workflows/ci.yml/badge.svg)](https://github.com/dogecoinfoundation/libdogecoin/actions/workflows/ci.yml)[![CodeQL](https://github.com/dogecoinfoundation/libdogecoin/actions/workflows/ql.yml/badge.svg)](https://github.com/dogecoinfoundation/libdogecoin/actions/workflows/ql.yml)

Libdogecoin will be a complete implementation of the Dogecoin Protocols, as a C library 
(and series of bindings to popular languages) which will allow anyone to build a Dogecoin 
compliant product, without needing to worry about the deeper specifics of the crypto 
functions.

This will be a pure library, not providing a ‘runnable’ node facility. Although we expect
building a Dogecoin Node will be a useful test and early outcome, that will live in another
repository.

It is intended that connecting the bits together into an engine be done at the level above, 
via the networking libraries of the host language.

[See the Dogecoin Trailmap for more on libdogecoin](https://foundation.dogecoin.com/trailmap/libdogecoin/)

### Dogecoin Standard/Spec

During the process of extracting the fundamentals from the Dogecoin Core Wallet (reference 
implementation) we aim to document ‘how Dogecoin works’ as a suite of tests and documents we 
are calling the Dogecoin Standard. 

See `/doc/spec`

By doing this we will be able to verify that the Libdogecoin implementation of Dogecoin’s 
internals is accurate to the OG wallet, and thus provide a mechanism for any future Dogecoin 
implementations to verify compliance with the Dogecoin Network.

### Why C? 

The Dogecoin Core project is written in C++, why move to C? This is a good question. 

The Dogecoin Core project was inherited when Dogecoin was originally forked and makes use of 
some reasonable heavy C++ libraries that add complexity to the build process, as well as 
cognitive complexity for new developers. 

The desire is to provide a simple to learn library with few external dependencies that can
be built with relatively little setup by new developers.  Furthermore the aim of providing
wrappers for a number of higher-level languages leans strongly toward either C or RUST from
a binding/support perspective, and we believe C still has significantly more support when
writing bindings for a wide variety of other languages.

### Code of Shibes

By contributing to this repository you agree to be a basic human being, please see `CONDUCT.md`

### Contributing

***TL;DR***: Initially during the early phase of development we'll keep this simple, after
the library starts to become a dependency for real projects this will likely change.

* Express interest and get added to the libdogecoin team on GitHub 
  and join the conversation in the Foundation discord server.
* Branch/PRs in this repository (see above point for access)
* Rebasing not merging
* Ensure tests
* Document how Dogecoin works as each feature is developed in `/doc/spec`
* 1 approval from another contributor required to merge to main
* Don't introduce dependencies without discussion (MIT)
* Collaborate before you innovate! 
* Have fun <3

### Structure

Advice on how to navigate this library:

`/include/*.h` provides header files for libdogecoin users, look here for .h</br>
`/src/<feature>/*.c,*.h` look here for local .c/.h source implementing the contracts in `/include`</br>
`/build/<arch>/*.a,*.so,*.dll` output targets, see `Makefile`, excluded in .gitignore</br>
`/contrib/<proj>` a place for misc non-core experiments, utils, demo-nodes etc</br>
`/bindings/<lang>/` individual language bindings</br>
`/test/` test suite</br>
`/doc/*.md` general library documentation</br>
`/doc/spec/*.md` A place to begin documenting the Dogecoin Standard as we go</br>
`/` Makefile, license etc.</br>

### Project stages

We understand that there's a steep lerning curve for most of the folk working
on this project, and that's OK. This is an inflection point for the Dogecoin 
community: moving from a tiny dev team to a wider #dogeDevArmy is great for 
derisking the bus-factor aspect of Dogecoin. The process of creating libdogecoin 
is an important step toward a broader and more sustainable community of devs.

With that in mind we're suggesting a staged approach to this project. Starting
with the basics and delivering working vertical slices of functionality 
as a useful C library with a handfull of higher level language wrappers early,
should force us to solve fundamental concerns such as language wrappers, testing
and other issues before getting too far down a rabbit hole.

![Stage 1 Diagram](/doc/diagrams/libdogecoin-stage1.png)

Stage one lets us learn and understand the lowest level building blocks of Dogecoin
as we build each slice of functionality and deliver incremental releases with full
tests, doc and perhaps even commandline utilities that exercise them. We expect 
that this approach will gain momentum after the first and second 'slice' as we face
and solve the problems of library design, building effective language wrappers etc.


![Stage 2 Diagram](/doc/diagrams/libdogecoin-stage2.png)

Stage two makes use of the low level building blocks we've delivered by combinging
them into higher level components that are needed to build wallets and nodes. This
is where we deliver the parts needed for other members of the community to cobble 
together operational doge projects.


![Stage 3a Diagram](/doc/diagrams/libdogecoin-stage3.png)

Stage three A takes what we've built and uses it to create a new Dogecoin Node 
service (in C) capable of joining the network and participating in the blockchain. 
The plan is to make this new DogeNode available for Windows, Linux, MacOS etc. in 
a simple-to-setup manner that will encourage new users to support the network.

This DogeNode should be far simpler to maintain, being abstracted from the many
'and the kitchen sink' additions that encumber the Dogecoin Core daemon.

![Stage 3b Diagram](/doc/diagrams/libdogecoin-stage3b.png)

At the same time, GigaWallet which is being built around the Dogecoin Core APIs
can be easily ported to libdogecoin so it can operate directly on L1 to transact
dogecoin. This will be the first major project using libdogecoin via a language
binding, and prove the ability for libdogecoin to enable greater flexibility in
how the community can get involved in development.

### Current features
----------------
* Generating and storing private and public keys
* ECDSA secp256k1 signing and verification (through [libsecp256k1](https://github.com/bitcoin-core/secp256k1) included as git subtree)
* Generate recoverable signatures (and recover pubkey from signatures)
* BIP32 hierarchical deterministic key derivation
* Transaction generation, manipulation, signing and ser-/deserialization including P2PKH, P2SH, multisig
* Address generation
* Base58check encoding
* Native implementation of SHA256, SHA512, SHA512_HMAC, RIPEMD-160 including NIST testvectors
* Native constant time AES (+256CBC) cipher implementation including NIST testvectors
* Keystore (wallet) databases (through logdb https://github.com/liblogdb/liblogdb)
* Event based dogecoin P2P client capable of connecting to multiple nodes in a single thread (requires [libevent](https://github.com/libevent/libevent))

#### Advantages of libdogecoin?
----------------

* No dependencies in case no p2p network client is required (only dependency is [libsecp256k1](https://github.com/bitcoin-core/secp256k1) added as git subtree)
* The only dependency for the p2p network client is [libevent](https://github.com/libevent/libevent) (very portable)
* optimized for MCU and low mem environments
* ~full test coverage
* mem leak free (valgrind check during CI)

### How to Build (Quick Start)
----------------

#### Install dependencies

##### Debian / Ubuntu
```
sudo apt-get install autoconf automake libtool build-essential libevent-dev
```

##### Other
Please submit a pull request to add dependencies for your system, or update these.

#### Full library including CLI tool and wallet database
```
./autogen.sh
./configure
make check
```

#### Pure library without net support
```
./autogen.sh
./configure --disable-net --disable-tools
make check
```

Brief examples of the such command line tool
----------------

##### Generate a new privatekey WIF and HEX encoded:

    ./such -c generate_private_key
    > privatekey WIF: QSPDnjzvrSPAeiM7N2jCkzv2dqsi7fxoHipgpPfz2zdE3ZpYp74j
    > privatekey HEX: 7073fa30281cf89195dca333134368d539e7abad712abb532c9eaf5f3666d9d1

##### Generate the public key, p2pkh and p2sh-p2pkh address from a WIF encoded private key

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