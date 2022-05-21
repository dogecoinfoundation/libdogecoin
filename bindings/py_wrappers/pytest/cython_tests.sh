#!/bin/bash
# helper script to install dependencies, clean, build and run cython unit tests:

# install:
python3-dbg -m venv .venv
source .venv/bin/activate
python3-dbg -m pip install --upgrade cython pip

# clean:
FILE=`pwd`/bindings/py_wrappers/libdogecoin/libdogecoin.c
if test -f "$FILE"; then
    rm `pwd`/libdogecoin.c `pwd`/libdogecoin.o
fi

# build:
python3-dbg bindings/py_wrappers/libdogecoin/setup.py build_ext --build-lib `pwd`/../pytest/ --build-temp `pwd`/../pytest/ --force --user

# # run:
python3-dbg bindings/py_wrappers/pytest/address_test.py 
PYTHONMALLOC=debug valgrind --tool=memcheck --leak-check=full --track-origins=yes -s \
--suppressions=`pwd`/valgrind-python.supp \
python3-dbg -Wd -X dev tracemalloc=5 bindings/py_wrappers/pytest/transaction_test.py 
deactivate
rm -rf .venv
