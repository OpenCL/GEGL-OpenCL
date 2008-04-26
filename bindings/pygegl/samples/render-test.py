#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the render-test.rb sample script
#written by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import Gegl
import sys

greyscale = ["█", "▓", "▒", "░", " "]
reverse_video = False

width = 40
height = 40

gegl    = Gegl.Node()
fractal = gegl.new_child("fractal-explorer",
                         width=width,
                         height=height,
                         ncolors=3)
contrast= gegl.new_child("threshold", value=0.5)
text    = gegl.new_child("text",
                         string="GEGL\n\n term",
                         size=height/4)
over    = gegl.new_child("over")
display = gegl.new_child("screen")

fractal >> contrast >> over >> display
text >> over["aux"]

buffer = over.render((0,0,width,height), "Y u8")

pos = 0
for y in range(height):
    for x in range(width):
        char = int(ord(buffer[pos]) / 256.0 * len(greyscale))
        if reverse_video:
            char = len(greyscale) - char - 1
        sys.stdout.write(greyscale[char])
        pos += 1
    print
