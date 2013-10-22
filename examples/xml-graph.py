#!/usr/bin/env python
from __future__ import print_function

from gi.repository import Gegl
import sys

def check_operations(required_ops):
  known_ops = Gegl.list_operations()

  missing_ops = []

  for op in required_ops:
    if not op in known_ops:
      print("Could not find required operation:", op)
      missing_ops.append(op)

  return not missing_ops

out_file_name = "xml-graph-output.png"

text_xml = """
<?xml version='1.0' encoding='UTF-8'?>
<gegl>
  <node operation='gegl:text'>
    <params>
      <param name='string'>Hello World</param>
      <param name='color'>rgb(1.0, 1.0, 1.0)</param>
      <param name='size'>24</param>
    </params>
  </node>
</gegl>
"""

background_xml = """
<?xml version='1.0' encoding='UTF-8'?>
<gegl>
  <node operation='gegl:plasma'>
    <params>
      <param name='width'>256</param>
      <param name='height'>128</param>
    </params>
  </node>
</gegl>
"""

if __name__ == '__main__':
  Gegl.init(sys.argv)

  if not check_operations(["gegl:png-save", "gegl:text", "gegl:plasma", "gegl:translate"]):
    sys.exit(1)

  graph = Gegl.Node()

  # Load the graphs from xml, if they were in external files
  # we would use Gegl.Node.new_from_file() instead.
  lower_graph = Gegl.Node.new_from_xml(background_xml, "/")
  upper_graph = Gegl.Node.new_from_xml(text_xml, "/")

  # Add a reference from our main node to the xml graphs so they
  # can't go out of scope as long as "graph" is alive.
  graph.add_child(lower_graph)
  graph.add_child(upper_graph)

  # Center the upper graph on the lower graph
  text_bbox = upper_graph.get_bounding_box()
  background_bbox = lower_graph.get_bounding_box()

  x_offset = max((background_bbox.width - text_bbox.width) / 2, 0)
  y_offset = max((background_bbox.height - text_bbox.height) / 2, 0)

  translate = graph.create_child("gegl:translate")
  translate.set_property("x", x_offset)
  translate.set_property("y", y_offset)

  upper_graph.connect_to("output", translate, "input")

  # Use the "gegl:over" to combine the two graphs
  over = graph.create_child("gegl:over")
  lower_graph.connect_to("output", over, "input")
  translate.connect_to("output", over, "aux")

  # Save the result to a png file
  save_node = graph.create_child ("gegl:png-save")
  save_node.set_property("path", out_file_name)
  over.connect_to("output", save_node, "input")
  save_node.process()

  print("Save to", out_file_name)
