# Libdogecoin, a clean C library of Dogecoin building blocks.

Libdogecoin will be a complete implementation of the Dogecoin Protocols, as a C library 
(and series of bindings to popular languages) which will allow anyone to build a Dogecoin 
compliant product, without needing to worry about the deeper specifics of the crypto 
functions.

This will be a pure library, not providing a ‘runnable’ node facility. Although we expect
building a Dogecoin Node will be a useful test and early outcome, that will live in another
repository.

It is intended that connecting the bits together into an engine be done at the level above, 
via the networking libraries of the host language.

### Dogecoin Standard/Spec

During the process of extracting the fundamentals from the Dogecoin Core Wallet (reference 
implementation) we aim to document ‘how Dogecoin works’ as a suite of tests and documents we 
are calling the Dogecoin Standard. 

See `/docs/spec`

By doing this we will be able to verify that the Libdogecoin implementation of Dogecoin’s 
internals is accurate to the OG wallet, and thus provide a mechanism for any future Dogecoin 
implementations to verify compliance with the Dogecoin Network.


### Contributing

***TL;DR***: Initially during the early phase of development we'll keep this simple, after
the library starts to become a dependency for real projects this will likely change.

* Contact Michi/Tim/Ross to get added to the libdogecoin team on GitHub and the Foundation discord server.
* Branch/PRs
* Rebasing before merging
* Write tests before merging to main
* Document how Dogecoin works as each feature is developed in `/docs/spec`
* 1 approval from another contributor required to merge to master
* Don't introduce dependencies without discussion
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

### Code of Shibes

By contributing to this repository you agree to be a basic human being, please see `CONDUCT.md`
