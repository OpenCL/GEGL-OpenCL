#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <glib-object.h>
#include "gegl.h"

gboolean
testutils_check_pixel_rgb_float(GeglImageOp *image_op, 
                           gfloat a, 
                           gfloat b, 
                           gfloat c);
gboolean
testutils_check_pixel_rgb_float_xy(GeglImageOp *image_op, 
                              gint x, gint y,
                              gfloat a, gfloat b, gfloat c);
gboolean
testutils_check_rgb_uint8(GeglImageOp *image_op, 
                           guint8 a, 
                           guint8 b, 
                           guint8 c);
gboolean
testutils_check_rgb_uint8_xy(GeglImageOp *image_op, 
                              gint x, gint y,
                              guint8 a, guint8 b, guint8 c);
#endif
