#!/usr/bin/env python

from gi.repository import Gegl
import sys

# extend GEGL binding with some utility function, making it possible to lookup
# a node among the children of a node giving one with a specified name. A
# GEGL binding based on gobject introspection could contain some other such,
# that makes << and other things used in the py/ruby bindings before possible,
# this binding would itself just import things using GI

# This is a very minimal example, that doesnt even construct the graph and
# instantiate the nodes, but cheats and uses XML to get at an intial graph.
# it expect a file /tmp/lena.png to exist

def locate_by_type(self, opname):
  for i in self.get_children():
     if i.get_operation() == opname:
        return i
Gegl.Node.locate_by_type = locate_by_type


if __name__ == '__main__':
    Gegl.init(sys.argv)

    node = Gegl.Node.new_from_xml("""
      <gegl>
        <gegl:save path='/tmp/output.png'/>
        <gegl:crop width='512' height='512'/>
        <gegl:over >
          <gegl:translate x='30' y='30'/>
          <gegl:dropshadow radius='1.5' x='3' y='3'/>
          <gegl:text size='80' color='white'
><params><param name='string'>GEGL
 I
 R</param></params></gegl:text>
        </gegl:over>
        <gegl:unsharp-mask std-dev='30'/>
        <gegl:load path='/tmp/lena.png'/>
      </gegl>
""", "/");

    node.locate_by_type('gegl:save').process()
