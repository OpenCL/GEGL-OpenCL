#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <glib-object.h>
#include "gegl.h"

GeglSampledImage * make_rgb_float_sampled_image(gint width, gint height, gfloat a, gfloat b, gfloat c); 
GeglSampledImage * make_gray_float_sampled_image(gint width, gint height, gfloat a); 

gboolean check_rgb_float_pixel(GeglImage *dest, gfloat a, gfloat b, gfloat c);
gboolean check_gray_float_pixel(GeglImage *dest, gfloat a);

#endif
