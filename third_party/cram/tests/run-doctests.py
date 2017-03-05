#!/usr/bin/env python

import doctest
import os
import sys

def _getmodules(pkgdir):
    """Import and yield modules in pkgdir"""
    for root, dirs, files in os.walk(pkgdir):
        if '__pycache__' in dirs:
            dirs.remove('__pycache__')
        for fn in files:
            if not fn.endswith('.py') or fn == '__main__.py':
                continue

            modname = fn.replace(os.sep, '.')[:-len('.py')]
            if modname.endswith('.__init__'):
                modname = modname[:-len('.__init__')]
            modname = '.'.join(['cram', modname])
            if '.' in modname:
                fromlist = [modname.rsplit('.', 1)[1]]
            else:
                fromlist = []

            yield __import__(modname, {}, {}, fromlist)

def rundoctests(pkgdir):
    """Run doctests in the given package directory"""
    totalfailures = totaltests = 0
    for module in _getmodules(pkgdir):
        failures, tests = doctest.testmod(module)
        totalfailures += failures
        totaltests += tests
    return totalfailures != 0

if __name__ == '__main__':
    try:
        sys.exit(rundoctests(sys.argv[1]))
    except KeyboardInterrupt:
        pass
