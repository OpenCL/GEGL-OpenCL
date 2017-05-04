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
 * Copyright (C) 1997 Andrew Kieschnick (andrewk@mail.utexas.edu)
 *
 * Original deinterlace for GIMP 0.54 API by Federico Mena Quintero
 *
 * Copyright (C) 1996 Federico Mena Quintero
 *
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_deinterlace_keep)
  enum_value (GEGL_DEINTERLACE_KEEP_EVEN, "even", N_("Keep even fields"))
  enum_value (GEGL_DEINTERLACE_KEEP_ODD,  "odd",  N_("Keep odd fields"))
enum_end (GeglDeinterlaceKeep)

property_enum (keep, _("Keep"),
  GeglDeinterlaceKeep, gegl_deinterlace_keep,
  GEGL_DEINTERLACE_KEEP_EVEN)
  description(_("Keep even or odd fields"))

property_enum (orientation, _("Orientation"),
  GeglOrientation, gegl_orientation,
  GEGL_ORIENTATION_HORIZONTAL)
  description(_("Deinterlace horizontally or vertically"))

property_int  (size, _("Block size"), 1)
  value_range (0, 100)
  description (_("Block size of deinterlacing rows/columns"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     deinterlace
#define GEGL_OP_C_SOURCE deinterlace.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties              *o       = GEGL_PROPERTIES (operation);

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      op_area->left = op_area->right = 0;
      op_area->top = op_area->bottom = o->size + 1;
    }
  else
    {
      op_area->left = op_area->right = o->size + 1;
      op_area->top = op_area->bottom = 0;
    }

  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = { 0, 0, 0, 0 };
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (!in_rect)
    return result;

  return *in_rect;
}

static void
deinterlace_horizontal (gfloat              *src_buf,
                        gfloat              *dest,
                        const GeglRectangle *result,
                        const GeglRectangle *extended,
                        const GeglRectangle *boundary,
                        gint                 inter,
                        gint                 y,
                        gint                 size)
{
  gfloat upper[4];
  gfloat lower[4];
  gfloat temp_buf[4];
  gint   x;
  gint   up_offset;
  gint   low_offset;
  gint   offset = 0;
  gint   i;

  for (x=0; x < result->width; x++)
    {
      gfloat ualpha, lalpha, temp;
      gfloat alpha = 0;

      temp_buf[0] = temp_buf[1] = temp_buf[2] = temp_buf[3] = 0;

      for (i = 0; i < size; i++)
        {
          gint b;

          if (y  - i> 0)
            up_offset = (y - i - extended->y) * extended->width * 4;
          else
            up_offset = inter * extended->width * 4;

          if (y + i + 1 < boundary->height)
            low_offset = (y + i + 1 - extended->y) * extended->width * 4;
          else
            low_offset = (y - 1 + inter - extended->y) * extended->width * 4;

          offset = (y - result->y) * extended->width * 4;

          for (b=0; b<4; b++)
            {
              upper[b] = src_buf[up_offset + x * 4 + b];
              lower[b] = src_buf[low_offset + x * 4 + b];
            }

          ualpha = upper[3];
          lalpha = lower[3];
          temp   = ualpha + lalpha;
          alpha += temp;

          for (b=0; b < 3; b++)
            temp_buf[b] += (upper[b] * ualpha +
                            lower[b] * lalpha);
        }

      if ((dest[offset + x * 4 + 3] = alpha / (2 * size)))
        {
          gint b;
          for (b=0; b < 3; b++)
            dest[offset + x * 4 + b] = temp_buf[b] / alpha;

        }
    }
}

