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


#ifdef GEGL_PROPERTIES

property_int (pattern, _("Bayer pattern"), 0)
   description (_("Bayer pattern used, 0 seems to work for some nikon files, 2 for some Fuji files."))
   value_range (0, 3)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME demosaic_simple
#define GEGL_OP_C_SOURCE demosaic-simple.c

#include "gegl-op.h"

static void
demosaic (GeglProperties          *op,
          GeglBuffer          *src,
          const GeglRectangle *src_rect,
          GeglBuffer          *dst,
          const GeglRectangle *dst_rect)
{
  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, src_rect->width * src_rect->height * 1);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 3);

  gegl_buffer_get (src, src_rect, 1.0, babl_format ("Y float"), src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  offset=0;
  for (y=src_rect->y; y < dst_rect->height + src_rect->y; y++)
    {
      gint src_offset = (y-src_rect->y) * src_rect->width;
      for (x=src_rect->x; x < dst_rect->width + src_rect->x; x++)
        {
          gfloat red=0.0;
          gfloat green=0.0;
          gfloat blue=0.0;

          if (y<dst_rect->height+dst_rect->y &&
              x<dst_rect->width+dst_rect->x)
            {
          if ((y + op->pattern%2)%2==0)
            {
              if ((x+op->pattern/2)%2==1)
                {
                  blue=src_buf[src_offset+1];
                  green=src_buf[src_offset];
                  red=src_buf[src_offset + src_rect->width];
                }
              else
                {
                  blue=src_buf[src_offset];
                  green=src_buf[src_offset+1];
                  red=src_buf[src_offset+1+src_rect->width];
                }
            }
          else
            {
              if ((x+op->pattern/2)%2==1)
                {
                  blue=src_buf[src_offset + src_rect->width + 1];
                  green=src_buf[src_offset + 1];
                  red=src_buf[src_offset];
                }
              else
                {
                  blue=src_buf[src_offset + src_rect->width];
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

  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGB float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->right = area->bottom = 1;

  gegl_operation_set_format (operation, "output", babl_format ("RGB float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  GeglRectangle compute = gegl_operation_get_required_for_output (operation, "input", result);

  demosaic (o, input, &compute, output, result);

  return  TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
  "name"        , "gegl:demosaic-simple",
  "title"       , _("Demosaic Without Interpolation"),
  "categories"  , "blur",
  "description" ,
        _("Performs a naive grayscale2color demosaicing of an image, no interpolation."),
        NULL);
}
#endif
