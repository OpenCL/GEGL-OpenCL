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
 * Copyright 2013 Daniel Sabo <DanielSabo@gmail.com>
"""

import unittest

from gi.repository import Gegl

class TestGeglColor(unittest.TestCase):
    def assertAlmostEqualComps(self, a, b):
        self.assertEqual(len(a), len(b))
        for acomp, bcomp in zip(a, b):
            self.assertAlmostEqual(acomp, bcomp)

    def test_new_color(self):
        Gegl.Color.new("rgba(0.6, 0.6, 0.6, 1.0)")

    def test_new_color_string(self):
        Gegl.Color(string="rgba(0.6, 0.6, 0.6, 1.0)")

    def test_color_set_rgba(self):
        c = Gegl.Color.new("rgba(1.0, 1.0, 1.0, 1.0)")
        values = c.get_rgba()
        self.assertAlmostEqual(values[0], 1.0)
        self.assertAlmostEqual(values[1], 1.0)
        self.assertAlmostEqual(values[2], 1.0)
        self.assertAlmostEqual(values[3], 1.0)
        c.set_rgba(0.3, 0.6, 0.9, 1.0)
        values = c.get_rgba()
        self.assertAlmostEqual(values[0], 0.3)
        self.assertAlmostEqual(values[1], 0.6)
        self.assertAlmostEqual(values[2], 0.9)
        self.assertAlmostEqual(values[3], 1.0)

    def test_color_get_components(self):
        c = Gegl.Color()
        c.set_components(Gegl.format("RGB float"), [1.0, 0.0, 0.0])

        values = c.get_components(Gegl.format("RGB float"))
        self.assertAlmostEqualComps(values, [1.0, 0.0, 0.0])

        values = c.get_components(Gegl.format("RGBA double"))
        self.assertAlmostEqualComps(values, [1.0, 0.0, 0.0, 1.0])

        values = c.get_components(Gegl.format("RGBA float"))
        self.assertAlmostEqualComps(values, [1.0, 0.0, 0.0, 1.0])

        values = c.get_components(Gegl.format("RGBA u32"))
        self.assertEqual(values, [float(0xFFFFFFFF), 0.0, 0.0, float(0xFFFFFFFF)])

        values = c.get_components(Gegl.format("RGBA u16"))
        self.assertEqual(values, [float(0xFFFF), 0.0, 0.0, float(0xFFFF)])

        values = c.get_components(Gegl.format("RGBA u8"))
        self.assertEqual(values, [float(0xFF), 0.0, 0.0, float(0xFF)])

        c.set_components(Gegl.format("R'G'B' u8"), [128, 0, 128])

        values = c.get_components(Gegl.format("R'G'B'A u8"))
        self.assertEqual(values, [float(128), 0.0, float(128), float(255)])

        c.set_components(Gegl.format("YA double"), [0.5, 0.5])

        values = c.get_components(Gegl.format("RGBA double"))
        self.assertAlmostEqualComps(values, [0.5, 0.5, 0.5, 0.5])

        values = c.get_components(Gegl.format("RaGaBaA double"))
        self.assertAlmostEqualComps(values, [0.25, 0.25, 0.25, 0.5])

if __name__ == '__main__':
    Gegl.init(None);
    unittest.main()
    Gegl.exit()
