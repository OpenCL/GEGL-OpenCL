#!/usr/bin/perl -w

# Copyright 2000-2001 Jay Cox <jaycox@gimp.org>

# this code may be used under the terms of the GPL


package Gegl::PointProcessor;

use Gegl::Config;
use Gegl::PointOp;

sub new
  {
    my $proto = shift;
    my $type = ref($proto) || $proto;
    $type = Gegl::PointProcessor unless $class;
    my $processor = {};
    # Do foo initialialization here
    return bless $processor, $type;
  }

sub setup
  {
    print "Need to write Gegl::Point::Processor::setup();\n";
  }

sub print_class
  {
    print "Need to write Gegl::Point::Processor::print_class();\n";
  }

sub print_scanline_function
  {
    my ($p) = shift;
    my ($op) = shift;
    my ($precision) = "u8";
    my ($colormodel) = $p->{colormodel};
    my $i;
    my $has_alpha = "";
    my ($per_pixel_alpha, $per_color_alpha);
    $has_alpha = 1 if ($p->{per_pixel} =~ /_has_alpha/ ||
		       $p->{per_color} =~ /_has_alpha/ ||
		       $p->{per_alpha} =~ /_has_alpha/ ||
		       $p->{cleanup}   =~ /_has_alpha/);

    print <<HERE;
  private
  void
  scanline_$p->{colormodel}_DATATYPE (GeglPointOp *self_point_op,
                       GeglImageIterator *dest_iter, 
		       GeglImageIterator **src_iters)
  {
    GeglOp *self_op = GEGL_OP(self_point_op);
    guint width = gegl_image_iterator_scanline_width (dest_iter);
    gulong mask_mask;
    /* --- User variables ---*/
    $p->{variables}
    /* ---------------------- */

    GENERIC_IMAGE_DECL_BEGIN
HERE
# Only compute the has_alpha variables if we are going to use them
    if ($has_alpha)
      {
        print "    Pixel dest(color, alpha, has_alpha);";
        foreach (@{$op->{buffer_args}})
          {
            print "    Pixel ${_}(color, alpha, has_alpha);\n";
          }
      }
    else
      {
        print "    Pixel dest(color,alpha);";
        foreach (@{$op->{buffer_args}})
          {
            print "    Pixel ${_}(color, alpha);\n";
          }
      }
    print <<HERE;
    GENERIC_IMAGE_DECL_END

HERE
    if ($has_alpha)
      {
        print "    dest_has_alpha = self_op->has_alpha;";
        $i = 0;
        foreach (@{$op->{buffer_args}})
          {
            print "    ${_}_has_alpha = self_op->src_has_alpha[$i];\n";
            $i++;
          }
      }

# Calculate the mask_mask bitmask
# moved gegl-n-src-op.gob
#    print "\n";
#    print "    mask_mask = 0;\n";
#    foreach (0 .. $#{$op->{buffer_args}})
#      {
#        my $i = $#{$op->{buffer_args}} - $_;
#        $print "    mask_mask = (mask_mask << 1) | $op->{buffer_args}[$i]_has_alpha;\n";
#      }
    print "\n";
    print "    mask_mask = self_n_src_op->_priv->mask_mask;\n";
    print "\n";

    print <<HERE;

    gegl_image_iterator_get_scanline_data (dest_iter,
					   (guchar**)dest_data);
HERE
    $i = 0;
    foreach (@{$op->{buffer_args}})
      {
	print "    gegl_image_iterator_get_scanline_data (src_iters[$i],\n".
	      "                                           (guchar**)${_}_data);\n";
	$i++;
      }

    my $cleanup = $p->{cleanup} || "";
    print <<HERE;
    GENERIC_IMAGE_IMAGE_DATA_INIT

    while (width--)
      {
        GENERIC_IMAGE_CODE_BEGIN
HERE

# per pixel
    if ($p->{per_pixel} =~ /_alpha/)
      {
        $per_pixel_alpha = 1; # need to do per pixel in alpha switch statment

        $per_color_alpha = 1; # must put per_color in switch also because
                              # it may depend on results from per_pixel
      }

    if ($p->{per_pixel} && !$per_pixel_alpha)
    {
      print "        /*-----------  <User per_pixel> -----------*/\n";
      print "        $p->{per_pixel}\n";
      print "        /*----------- </User per_pixel> -----------*/\n\n";
    }

# per color
    if ($p->{per_color} =~ /_alpha/)
      {
        $per_color_alpha = 1; # need to do per color in alpha switch statment
      }
    if ($p->{per_color} && !$per_color_alpha)
    {
      my $tmp = per_colorify($op, $p->{per_color});

      print "        /*-----------  <User per_color> -----------*/\n";
      print "        $tmp\n";
      print "        /*----------- </User per_color> -----------*/\n\n";

    }

    print <<HERE;

        if (dest_has_alpha)
          {
HERE


# Per Alpha
    if ($p->{per_alpha} || $per_color_alpha)
    {
      my $bits;
      print "           switch (mask_mask)\n";
      print "             {\n";
      $bits = (1 + length @{$op->{buffer_args}}) ** 2;

      while ($bits > 0)
        {
          $bits = $bits - 1;
          print "               case $bits:\n";
          if ($per_pixel_alpha)
            {
              print "                 " .
                    my_bit_alpha($p->{per_pixel}, $bits,
                                 [@{$op->{buffer_args}}]) . "\n";
            }
          if ($per_color_alpha)
            {
              print "                 " .
                    my_bit_alpha(per_colorify($op, $p->{per_color}), $bits,
                                 [@{$op->{buffer_args}}]) . "\n";
            }
          if ($p->{per_alpha})
            {
              print "                 " .
                    my_bit_alpha(per_alphify($op, $p->{per_alpha}), $bits,
                                 [@{$op->{buffer_args}}]) . "\n";
            }
          print "               break:\n";
        }
      print "               default:\n";
      print "                 g_error(\"Bad mask_mask value: %d\", mask_mask);\n";
      print "               break:\n";
      print "             }\n";
    }
    else
    {
      print "              default:\n";
      print "                dest_alpha = WP;\n";
    }

print <<HERE;

          }

        dX(dest, 1);
HERE

    foreach (@{$op->{buffer_args}})
      {
	print "        dX($_, 1);\n";
      }

   print <<HERE

        GENERIC_IMAGE_CODE_END
      }
      /* ----------  <user cleanup> ---------- */
      ${cleanup}
      /* ---------- </user cleanup> ---------- */
    }
HERE


  }

sub per_colorify
  {
    my $op  = $_[0];
    my $tmp = $_[1];

    $tmp =~ s/dest(?=[^_])/dest_color/g;

    foreach (@{$op->{buffer_args}})
      {
        my ($t2) = "$_" . "_color";
        $tmp =~ s/$_(?=[^_])/$t2/g;
      }

    return $tmp;
  }

sub per_alphify
  {
    my $op  = $_[0];
    my $tmp = $_[1];

    $tmp =~ s/dest(?=[^_])/dest_alpha/g;

    foreach (@{$op->{buffer_args}})
      {
        my ($t2) = "$_" . "_alpha";
        $tmp =~ s/$_(?=[^_])/$t2/g;
      }

    return $tmp;
  }


sub my_bit_alpha
  {
    my $tmp   = $_[0];
    my $num   = $_[1];
    my @buffs = @{$_[2]};
    my $len   = $#buffs;
    my $i = 1;
    my $first = 1;

    $tmp =~ s/dest(?=[^_])/dest_alpha/g;
    foreach (@buffs)
      {
        unless ($i & $num)
        {
          $tmp =~ s/${_}_alpha/WP/g;
        }
        $i = $i << 1;
      }
    return $tmp;
  }

1;  # package return code

