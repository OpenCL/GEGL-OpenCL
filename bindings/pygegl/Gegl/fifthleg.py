# Copyright (C) 2007 Manish Singh
#
#   fifthleg.py: higher level API for the Gegl package
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

import _gegl

def node_new_from_file(file=None):
    path_root = None

    if file is None:
        import sys
        file = sys.stdin

    if isinstance(file, basestring):
        filename = file
        file = open(filename)

        import os
        path_root = os.path.realpath(os.path.dirname(filename))
        
    data = file.read()
    return _gegl.parse_xml(data)
