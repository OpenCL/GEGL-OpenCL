#!/usr/bin/env ruby

$LOAD_PATH.unshift "../src"
$LOAD_PATH.unshift "../src/lib"
require 'gtk2'
require 'gegl'
require 'gegl-view'

gegl=Gegl.parse_xml(
"<gegl>
   <over>
     <gaussian-blur std_dev_y='0' name='blur'/>
     <shift x='20' y='170' name='shift'/>
     <text string='rgegl' size='120' color='rgb(0.5,0.5,1.0)'/>
   </over>
   <fractal-explorer xmin='0.2' ymin='0' xmax='0.5' ymax='0.45'
                    width='400' height='400'/>
</gegl>", "")

app = Gtk::Window.new
view = Gegl::View.new
view.node = gegl
app.set_default_size 400,400

app << view
app.show_all

GLib::Timeout.add(100){
   view.scale *= 1.01
   view.x += 2
   view.y += 2
   true
}

Gtk.main
