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
