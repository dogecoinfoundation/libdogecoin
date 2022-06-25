from setuptools import setup, Extension, Command
from Cython.Build import cythonize
from Cython.Distutils import build_ext
        
libdoge_extension = [Extension(
    name=               "libdogecoin",
    language=           "c",
    sources=            ["wrappers/python/libdogecoin/libdogecoin.pyx"],
    include_dirs=       [".",
                        "include",
                        "include/dogecoin",
                        "secp256k1/include"],
    libraries =         ["event", "event_core", "pthread", "m"],
    extra_objects=      [".libs/libdogecoin.a", 
                        "src/secp256k1/.libs/libsecp256k1.a", 
                        "src/secp256k1/.libs/libsecp256k1_precomputed.a"]
)]

setup(
    name=                           "libdogecoin",
    version=                        "0.1", 
    author=                         "Jackie McAninch",
    author_email=                   "jackie.mcaninch.2019@gmail.com",
    description=                    "Python interface for the libdogecoin C library",
    license=                        "MIT",
    url=                            "https://github.com/dogecoinfoundation/libdogecoin",
    cmdclass =                      {'build_ext': build_ext},
    ext_modules=                    cythonize(libdoge_extension, language_level = "3")
)
