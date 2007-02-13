#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the graph_construction.rb sample script
#written by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import gegl as Gegl
from time import sleep

gegl = Gegl.Node()                    # create a new GEGL graph

fractal = gegl.new_child("FractalExplorer",
                         xmin      = 0,
                         ymin      = 0,
                         xmax      = 0.5,
                         ymax      = 0.5)
over    = gegl.new_child("over")
text    = gegl.new_child("text",
                         string    = "pygegl",
                         size      = 100,
                         color     = Gegl.Color("rgb(0.4,0.5,1.0)"))
blur    = gegl.new_child("gaussian-blur",
                         std_dev_y = 0)
shift   = gegl.new_child("shift",
                         x         = 20,
                         y         = 170)
crop    = gegl.new_child("crop",
                         x         = 0,
                         y         = 145,
                         width     = 400,
                         height    = 200)
display = gegl.new_child("display",
                         window_title = "pygegl")

fractal.link(over, crop, display)     # connect the nodes
text.link(blur, shift)                #
shift.connect_to("output", over, "aux")

frames=10
for frame in range(frames+1):
  blur.set(std_dev_x = (frames-frame)*3.0) # animate the composition
  shift.set(y       = 20*frame)            #
  display.process()

sleep(2)
