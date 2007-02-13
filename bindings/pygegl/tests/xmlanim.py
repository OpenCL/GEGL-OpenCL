#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the xmlanim.rb sample script written
#by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import gegl

node = gegl.parse_xml(
"""
<gegl>
   <crop x='0' y='145' width='400' height='200'/>
   <over>
     <gaussian-blur std_dev_y='0' name='blur'/>
     <shift x='20' y='170' name='shift'/>
     <text string='pygegl' size='120' color='rgb(0.5,0.5,1.0)'/>
   </over>
   <FractalExplorer xmin='0.2' ymin='0' xmax='0.5' ymax='0.45'
                    width='400' height='400'/>
</gegl>
""", "")

display = node.new_child("display")
node.link(display)

frames=10
for frame in range(frames+1):
  blur.set(std_dev_x = (frames-frame)*3.0) # animate the composition
  shift.set(y       = 20*frame)            #
  display.process()

