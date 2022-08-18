#!/bin/bash
# helper script to install dependencies, clean, build and run cython unit tests:

# install:
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install --upgrade cython setuptools

# clean:
FILE=`pwd`/wrappers/python/libdogecoin/libdogecoin.c
if test -f "$FILE"; then
    rm `pwd`/wrappers/python/libdogecoin/libdogecoin.c
fi
FILE=`pwd`/wrappers/python/libdogecoin/libdogecoin.o
if test -f "$FILE"; then
    rm `pwd`/wrappers/python/libdogecoin/libdogecoin.o
fi

# build:
python3 wrappers/python/libdogecoin/setup.py build_ext --build-lib `pwd`/wrappers/python/pytest/ --build-temp `pwd`/ --force --user

# # run:
python3 wrappers/python/pytest/address_test.py 
# PYTHONDEBUG=1 PYTHONMALLOC=debug valgrind --tool=memcheck --leak-check=full --track-origins=yes -s \
# --suppressions=`pwd`/wrappers/python/pytest/valgrind-python.supp \
# --log-file=`pwd`/wrappers/python/pytest/minimal.valgrind.log \
# python3-dbg -Wd -X tracemalloc=5 wrappers/python/pytest/transaction_test.py -v
python3 wrappers/python/pytest/transaction_test.py -v
deactivate
rm -rf .venv
