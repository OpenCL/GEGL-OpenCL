#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <glib-object.h>
#include "gegl.h"

gboolean
testutils_check_pixel_rgb_float(GeglImage *image, 
                           gfloat a, 
                           gfloat b, 
                           gfloat c);
gboolean
testutils_check_pixel_rgb_float_xy(GeglImage *image, 
                              gint x, gint y,
                              gfloat a, gfloat b, gfloat c);
gboolean
testutils_check_rgb_uint8(GeglImage *image, 
                           guint8 a, 
                           guint8 b, 
                           guint8 c);
gboolean
testutils_check_rgb_uint8_xy(GeglImage *image, 
                              gint x, gint y,
                              guint8 a, guint8 b, guint8 c);
#endif
