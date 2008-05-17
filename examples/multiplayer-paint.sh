#!/bin/sh

./2gegl data/car-stack.jpg test.gegl
./clock test.gegl &
./gegl-paint test.gegl &
./gegl-paint test.gegl &
gegl -x '<gegl><open-buffer path="test.gegl"/></gegl>'

killall -9 clock gegl-paint lt-clock lt-gegl-paint


