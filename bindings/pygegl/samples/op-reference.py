#!/usr/bin/env python
# -*- coding: utf-8 -*-
#This is a Python version of the op-reference.rb sample script written
#by Øyvind Kolås for the Ruby binding (rgegl) of gegl.

import Gegl
import gobject

operations = Gegl.list_operations()

print "<html><body>"

print "<ul>"
for operation_name in operations:
   print "<li><a href='#%s'>%s</li>" % (operation_name, operation_name)

print "</ul>"

for operation_name in operations:
    print "<a name='%s'></a><h3>%s</h3>" % (operation_name, operation_name)
    print "<table>"
    for property in Gegl.list_properties(operation_name):
        if property.value_type == gobject.TYPE_DOUBLE:
            print "<tr><td>%s</td><td>%s</td><td>%s</td><td>min:%s max:%s default:%s</td></tr>" % (property.value_type.name, property.name, property.blurb, property.minimum, property.maximum, property.default_value)
        else:
            print "<tr><td>%s</td><td>%s</td><td>%s</td></tr>" % (property.value_type.name, property.name, property.blurb)
    print "</table>"

print "</body></html>"
