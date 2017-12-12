#!/usr/bin/python

"""
setup for divvun-swig
"""

import os
from distutils.core import setup, Extension
from sys import platform

# handle divvun-specific commands
import sys
local_divvun = False
if '--local-divvun' in sys.argv:
        index = sys.argv.index('--local-divvun')
        sys.argv.pop(index)
        local_divvun = True


libdivvun_src_path = '../src/'
absolute_libdivvun_src_path = os.path.abspath(libdivvun_src_path)

extra_link_arguments = []
if platform == "darwin":
        extra_link_arguments.extend(['-mmacosx-version-min=10.7'])

# If you wish to link to the local divvun library:
if local_divvun:
        extra_link_arguments.extend(["-Wl,-rpath="
                                     + absolute_libdivvun_src_path
                                     + "/.libs"])

extra_compile_arguments = ['-std=c++11']
if platform == "darwin":
        extra_compile_arguments.extend(["-stdlib=libc++",
                                        "-mmacosx-version-min=10.7"])

# If you wish to link divvun C++ library statically, use:
# library_dirs = []
# libraries = []
# extra_objects = absolute_libdivvun_src_path + "/.libs/libdivvun.a"

libdivvun_module = Extension('_libdivvun',
                             language="c++",
                             sources=["libdivvun.i"],
                             swig_opts=["-c++",
                                        "-I" + absolute_libdivvun_src_path,
                                        "-Wall"],
                             include_dirs=[absolute_libdivvun_src_path],
                             library_dirs=[absolute_libdivvun_src_path
                                           + "/.libs"],
                             libraries=["divvun"],
                             extra_compile_args=extra_compile_arguments,
                             extra_link_args=extra_link_arguments)

setup(name='libdivvun_swig',
      version='3.13.0_beta',
      author='divvun team',
      author_email='divvun-bugs@helsinki.fi',
      url='http://divvun.github.io/',
      description='SWIG-bound divvun interface',
      ext_modules=[libdivvun_module],
      py_modules=["libdivvun"],
      packages=["divvun"],
      data_files=[])
