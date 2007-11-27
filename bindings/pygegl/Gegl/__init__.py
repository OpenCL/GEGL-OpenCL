# PyGEGL - Python bindings for the GEGL image processing library
# Copyright (C) 2007 Manish Singh
#
#   __init__.py: initialization file for the Gegl package
#
# PyGEGL is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 3 of the License, or (at your option) any later version.
#
# PyGEGL is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with PyGEGL; if not, see <http://www.gnu.org/licenses/>.

# dl tricks from GST python's __init__.py
import sys

def setdlopenflags():
    oldflags = sys.getdlopenflags()
    try:
        from DLFCN import RTLD_GLOBAL, RTLD_LAZY
    except ImportError:
        RTLD_GLOBAL = -1
        RTLD_LAZY = -1
        import os
        osname = os.uname()[0]
        if osname == 'Linux' or osname == 'SunOS' or osname == 'FreeBSD':
            RTLD_GLOBAL = 0x100
            RTLD_LAZY = 0x1
        elif osname == 'Darwin':
            RTLD_GLOBAL = 0x8
            RTLD_LAZY = 0x1
        del os
    except:
        RTLD_GLOBAL = -1
        RTLD_LAZY = -1

    if RTLD_GLOBAL != -1 and RTLD_LAZY != -1:
        sys.setdlopenflags(RTLD_LAZY | RTLD_GLOBAL)

    return oldflags

oldflags = setdlopenflags()

from _gegl import *

sys.setdlopenflags(oldflags)
del sys, setdlopenflags

from fifthleg import *

import atexit
atexit.register(exit)
del exit, atexit

del _gegl
