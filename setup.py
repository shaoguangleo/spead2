#!/usr/bin/env python

# Copyright 2015 SKA South Africa
#
# This program is free software: you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from setuptools import setup, Extension
import glob
import sys
import os
import os.path
import ctypes.util
try:
    import numpy
    numpy_include = numpy.get_include()
except ImportError:
    numpy_include = None

def find_version():
    # Cannot simply import it, since that tries to import spead2 as well, which
    # isn't built yet.
    globals_ = {}
    with open(os.path.join(os.path.dirname(__file__), 'spead2', '_version.py')) as f:
        code = f.read()
    exec(code, globals_)
    return globals_['__version__']

# Can't actually install on readthedocs.org because Boost.Python is missing,
# but we need setup.py to still be successful to make the doc build work.
rtd = os.environ.get('READTHEDOCS') == 'True'

if not rtd:
    # Different OSes install the Boost.Python library under different names
    bp_library_names = [
        'boost_python-py{0}{1}'.format(sys.version_info.major, sys.version_info.minor),
        'boost_python{0}'.format(sys.version_info.major),
        'boost_python',
        'boost_python-mt']
    for name in bp_library_names:
        if ctypes.util.find_library(name):
            bp_library = name
            break
    else:
        raise RuntimeError('Cannot find Boost.Python library')

    extensions = [
        Extension(
            '_spead2',
            sources=(glob.glob('src/common_*.cpp') +
                     glob.glob('src/recv_*.cpp') +
                     glob.glob('src/send_*.cpp') +
                     glob.glob('src/py_*.cpp')),
            depends=glob.glob('src/*.h'),
            language='c++',
            include_dirs=['src', numpy_include],
            extra_compile_args=['-std=c++11', '-g0'],
            libraries=[bp_library, 'boost_system'])
    ]
else:
    extensions = []

setup(
    author='Bruce Merry',
    author_email='bmerry@ska.ac.za',
    name='spead2',
    version=find_version(),
    description='High-performance SPEAD implementation',
    url='https://github.com/ska-sa/spead2',
    classifiers = [
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU Lesser General Public License v3 or later (LGPLv3+)',
        'Operating System :: POSIX',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 3',
        'Topic :: Software Development :: Libraries',
        'Topic :: System :: Networking'],
    ext_package='spead2',
    ext_modules=extensions,
    setup_requires=['numpy'],
    install_requires=['numpy>=1.9.2', 'six'],
    tests_require=['nose', 'decorator', 'trollius'],
    test_suite='nose.collector',
    packages=['spead2', 'spead2.recv', 'spead2.send'])
