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
 * Copyright 2013 Daniel Sabo
"""

import unittest

from gi.repository import Gegl

class TestGeglFormat(unittest.TestCase):
    def test_format(self):
      rgb_float = Gegl.format("RGB float")
      rgba_u8 = Gegl.format("RGBA u8")

      # Just ensure these don't crash
      str(rgb_float)
      repr(rgb_float)

      self.assertEqual("RGB float", Gegl.format_get_name(rgb_float))
      self.assertEqual("RGBA u8", Gegl.format_get_name(rgba_u8))

    def test_buffer(self):
      rgb_float = Gegl.format("RGB float")
      rgba_u8 = Gegl.format("RGBA u8")

      buf_float = Gegl.Buffer(format=rgb_float)
      buf_u8 = Gegl.Buffer(format=rgba_u8)

      self.assertEqual("RGB float", Gegl.format_get_name(buf_float.get_property("format")))
      self.assertEqual("RGBA u8", Gegl.format_get_name(buf_u8.get_property("format")))


if __name__ == '__main__':
    Gegl.init(None);
    unittest.main()
    Gegl.exit()