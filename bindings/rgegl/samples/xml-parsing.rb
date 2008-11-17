#!/usr/bin/env ruby

$LOAD_PATH.unshift "../src"
$LOAD_PATH.unshift "../src/lib"
require 'gtk2'
require 'gegl'


gegl=Gegl.parse_xml(
"<gegl>
   <gegl:display name='display'/>
   <gegl:crop x='0' y='145' width='320' height='240'/>
   <gegl:over>
     <gegl:gaussian-blur std_dev_y='0' name='blur'/>
     <gegl:shift x='20' y='170' name='shift'/>
     <gegl:text string='rgegl' size='120' color='rgb(0.5,0.5,1.0)'/>
   </gegl:over>
   <gegl:fractal-explorer xmin='0.2' ymin='0' xmax='0.5' ymax='0.45'
                    width='320' height='400'/>
</gegl>", "")

frames=50
frames.times do |frame|
  t = 1.0*frame/frames
  gegl.lookup("blur").std_dev_x = t * 20.0
  gegl.lookup("shift").y       =  t * 200
  gegl.lookup("display").process
end
