#!/bin/bash
# helper script to install dependencies, clean, build and run cython unit tests:

# install:
python3 -m pip install --upgrade pip
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install --upgrade cython

# clean:
FILE=`pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.c
if test -f "$FILE"; then
    rm bindings/py_wrappers/libdogecoin/libdogecoin.c bindings/py_wrappers/libdogecoin/bindings/libdogecoin.o
fi

# build:
python3 bindings/py_wrappers/libdogecoin/setup.py build_ext --build-lib bindings/py_wrappers/pytest/ --build-temp bindings/py_wrappers/libdogecoin/bindings/ --force --user

# # run:
python3 bindings/py_wrappers/pytest/address_test.py 
python3 bindings/py_wrappers/pytest/transaction_test.py 
deactivate
rm -rf .venv