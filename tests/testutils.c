#include <stdio.h>
#include <stdlib.h>
#include "gegl.h"
#include "gegl-check.h"
#include "testutils.h"

gboolean
testutils_check_pixel_rgb_float(GeglImageOp *image_op, 
                           gfloat a, 
                           gfloat b, 
                           gfloat c)
{
  return testutils_check_pixel_rgb_float_xy(image_op, 0, 0, a, b, c);
}

gboolean
testutils_check_pixel_rgb_float_xy(GeglImageOp *image_op, 
                              gint x, gint y,
                              gfloat a, gfloat b, gfloat c)
{
  gboolean success;

  GeglImage * image = gegl_image_op_get_image(image_op);
  GeglOp * check = g_object_new(GEGL_TYPE_CHECK, 
                                "pixel-rgb-float", a, b, c, 
                                "x", x, "y", y,
                                "image", image,
                                NULL);
  gegl_op_apply(check); 

  success = gegl_check_get_success(GEGL_CHECK(check)); 

  g_object_unref(check);

  return success;
}

gboolean
testutils_check_rgb_uint8(GeglImageOp *image_op, 
                           guint8 a, 
                           guint8 b, 
                           guint8 c)
{
  return testutils_check_rgb_uint8_xy(image_op, 0, 0, a, b, c);
}

gboolean
testutils_check_rgb_uint8_xy(GeglImageOp *image_op, 
                             gint x, gint y,
                             guint8 a, guint8 b, guint8 c)
{
  gboolean success;

  GeglImage * image = gegl_image_op_get_image(image_op);
  GeglOp * check = g_object_new(GEGL_TYPE_CHECK, 
                                "pixel-rgb-uint8", a, b, c, 
                                "x", x, "y", y,
                                "image", image,
                                NULL);
  gegl_op_apply(check); 

  success = gegl_check_get_success(GEGL_CHECK(check)); 

  g_object_unref(check);

  return success;
}
