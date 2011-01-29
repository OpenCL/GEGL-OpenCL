/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES


gegl_chant_double   (length, _("Length"), 0.0, 200.0, 10.0,
                  _("Length of blur in pixels"))
gegl_chant_double   (angle,  _("Angle"),  -360, 360, 0,
                  _("Angle of blur in degrees"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "motion-blur.c"

#include "gegl-chant.h"

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter* op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO* o = GEGL_CHANT_PROPERTIES (operation);

  gdouble theta = o->angle * M_PI / 180.0;
  gdouble offset_x = fabs(o->length * cos(theta));
  gdouble offset_y = fabs(o->length * sin(theta));

  op_area->left   =
  op_area->right  = (gint)ceil(0.5 * offset_x);
  op_area->top    =
  op_area->bottom = (gint)ceil(0.5 * offset_y);

  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

static inline gfloat*
get_pixel_color(gfloat* in_buf, 
                const GeglRectangle* rect, 
                gint x,
                gint y)
{
  gint ix = x - rect->x;
  gint iy = y - rect->y;
  ix = CLAMP(ix, 0, rect->width-1);
  iy = CLAMP(iy, 0, rect->height-1);
  
  return &in_buf[(iy*rect->width + ix) * 4];
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi)
{
  GeglRectangle src_rect;
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area;
  gfloat* in_buf;
  gfloat* out_buf;
  gfloat* out_pixel;
  gint x,y;
  
  gdouble theta = o->angle * M_PI / 180.0;
  gdouble offset_x = o->length * cos(theta);
  gdouble offset_y = o->length * sin(theta);
  gint num_steps = (gint)ceil(o->length) + 1;
  gfloat inv_num_steps = 1.0f / num_steps;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);

  src_rect = *roi;
  src_rect.x -= op_area->left;
  src_rect.y -= op_area->top;
  src_rect.width += op_area->left + op_area->right;
  src_rect.height += op_area->top + op_area->bottom;

  in_buf = g_new (gfloat, src_rect.width * src_rect.height * 4);
  out_buf = g_new0 (gfloat, roi->width * roi->height * 4);
  out_pixel = out_buf;

  gegl_buffer_get (input, 1.0, &src_rect, babl_format ("RaGaBaA float"), in_buf, GEGL_AUTO_ROWSTRIDE);
    
  for (y=0; y<roi->height; ++y)
    {
      for (x=0; x<roi->width; ++x)
        {
          gint step;
          gint c;
          gint px = x+roi->x;
          gint py = y+roi->y;
          gfloat sum[4] = {0,0,0,0};
          for (step=0; step<num_steps; ++step)
            {
              gdouble t = num_steps == 1 ? 0.0 : step / (gdouble)(num_steps-1) - 0.5;
              
              /* get the interpolated pixel position for this step */
              gdouble xx = px + t*offset_x;
              gdouble yy = py + t*offset_y;
              gint ix = (gint)floor(xx);
              gint iy = (gint)floor(yy);
              gdouble dx = xx - floor(xx);
              gdouble dy = yy - floor(yy);

              /* do bilinear interpolation to get a nice smooth result */
              gfloat *pix0, *pix1, *pix2, *pix3;
              gfloat mixy0[4];
              gfloat mixy1[4];
              
              pix0 = get_pixel_color(in_buf, &src_rect, ix, iy);
              pix1 = get_pixel_color(in_buf, &src_rect, ix+1, iy);
              pix2 = get_pixel_color(in_buf, &src_rect, ix, iy+1);
              pix3 = get_pixel_color(in_buf, &src_rect, ix+1, iy+1);
              for (c=0; c<4; ++c)
                {
                  mixy0[c] = dy*(pix2[c] - pix0[c]) + pix0[c];
                  mixy1[c] = dy*(pix3[c] - pix1[c]) + pix1[c];
                  sum[c] += dx*(mixy1[c] - mixy0[c]) + mixy0[c];
                }
            }
            
          for (c=0; c<4; ++c)
            *out_pixel++ = sum[c] * inv_num_steps;
        }
    }
  
  gegl_buffer_set (output, roi, babl_format ("RaGaBaA float"), out_buf, GEGL_AUTO_ROWSTRIDE);
  
  g_free (in_buf);
  g_free (out_buf);

   
  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationFilterClass      *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:motion-blur";
  operation_class->categories  = "blur";
  operation_class->description = _("Linear motion blur");
}

#endif
