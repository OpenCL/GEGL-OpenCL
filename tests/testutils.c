#include <stdio.h>
#include <stdlib.h>
#include "gegl.h"
#include "testutils.h"

GeglOp *
testutils_fill(gchar * color_model_name,
               gfloat a,
               gfloat b,
               gfloat c,
               gfloat d)
{
  GeglColorModel *color_model = gegl_color_model_instance(color_model_name);
  GeglColor * color = g_object_new (GEGL_TYPE_COLOR, "colormodel", color_model,  NULL);  
  GeglChannelValue * chans = gegl_color_get_channel_values(color);
  GeglColorModelType type = gegl_color_model_color_model_type(color_model);
  GeglOp * fill = NULL;

  switch(type)
    {
    case GEGL_COLOR_MODEL_TYPE_RGB_FLOAT:
      chans[0].f = a; 
      chans[1].f = b; 
      chans[2].f = c;
      break;
    case GEGL_COLOR_MODEL_TYPE_RGBA_FLOAT:
      chans[0].f = a; 
      chans[1].f = b; 
      chans[2].f = c;
      chans[3].f = d;
      break;
    case GEGL_COLOR_MODEL_TYPE_GRAY_FLOAT:
      chans[0].f = a; 
      break;
    case GEGL_COLOR_MODEL_TYPE_GRAYA_FLOAT:
      chans[0].f = a; 
      chans[1].f = b; 
      break;
    default:
      g_warning("Cant create fill for color model type\n");
      return NULL;
    }

  fill = g_object_new(GEGL_TYPE_FILL, "fillcolor", color, NULL);

  g_object_unref(color);

  return fill;
}

GeglOp *
testutils_sampled_image(gchar * color_model_name,
                        gint width, gint height, 
                        gfloat a, gfloat b, gfloat c, gfloat d) 
{
  GeglColorModel *color_model = gegl_color_model_instance(color_model_name); 
  GeglOp * fill = testutils_fill(color_model_name, a, b, c, d);
  GeglOp * sampled_image;

  sampled_image = g_object_new (GEGL_TYPE_SAMPLED_IMAGE,
                                "width", width, 
                                "height", height,
                                "colormodel", color_model,
                                NULL);  

  gegl_op_apply_image(fill, sampled_image, NULL); 

  g_object_unref(fill);

  return sampled_image;
}


gboolean
testutils_check_pixel(GeglImage *dest, 
                      gchar * color_model_name,  
                      gfloat a, gfloat b, gfloat c, gfloat d)
{
  GeglTile *tile = dest->tile; 
  return testutils_check_pixel_at_xy_tile(tile, color_model_name, 
                                          0,0,
                                          a,b,c,d);
}

gboolean
testutils_check_pixel_tile(GeglTile *tile, 
                           gchar * color_model_name,
                           gfloat a, gfloat b, gfloat c, gfloat d)
{
  return testutils_check_pixel_at_xy_tile(tile, color_model_name,
                                          0,0,
                                          a,b,c,d);

}

gboolean
testutils_check_pixel_at_xy(GeglImage *dest, 
                            gchar * color_model_name,  
                            gint x, gint y,
                            gfloat a, gfloat b, gfloat c, gfloat d)
{
  GeglTile *tile = dest->tile; 
  return testutils_check_pixel_at_xy_tile(tile, color_model_name, 
                                          x,y,
                                          a,b,c,d);
}

