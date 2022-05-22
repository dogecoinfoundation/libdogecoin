#!/bin/bash
# helper script to install dependencies, clean, build and run cython unit tests:

# install:
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install --upgrade cython setuptools

# clean:
FILE=`pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.c
if test -f "$FILE"; then
    rm `pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.c `pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.o
fi

# build:
python3 bindings/py_wrappers/libdogecoin/setup.py build_ext --build-lib `pwd`/bindings/py_wrappers/pytest/ --build-temp `pwd`/ --force --user

# # run:
python3 bindings/py_wrappers/pytest/address_test.py 
# PYTHONDEBUG=1 PYTHONMALLOC=debug valgrind --tool=memcheck --leak-check=full --track-origins=yes -s \
# --suppressions=`pwd`/bindings/py_wrappers/pytest/valgrind-python3.supp \
# --log-file=`pwd`/bindings/py_wrappers/pytest/minimal.valgrind.log \
# python33-dbg -Wd -X tracemalloc=5 bindings/py_wrappers/pytest/transaction_test.py -v
python3 bindings/py_wrappers/pytest/transaction_test.py
deactivate
rm -rf .venv
