#include <stdio.h>
#include <stdlib.h>
#include "gegl.h"
#include "gegl-check.h"
#include "testutils.h"

gboolean
testutils_check_pixel_rgb_float(GeglImage *image, 
                           gfloat a, 
                           gfloat b, 
                           gfloat c)
{
  return testutils_check_pixel_rgb_float_xy(image, 0, 0, a, b, c);
}

gboolean
testutils_check_pixel_rgb_float_xy(GeglImage *image, 
                              gint x, gint y,
                              gfloat a, gfloat b, gfloat c)
{
  gboolean success;

  GeglImageBuffer * image_buffer = gegl_image_get_image_buffer(image);
  GeglOp * check = g_object_new(GEGL_TYPE_CHECK, 
                                "pixel-rgb-float", a, b, c, 
                                "x", x, "y", y,
                                "image_buffer", image_buffer,
                                NULL);
  gegl_op_apply(check); 

  success = gegl_check_get_success(GEGL_CHECK(check)); 

  g_object_unref(check);

  return success;
}

gboolean
testutils_check_rgb_uint8(GeglImage *image, 
                           guint8 a, 
                           guint8 b, 
                           guint8 c)
{
  return testutils_check_rgb_uint8_xy(image, 0, 0, a, b, c);
}

gboolean
testutils_check_rgb_uint8_xy(GeglImage *image, 
                             gint x, gint y,
                             guint8 a, guint8 b, guint8 c)
{
  gboolean success;

  GeglImageBuffer * image_buffer = gegl_image_get_image_buffer(image);
  GeglOp * check = g_object_new(GEGL_TYPE_CHECK, 
                                "pixel-rgb-uint8", a, b, c, 
                                "x", x, "y", y,
                                "image_buffer", image_buffer,
                                NULL);
  gegl_op_apply(check); 

  success = gegl_check_get_success(GEGL_CHECK(check)); 

  g_object_unref(check);

  return success;
}
