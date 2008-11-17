#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the subgraph.rb sample script written
#by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import Gegl
from time import sleep

gegl = Gegl.Node()                    # create a new GEGL graph

fractal = gegl.new_child("gegl:fractal-explorer")

unsharp = gegl.new_child("gegl:unsharp-mask")

in_proxy  = unsharp.get_input_proxy("input")
out_proxy = unsharp.get_output_proxy("output")
blur      = unsharp.new_child("gegl:gaussian-blur",
                              std_dev_x=5.0, std_dev_y=5.0)
subtract  = unsharp.new_child("gegl:subtract")
add       = unsharp.new_child("gegl:add")

in_proxy >> subtract
in_proxy >> add
in_proxy >> blur >> subtract["aux"] >> add["aux"] >> out_proxy

display = gegl.new_child("gegl:png-save", path="test.png")

fractal >> unsharp >> display # connect the nodes

display.process()
sleep(2)
