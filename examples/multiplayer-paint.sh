#!/bin/sh

./2geglbuffer data/surfer.png test.gegl
./geglbuffer-add-image test.gegl data/surfer.png 64 64
./geglbuffer-clock test.gegl &

./gegl-paint test.gegl &
./gegl-paint test.gegl &

gegl -x '<gegl><open-buffer path="test.gegl"/></gegl>'

killall -9 geglbuffer-clock gegl-paint lt-geglbuffer-clock lt-gegl-paint
