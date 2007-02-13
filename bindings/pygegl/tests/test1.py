#This is the original test script which came with the pygegl source.

import gegl
from random import randrange

print "Press ctrl-C to stop";

node = gegl.Node()

disp = node.new_child("display",
                      window_title="test of pygegl")

vortex = node.new_child("jpg-load",
                        path="images/vortex.jpg")

eye = node.new_child("png-load",
                     path="images/olho.png")

# operations

trans = node.new_child("translate",
                       x=50.0,
                       y=50.0)

scale = node.new_child("scale",
                       x=0.5,
                       y=0.5)

invert = node.new_child("invert")

over = node.new_child("over")


scale.connect_from    ("input", eye, "output")
trans.connect_from    ("input", scale, "output")

invert.connect_from   ("input", vortex, "output")

over.connect_from     ("input", invert, "output")
over.connect_from     ("aux",   trans, "output")

disp.connect_from     ("input", over, "output")


#, (0, 0, 738, 517))

while 1:
  disp.process()
  trans.set(x = randrange(0,640))
  trans.set(y = randrange(0,480))

