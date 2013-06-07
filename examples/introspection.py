#!/usr/bin/env python

# This file is a part of GEGL, and is licensed under the same conditions

"""Example script showing how GEGL can be used from Python using
GObject introspection."""

# Requires PyGObject 2.26 or later
# To use a typelib from a non-standard location, set the env var
# GI_TYPELIB_PATH to the directory with your Gegl-$apiversion-.typelib

from gi.repository import Gegl
import sys

# extend GEGL binding with some utility function that makes it possible
# to lookup a node among the children of a node giving one with a specified name.

def locate_by_type(self, opname):
  for i in self.get_children():
     if i.get_operation() == opname:
        return i
Gegl.Node.locate_by_type = locate_by_type

if __name__ == '__main__':
    Gegl.init(sys.argv)
    node = Gegl.Node.new()
    node = Gegl.Node.new_from_xml("""
      <gegl>
        <gegl:save path='/tmp/a.png'/>
        <gegl:crop width='512' height='512'/>
        <!--<gegl:over >
          <gegl:translate x='30' y='30'/>
          <gegl:dropshadow radius='1.5' x='3' y='3'/>
          <gegl:text size='80' color='white'
><params><param name='string'>GEGL
 I
 R</param></params></gegl:text>
        </gegl:over>-->
 
        <gegl:over>
        <gegl:vignette amount='1.0' radius='0.71' width='0.8' color='#0000' gamma='1.0'/>
        <gegl:crop width='512' height='512'/>
        <!-- <gegl:unsharp-mask std-dev='30'/> -->
        <!-- <gegl:stress /> -->
        <gegl:color value='red'/>
        <!-- <gegl:load path='/home/pippin/images/lena.png'/> -->
        </gegl:over>
        <gegl:color value='black'/>
        <!-- <gegl:checkerboard /> -->
 
      </gegl>
""", "/");

    node.locate_by_type('gegl:save').process()
