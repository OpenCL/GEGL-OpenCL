#!/usr/bin/perl -w

# Copyright 2000-2001 Jay Cox <jaycox@gimp.org>

# this code may be used under the terms of the GPL

#package Gegl::Xml::Op;

use Gegl::XML;

#BEGIN

@ops = Gegl::XML::parse_file($ARGV[0]);
foreach (@ops)
  {
#    print "---------- printing an op --------------\n";
    $_->print_class();
  }
