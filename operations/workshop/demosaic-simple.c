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


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (pattern, _("Bayer pattern"), 0, 3, 0,
                _("Bayer pattern used, 0 seems to work for some nikon files, 2 for some Fuji files."))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "demosaic-simple.c"

#include "gegl-chant.h"

static void
demosaic (GeglChantO *op,
          GeglBuffer *src,
          GeglBuffer *dst)
{
  const GeglRectangle *src_extent = gegl_buffer_get_extent (src);
  const GeglRectangle *dst_extent = gegl_buffer_get_extent (dst);
  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src));
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 3);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("Y float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset=0;
  for (y=src_extent->y; y < dst_extent->height + src_extent->y; y++)
    {
      gint src_offset = (y-src_extent->y) * src_extent->width;
      for (x=src_extent->x; x < dst_extent->width + src_extent->x; x++)
        {
          gfloat red=0.0;
          gfloat green=0.0;
          gfloat blue=0.0;

          if (y<dst_extent->height+dst_extent->y &&
              x<dst_extent->width+dst_extent->x)
            {
          if ((y + op->pattern%2)%2==0)
            {
              if ((x+op->pattern/2)%2==1)
                {
                  blue=src_buf[src_offset+1];
                  green=src_buf[src_offset];
                  red=src_buf[src_offset + src_extent->width];
                }
              else
                {
                  blue=src_buf[src_offset];
                  green=src_buf[src_offset+1];
                  red=src_buf[src_offset+1+src_extent->width];
                }
            }
          else
            {
              if ((x+op->pattern/2)%2==1)
                {
                  blue=src_buf[src_offset + src_extent->width + 1];
                  green=src_buf[src_offset + 1];
                  red=src_buf[src_offset];
                }
              else
                {
                  blue=src_buf[src_offset + src_extent->width];
                  green=src_buf[src_offset];
                  red=src_buf[src_offset + 1];
                }
            }
          }

          dst_buf [offset*3+0] = red;
          dst_buf [offset*3+1] = green;
          dst_buf [offset*3+2] = blue;

          offset++;
          src_offset++;
        }
    }

  gegl_buffer_set (dst, NULL, babl_format ("RGB float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->right = area->bottom = 1;
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer   *temp_in;
  GeglRectangle compute = gegl_operation_get_required_for_output (operation, "input", result);

  temp_in = gegl_buffer_create_sub_buffer (input, &compute);

  demosaic (o, temp_in, output);

  g_object_unref (temp_in);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->name        = "gegl:demosaic-simple";
  operation_class->categories  = "blur";
  operation_class->description =
        _("Performs a naive grayscale2color demosaicing of an image, no interpolation.");
}
#endif