gboolean
testutils_check_pixel_at_xy_tile(GeglTile *tile, 
                                 gchar *color_model_name,
                                 gint x, gint y,
                                 gfloat a, gfloat b, gfloat c, gfloat d)
{
  GeglColorModel *color_model_to_check = gegl_color_model_instance(color_model_name);
  GeglColorModel *tile_color_model = gegl_tile_get_color_model(tile);
  GeglColorModelType type = gegl_color_model_color_model_type(tile_color_model);
  gfloat *data[4];

  if(color_model_to_check != tile_color_model)
    {
      g_warning("Unexpected color model for tile\n");
      return FALSE; 
    }

  switch(type)
    {
    case GEGL_COLOR_MODEL_TYPE_RGB_FLOAT:
      {
        gegl_tile_get_data_at(tile, (gpointer*)data, x,y);
        if (GEGL_FLOAT_EQUAL(*data[0] , a) &&
            GEGL_FLOAT_EQUAL(*data[1] , b) &&
            GEGL_FLOAT_EQUAL(*data[2] , c))
          return TRUE;
        else
          g_warning("expected: %f %f %f,  found %f %f %f\n",
                     a, b, c, *data[0], *data[1], *data[2]); 
      }
      break;
    case GEGL_COLOR_MODEL_TYPE_RGBA_FLOAT:
      {
        gegl_tile_get_data_at(tile, (gpointer*)data, x,y);
        if (GEGL_FLOAT_EQUAL(*data[0] , a) &&
            GEGL_FLOAT_EQUAL(*data[1] , b) &&
            GEGL_FLOAT_EQUAL(*data[2] , c) &&
            GEGL_FLOAT_EQUAL(*data[3] , d))
          return TRUE;
        else
          g_warning("expected: %f %f %f %f,  found %f %f %f %f\n",
                     a, b, c, d, *data[0], *data[1], *data[2], *data[3]); 
      }
      break;
    case GEGL_COLOR_MODEL_TYPE_GRAY_FLOAT:
      {
        gegl_tile_get_data_at(tile, (gpointer*)data, x,y);
        if (GEGL_FLOAT_EQUAL(*data[0] , a))
          return TRUE;
        else
          g_warning("expected: %f,  found %f\n",
                     a, *data[0]); 
      }
      break;
    case GEGL_COLOR_MODEL_TYPE_GRAYA_FLOAT:
      {
        gegl_tile_get_data_at(tile, (gpointer*)data, x,y);
        if (GEGL_FLOAT_EQUAL(*data[0] , a) &&
            GEGL_FLOAT_EQUAL(*data[1] , b))
          return TRUE;
        else
          g_warning("expected: %f %f,  found %f %f\n",
                     a, b, *data[0], *data[1]); 

      }
      break;
    default:
      g_warning("Cant check pixel for color model type\n");
      break;
    }

  return FALSE;
}


/* RgbFloat routines */
GeglOp *
testutils_rgb_float_fill(gfloat a, 
                         gfloat b, 
                         gfloat c)
{
  return testutils_fill("RgbFloat", a, b, c, 0);
}

GeglOp *
testutils_rgb_float_sampled_image(gint width, 
                                  gint height, 
                                  gfloat a, 
                                  gfloat b, 
                                  gfloat c) 
{
  return testutils_sampled_image("RgbFloat",
                                 width, height, 
                                 a, b, c,  0); 

}

gboolean
testutils_check_rgb_float_pixel(GeglImage *dest, 
                                gfloat a, 
                                gfloat b, 
                                gfloat c)
{
  GeglTile *tile = dest->tile; 
  return testutils_check_rgb_float_pixel_xy_tile(tile,0,0, a,b,c);
} 


gboolean
testutils_check_rgb_float_pixel_tile(GeglTile *tile, 
                                     gfloat a, 
                                     gfloat b, 
                                     gfloat c)
{
  return testutils_check_rgb_float_pixel_xy_tile(tile, 0,0,a,b,c);
} 

gboolean
testutils_check_rgb_float_pixel_xy(GeglImage *dest, 
                                   gint x, gint y,
                                   gfloat a, gfloat b, gfloat c)
{
  GeglTile *tile = dest->tile; 
  return testutils_check_rgb_float_pixel_xy_tile(tile,x,y,a,b,c);
} 

gboolean
testutils_check_rgb_float_pixel_xy_tile(GeglTile *tile, 
                                        gint x, gint y,
                                        gfloat a, gfloat b, gfloat c)
{
  return testutils_check_pixel_at_xy_tile(tile, "RgbFloat",  
                                          x, y, a, b, c, 0);
} 
