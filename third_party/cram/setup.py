#!/usr/bin/env python
"""Installs cram"""

import os
import sys
from distutils.core import setup

COMMANDS = {}
CRAM_DIR = os.path.abspath(os.path.dirname(__file__))

try:
    from wheel.bdist_wheel import bdist_wheel
except ImportError:
    pass
else:
    COMMANDS['bdist_wheel'] = bdist_wheel

def long_description():
    """Get the long description from the README"""
    return open(os.path.join(sys.path[0], 'README.rst')).read()

setup(
    author='Brodie Rao',
    author_email='brodie@bitheap.org',
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Environment :: Console',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: GNU General Public License (GPL)',
        ('License :: OSI Approved :: GNU General Public License v2 '
         'or later (GPLv2+)'),
        'Natural Language :: English',
        'Operating System :: OS Independent',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 3',
        'Programming Language :: Unix Shell',
        'Topic :: Software Development :: Testing',
    ],
    cmdclass=COMMANDS,
    description='Functional tests for command line applications',
    download_url='https://bitheap.org/cram/cram-0.7.tar.gz',
    keywords='automatic functional test framework',
    license='GNU GPLv2 or any later version',
    long_description=long_description(),
    name='cram',
    packages=['cram'],
    scripts=['scripts/cram'],
    url='https://bitheap.org/cram/',
    version='0.7',
)
