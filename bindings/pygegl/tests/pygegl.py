#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of an older copy of the rgegl.rb sample
#script written by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import gegl

print "Press ctrl-C to stop"

node = gegl.Node()

# create nodes belonging in our graph
fractal = node.new_child("FractalExplorer",
                         xmin = 0.0,
                         ymin = 0.0,
                         xmax = 0.5,
                         ymax = 0.5)
blur    = node.new_child("gaussian-blur",
                         std_dev_x = 10,
                         std_dev_y = 10)
over    = node.new_child("over")
text    = node.new_child("text",
                         color = gegl.Color("rgb(0.4,0.5,1.0)"),
                         size=130,
                         string="pygegl")
shift   = node.new_child("shift", x=20, y=170)
save    = node.new_child("png-save",
                         path="pygegl.png")

# chain bits of the graph together
text.link(blur, shift)
fractal.link(over, save)
shift.connect_to("output", over, "aux")

save.process()
