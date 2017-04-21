#!/usr/bin/env python
from __future__ import print_function

import os
import sys

if len(sys.argv) != 2:
  print("Usage: %s file.c" % sys.argv[0])
  sys.exit(1)

# From http://stackoverflow.com/questions/14945095/how-to-escape-string-for-generated-c
def escape_string(s):
  return s.replace('\\', r'\\').replace('"', r'\"')

if "module.c" in sys.argv[1]:
   exit()

infile  = open(sys.argv[1], "r")
outfile = open(sys.argv[1] + ".h", "w")

cl_source = infile.read()
infile.close()

string_var_name = os.path.basename(sys.argv[1]).replace("-", "_").replace(":", "_")
if string_var_name.endswith(".c"):
  string_var_name = string_var_name.rstrip(".c")

outfile.write("static const char* %s_c_source =\n" % "op")
for line in cl_source.rstrip().split("\n"):
  line = line.rstrip()
  line = escape_string(line)
  line = '"%-78s\\n"\n' % line
  outfile.write(line)
outfile.write(";\n")
outfile.close()
