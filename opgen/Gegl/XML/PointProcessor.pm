#!/usr/bin/perl -w

# Copyright 2000-2001 Jay Cox <jaycox@gimp.org>

# this code may be used under the terms of the GPL

package Gegl::XML::PointProcessor;

use Gegl::XML;
use Gegl::PointProcessor;

sub parse
  {
    my ($tree) = @_;
    my (%processor, $tmp);

    $processor = Gegl::PointProcessor::new();

    # It may look like we are violating the encapsulation of the
    # Gegl::Point::Processor class, but we are "friends with privileges"

    $processor->{precision}  = lc $tree->getAttributeNode ("precision" )->getValue;
    $processor->{colormodel} = lc $tree->getAttributeNode ("colormodel")->getValue;

    foreach (qw/variables per_pixel per_color per_alpha cleanup/)
      {
	if ($tmp = Gegl::XML::query_single($tree, $_))
	  { $processor->{$_} = $tmp->xql_toString(); }
      }

    return $processor;
  }


1;  # package return code
