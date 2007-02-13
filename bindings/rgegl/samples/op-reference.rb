#!/usr/bin/env ruby

$LOAD_PATH.unshift "../src"
$LOAD_PATH.unshift "../src/lib"

require 'glib2'
require 'gegl'


puts "<html><body>"

puts "<ul>"
Gegl.operations.each {|operation_name| puts "<li><a href='##{operation_name}'>#{operation_name}</li>"}
puts "</ul>"

Gegl.operations.each {|operation_name|
    puts "<a name='#{operation_name}'></a><h3>#{operation_name}</h3>"
    puts "<table>"
    Gegl.properties(operation_name).each {|property|
        case property.gtype
        when GLib::Type["GParamDouble"]
                puts "<tr><td>#{property.value_type}</td><td>#{property.name}</td><td>#{property.blurb}</td><td>min:#{property.minimum} max:#{property.maximum} default:#{property.default}</td></tr>"
        else
                puts "<tr><td>#{property.value_type}</td><td>#{property.name}</td><td>#{property.blurb}</td></tr>"
        end

    }
    puts "</table>"
}

puts "</body></html>"
