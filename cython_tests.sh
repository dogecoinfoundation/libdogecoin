#!/bin/bash
# helper script to install dependencies, clean, build and run cython unit tests:

# install:
python -m pip install --user pipenv
python -m pip install --upgrade cython

# clean:
# rm bindings/py_wrappers/libdogecoin/libdogecoin.c

# build:
python setup.py build_ext --inplace

# run:
python -v
python -m unittest discover -p '*_test.py'