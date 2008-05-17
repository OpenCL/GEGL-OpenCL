#!/bin/sh

./2gegl data/car-stack.jpg test.gegl
./clock test.gegl &
./gegl-paint test.gegl &
./gegl-paint test.gegl

killall -9 clock gegl-paint


