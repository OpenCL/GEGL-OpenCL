#!/usr/bin/env luajit

local lgi = require 'lgi'
local GObject = lgi.GObject --
local Gegl = lgi.Gegl 

Gegl.init(arg)
local graph = Gegl.Node()

local src = graph:create_child("gegl:load")

src:set_property("path", GObject.Value(GObject.Type.STRING, "data/surfer.png"))

local crop = graph:create_child('gegl:crop')
crop:set_property("x", GObject.Value(GObject.Type.INT, 0))
crop:set_property("y", GObject.Value(GObject.Type.INT, 0))
crop:set_property("width", GObject.Value(GObject.Type.INT, 300))
crop:set_property("height", GObject.Value(GObject.Type.INT, 122))

local dst = graph:create_child("gegl:save")

dst:set_property("path", GObject.Value(GObject.Type.STRING, "lgi.png"))

local text = graph:create_child("gegl:text")
local white = Gegl.Color()
white:set_rgba(1,1,1,1)
text:set_property("string", GObject.Value(GObject.Type.STRING, "luajit + lgi + GEGL test"))
text:set_property("color", GObject.Value(GObject.Type.OBJECT, white))

local over = graph:create_child("gegl:over")

src:connect_to("output", crop, "input")
crop:connect_to("output", over, "input")
text:connect_to("output", over, "aux")
over:connect_to("output", dst, "input")
dst:process()

