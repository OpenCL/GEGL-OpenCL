#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <glib-object.h>
#include "gegl.h"

GeglOp * testutils_rgb_fill(gfloat a, gfloat b, gfloat c);
GeglOp * testutils_rgb_float_sampled_image(gint width, gint height, gfloat a, gfloat b, gfloat c); 
GeglOp * testutils_gray_float_sampled_image(gint width, gint height, gfloat a); 

gboolean testutils_check_rgb_float_pixel(GeglImage *dest, gfloat a, gfloat b, gfloat c);
gboolean testutils_check_gray_float_pixel(GeglImage *dest, gfloat a);

#endif
