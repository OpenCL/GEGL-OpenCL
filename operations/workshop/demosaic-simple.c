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
#if GEGL_CHANT_PROPERTIES

gegl_chant_int (pattern, 0, 3, 0, "Bayer pattern used, 0 seems to work for some nikon files, 2 for some Fuji files.")
 
#else

#define GEGL_CHANT_NAME            demosaic_simple
#define GEGL_CHANT_SELF            "demosaic-simple.c"
#define GEGL_CHANT_DESCRIPTION     "Performs a naive grayscale2color demosaicing of an image, no interpolation."
#define GEGL_CHANT_CATEGORIES      "blur"

#define GEGL_CHANT_AREA_FILTER

#include "gegl-chant.h"

static void
demosaic (GeglChantOperation *op,
          GeglBuffer *src,
          GeglBuffer *dst);

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  GeglOperationFilter *filter;
  GeglChantOperation  *self;
  GeglBuffer          *input;
  GeglBuffer          *output;

  filter = GEGL_OPERATION_FILTER (operation);
  self   = GEGL_CHANT_OPERATION (operation);

  input  = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));

    {
      const GeglRectangle *result = gegl_operation_result_rect (operation, context_id);
      GeglBuffer    *temp_in;
      GeglRectangle    compute  = gegl_operation_compute_input_request (operation, "inputt", gegl_operation_need_rect (operation, context_id));

      
      temp_in = gegl_buffer_create_sub_buffer (input, &compute);
      output = gegl_buffer_new (&compute, babl_format ("RGBA float"));

      demosaic (self, temp_in, output);
      g_object_unref (temp_in);
      

      {
        GeglBuffer *cropped = gegl_buffer_create_sub_buffer (output, result);
        gegl_operation_set_data (operation, context_id, "output", G_OBJECT (cropped));
        g_object_unref (output);
      }
    }


  return  TRUE;
}

static void
demosaic (GeglChantOperation *op,
          GeglBuffer *src,
          GeglBuffer *dst)
{
  const GeglRectangle *src_extent = gegl_buffer_get_extent (src);
  const GeglRectangle *dst_extent = gegl_buffer_get_extent (dst);
  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_malloc0 (gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_malloc0 (gegl_buffer_get_pixel_count (dst) * 4 * 3);
  
  gegl_buffer_get (src, 1.0, NULL, babl_format ("Y float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset=0;
  for (y=src_extent->y; y<dst_extent->height + src_extent->y; y++)
    for (x=src_extent->x; x<dst_extent->width + src_extent->x; x++)
      {
        gfloat red=0.0;
        gfloat green=0.0;
        gfloat blue=0.0;

        if (y<dst_extent->height+src_extent->y-1 &&
            x<dst_extent->width+src_extent->x-1)
          {
        if ((y + op->pattern%2)%2==0)
          {
            if ((x+op->pattern/2)%2==1)
              {
                blue=src_buf[offset+1];
                green=src_buf[offset];
                red=src_buf[offset + src_extent->width];
              }
            else
              {
                blue=src_buf[offset];
                green=src_buf[offset+1];
                red=src_buf[offset+1+src_extent->width];
              }
          }
        else
          {
            if ((x+op->pattern/2)%2==1)
              {
                blue=src_buf[offset + src_extent->width + 1];
                green=src_buf[offset + 1];
                red=src_buf[offset];
              }
            else
              {
                blue=src_buf[offset + src_extent->width];
                green=src_buf[offset];
                red=src_buf[offset + 1];
              }
          }
        }
        
        dst_buf [offset*3+0] = red;
        dst_buf [offset*3+1] = green;
        dst_buf [offset*3+2] = blue;

        offset++;
      }

  gegl_buffer_set (dst, NULL, babl_format ("RGB float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void tickle (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->right = area->bottom = 1;
}

#endif
