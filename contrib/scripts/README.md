### Usage

This directory contains various helper scripts to assist primarily with internal development but are provided to also help those potentially unfamiliar with this particular type of project to bootstrap their development environment in order to build, test, install or package their build output as an importable library for use in external development projects. These scripts are admittedly biased with the assumption that the primary users of such will be working within a linux environment and more specifically a flavor of Debian or Ubuntu. We will however eventually support as many alternative distros as possible.

These scripts are partitioned to support different phases of the development process as follows: fetching updates for package management, installation of required dependencies needed to build, building of libdogecoin and finally testing of built binaries. Each script was written with the intention of supporting multi-architecture builds that can run on MacOS, Windows and Linux. We currently support:

 - x86_64-pc-linux-gnu
 - i686-pc-linux-gnu
 - arm-linux-gnueabihf
 - aarch64-linux-gnu
 - x86_64-w64-mingw32
 - i686-w64-mingw32
 - x86_64-apple-darwin14

Please take note of these host triplets as they're required when using these scripts!

#### Setup
For instance, to setup your development environment on `x86_64-pc-linux-gnu`, ensure you invoke the 'setup' script from the root directory of this project as follows:

```
sudo -u $(whoami) -H bash -c "./contrib/scripts/setup.sh --host x86_64-pc-linux-gnu"
```

This will fetch updates by running sudo apt-get update and install the packages needed in order for us to be able to build libdogecoin. An alternative to basic package installation is to 'build depends' which can be accomplished by appending the following 'flag' to the command we saw above like so:

```
sudo -u $(whoami) -H bash -c "./contrib/scripts/setup.sh --host x86_64-pc-linux-gnu --depends"
```

Another option which may be useful is to pass the Docker flag which strips the usage of sudo from various commands needed to set up a similar development environment but within a container. Note the need to not pass 'sudo':

```
./contrib/scripts/setup.sh --host x86_64-pc-linux-gnu --depends --docker
```