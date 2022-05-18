from setuptools import setup, Extension
from Cython.Build import cythonize

libdoge_extension = Extension(
    name=               "libdogecoin",
    sources=            ["bindings/py_wrappers/libdogecoin/libdogecoin.pyx"],
    include_dirs=       ["include",
                        "include/dogecoin",
                        "include/dogecoin/crypto",
                        "src/secp256k1/include"],
    extra_objects=      ["src/secp256k1/.libs/libsecp256k1.a",
                        ".libs/libdogecoin.a"]
)

setup(
    name=               "libdogecoin",
    version=            "0.1",
    description=        "Python interface for the libdogecoin C library",
    author=             "Jackie McAninch",
    author_email=       "jackie.mcaninch.2019@gmail.com",
    license=            "MIT",
    python_requires=    ">=3.8.10",
    ext_modules=        cythonize(libdoge_extension)
)
