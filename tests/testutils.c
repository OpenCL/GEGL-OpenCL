#include <stdio.h>
#include <stdlib.h>
#include "gegl.h"
#include "testutils.h"

GeglOp *
testutils_rgb_fill(gfloat a, 
                   gfloat b, 
                   gfloat c)
{
  GeglOp * fill = NULL;
  GeglColorModel *rgb_float = gegl_color_model_instance("RgbFloat");
  GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                    "colormodel", rgb_float,  
                                    NULL);  

  GeglChannelValue * chans = gegl_color_get_channel_values(color);

  chans[0].f = a; 
  chans[1].f = b; 
  chans[2].f = c;

  fill = g_object_new(GEGL_TYPE_FILL, 
                      "fillcolor", color,
                      NULL);

  g_object_unref(rgb_float);
  g_object_unref(color);

  return fill;
}


GeglOp *
testutils_rgb_float_sampled_image(gint width, 
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

  GeglOp * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
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

  gegl_op_apply_image(fill, sampled_image, NULL); 

  g_object_unref(fill);
  g_object_unref(color);
  g_object_unref(rgb_float);

  return sampled_image;
}

GeglOp *
testutils_gray_float_sampled_image(gint width, 
                                   gint height, 
                                   gfloat a) 
{
  GeglColorModel *gray_float = gegl_color_model_instance("GrayFloat");

  GeglColor * color = g_object_new (GEGL_TYPE_COLOR, 
                                    "colormodel", gray_float,  
                                    NULL);  

  GeglChannelValue * chans = gegl_color_get_channel_values(color);

  GeglOp * sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                         "colormodel", gray_float,
                                         "width", width, 
                                         "height", height,
                                         NULL);  

  GeglOp *fill = g_object_new(GEGL_TYPE_FILL, 
                            "fillcolor", color, 
                            NULL);

  chans[0].f = a;

  gegl_op_apply_image(fill, sampled_image, NULL); 

  g_object_unref(fill);
  g_object_unref(color);
  g_object_unref(gray_float);

  return sampled_image;
}

gboolean
testutils_check_rgb_float_pixel(GeglImage *dest, 
                                gfloat a, 
                                gfloat b, 
                                gfloat c)
{
  gfloat *data[3];
  GeglTile *tile = dest->tile; 

  gegl_tile_get_data_at(tile, (gpointer*)data, 0,0);

  LOG_DEBUG("testutils_check_rgb_float_pixel",  "data %f %f %f" ,*data[0], *data[1], *data[2]);
  LOG_DEBUG("testutils_check_rgb_float_pixel",  "a,b,c %f %f %f" , a, b, c);

  if (GEGL_FLOAT_EQUAL(*data[0] , a) &&
      GEGL_FLOAT_EQUAL(*data[1] , b) &&
      GEGL_FLOAT_EQUAL(*data[2] , c))
    return TRUE;
  else
    return FALSE;
} 

gboolean
testutils_check_gray_float_pixel(GeglImage *dest, 
                                 gfloat a)
{
  gfloat *data[1];
  GeglTile *tile = dest->tile; 
  gegl_tile_get_data_at(tile, (gpointer*)data, 0,0);

  if (GEGL_FLOAT_EQUAL(*data[0] , a))
    return TRUE;
  else
    return FALSE;
} 
