#!/usr/bin/perl

# Copyright 2000-2001 Jay Cox <jaycox@gimp.org>

# this code may be used under the terms of the GPL


package Gegl::PointOp;

use Gegl::Config;
use Gegl::Op;

@ISA = ( Gegl::Op );

sub new
  {
    my($class) = @_;
    $class = Gegl::PointOp unless $class;
    my $op = Gegl::Op::new(%data);
    my $i;
    # Initialize the data members to help with error checking

    $op->{parent_class} = "Gegl::N::Src::Op";

    bless ($op, $class);
    return $op;
  }


sub print_variables
  {
    my $op = shift;
   #use precisions, colorspaces +1 because of COLORSPACE_NONE, PRECISION_NONE
    print "  classwide SCANLINE_FUNC_P scanline_funcs[" . (@Gegl::precisions + 1) . "][" . (@Gegl::colorspaces + 1) . "];\n";
    print "\n";
    if ($op->{variables})
      {
	print "  /******************* <user variables>  ******************/\n";
	print "  $op->{variables}\n";
	print "  /******************* </user variables> ******************/\n";
	print "\n";      }
  }

sub print_constructor
  {
    my $op = shift;
    my $nargs = @{$op->{buffer_args}};
    my $chain_args = "";

    foreach (@{$op->{buffer_args}})
      {
	$chain_args = $chain_args  . ", " . $_;
      }
    print <<HERE;
  protected gboolean constructor(self, $op->{arguments})
  {
    GList *inputs = NULL;

    if(GEGL_OBJECT(self)->constructed)
      return FALSE;
HERE
    foreach (@{$op->{buffer_args}})
      {
        print "    inputs = g_list_append(inputs, $_);\n"
      }

    print <<HERE;

    /* Chain up */
    if (!gegl_n_src_op_constructor (GEGL_N_SRC_OP(self), inputs)
      return FALSE;

    return TRUE;
  }

HERE
  }

sub pad_string # This sub should probably go somewhere else
  {
    my ($s, $len) = @_;
    return $s . ' ' x ($len - (length $s));
  }

sub print_class_init
  {
    my $op = shift;
    print <<HERE;
  class_init(class)
  {
HERE

    foreach $cspace (@Gegl::colorspaces)
      {
	foreach $prec (@Gegl::precisions)
	  {
	    $var = "    class->scanline_funcs[" .
	      pad_string($Gegl::precision{$prec}{enum}, 11) . "][" .
		pad_string("$Gegl::colormodels{$cspace}{enum}", 9) . "]";
	    print $var;
	    print " = ";
	    $func = $op->get_handler($cspace, $prec);
	    if ($func)
	      {
		print $func . ";\n";
	      }
	    else
	      {
		print "NULL;\n";
	      }
	  }
      }
    print "  }\n\n";
  }

sub print_prepare
  {
    my $op = shift;
    return unless $op->{prepare};
    print <<HERE;
  override(Gegl:N:Src:Op)
  void
  prepare(GeglOp *self_op,
          GeglImage *dest,
	  GeglRect *dest_rect)
  {
    $op->{prepare}

    gegl_n_src_op_prepare(self_op, dest, dest_rect);
  }

HERE
  }

sub print_class_specific
  {
    my ($op) = shift;
    my $processor;
    foreach $processor (@{$op->{processors}})
      {
	#foreach precision???
	$processor->print_scanline_function($op, $processor);
      }
  }


sub get_handler
  {
# need to look at precision also
# need to handle multiple colormodels in a processor
# how do we name the scanline func if it handles multiple?
    my ($op, $cspace, $prec) = @_;
    my ($i, $j);
    foreach $i (@{%Gegl::colormodels->{$cspace}->{hierarchy}})
      {
        foreach $j (@{$op->{processors}})
	  {
	    if ($j->{colormodel} eq $i || $j->{colormodel} eq "all")
	      {
		return "scanline_${prec}_$j->{colormodel}";
	      }
	  }
      }
  }

1;  # package return code
