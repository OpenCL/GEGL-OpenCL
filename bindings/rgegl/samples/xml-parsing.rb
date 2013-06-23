#!/usr/bin/env ruby

$LOAD_PATH.unshift "../src"
$LOAD_PATH.unshift "../src/lib"
require 'gtk2'
require 'gegl'


gegl=Gegl.parse_xml(
"<gegl>
   <gegl:display name='display'/>
   <gegl:crop x='0' y='0' width='640' height='300'/>
   <gegl:over>
     <gegl:translate x='20.0' y='120.0' name='shift'/>
     <gegl:opacity value='0.4' name='opacity' />
     <gegl:gaussian-blur std_dev_y='0' name='blur'/>
     <gegl:text string='GEGL' name='text' size='60' color='rgb(0.0,0.0,0.0)'/>
   </gegl:over>
   <gegl:invert-linear/>
   <gegl:brightness-contrast contrast='0.4' brightness='-0.3'/>
   <gegl:gaussian-blur />
   <gegl:fractal-explorer xmin='0.2' ymin='0' xmax='0.5' ymax='0.45'
     width='640' height='300'/>
</gegl>", "")

"GEGL
image processing
compositing graph
linear RGB
32bit float RGB
128bit/pixel
caching
".split("\n").each {|title|

  gegl.lookup("text").string = title
  gegl.lookup('shift').x = 640/2- gegl.lookup('text').bounding_box.width/2;

# should use a realtime frame rate for this demo instead.
frames=20
frames.times do |frame|
  t = 1.0*frame/(frames-1)
  #gegl.lookup("shift").y       =  t * 200 - 40
  gegl.lookup("opacity").value  =  t
  gegl.lookup("blur").std_dev_x = (1.0-t) * 100.0
  gegl.lookup("blur").std_dev_y = (1.0-t) * 0.0
  gegl.lookup("display").process
end

sleep 2

frames.times do |frame|
  t = 1.0*frame/(frames-1)
  gegl.lookup("opacity").value  =  1.0-t
  gegl.lookup("blur").std_dev_x = (t) * 100.0
  gegl.lookup("blur").std_dev_y = (t) * 0.0
  gegl.lookup("display").process
end
}
