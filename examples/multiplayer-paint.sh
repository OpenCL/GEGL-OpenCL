#!/bin/sh

killall -9 geglbuffer-clock gegl-paint lt-geglbuffer-clock lt-gegl-paint

./2geglbuffer data/surfer.png test.gegl

./gegl-paint test.gegl &
./gegl-paint test.gegl &
sleep 3
#./geglbuffer-add-image test.gegl data/surfer.png 64 64
./geglbuffer-clock test.gegl 

