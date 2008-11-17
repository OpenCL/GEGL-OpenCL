#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the xmlanim.rb sample script written
#by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import Gegl

gegl = Gegl.node_new_from_xml(
"""
<gegl>
   <gegl:display name='display'/>
   <gegl:crop x='0' y='145' width='400' height='200'/>
   <gegl:over>
     <gegl:translate x='20' y='170' name='translate'/>
     <gegl:gaussian-blur std_dev_x='10' std_dev_y='0' name='blur'/>
     <gegl:text string='pygegl' size='110' color='rgb(0.5,0.5,1.0)'/>
   </gegl:over>
   <gegl:fractal-explorer xmin='0.2' ymin='0' xmax='0.5' ymax='0.45'
                          width='400' height='400'/>
</gegl>
""")

display = gegl.lookup("display")

frames=30
for frame in range(frames):
    gegl.lookup("translate").y        = (frame*1.0)/frames*200.0           #
    display.process()

frames=10
for frame in range(frames):
    gegl.lookup("blur").std_dev_x = (1.0-((frame*1.0)/frames))*10.0 # animate the composition
    display.process()

frames=10
for frame in range(frames):
    gegl.lookup("translate").y        = 200+(frame*1.0)/frames*200.0           #
    display.process()