static void
deinterlace_vertical (gfloat              *src_buf,
                      gfloat              *dest,
                      const GeglRectangle *result,
                      const GeglRectangle *extended,
                      const GeglRectangle *boundary,
                      gint                 inter,
                      gint                 x,
                      gint                 size)
{
  gfloat upper[4];
  gfloat lower[4];
  gfloat temp_buf[4];
  gint   y;
  gint   up_offset;
  gint   low_offset;
  gint   offset = 0;
  gint   i;

  for (y=result->y; y < result->y + result->height; y++)
    {
      gfloat ualpha, lalpha, temp;
      gfloat alpha = 0;

      temp_buf[0] = temp_buf[1] = temp_buf[2] = temp_buf[3] = 0;

      for (i = 0; i < size; i++)
        {
          gint b;

          if (x  - i > 0)
            up_offset = (y - extended->y) * extended->width * 4
              + (x - i - extended->x) * 4;
          else
            up_offset = (y - extended->y) * extended->width * 4 + inter * 4;

          if (x + i + 1 < boundary->width)
            low_offset = (y - extended->y) * extended->width * 4 +
              (x + i + 1 - extended->x) * 4;
          else
            low_offset = (y - extended->y) * extended->width * 4 +
              (x + i - 1 + inter - extended->x) * 4;

          offset = (y - result->y) * result->width * 4 + (x - result->x) * 4;

          for (b=0; b<4; b++)
            {
              upper[b] = src_buf[up_offset + b];
              lower[b] = src_buf[low_offset + b];
            }

          ualpha = upper[3];
          lalpha = lower[3];
          temp   = ualpha + lalpha;
          alpha += temp;

          for (b=0; b < 3; b++)
            temp_buf[b] += (upper[b] * ualpha +
                            lower[b] * lalpha);
        }

      if ((dest[offset + 3] = alpha / (2 * size)))
        {
          gint b;
          for (b=0; b < 3; b++)
            dest[offset + b] = temp_buf[b] / alpha;
        }
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties              *o        = GEGL_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area  = GEGL_OPERATION_AREA_FILTER (operation);
  const Babl              *format   = babl_format ("RGBA float");
  GeglRectangle            rect;
  GeglRectangle            boundary = get_bounding_box (operation);
  gint                     x, y;
  gfloat                  *dst_buf, *src_buf;

  rect.x      = CLAMP (result->x - op_area->left, boundary.x, boundary.x +
                       boundary.width);
  rect.width  = CLAMP (result->width + op_area->left + op_area->right, 0,
                       boundary.width);
  rect.y      = CLAMP (result->y - op_area->top, boundary.y, boundary.y +
                       boundary.width);
  rect.height = CLAMP (result->height + op_area->top + op_area->bottom, 0,
                       boundary.height);

  dst_buf = g_new0 (gfloat, result->height * result->width * 4);
  src_buf = g_new0 (gfloat, rect.height * rect.width * 4);

  gegl_buffer_get (input, result, 1.0, format, dst_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_get (input, &rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (o->orientation == GEGL_ORIENTATION_HORIZONTAL)
    {
      for (y = result->y; y < result->y + result->height; y++)
        if ((o->keep == GEGL_DEINTERLACE_KEEP_EVEN && (y % 2 == 0)) ||
            (o->keep == GEGL_DEINTERLACE_KEEP_ODD  && (y % 2 != 0)))
          {
            deinterlace_horizontal (src_buf, dst_buf, result, &rect, &boundary,
                                    o->keep,
                                    y, o->size);
          }
    }
  else
    {
      for (x = result->x; x < result->x + result->width; x++)
        if ((o->keep == GEGL_DEINTERLACE_KEEP_EVEN && (x % 2 == 0)) ||
            (o->keep == GEGL_DEINTERLACE_KEEP_ODD  && (x % 2 != 0)))
          {
            deinterlace_vertical (src_buf, dst_buf, result, &rect, &boundary,
                                  o->keep,
                                  x, o->size);
          }
    }

  gegl_buffer_set (output, result, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare           = prepare;
  operation_class->get_bounding_box  = get_bounding_box;
  filter_class->process              = process;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:deinterlace",
    "title",              _("Deinterlace"),
    "categories",         "enhance",
    "license",            "GPL3+",
    "position-dependent", "true",
    "reference-hash",     "663e775f191a41aaf1039b883615d786",
    "description", _("Fix images where every other row or column is missing"),
    NULL);
}

#endif
