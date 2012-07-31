#!/usr/bin/env python
""" This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2011 Jon Nordby <jononor@gmail.com>
"""

import unittest

import gi
from gi.repository import Gegl

class TestGegl(unittest.TestCase):
    """Tests the Gegl"""

    def test_init(self):
        Gegl.init(0, "");

    def test_exit(self):
        pass
        #Gegl.exit()

    def test_init_exit(self):
        Gegl.init(0, "");
        Gegl.exit();

if __name__ == '__main__':
    #print dir(Gegl)
    unittest.main()

