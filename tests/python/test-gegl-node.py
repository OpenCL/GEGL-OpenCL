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

invert_crop_xml = """<?xml version='1.0' encoding='UTF-8'?>
<gegl>
  <node operation='gegl:invert'>
  </node>
  <node operation='gegl:crop'>
      <params>
        <param name='x'>0</param>
        <param name='y'>0</param>
        <param name='width'>0</param>
        <param name='height'>0</param>
      </params>
  </node>
</gegl>
"""

class TestGeglNode(unittest.TestCase):

    def test_exists(self):
        Gegl.Node

    def test_new(self):
        graph = Gegl.Node.new()
        self.assertEqual(type(graph), gi.repository.Gegl.Node)

class TestGeglNodeLoadXml(unittest.TestCase):

    def setUp(self):
        self.graph = Gegl.Node.new_from_xml(invert_crop_xml, "")

    def test_number_of_children(self):
        children = self.graph.get_children()

        self.assertEqual(len(children), 2)

    def test_child_operation(self):
        children = self.graph.get_children()

        self.assertEqual(children[0].get_operation(), "gegl:crop")
        self.assertEqual(children[1].get_operation(), "gegl:invert")


class TestGeglNodeSaveXml(unittest.TestCase):

    def setUp(self):
        self.graph = Gegl.Node.new()

    # Easiest way to test to_xml when we can't yet build graphs programatically
    def test_load_save_roundtrip(self):
        graph = Gegl.Node.new_from_xml(invert_crop_xml, "")
        output = graph.to_xml("")

        self.assertEqual(output, invert_crop_xml)

if __name__ == '__main__':
    Gegl.init(0, "");
    #print dir(Gegl.Node)

    unittest.main()

