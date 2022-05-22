#!/bin/bash
# helper script to install dependencies, clean, build and run cython unit tests:

# install:
python -m venv .venv
source .venv/bin/activate
python -m pip install --upgrade cython

# clean:
FILE=`pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.c
if test -f "$FILE"; then
    rm `pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.c `pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.o
fi

# build:
python bindings/py_wrappers/libdogecoin/setup.py build_ext --build-lib `pwd`/bindings/py_wrappers/pytest/ --build-temp `pwd`/ --force --user

# # run:
python bindings/py_wrappers/pytest/address_test.py 
# PYTHONDEBUG=1 PYTHONMALLOC=debug valgrind --tool=memcheck --leak-check=full --track-origins=yes -s \
# --suppressions=`pwd`/bindings/py_wrappers/pytest/valgrind-python.supp \
# --log-file=`pwd`/bindings/py_wrappers/pytest/minimal.valgrind.log \
# python3-dbg -Wd -X tracemalloc=5 bindings/py_wrappers/pytest/transaction_test.py -v
python bindings/py_wrappers/pytest/transaction_test.py
deactivate
rm -rf .venv
