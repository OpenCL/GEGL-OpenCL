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
 * This operation is inspired from the color enhance gimp plugin.
 * It alters the chroma component to cover maximum possible range,
 * keeping hue and lightness untouched.
 *
 * Thomas Manni <thomas.manni@free.fr>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE color-enhance.c

#include "gegl-op.h"

static void
buffer_get_min_max (GeglBuffer *buffer,
                    gdouble    *min,
                    gdouble    *max)
{
  GeglBufferIterator *gi;

  gi = gegl_buffer_iterator_new (buffer, NULL, 0, babl_format ("CIE LCH(ab) float"),
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  *min = G_MAXDOUBLE;
  *max = -G_MAXDOUBLE;

  while (gegl_buffer_iterator_next (gi))
    {
      gint o;
      gfloat *buf = gi->data[0];

      for (o = 0; o < gi->length; o++)
        {
          *min = MIN (buf[1], *min);
          *max = MAX (buf[1], *max);
          buf += 3;
        }
    }
}

static void prepare (GeglOperation *operation)
{
  const Babl *format;
  const Babl *in_format = gegl_operation_get_source_format (operation, "input");

  if (in_format)
    {
       if (babl_format_has_alpha (in_format))
         format = babl_format ("CIE LCH(ab) alpha float");
       else
         format = babl_format ("CIE LCH(ab) float");
    }
  else
    {
      format = babl_format ("CIE LCH(ab) float");
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");

  /* Don't request an infinite plane */
  if (gegl_rectangle_is_infinite_plane (&result))
    return *roi;

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


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  const Babl *format = gegl_operation_get_format (operation, "output");
  gboolean has_alpha = babl_format_has_alpha (format);
  GeglBufferIterator *gi;
  gdouble  min;
  gdouble  max;
  gdouble  delta;

  buffer_get_min_max (input, &min, &max);

  gi = gegl_buffer_iterator_new (input, result, 0, format,
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (gi, output, result, 0, format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  delta = max - min;

  if (! delta)
    {
      gegl_buffer_copy (input, NULL, GEGL_ABYSS_NONE,
                        output, NULL);
      return TRUE;
    }

  if (has_alpha)
    {
      while (gegl_buffer_iterator_next (gi))
        {
          gfloat *in  = gi->data[0];
          gfloat *out = gi->data[1];

          gint i;
          for (i = 0; i < gi->length; i++)
            {
              out[0] = in[0];
              out[1] = (in[1] - min) / delta * 100.0;
              out[2] = in[2];
              out[3] = in[3];

              in  += 4;
              out += 4;
            }
        }
    }
  else
    {
       while (gegl_buffer_iterator_next (gi))
        {
          gfloat *in  = gi->data[0];
          gfloat *out = gi->data[1];

          gint i;
          for (i = 0; i < gi->length; i++)
            {
              out[0] = in[0];
              out[1] = (in[1] - min) / delta * 100.0;
              out[2] = in[2];

              in  += 3;
              out += 3;
            }
        }
    }

  return TRUE;
}

/* Pass-through when trying to perform a reduction on an infinite plane
 */
static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
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
  operation_class->process = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  operation_class->opencl_support = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:color-enhance",
    "title",       _("Color Enhance"),
    "categories" , "color:enhance",
    "description",
        _("Stretch color chroma to cover maximum possible range, "
          "keeping hue and lightness untouched."),
        NULL);
}

#endif
