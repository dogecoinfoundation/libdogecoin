import os
import pathlib
from setuptools import setup, Extension, Command, find_packages
from Cython.Build import cythonize
from Cython.Distutils import build_ext

# build extersions without beware working directory
setup_py_dir = pathlib.Path(__file__).parent
setup_py_relative_path = "wrappers/python"
libdogecoin_project_root_directory = str(setup_py_dir).removesuffix(setup_py_relative_path)

include_dirs=       [".",
                     "include",
                     "include/dogecoin",
                     "secp256k1/include"]
extra_objects=      [".libs/libdogecoin.a", 
                     "src/secp256k1/.libs/libsecp256k1.a", 
                     "src/secp256k1/.libs/libsecp256k1_precomputed.a"]

include_dirs_abspath = [os.path.join(libdogecoin_project_root_directory, path) for path in include_dirs]
extra_objects_abspath = [os.path.join(libdogecoin_project_root_directory, path) for path in extra_objects]

libdoge_extension = [Extension(
    name=               "libdogecoin.libdogecoin",
    language=           "c",
    sources=            [os.path.join(setup_py_dir, "libdogecoin/libdogecoin.pyx")],
    include_dirs=       include_dirs_abspath,
    libraries =         ["event", "event_core", "pthread", "m", "unistring"],
    extra_objects=      extra_objects_abspath
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
    ext_modules=                    cythonize(libdoge_extension, language_level = "3",),
    packages =                      ["libdogecoin"],
    zip_safe=                       False)
