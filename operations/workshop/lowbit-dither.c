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
 * Copyright 2016 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE lowbit-dither.c
#define GEGL_OP_NAME     lowbit_dither

#include "gegl-op.h"

#include <math.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  //GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat     *in_pixel =  in_buf;
  gfloat     *out_pixel = out_buf;
  gint        x, y;

  x = roi->x;
  y = roi->y;

  while (n_pixels--)
    {
      int ch;
      for (ch = 0; ch < 4; ch++)
      {
       if (in_pixel [ch] > 0.0000001f && in_pixel [ch] < 1.0000000f)
         out_pixel [ch] = in_pixel [ch] + ((((((x+ch * 67) + y * 236) * 119) & 255)/255.0f)-0.5f )/255.0f;
         /* this computes an 8bit http://pippin.gimp.org/a_dither/ mask */
       else
         out_pixel [ch] = in_pixel [ch];
      }
      out_pixel += 4;
      in_pixel  += 4;

      /* update x and y coordinates */
      if (++x>=roi->x + roi->width)
        {
          x=roi->x;
          y++;
        }
    }

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  operation_class->no_cache       = TRUE;
  operation_class->opencl_support = TRUE;

  point_filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:lowbit-dither",
    "title",              _("Lowbit dither"),
    "position-dependent", "true",
    "description", _("dithers adequately to mask out 8bit sRGB TRC quantization for high bitdepth formats"),
    NULL);
}

#endif
