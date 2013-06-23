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

invert_crop_xml = """<?xml version='1.0' encoding='UTF-8'?>
<gegl>
  <node operation='gegl:invert-linear'>
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

class TestGeglNodes(unittest.TestCase):

    def test_exists(self):
        Gegl.Node

    def test_new(self):
        graph = Gegl.Node.new()
        self.assertEqual(type(graph), Gegl.Node)

    def test_node_properties(self):
        graph = Gegl.Node()
        node  = graph.create_child("gegl:nop")

        self.assertEqual("gegl:nop", node.get_property("operation"))

        node.set_property("operation", "gegl:translate")
        self.assertEqual("gegl:translate", node.get_property("operation"))

        default_x = node.get_property("x")
        default_sampler = node.get_property("sampler")

        self.assertIsNotNone(default_x)
        self.assertIsNotNone(default_sampler)

        node.set_property("x", 10)
        self.assertEqual(node.get_property("x"), 10)

        node.set_property("x", -10)
        self.assertEqual(node.get_property("x"), -10)

        node.set_property("sampler", Gegl.SamplerType.NEAREST)
        self.assertEqual(node.get_property("sampler"), Gegl.SamplerType.NEAREST)

        node.set_property("sampler", "linear")
        self.assertEqual(node.get_property("sampler"), Gegl.SamplerType.LINEAR)

        node.set_property("operation", "gegl:nop")
        self.assertEqual("gegl:nop", node.get_property("operation"))

        node.set_property("operation", "gegl:translate")
        self.assertEqual("gegl:translate", node.get_property("operation"))

        self.assertEqual(node.get_property("x"), default_x)
        self.assertEqual(node.get_property("sampler"), default_sampler)

    def test_create_graph(self):
        graph = Gegl.Node()
        color_node = graph.create_child("gegl:color")
        crop_node  = graph.create_child("gegl:crop")

        self.assertEqual(color_node.get_operation(), "gegl:color")
        self.assertEqual(crop_node.get_operation(), "gegl:crop")

        crop_rect = Gegl.Rectangle.new(10, 20, 5, 15)

        crop_node.set_property("x", crop_rect.x)
        crop_node.set_property("y", crop_rect.y)
        crop_node.set_property("width", crop_rect.width)
        crop_node.set_property("height", crop_rect.height)

        color_node.connect_to("output", crop_node, "input")

        self.assertTrue(crop_rect.equal(crop_node.get_bounding_box()))

        trans_node = graph.create_child("gegl:translate")

        crop_node.connect_to("output", trans_node, "input")

        self.assertTrue(crop_rect.equal(trans_node.get_bounding_box()))

        trans_node.set_property("x", 10)

        self.assertFalse(crop_rect.equal(trans_node.get_bounding_box()))

        trans_rect = crop_rect.dup()
        trans_rect.x += 10

        self.assertTrue(trans_rect.equal(trans_node.get_bounding_box()))

class TestGeglXml(unittest.TestCase):

    def test_load_xml(self):
        graph = Gegl.Node.new_from_xml(invert_crop_xml, "")

        children = graph.get_children()

        self.assertEqual(len(children), 2)

        self.assertEqual(children[0].get_operation(), "gegl:crop")
        self.assertEqual(children[1].get_operation(), "gegl:invert-linear")

    def test_load_save_roundtrip(self):
        graph = Gegl.Node.new_from_xml(invert_crop_xml, "")
        output = graph.to_xml("")

        self.assertEqual(output, invert_crop_xml)

if __name__ == '__main__':
    Gegl.init(None);
    unittest.main()
    Gegl.exit()
