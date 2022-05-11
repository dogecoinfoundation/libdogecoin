from setuptools import setup, Extension
from Cython.Build import cythonize
from Cython.Distutils import build_ext

libdoge_extension = [Extension(
    name=               "libdogecoin",
    language=           "c",
    sources=            ["bindings/py_wrappers/libdogecoin/libdogecoin.pyx"],
    include_dirs=       [".",
                        "include",
                        "include/dogecoin",
                        "include/dogecoin/crypto",
                        "secp256k1/include"],
    libraries =         ["event", "event_core", "event_pthreads", "m"],
    library_dirs =      ["depends/x86_64-pc-linux-gnu/lib"],
    extra_objects=      [".libs/libdogecoin.a", 
                        "src/secp256k1/.libs/libsecp256k1.a", 
                        "src/secp256k1/.libs/libsecp256k1_precomputed.a"],
)]

setup(
    name=               "libdogecoin",
    version=            "0.5",
    description=        "Python interface for the libdogecoin C library",
    author=             "Jackie McAninch",
    author_email=       "jackie.mcaninch.2019@gmail.com",
    license=            "MIT",
    cmdclass =          {'build_ext': build_ext},
    ext_modules=        cythonize(libdoge_extension)
)
