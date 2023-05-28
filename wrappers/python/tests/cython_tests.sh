#!/bin/bash
# helper script to install dependencies, clean, build and run cython unit tests:

set -u
set -x
set -e

# get wrappers/python dir
script_abspath=$(realpath "${BASH_SOURCE:-$0}")
script_dirname=$(dirname $script_abspath)

# wrappers/python
wrappers_python_abspath=$(cd $script_dirname && cd .. && pwd)

# wrappers/python/tests
tests_dir=${script_dirname}

# wrappers/python/.venv
virtualenv_dest=${wrappers_python_abspath}/.venv

cd $wrappers_python_abspath

#alias
#alias python="python3 "
#alias pip="pip3 "

set +x

# install
python3 -m venv --clear $virtualenv_dest
source ${virtualenv_dest}/bin/activate
python3 -m pip install --upgrade cython setuptools pip pytest

# build
python3 -m pip install -v -e .
# test
cd $virtualenv_dest
python3 -m pytest -v ${tests_dir}

# uninstall venv
deactivate
rm -rf ${virtualenv_dest}

exit 0
