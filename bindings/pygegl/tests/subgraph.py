#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the subgraph.rb sample script written
#by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import gegl
from time import sleep

node = gegl.Node()                    # create a new GEGL graph

fractal = node.new_child("FractalExplorer")

unsharp   = node.new_child()
    in_proxy  = unsharp.input_proxy "input"
    out_proxy = unsharp.output_proxy "output"
    blur      = unsharp.new_child :gaussian_blur, :std_dev_x=>5.0, :std_dev_y=>5.0
    subtract  = unsharp.new_child :subtract
    add       = unsharp.new_child :add

    in_proxy >> subtract
    in_proxy >> add
    in_proxy >> blur >> subtract[:aux] >> add[:aux] >> out_proxy

display = node.new_child("display",
                         window_title => "rnode subgraph test")

fractal.link(unsharp, display)        # connect the nodes

display.process()
sleep(2)
