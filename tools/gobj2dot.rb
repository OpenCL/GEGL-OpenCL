#!/usr/bin/ruby

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
# Copyright (C) 2009 Henrik Akesson

require 'find'

if ARGV[0] == nil or ARGV.length != 1 or ARGV[0] == "-h"
  puts "Utility for generating inheritance diagrams from c-code using "
  puts "the gobject library"
  puts "Usage: gobjviz.rb path"
  puts "       or"
  puts "       gobjviz.rb path | dot -Tpng > output.png"
  exit;
end

dir = ARGV[0]

header = <<HDR
digraph G {
        rankdir = RL

        fontname = "Bitstream Vera Sans"
        fontsize = 8

        node [ shape = "record" ]

        edge [ arrowhead = empty ]
HDR

footer = <<FTR
}
FTR

def tocamelcase( underscored )
  underscored.gsub!( /^[a-z]|_[a-z]/ ) { |a| a.upcase }
  underscored.gsub!( /_/, '')
end

def gegltocamelcase( gegltype )
  gegltype =~ /([a-zA-Z0-9_]+)_TYPE_([a-zA-Z0-9_]+)/
  gegltype = tocamelcase( $1.downcase + "_" + $2.downcase )
end

def inheritance( from, to )
  puts "\t" + from + " -> " + to if ( to != "GObject")
end

def implementation( klass, interf )
  puts "\t" + klass + " -> " + interf + " [ style = dashed ]"
end

def interfacedecl( interf )
  puts "\t" + interf + " [ label = \"{\\<\\<interface\\>\\>\\l" + interf + "}\" ]"
end

puts header
Find.find( dir ) do |path| 
  Find.prune if File.basename(path) == '.git' 
  if ( ( File.extname(path) == '.c' or File.extname(path) == '.h' ) and File.file? path )
    open( path ) do |file| 
      content = file.read
      if( content =~ /G_DEFINE_TYPE\s*\(\s*\w+,\s*(\w+),\s*(\w+)\s*\)/m )
        inheritance( tocamelcase( $1 ), gegltocamelcase( $2 ) )
      elsif( content =~ /G_DEFINE_TYPE_WITH_CODE\s*\(\s*(\w+).*G_IMPLEMENT_INTERFACE\s*\s*\(\s*(\w+)/m )
        implementation( $1, gegltocamelcase( $2 ) )
      elsif( content =~ /G_TYPE_INTERFACE\s*,\s*\"([^\"]+)\"/m )
        interfacedecl( $1 )
      end
    end
  end
end
puts footer
