# Getting Started with Libdogecoin

## Download Pre-built Binaries

To use Libdogecoin directly out of the box without making any modifications to the source code, you can download the pre-made binaries from [https://github.com/dogecoinfoundation/libdogecoin/releases](https://github.com/dogecoinfoundation/libdogecoin/releases).
This is the easiest to start with for new developers, but does not enable customization of the library. In order increase control over installation, refer to the section below on manually building Libdogecoin.

## Building Manually

### Step 1: Install Dependencies

For Ubuntu and Debian Linux, you will need to install the following dependencies before building:

- autoconf
- automake
- libtool
- libevent-dev
- libunistring-dev
- build-essential

This can be done in the following commands using Linux CLI:

```c
sudo apt-get update
sudo apt-get install autoconf automake libtool libevent-dev libunistring-dev build-essential
```

For Windows, you will need to install the following dependencies before building:

- [Visual Studio Code](https://code.visualstudio.com/download)
- [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019)
- [C++ and CMake Extensions](https://code.visualstudio.com/docs/editor/extension-gallery)
- [CMake](https://cmake.org/download/)
- [Git](https://git-scm.com/downloads)
- [Python](https://www.python.org/downloads/)

### Step 2: Build Libraries

If all the necessary dependencies have been installed, you can now proceed in building the library. Using autoconf tools, run the `./autogen.sh` command in terminal:

```c
./autogen.sh
```

Next, you can configure the library to your liking by specifying flags on the `./configure` command. (This is especially important for cross compilation as shown in the [cross compilation section](#cross-compilation-with-depends).)

At this step there are plenty of flags that can be specified, the two most pertinent ones being `enable-tools/disable-tools` and `enable-net/disable-net`. Both are enabled by default, but if you do not need to use the CLI tools mentioned in [tools.md](tools.md) or are not planning to send transactions using Libdogecoin, you may want to consider disabling these flags for simplicity and speed. Here are some examples of possible configurations you can build:

```c
./configure
./configure --disable-net --disable-tools
./configure LD_LIBRARY_PATH='path/to/additional/libraries'
./configure CFLAGS='-Ipath/to/additional/include/files'
```
If you're building on Windows, you'll need to use `cmake` instead of `./configure`:

```c
mkdir build
cd build
cmake ..
```
Another useful flag is `--enable-test-passwd`, which will generate a random password for testing software encryption/decryption. This flag disables the need for a password to be entered when testing TPM encryption/decryption. _Note: this flag is for testing purposes only._ This flag is disabled by default, but can be enabled with the `./configure` command or by using `cmake`:
```c
./configure --enable-test-passwd
```
```c
cmake -DTEST_PASSWD=TRUE ..
```
## _`--enable-test-passwd` is for **testing purposes only**._
For a complete list of all different configuration options, you can run the command `./configure --help`.

Finally, once you have configured the library to your liking, it is ready to be built. This can be done with the simple `make` command:

```c
make
```

Or, if you would like to also run our basic unit tests, you can run the command `make check` which will both build the library and give output showing which files are not passing tests. _Note: when compiling with net enabled, this will appear to hang for a couple of seconds after test_tool(), but rest assured that it is running._

```c
make check
```

_Output:_

```c
make[1]: Entering directory '/home/username/libdogecoin'
make  check-TESTS
make[2]: Entering directory '/home/username/libdogecoin'
PASSED - test_address()
PASSED - test_aes()
PASSED - test_base58()
PASSED - test_bip32()

...

PASSED - test_tool()
PASSED - test_net_basics_plus_download_block()
PASSED - test_protocol()
PASS: tests
=============
1 test passed
=============
make[2]: Leaving directory '/home/username/libdogecoin'
make[1]: Leaving directory '/home/username/libdogecoin'
```

On Windows, you will need to run the following commands in the Visual Studio Developer Command Prompt:

```c
mkdir build
cd build
cmake ..
cmake --build .
Debug\tests.exe
```

### Step 3: Integrate in Your Project

At this point, the library file `libdogecoin.a` located in the `/.libs` folder is now fully built and can be moved to any location on your file system. To integrate into your project, you will want to move this file by whatever means (copy-paste, drag-and-drop, `mv` command, etc.) to a location inside of your project where the linker can find it. If it is not directly in your project folder, you will need to specify the path by using the `-L` flag during compilation.

You will want to do the same with the `libdogecoin.h` header (located in `/include/dogecoin`) in order to integrate the Libdogecoin functions into your source code. Be sure to write an include statement for this file at the top of any source code which uses Libdogecoin, like the example below:

_main.c:_

```c
#include <stdio.h>
#include "libdogecoin.h"

int main() {
    // your code here...
}
```

Again, for compilation later, if `libdogecoin.h` is not directly next to your source code, specify the path to the header file using the `-I` flag.

From here, you can call the Libdogecoin API from your program as you normally would any function, as shown below. For more examples on the context in which to use these functions, please refer to [address.md](address.md) and [transaction.md](transaction.md). In this particular script, the user is generating a new key pair at which to receive funds, and is constructing a transaction to send funds to this new address from an existing wallet.

_main.c:_

```c
#include "libdogecoin.h"
#include <stdio.h>

int main() {
    dogecoin_ecc_start();

    // establish existing info (utxo is worth 2 doge)
    char *oldPrivKey = "ci5prbqz7jXyFPVWKkHhPq4a9N8Dag3TpeRfuqqC2Nfr7gSqx1fy";
    char *oldPubKey = "031dc1e49cfa6ae15edd6fa871a91b1f768e6f6cab06bf7a87ac0d8beb9229075b";
    char *oldScriptPubKey = "76a914d8c43e6f68ca4ea1e9b93da2d1e3a95118fa4a7c88ac";
    char* utxo_id = "b4455e7b7b7acb51fb6feba7a2702c42a5100f61f61abafa31851ed6ae076074";
    int utxo_vout = 1;
    char* amt_total = "2.0";

    // generate new key pair to send to
    char newPrivKey[PRIVKEYWIFLEN];
    char newPubKey[P2PKHLEN];
    generatePrivPubKeypair(newPrivKey, newPubKey, false);

    // build and sign the transaction
    int idx = start_transaction();
    add_utxo(idx, utxo_id, utxo_vout);
    add_output(idx, newPubKey, "0.69");
    finalize_transaction(idx, newPubKey, "0.00226", amt_total, oldPubKey);
    sign_transaction(idx, oldScriptPubKey, oldPrivKey);

    // print result
    printf("\nFinal signed transaction hex: %s\n\n", get_raw_transaction(idx));

    dogecoin_ecc_stop();
}
```

The last step is to specify the libraries you will need to link into your project, done by using the `-l` flag. The libraries that are required to use Libdogecoin in your project are:

- libdogecoin (of course!)
- libevent
- libunistring

On the command line, your final compilation will look something like the command below, factoring in all of the steps previously mentioned. _Note: the prefix "lib" is excluded when specifying libraries to link._

```
gcc main.c -L./path/to/library/file -I./path/to/header/file -ldogecoin -levent -lunistring -o myprojectname
```

Congratulations, you have just built an executable program that implements Libdogecoin!

## Cross Compilation with Depends

There may be times when you would like to build the library for a different operating system than you are currently running. You can do this relatively easily with `depends`, included in the Libdogecoin repo! The available operating systems you can choose from are the following:

- arm-linux-gnueabihf
- armv7a-linux-android
- aarch64-linux-gnu
- aarch64-linux-android
- x86_64-pc-linux-gnu
- x86_64-apple-darwin
- x86_64-w64-mingw32
- x86_64-linux-android
- i686-w64-mingw32
- i686-pc-linux-gnu

The build steps for cross compilation are very similar to the native build steps listed above. Specify the desired architecture from the list above under by using the `host` flag, and include any necessary configuration flags on the `./configure` command:

```c
make -C depends host=<target_architecture>
./autogen.sh
./configure
make check
```

It is important to run `make check` after cross compiling to make sure that everything is running properly for your architecture. For some guidance on which configuration flags to run, you can refer to our [CI test file](../.github/workflows/ci.yml).
