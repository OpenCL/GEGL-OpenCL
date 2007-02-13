#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the op_reference.rb sample script written
#by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import gegl

operations = gegl.gegl_list_operations()

print "<html><body>"

print "<ul>"
for operation_name in operations:
   print "<li><a href='%s'>%s</li>", operation_name, operation_name

print "</ul>"

for operation_name in operations:
    print "<a name='%s'></a><h3>%s</h3>", operation_name, operation_name
    print "<table>"
    Gegl.properties(operation_name).each {|property|
        case property.gtype
        when GLib::Type["GParamDouble"]
                print "<tr><td>#{property.value_type}</td><td>#{property.name}</td><td>#{property.blurb}</td><td>min:#{property.minimum} max:#{property.maximum} default:#{property.default}</td></tr>"
        else
                print "<tr><td>#{property.value_type}</td><td>#{property.name}</td><td>#{property.blurb}</td></tr>"
        end

    }
    print "</table>"
}

print "</body></html>"
