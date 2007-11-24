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

import dl
libgegl = dl.open("libgegl-1.0.so.0", dl.RTLD_NOW|dl.RTLD_GLOBAL)
libbabl = dl.open("libbabl-0.0.so.0", dl.RTLD_NOW|dl.RTLD_GLOBAL)

from _gegl import *

import atexit
atexit.register(exit)
del exit, atexit

del _gegl
