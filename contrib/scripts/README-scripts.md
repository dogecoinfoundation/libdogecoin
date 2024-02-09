### Usage

This directory contains various helper scripts to assist primarily with internal development when building from source but are provided to also help those potentially unfamiliar with this particular type of project to bootstrap their development environment in order to build, test, install or package their build output as an importable library for use in external development projects. These scripts are admittedly biased with the assumption that the primary users of such will be working within a linux environment and more specifically a flavor of Debian or Ubuntu. We will however eventually support as many alternative distros as possible.

These scripts are partitioned to support different phases of the development process as follows: fetching updates for package management, installation of required dependencies needed to build, building of libdogecoin and finally testing of built binaries. Each script was written with the intention of supporting multi-architecture builds that can run on MacOS, Windows and Linux. We currently support:

 - x86_64-pc-linux-gnu
 - i686-pc-linux-gnu
 - arm-linux-gnueabihf
 - aarch64-linux-gnu
 - x86_64-w64-mingw32
 - i686-w64-mingw32
 - x86_64-apple-darwin15
 - arm64-apple-darwin

Please take note of these target host triplets as they're required when using these scripts!

#### Setup

For instance, to setup your development environment using the host triplet `x86_64-pc-linux-gnu`, ensure you invoke the 'setup' script from the root directory of this project as follows:

```
./contrib/scripts/setup.sh --host x86_64-pc-linux-gnu
```

This will fetch updates by running sudo apt-get update or if building natively on MacOS, brew update, and install the packages needed in order to build libdogecoin. An alternative to basic package installation is to 'build depends' which can be accomplished by appending the following flag to the command we saw above like so:

```
./contrib/scripts/setup.sh --host x86_64-pc-linux-gnu --depends
```

#### Build

Once you've completed running the script above, you can now build libdogecoin like so:
```
./contrib/scripts/build.sh --host x86_64-pc-linux-gnu
```
or if you built with depends:

```
./contrib/scripts/build.sh --host x86_64-pc-linux-gnu --depends
```

#### Test

Assuming you've completed the preceeding steps successfully you can run tests by running the following command:
```
./contrib/scripts/test.sh --host x86_64-pc-linux-gnu
```

##### Below are instructions for running extended tests

To run the python valgrind tooltests run:
```
./contrib/scripts/test.sh --host x86_64-pc-linux-gnu --extended --valgrind
```

Note that this was an example for setting up, building and testing the target-host-triplet `x86_64-pc-linux-gnu`.

-----------

#### Run

To automate running all the aforementioned tests, for your convenience there is an aggregate script called run.sh. Following the proceeding example, one could run all those steps building either with depends or using host packages in addition to purging cached directories in order to perform a clean build with the following flags:

Required flag needed to specify host platform triplet:
```
--host <target-host-platfrom-triplet>
```

Optional flag to build libdogecoin using depends:

```
--depends
```

Optional flag to purge cached dependencies and previously built files:

```
--clean
```

Note that if building on linux, the preceeding examples can be used to cross compile every supported host platform triplet with exception to being capable of running tests on `x86_64-apple-darwin15` and `arm64-apple-darwin`.
