#!/usr/bin/perl -w

# Copyright 2000-2001 Jay Cox <jaycox@gimp.org>

# this code may be used under the terms of the GPL

package Gegl::XML::PointOp;

use Gegl::PointOp;
use Gegl::XML::PointProcessor;

@ISA = qw( Gegl::XML );

sub new
  {
    my $proto = shift;
    my $type = ref($proto) || $proto;
    my $self = $type->SUPER::new();

  }

sub parse
  {
    my ($tree) = @_;
    my ($tmp, @tmps);
    my $op;

    $op = Gegl::PointOp::new();

    # It may look like we are violating the encapsulation of the
    # Gegl::Point::Op class, but we are "friends with privileges"

    $op->{name} = $tree->getAttributeNode ("name")->getValue;
    $op->{arguments} = $tree->getAttributeNode ("arguments")->getValue;
# The XML Parser should fill in the default value for the language attribute
# but it doesn't so we have to fill it in ourselves :(
    if (defined $tree->getAttributeNode ("language"))
      {
	$op->{language} = $tree->getAttributeNode ("language")->getValue;
      }
    else
      {
	$op->{language} = "c";
      };

    foreach (qw/description author copyright license libraries 
                constructor destructor variables prepare/)
      {
	if ($tmp = Gegl::XML::query_single($tree, $_))
	  { $processor->{$_} = $tmp->xql_toString(); }
      }

    @tmps = Gegl::XML::query_multiple($tree, "PointProcessor");
    $i = 0;
    foreach (@tmps)
      {
	$op->{processors}[$i] = Gegl::XML::PointProcessor::parse($_);
#	print_hash(%{$op->{processors}[$i]});
	$i++
      };
    return $op;
  }


1;  # package return code
