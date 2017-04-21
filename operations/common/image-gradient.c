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
 * Copyright 2016 Thomas Manni <thomas.manni@free.fr>
 */

 /* compute gradient magnitude and/or direction by central differencies */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_imagegradient_output)
   enum_value (GEGL_IMAGEGRADIENT_MAGNITUDE, "magnitude", N_("Magnitude"))
   enum_value (GEGL_IMAGEGRADIENT_DIRECTION, "direction", N_("Direction"))
   enum_value (GEGL_IMAGEGRADIENT_BOTH,      "both",      N_("Both"))
enum_end (GeglImageGradientOutput)

property_enum (output_mode, _("Output mode"),
               GeglImageGradientOutput, gegl_imagegradient_output,
               GEGL_IMAGEGRADIENT_MAGNITUDE)
  description (_("Output Mode"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME         image_gradient
#define GEGL_OP_C_SOURCE     image-gradient.c

#define POW2(x) ((x)*(x))

#include "gegl-op.h"
#include <math.h>

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area       = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o          = GEGL_PROPERTIES (operation);
  const Babl              *rgb_format = babl_format ("R'G'B' float");
  const Babl              *out_format = babl_format_n (babl_type ("float"), 2);

  area->left   =
  area->top    =
  area->right  =
  area->bottom = 1;

  if (o->output_mode == GEGL_IMAGEGRADIENT_MAGNITUDE ||
      o->output_mode == GEGL_IMAGEGRADIENT_DIRECTION)
    {
      out_format = babl_format_n (babl_type ("float"), 1);
    }

  gegl_operation_set_format (operation, "input",  rgb_format);
  gegl_operation_set_format (operation, "output", out_format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (in_rect)
    {
      result = *in_rect;
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties  *o          = GEGL_PROPERTIES (operation);
  const Babl      *in_format  = babl_format ("R'G'B' float");
  const Babl      *out_format = gegl_operation_get_format (operation, "output");
  gfloat *row1;
  gfloat *row2;
  gfloat *row3;
  gfloat *row4;
  gfloat *top_ptr;
  gfloat *mid_ptr;
  gfloat *down_ptr;
  gfloat *tmp_ptr;
  gint    x, y;
  gint    n_components;

  GeglRectangle row_rect;
  GeglRectangle out_rect;

  n_components = babl_format_get_n_components (out_format);
  row1 = g_new (gfloat, (roi->width + 2) * 3);
  row2 = g_new (gfloat, (roi->width + 2) * 3);
  row3 = g_new (gfloat, (roi->width + 2) * 3);
  row4 = g_new0 (gfloat, roi->width * n_components);

  top_ptr  = row1;
  mid_ptr  = row2;
  down_ptr = row3;

  row_rect.width = roi->width + 2;
  row_rect.height = 1;
  row_rect.x = roi->x - 1;
  row_rect.y = roi->y - 1;

  out_rect.x      = roi->x;
  out_rect.width  = roi->width;
  out_rect.height = 1;

  gegl_buffer_get (input, &row_rect, 1.0, in_format, top_ptr,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  row_rect.y++;
  gegl_buffer_get (input, &row_rect, 1.0, in_format, mid_ptr,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP); 

  for (y = roi->y; y < roi->y + roi->height; y++)
    {
      row_rect.y = y + 1;
      out_rect.y = y;

      gegl_buffer_get (input, &row_rect, 1.0, in_format, down_ptr,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

      for (x = 1; x < row_rect.width - 1; x++)
        {
          gfloat dx[3];
          gfloat dy[3];
          gfloat magnitude[3];
          gint   max_index;

          dx[0] = (mid_ptr[(x-1) * 3] - mid_ptr[(x+1) * 3]);
          dy[0] = (top_ptr[x*3] - down_ptr[x*3]);
          magnitude[0] = sqrtf(POW2(dx[0]) + POW2(dy[0]));

          dx[1] = (mid_ptr[(x-1) * 3 + 1] - mid_ptr[(x+1) * 3 + 1]);
          dy[1] = (top_ptr[x*3 + 1] - down_ptr[x*3 + 1]);
          magnitude[1] = sqrtf(POW2(dx[1]) + POW2(dy[1]));

          dx[2] = (mid_ptr[(x-1) * 3 + 2] - mid_ptr[(x+1) * 3 + 2]);
          dy[2] = (top_ptr[x*3 + 2] - down_ptr[x*3 + 2]);
          magnitude[2] = sqrtf(POW2(dx[2]) + POW2(dy[2]));

          if (magnitude[0] > magnitude[1])
            max_index = 0;
          else
            max_index = 1;

          if (magnitude[2] > magnitude[max_index])
            max_index = 2;

          if (o->output_mode == GEGL_IMAGEGRADIENT_MAGNITUDE)
            {
              row4[(x-1) * n_components] = magnitude[max_index];
            }
          else
           {
              gfloat direction = atan2 (dy[max_index], dx[max_index]);

              if (o->output_mode == GEGL_IMAGEGRADIENT_DIRECTION)
                {
                  row4[(x-1) * n_components] = direction;
                }
              else
                {
                  row4[(x-1) * n_components] = magnitude[max_index];
                  row4[(x-1) * n_components + 1] = direction;
                }
           }
        }

      gegl_buffer_set (output, &out_rect, level, out_format, row4,
                       GEGL_AUTO_ROWSTRIDE);

      tmp_ptr = top_ptr;
      top_ptr = mid_ptr;
      mid_ptr = down_ptr;
      down_ptr = tmp_ptr;
    }

  g_free (row1);
  g_free (row2);
  g_free (row3);
  g_free (row4);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process             = process;
  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->opencl_support   = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:image-gradient",
    "title",       _("Image Gradient"),
    "categories",  "edge-detect",
    "reference-hash", "3bc1f4413a06969bf86606d621969651",
    "description", _("Compute gradient magnitude and/or direction by "
                     "central differencies"),
    NULL);
}

#endif
