from setuptools import setup, Extension

SOURCES = ["src/address.c",
           "src/crypto/aes.c",
           "src/crypto/base58.c",
           "src/bip32.c",
           "src/buffer.c",
           "src/chainparams.c",
           "src/cstr.c",
           "src/crypto/ecc.c",
           "src/crypto/key.c",
           "src/mem.c",
           "src/crypto/random.c",
           "src/crypto/rmd160.c",
           "src/script.c",
           "src/crypto/segwit_addr.c",
           "src/serialize.c",
           "src/crypto/sha2.c",
           "src/cli/such.c",
           "src/cli/tool.c",
           "src/tx.c",
           "src/utils.c",
           "src/vector.c"]

INCLUDES = ["include/",
            "src/"]

C_LIB = Extension(name=                 "libdogecoin",
                  sources=              SOURCES,
                  include_dirs=         INCLUDES,
                  libraries=            ["dogecoin"],
                  runtime_library_dirs= [".libs"])


setup(
    name=               "libdogecoin",
    version=            "0.1",
    description=        "Python interface for the libdogecoin C library",
    author=             "Jackie McAninch",
    author_email=       "jackie.mcaninch.2019@gmail.com",
    license=            "MIT",
    install_requires=   ['ctypes'],
    python_requires=    ">=3.8.10",
    ext_modules=        [C_LIB],
    packages=           ["bindings/py_wrappers/libdogecoin"]
    )