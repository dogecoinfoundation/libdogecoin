#!/bin/bash
# helper script to install dependencies, clean, build and run cython unit tests:

# install:
python3 -m pip install --upgrade cython

# clean:
FILE=`pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.c
if test -f "$FILE"; then
    rm bindings/py_wrappers/libdogecoin/libdogecoin.c
fi

# build:
python3 bindings/py_wrappers/libdogecoin/setup.py build_ext --build-lib bindings/py_wrappers/pytest/

# # run:
python3 bindings/py_wrappers/pytest/address_test.py 
python3 bindings/py_wrappers/pytest/transaction_test.py 