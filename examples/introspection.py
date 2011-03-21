#!/usr/bin/env python

# This file is a part of GEGL, and is licensed under the same conditions

"""Example script showing how GEGL can be used from Python using
GObject introspection."""

# Requires PyGObject 2.26 or later
# To use a typelib from a non-standard location, set the env var
# GI_TYPELIB_PATH to the directory with your Gegl-$apiversion-.typelib

import gi
from gi.repository import Gegl

if __name__ == '__main__':
    # Right now just a sanity check
    node = Gegl.Node.new()
    print dir(node)
