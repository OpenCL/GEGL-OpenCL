package Gegl;
# Copyright 2001 Jay Cox <jaycox@gimp.org>
# This code may be used under the terms of the LGPL v2 or later

#need to clean up the variable names here

@colorspaces = ( "grey", "rgb", "cmyk", "xyz", "hsv");
@precisions = ( "u8", "u16", "u16_4k", "float");


%precision = (
	      u8     => {
			   enum  => "GEGL_PRECISION_U8"
			},
	      u16    => {
			   enum  => "GEGL_PRECISION_U16"
			},
	      u16_4k => {
			   enum  => "GEGL_PRECISION_U16_4k"
			},
	      float  => {
			   enum  => "GEGL_PRECISION_FLOAT"
			}
	     );

# Heirarchy values:  intensity additive subtractive <need more>

%colormodels = (
		grey => {
			 hierarchy => [ "grey", "additive", "intensity" ],
			 channeled => 1,
			 additive  => 1,
			 nchannels => 1,
			 channels  => ["grey"],
			 enum      => "GEGL_COLORSPACE_GREY"
			},
		rgb  => {
			 hierarchy => [ "rgb", "additive", "intensity" ],
			 channeled => 1,
			 additive  => 1,
			 nchannels => 3,
			 channels  => [ "red", "green", "blue" ],
			 enum      => "GEGL_COLORSPACE_RGB"
			},
		cmyk => {
			 hierarchy => [ "cmyk", "subtractive", "intensity" ],
			 channeled => 1,
			 additive  => 1,
			 nchannels => 4,
			 channels  => [ "cyan", "magenta", "yellow", "black"],
			 enum      => "GEGL_COLORSPACE_CMYK"
			},
		xyz  => {
			 hierarchy => [ "xyz", "additive", "intensity" ],
			 channeled => 1,
			 additive  => 1,
			 nchannels => 3,
			 channels  => [ "x", "y", "z"],
			 enum      => "GEGL_COLORSPACE_XYZ"
			},
		hsv  => { # need to figure out real heirarchy for hsv
			 hierarchy => [ "hsv", "hued", "intensity" ],
			 channeled => 1,
			 nchannels => 3,
			 channels  => [ "hue", "saturation", "value"],
			 enum      => "GEGL_COLORSPACE_HSV"
			},
	       );


1;  # module return code
