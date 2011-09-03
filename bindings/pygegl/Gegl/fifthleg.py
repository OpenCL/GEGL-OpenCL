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
from sys import maxint

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
    return _gegl.node_new_from_xml(data)

class Rectangle(_gegl.Rectangle):
    """A Rectangle object usable in varios circunstances.
       takes x,y, width, height or a 4-tuple as parameters
    """
    def __init__(self, x=None, y=None, width=None, height=None):
        if hasattr(x, "__len__"):
            if len(x) != 4:
                raise TypeError("Rectangle sequence needs 4 integers"
                                " for x, y ,width, height")
            _gegl.Rectangle.__init__(self, *x)
        else:
            _gegl.Rectangle.__init__(self, x, y, width, height)

    def __add__(self, r2):
        """Either provides a translation of current rect, or
           returns  the smallest rectangle containing the second rectangle
        """
        if not hasattr(r2, "__len__") or (len(r2) not in (2,4)):
            raise TypeError("Rectangle adition supported for 2-tuples"
                            " (translation) or 4-tuples (and rectangles)")
        if len(r2) == 2:
            x = self.x + r2[0]
            y = self.y + r2[1]
            return Rectangle(x, y , self.width, self.height)
        x = min(self.x, r2[0])
        y = min(self.y, r2[1])
        width = max(self.width + x, r2[0] + r2[2]) - x
        height = max(self.height + y, r2[1] + r2[3]) - y
        return Rectangle(x, y, width, height)

    def intersect(self, r2):
        return Rectangle(_gegl.Rectangle.intersect(self, r2))

    def __getslice__(self, i0, i1):
        """implemented in order to have a handy way to retrieve
           Rectangle parametners (i.e. self[:] returns 
           a (x, y, width, height) tuple
        """

        #allow open ended slices ( [0:] ) 
        if i1 == maxint:
            i1 = 4
        values = []
        for i in xrange(i0, i1):
            values.append(self[i])
        return tuple(values)

    def __mul__(self, operator):
        return Rectangle([component * operator for component in self])

    def __repr__(self):
        return "%s.Rectangle(%d, %d, %d, %d)" % (self.__module__,
                                                 self.x, self.y,
                                                 self.width, self.height)
