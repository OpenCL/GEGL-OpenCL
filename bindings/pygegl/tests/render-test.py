#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the render-test.rb sample script
#written by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

class Rectangle:
    def __init__(self, x=0, y=0, width=5, height=5):
	self.x = x
	self.y = y
	self.width = width
	self.height = height

import gegl

greyscale=["█", "▓", "▒", "░", " "]
reverse_video=False

width=40
height=40

gegl    = gegl.Node()
fractal = gegl.new_child("FractalExplorer",
                         width=width,
                         height=height,
                         ncolors=3)
contrast= gegl.new_child("threshold", value=0.5)
text    = gegl.new_child("text",
                         string="GEGL\n\n term",
                         size=height/4)
over    = gegl.new_child("over")
display = gegl.new_child("display")

fractal.link(contrast, over, display)
text.connect_to("output", over, "aux")

buffer = over.render(Rectangle(0.0,0.0,width,height), 1.0, "Y u8", 0)

pos=0
for y in range(height):
    for x in range(width):
        char = int( ord(buffer[pos])/256.0*len(greyscale) )
        if reverse_video:
            char=len(greyscale)-char-1
        sys.stdout.write(greyscale[char])
        pos += 1
    print

