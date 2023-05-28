libdogecoin - libdogecoin cython wrapper
=====
This library is a new set of wrappers using Cython on top of libdogecoin(c language).

build
-----
```
python setup build_ext --clean all # clean all build
python setup build_ext # build libdogecoin cython module
```

install
-------
```pip install .  # install libdogecoin package```

test
------
```
tests/cython_tests.sh # test all unittests
pytest tests/foo_test.py   # test foo
```

how to wrapper LIBDOGECOIN_API
-------------
some LIBDOGECOIN_API defines in foo.h, command used to wrapper foo.h:
```
touch libdogecoin/_foo.pxd # defines LIBDOGECOIN_API
touch tests/foo_test.py    # unittest for foo
```
in libdogecoin/libdogecoin.pyx insert: from . cimport _foo




