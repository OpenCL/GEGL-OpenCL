#!/usr/bin/env python

from gi.repository import Gegl
import sys

if __name__ == '__main__':
  Gegl.init(sys.argv)
  
  ptn = Gegl.Node()
  
  # Disable caching on all child nodes
  ptn.set_property("dont-cache", True)
  
  # Create our background buffer. A gegl:color node would
  # make more sense, we just use a buffer here as an example.
  background_buffer = Gegl.Buffer.new("RGBA float", 246, -10, 276, 276)
  white = Gegl.Color.new("#FFF")
  background_buffer.set_color(background_buffer.get_extent(), white)
  
  src = ptn.create_child("gegl:load")
  src.set_property("path", "data/surfer.png")
  
  crop = ptn.create_child("gegl:crop")
  crop.set_property("x", 256)
  crop.set_property("y", 0)
  crop.set_property("width", 256)
  crop.set_property("height", 256)
  
  buffer_src = ptn.create_child("gegl:buffer-source")
  buffer_src.set_property("buffer",background_buffer)
  
  over = ptn.create_child("gegl:over")
  
  dst = ptn.create_child("gegl:save")
  dst.set_property("path", "cropped.png")
  
  # The parent node is only for reference tracking, we need to
  # connect the node's pads to actualy pass data between them.
  buffer_src.connect_to("output", over, "input")
  src.connect_to("output", crop, "input")
  crop.connect_to("output", over, "aux")
  over.connect_to("output", dst, "input")
  
  # Will create "cropped.png" in the current directory
  dst.process()

