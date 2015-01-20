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
 * Copyright 1996 Federico Mena Quintero
 * Copyright 1997 Scott Goehring
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2013 Carlos Zubieta <czubieta.dev@gmail.com>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE stretch-contrast-hsv.c

#include "gegl-op.h"

typedef struct {
  gfloat slo;
  gfloat sdiff;
  gfloat vlo;
  gfloat vdiff;
} AutostretchData;

static void
buffer_get_auto_strech_data (GeglBuffer      *buffer,
                             AutostretchData *data)
{
  gfloat smin =  G_MAXFLOAT;
  gfloat smax = -G_MAXFLOAT;
  gfloat vmin =  G_MAXFLOAT;
  gfloat vmax = -G_MAXFLOAT;

  GeglBufferIterator *gi;

  gi = gegl_buffer_iterator_new (buffer, NULL, 0, babl_format ("HSVA float"),
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *buf = gi->data[0];
      gint    i;

      for (i = 0; i < gi->length; i++)
        {
          gfloat sval = buf[1];
          gfloat vval = buf[2];

          smin = MIN (sval, smin);
          smax = MAX (sval, smax);
          vmin = MIN (vval, vmin);
          vmax = MAX (vval, vmax);

          buf += 4;
        }
    }

  if (data)
    {
      data->slo   = smin;
      data->sdiff = smax - smin;
      data->vlo   = vmin;
      data->vdiff = vmax - vmin;
    }
}

static void
clean_autostretch_data (AutostretchData *data)
{
  if (data->sdiff < GEGL_FLOAT_EPSILON)
    {
      data->sdiff = 1.0;
      data->slo   = 0.0;
    }

  if (data->vdiff < GEGL_FLOAT_EPSILON)
    {
      data->vdiff = 1.0;
      data->vlo   = 0.0;
    }
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format ("HSVA float"));
  gegl_operation_set_format (operation, "output", babl_format ("HSVA float"));
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
  AutostretchData     data;
  GeglBufferIterator *gi;

  buffer_get_auto_strech_data (input, &data);
  clean_autostretch_data (&data);

  gi = gegl_buffer_iterator_new (input, result, 0, babl_format ("HSVA float"),
                                 GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (gi, output, result, 0, babl_format ("HSVA float"),
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *in  = gi->data[0];
      gfloat *out = gi->data[1];
      gint    i;

      for (i = 0; i < gi->length; i++)
        {
          out[0] = in[0]; /* Keep hue */
          out[1] = (in[1] - data.slo) / data.sdiff;
          out[2] = (in[2] - data.vlo) / data.vdiff;
          out[3] = in[3]; /* Keep alpha */

          in  += 4;
          out += 4;
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

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:stretch-contrast-hsv",
    "title",       _("Stretch Contrast HSV"),
    "categories" , "color:enhance",
    "description",
        _("Scales the components of the buffer to be in the 0.0-1.0 range. "
          "This improves images that make poor use of the available contrast "
          "(little contrast, very dark, or very bright images). "
          "This version differs from Contrast Autostretch in that it works "
          "in HSV scpace, and preserves hue."),
        NULL);
}

#endif
