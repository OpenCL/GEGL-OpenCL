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
 * Copyright 2008 Bradley Broom <bmbroom@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (pattern, _("Bayer pattern"), 0, 3, 0,
                _("Bayer pattern used, 0 seems to work for some nikon files, 2 for some Fuji files."))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "demosaic-bimedian.c"

#include "gegl-chant.h"


/* Returns the median of four floats. We define the median as the average of
 * the central two elements.
 */
static inline gfloat
m4 (gfloat a, gfloat b, gfloat c, gfloat d)
{
  gfloat t;

  /* Sort ab */
  if (a > b)
    {
      t = b;
      b = a;
      a = t;
    }
  /* Sort abc */
  if (b > c)
    {
      t = c;
      c = b;
      if (a > t)
        {
          b = a;
          a = t;
        }
      else
    b = t;
    }
  /* Return average of central two elements. */
  if (d >= c) /* Sorted order would be abcd */
    return (b + c) / 2.0;
  else if (d >= a) /* Sorted order would be either abdc or adbc */
    return (b + d) / 2.0;
  else /* Sorted order would be dabc */
    return (a + b) / 2.0;
}

/* Defines to make the row/col offsets below obvious. */
#define ROW src_extent->width
#define COL 1

/* We expect src_extent to have a one pixel border around all four sides
 * of dst_extent.
 */
static void
demosaic (GeglChantO *op,
          GeglBuffer *src,
          GeglBuffer *dst)
{
  const GeglRectangle *src_extent = gegl_buffer_get_extent (src);
  const GeglRectangle *dst_extent = gegl_buffer_get_extent (dst);
  gint x,y;
  gint offset, doffset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src));
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 3);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("Y float"), src_buf,
           GEGL_AUTO_ROWSTRIDE);

  offset = ROW + COL;
  doffset = 0;
  for (y=dst_extent->y; y<dst_extent->height + dst_extent->y; y++)
    {
      for (x=dst_extent->x; x<dst_extent->width + dst_extent->x; x++)
        {
          gfloat red=0.0;
          gfloat green=0.0;
          gfloat blue=0.0;

          if ((y + op->pattern%2)%2==0)
            {
              if ((x+op->pattern/2)%2==1)
                {
                  /* GRG
                   * BGB
                   * GRG
                   */
                  blue =(src_buf[offset-COL]+src_buf[offset+COL])/2.0;
                  green=src_buf[offset];
                  red  =(src_buf[offset-ROW]+src_buf[offset+ROW])/2.0;
                }
              else
                {
                  /* RGR
                   * GBG
                   * RGR
                   */
                  blue =src_buf[offset];
                  green=m4(src_buf[offset-ROW], src_buf[offset-COL],
                           src_buf[offset+COL], src_buf[offset+ROW]);
                  red  =m4(src_buf[offset-ROW-COL], src_buf[offset-ROW+COL],
                           src_buf[offset+ROW-COL], src_buf[offset+ROW+COL]);
                }
            }
          else
            {
              if ((x+op->pattern/2)%2==1)
                {
                  /* BGB
                   * GRG
                   * BGB
                   */
                  blue =m4(src_buf[offset-ROW-COL], src_buf[offset-ROW+COL],
                       src_buf[offset+ROW-COL], src_buf[offset+ROW+COL]);
                  green=m4(src_buf[offset-ROW], src_buf[offset-COL],
                       src_buf[offset+COL], src_buf[offset+ROW]);
                  red  =src_buf[offset];
                }
              else
                {
                  /* GBG
                   * RGR
                   * GBG
                   */
                  blue =(src_buf[offset-ROW]+src_buf[offset+ROW])/2.0;
                  green=src_buf[offset];
                  red  =(src_buf[offset-COL]+src_buf[offset+COL])/2.0;
                }
            }

          dst_buf [doffset*3+0] = red;
          dst_buf [doffset*3+1] = green;
          dst_buf [doffset*3+2] = blue;

          offset++;
          doffset++;
        }
      offset+=2;
    }

  gegl_buffer_set (dst, NULL, babl_format ("RGB float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

/* Specify required extra pixels around dst_extent: one pixel on every side.
 */
static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->right = area->bottom = 1;
  area->left = area->top = 1;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  demosaic (o, input, output);

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

  operation_class->name        = "gegl:demosaic-bimedian";
  operation_class->categories  = "blur";
  operation_class->description =
        _("Performs a grayscale2color demosaicing of an image, using bimedian interpolation.");
}

#endif
