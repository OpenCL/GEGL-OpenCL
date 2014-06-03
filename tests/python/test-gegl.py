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

from gi.repository import Gegl

class TestGegl(unittest.TestCase):
    """Tests the Gegl global functions, initialization and configuration handling."""

    def test_100_init(self):
        Gegl.init(None);

    def test_200_config_defaults(self):
        gegl_config = Gegl.config()
        # Some default that are unlikely to change
        self.assertEqual(gegl_config.props.quality, 1.0)

    def test_300_exit(self):
        Gegl.exit()

if __name__ == '__main__':
    unittest.main()

