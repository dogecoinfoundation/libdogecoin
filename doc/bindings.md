# Using Libdogecoin Bindings

## Introduction
The Libdogecoin repo contains not only the source code for a clean C library, but also includes a set of wrappers for all the high-level functions of Libdogecoin. These specific functions are defined in address.c and transaction.c, and enable the user to perform the following operations:
- Generate different types of valid public/private keypairs
- Verify that a given keypair or address is valid for the Dogecoin network
- Build a transaction from scratch by adding inputs and outputs
- Sign a raw transaction using a private key
- Manage the set of working transactions within a user session

You can implement these functions in your own projects by following the directions below to import the Libdogecoin bindings packages. Much wow!

## Python Wrappers
The Python Libdogecoin module uses Cython to build wrappers for the functions mentioned above. Cython creates a shared library which can be normally imported like any other Python package, so long as it resides on the PYTHONPATH variable which accessible through Python's `sys` module. You can achieve this either by specifying the export location of the shared library at build time, or by appending the relative location to your PYTHONPATH using `sys.path.append()` within your project. 

### Building and Testing
Building the shared library is simple! Firstly, make sure the Libdogecoin C library is built and functional (confirm by running `make check`). Then, run the following command from the root Libdogecoin folder, which exports the library to the pytest folder:
```
python3 bindings/py_wrappers/libdogecoin/setup.py build_ext --build-lib bindings/py_wrappers/pytest
```
Congrats, you've just built the Python wrapper package! Test that they are functional by running unit tests for address and transaction functions:
```
python3 bindings/py_wrappers/pytest/address_test.py
python3 bindings/py_wrappers/pytest/transaction_test.py
```
If the wrappers are passing all the unit tests and you'd like to become a little more familiar with address functions, you can run the interactive address CLI program. Keep in mind this requires the shared library file to be in the pytest folder as well.
```
python3 bindings/py_wrappers/pytest/address_cli.py
```
From the CLI you can generate different types of keypairs to see what they look like, as well as verify existing keypairs or addresses. You may enter the command `q` to exit the program. If you want to repeat the previous command but with different arguments you can use the `w` command in place of typing out its full name. For example, if you would like to generate one keypair for mainnet and another for testnet, here's what that would look like:

_Example:_
```
=====================================================================================
Press [q] to quit CLI
Press [w] to repeat previous command

Available commands:

        gen_keypair <which_chain | 0:main, 1:test>
        gen_hdkeypair <which_chain | 0:main, 1:test>
        derive_hdpubkey <master_privkey_wif>
        verify_keypair <privkey_wif> <p2pkh address> <which_chain | 0:main, 1:test>
        verify_hdkeypair <privkey_wif_master> <p2pkh address_master> <which_chain | 0:main, 1:test>
        verify_address <p2pkh address>


$ gen_keypair 0
WIF-encoded private key: QWMRfPdpXFwDZgiM3SxGGiV84PkXAz5YcadHx1m5YQudPsZQ8xQi
P2PKH address: DAyHW3EKctXVA9Xiiu6KAnS9Sowfs6BHEf

$ w 1
WIF-encoded private key: cmFoycxSTprQeZcyWcv1zENjXUYnefGuaLWPret6jJYEYuTUhtMj
P2PKH address: np3j4qxP5qFrTJwi86GStq1a7tdLFKghYq

$ q
```


### Integration
Once you've confirmed that the wrappers pass unit tests, you can move the shared library to your project directory, or run the above build command again specifying your project directory after the `--build-lib` flag. Now try calling some Libdogecoin functions from your project!

_Example:_
```py
import libdogecoin

if __name__ == "__main__":
    libdogecoin.context_start()

    my_keypair = libdogecoin.generate_priv_pub_key_pair()
    print(f"My wif-encoded private key is {my_keypair[0]}.")
    print(f"My public p2pkh address is {my_keypair[1]}.")
    
    # your code here

    libdogecoin.context_stop()
```
Keep in mind that any time a function related to a private key is called, it must be from within a secp256k1 context. Only one context should be started per session using `libdogecoin.context_start()`, and should be stopped when the session is done using `libdogecoin.context_stop()`. If a function is called outside of a secp256k1 context, you will receive an error resembling the following:
```
python3: src/crypto/ecc.c:73: dogecoin_ecc_verify_privatekey: Assertion `secp256k1_ctx' failed.
Aborted (core dumped)
```

For more information and documentation on how to properly call all the functions of Libdogecoin from Python, see address.md.

### Modifying Wrappers
If you are interested in making your own modifications to these wrappers, you can edit the `bindings/py_wrappers/libdogecoin/libdogecoin.pyx` file. Once your changes have been made and you are ready to build again, make sure to delete both the previous shared library file _and_ the Cython-generated `libdogecoin.c` file adjacent to `libdogecoin.pyx` prior to running the build command.

## Golang Wrappers

#TODO