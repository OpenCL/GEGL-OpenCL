#!/usr/bin/env ruby

$LOAD_PATH.unshift "../src"
$LOAD_PATH.unshift "../src/lib"

require 'gegl'

gegl = Gegl::Node.new                 # create a new GEGL graph

fractal = gegl.new_child :FractalExplorer

unsharp   = gegl.new_child
    in_proxy  = unsharp.input_proxy "input"
    out_proxy = unsharp.output_proxy "output"
    blur      = unsharp.new_child :gaussian_blur, :std_dev_x=>5.0, :std_dev_y=>5.0
    subtract  = unsharp.new_child :subtract
    add       = unsharp.new_child :add

    in_proxy >> subtract
    in_proxy >> add
    in_proxy >> blur >> subtract[:aux] >> add[:aux] >> out_proxy

display = gegl.new_child :png_save,
                         :path => "test.png"

fractal >> unsharp >> display # connect the nodes

display.process
sleep 2
