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
 * Copyright 2017 Thomas Manni <thomas.manni@free.fr>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_boolean (squared, _("squared integral"), FALSE)
  description (_("Add squared values sum to the output"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME         integral_image
#define GEGL_OP_C_SOURCE     integral-image.c

#include "gegl-op.h"

#define SQR(x) ((x) * (x))

static inline void
compute_row_integral (gdouble  *src_row,
                      gdouble  *top_row,
                      gdouble  *dst_row,
                      gint      width,
                      gint      n_components,
                      gboolean  squared)
{
  gint b;

  if (squared)
    {
      while (width--)
        {
          for (b = 0; b < n_components; b++)
            {
              dst_row[b] = src_row[b] +
                           dst_row[b - n_components] +
                           top_row[b] -
                           top_row[b - n_components];

              dst_row[b + n_components] =   SQR(src_row[b])
                                          + dst_row[b - n_components * 2]
                                          + top_row[b + n_components]
                                          - top_row[b - n_components * 2];
            }

          src_row += n_components;
          top_row += n_components * 2;
          dst_row += n_components * 2;
        }
    }
  else
    {
      while (width--)
        {
          for (b = 0; b < n_components; b++)
            dst_row[b] =   src_row[b]
                         + dst_row[b - n_components]
                         + top_row[b]
                         - top_row[b - n_components];

          src_row += n_components;
          top_row += n_components;
          dst_row += n_components;
        }
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *src_format   = gegl_operation_get_source_format (operation, "input");
  const Babl *input_format = babl_format ("RGB double");
  gint        n_components = 3;
  const Babl *output_format;

  if (src_format)
    {
      const Babl *model = babl_format_get_model (src_format);

      if (model == babl_model ("RGB") || model == babl_model ("R'G'B'") ||
          model == babl_model ("RGBA") || model == babl_model ("R'G'B'A"))
        {
          input_format = babl_format ("RGB double");
          n_components = 3;
        }
      else if (model == babl_model ("Y") || model == babl_model ("Y'") ||
               model == babl_model ("YA") || model == babl_model ("Y'A"))
        {
          input_format = babl_format ("Y double");
          n_components = 1;
        }
    }

  if (o->squared)
    n_components *= 2;

  output_format = babl_format_n (babl_type ("double"), n_components);

  gegl_operation_set_format (operation, "input", input_format);
  gegl_operation_set_format (operation, "output", output_format);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o        = GEGL_PROPERTIES (operation);
  const Babl *src_format   = gegl_operation_get_source_format (operation, "input");
  const Babl *dst_format   = gegl_operation_get_format (operation, "output");
  gint        src_components = babl_format_get_n_components (src_format);
  gint        dst_components = babl_format_get_n_components (dst_format);

  gint     width, height;
  gint     row_size;
  gint     y;
  gdouble *top_row  = NULL;
  gdouble *dst_row  = NULL;
  gdouble *src_row  = NULL;
  gdouble *p_top    = NULL;
  gdouble *p_dst    = NULL;

  width = gegl_buffer_get_width (input);
  height = gegl_buffer_get_height (input);
  row_size = width + 1;

  top_row = g_new0 (gdouble, row_size * dst_components);
  dst_row = g_new0 (gdouble, row_size * dst_components);
  src_row = g_new  (gdouble, row_size * src_components);

  p_top = top_row;
  p_dst = dst_row;

  for (y = 0; y < height; y++)
    {
      GeglRectangle row_rect = {-1, y, width + 1, 1};

      gegl_buffer_get (input, &row_rect, 1.0, src_format, src_row,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      compute_row_integral (src_row + src_components,
                            p_top + dst_components,
                            p_dst + dst_components,
                            width,
                            src_components,
                            o->squared);

      gegl_buffer_set (output, &row_rect, 0, dst_format, p_dst,
                       GEGL_AUTO_ROWSTRIDE);

      p_top = p_dst;

      if (p_dst == top_row)
        p_dst = dst_row;
      else
        p_dst = top_row;
    }

  g_free (top_row);
  g_free (dst_row);
  g_free (src_row);

  return TRUE;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle result;
  result.x = 0;
  result.y = 0;
  result.width = region->x + region->width;
  result.height = region->y + region->height;

  return result;
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
  GeglRectangle  result;
  GeglRectangle *src = gegl_operation_source_get_bounding_box (operation, "input");

  if (!src)
    return *input_region;

  result.x = input_region->x;
  result.y = input_region->y;
  result.width  = src->width - input_region->x;
  result.height = src->height - input_region->y;

  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

  return result;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                      = process;
  operation_class->get_cached_region         = get_cached_region;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->get_required_for_output   = get_required_for_output;
  operation_class->prepare                   = prepare;
  operation_class->threaded                  = FALSE;
  operation_class->opencl_support            = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:integral-image",
    "title",       _("Integral Image"),
    "categories",  "hidden",
    "description", _("Compute integral and squared integral image"),
    NULL);
}

#endif
