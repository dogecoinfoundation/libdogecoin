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
                        "src/secp256k1/include"],
    libraries =         ["event", "event_core", "event_pthreads", "m"],
    library_dirs =      ['/mnt/source/repos/libdogecoin/depends/x86_64-pc-linux-gnu/lib /mnt/source/repos/libdogecoin/src/secp256k1/libsecp256k1.la', '/mnt/source/repos/libdogecoin/src/secp256k1/depends/x86_64-pc-linux-gnu/lib', './src'],
    extra_objects=      ["src/secp256k1/.libs/libsecp256k1.a",
                        ".libs/libdogecoin.so"],
    extra_compile_args= ['-s', '-static', '--static', '-lstdc++','-std=c++11', '-v'],
    extra_link_args=    ['-lstdc++', '-v'],
)]

setup(
    name=               "libdogecoin",
    version=            "0.1",
    description=        "Python interface for the libdogecoin C library",
    author=             "Jackie McAninch",
    author_email=       "jackie.mcaninch.2019@gmail.com",
    license=            "MIT",
    python_requires=    ">=3.8.10",
    cmdclass =          {'build_ext': build_ext},
    ext_modules=        cythonize(libdoge_extension)
)
