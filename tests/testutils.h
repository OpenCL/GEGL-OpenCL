#ifndef __TEST_UTILS_H__
#define __TEST_UTILS_H__

#include <glib-object.h>
#include "gegl.h"

GeglOp *
testutils_fill(gchar * color_model_name,
                    gfloat a,
                    gfloat b,
                    gfloat c,
                    gfloat d);
GeglOp *
testutils_sampled_image(gchar * color_model_name,
                        gint width, gint height, 
                        gfloat a, gfloat b, gfloat c, gfloat d); 

gboolean testutils_check_pixel(GeglImage *dest, 
                               gchar * color_model_name,
                               gfloat a, gfloat b, gfloat c, gfloat d);
gboolean testutils_check_pixel_tile(GeglTile *tile, 
                               gchar * color_model_name,
                               gfloat a, gfloat b, gfloat c, gfloat d);

gboolean testutils_check_pixel_at_xy(GeglImage *dest, 
                                     gchar * color_model_name,
                                     gint x, gint y,
                                     gfloat a, gfloat b, gfloat c, gfloat d);
gboolean testutils_check_pixel_at_xy_tile(GeglTile *tile, 
                                          gchar * color_model_name,
                                          gint x, gint y,
                                          gfloat a, gfloat b, gfloat c, gfloat d);

GeglOp * testutils_rgb_float_fill(gfloat a, gfloat b, gfloat c);
GeglOp * testutils_rgb_float_sampled_image(gint width, gint height, gfloat a, gfloat b, gfloat c); 

gboolean testutils_check_rgb_float_pixel(GeglImage *dest, 
                                         gfloat a, gfloat b, gfloat c);
gboolean testutils_check_rgb_float_pixel_xy(GeglImage *dest, 
                                            gint x, gint y, 
                                            gfloat a, gfloat b, gfloat c);
gboolean testutils_check_rgb_float_pixel_tile(GeglTile *tile, 
                                         gfloat a, gfloat b, gfloat c);
gboolean testutils_check_rgb_float_pixel_xy_tile(GeglTile *tile, 
                                         gint x, gint y, 
                                         gfloat a, gfloat b, gfloat c);

#endif
