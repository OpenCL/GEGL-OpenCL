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

class TestGeglBuffer(unittest.TestCase):
    def test_buffer_new(self):
      Gegl.Buffer.new("RGBA u8", 0, 0, 10, 10)
      Gegl.Buffer.new("RGB u16", 0, 0, 10, 10)
      Gegl.Buffer.new("RGBA float", 0, 0, 10, 10)

    def test_buffer_extent(self):
      buf  = Gegl.Buffer.new("RGBA float", 0, 5, 10, 15)
      rect = buf.get_extent()
      self.assertEqual(rect.x, 0)
      self.assertEqual(rect.y, 5)
      self.assertEqual(rect.width, 10)
      self.assertEqual(rect.height, 15)

    def test_buffer_access(self):
      buf = Gegl.Buffer.new("RGBA float", 0, 0, 4, 4)

      # Test buffer_get
      buffer_data = buf.get(buf.get_extent(), 1.0, "RGBA u8", Gegl.AbyssPolicy.NONE)
      self.assertEqual(len(buffer_data), 64)
      self.assertEqual(buffer_data[:4], "\x00\x00\x00\x00")

      # Check that we get the fresh data after the buffer is changed
      c = Gegl.Color.new("#000F")
      buf.set_color(buf.get_extent(), c)
      buffer_data = buf.get(buf.get_extent(), 1.0, "RGBA u8", Gegl.AbyssPolicy.NONE)
      self.assertEqual(len(buffer_data), 64)
      self.assertEqual(buffer_data[:4], "\x00\x00\x00\xFF")

      # Check that format works
      buffer_data = buf.get(buf.get_extent(), 1.0, "RGBA u16", Gegl.AbyssPolicy.NONE)
      self.assertEqual(len(buffer_data), 128)
      self.assertEqual(buffer_data[:8], "\x00\x00\x00\x00\x00\x00\xFF\xFF")

      # Invalid rect
      buffer_data = buf.get(Gegl.Rectangle.new(0, 0, 0, 0), 1.0, "RGBA u8", Gegl.AbyssPolicy.NONE)
      self.assertEqual(len(buffer_data), 0)

      # Invalid scale
      buffer_data = buf.get(buf.get_extent(), -1.0, "RGBA u8", Gegl.AbyssPolicy.NONE)
      self.assertEqual(len(buffer_data), 0)

      # Auto format (will use the buffer's format, RGBA float)
      buffer_data = buf.get(buf.get_extent(), 1.0, None, Gegl.AbyssPolicy.NONE)
      self.assertEqual(len(buffer_data), 64 * 4)

      # Set data
      buf.set(Gegl.Rectangle.new(0,0,1,1), "RGB u8", "\xFF\xFF\xFF")
      buffer_data = buf.get(Gegl.Rectangle.new(0,0,2,1), 1.0, "RGBA u8", Gegl.AbyssPolicy.NONE)
      self.assertEqual(len(buffer_data), 8)
      self.assertEqual(buffer_data, "\xFF\xFF\xFF\xFF\x00\x00\x00\xFF")


if __name__ == '__main__':
    Gegl.init(None);
    unittest.main()
    Gegl.exit()
