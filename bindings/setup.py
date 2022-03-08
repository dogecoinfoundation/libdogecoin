from distutils.core import setup, Extension

def main():
    setup(name= "libdogecoin",
          version= "0.1",
          description= "Python interface for the lidogecoin C library",
          author= "Jackie McAninch",
          author_email= "jackie.mcaninch.2019@gmail.com",
          #ext_modules=[Extension("libdoge", []
          )