#include <stdio.h>
#include <stdlib.h>
#include <glib-object.h>
#include "gegl.h"
#include "testutils.h"

GeglSampledImage *
make_rgb_float_sampled_image(gint width, 
                            gint height, 
                            gfloat a, 
                            gfloat b, 
                            gfloat c) 
{
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");

  GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                    "colormodel", rgb_float,  
                                    NULL);  

  GeglChannelValue * chans = gegl_color_get_channel_values(color);

  GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                                 "colormodel", rgb_float,
                                                 "width", width, 
                                                 "height", height,
                                                 NULL);  

  GeglOp *fill = g_object_new(GEGL_TYPE_FILL, 
                              "fillcolor", color, 
                              NULL);

  chans[0].f = a;
  chans[1].f = b;
  chans[2].f = c;

  gegl_op_apply(fill, sampled_image, NULL); 

  g_object_unref(fill);
  g_object_unref(color);
  g_object_unref(rgb_float);

  return sampled_image;
}

GeglSampledImage *
make_gray_float_sampled_image(gint width, 
                             gint height, 
                             gfloat a) 
{
  GeglColorModel *gray_float = gegl_color_model_instance("GrayFloat");

  GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                    "colormodel", gray_float,  
                                    NULL);  

  GeglChannelValue * chans = gegl_color_get_channel_values(color);

  GeglSampledImage * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                                 "colormodel", gray_float,
                                                 "width", width, 
                                                 "height", height,
                                                 NULL);  

  GeglOp *fill = g_object_new(GEGL_TYPE_FILL, 
                            "fillcolor", color, 
                            NULL);

  chans[0].f = a;

  gegl_op_apply(fill, sampled_image, NULL); 

  g_object_unref(fill);
  g_object_unref(color);
  g_object_unref(gray_float);

  return sampled_image;
}

gboolean
check_rgb_float_pixel(GeglImage *dest, gfloat a, gfloat b, gfloat c)
{
  gfloat *data[3];
  GeglImageImpl * image_impl = (GeglImageImpl*)gegl_op_get_op_impl(GEGL_OP(dest));
  GeglImageMgr *image_mgr = gegl_image_mgr_instance();
  GeglTile *tile = gegl_simple_image_mgr_get_tile(GEGL_SIMPLE_IMAGE_MGR(image_mgr), image_impl); 
  g_object_unref(image_impl);
  g_object_unref(image_mgr);
  gegl_tile_get_data_at(tile, (gpointer*)data, 0,0);

  if (GEGL_FLOAT_EQUAL(*data[0] , a) &&
      GEGL_FLOAT_EQUAL(*data[1] , b) &&
      GEGL_FLOAT_EQUAL(*data[2] , c))
    return TRUE;
  else
    return FALSE;
} 

gboolean
check_gray_float_pixel(GeglImage *dest, gfloat a)
{
  gfloat *data[1];
  GeglImageImpl * image_impl = (GeglImageImpl*)gegl_op_get_op_impl(GEGL_OP(dest));
  GeglImageMgr *image_mgr = gegl_image_mgr_instance();
  GeglTile *tile = gegl_simple_image_mgr_get_tile(GEGL_SIMPLE_IMAGE_MGR(image_mgr), image_impl); 
  g_object_unref(image_impl);
  g_object_unref(image_mgr);
  gegl_tile_get_data_at(tile, (gpointer*)data, 0,0);

  if (GEGL_FLOAT_EQUAL(*data[0] , a))
    return TRUE;
  else
    return FALSE;
} 
