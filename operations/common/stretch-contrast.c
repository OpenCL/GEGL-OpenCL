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

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "stretch-contrast.c"

#include "gegl-chant.h"

static void
buffer_get_min_max (GeglBuffer *buffer,
                    gdouble    *min,
                    gdouble    *max)
{
  gfloat tmin =  G_MAXFLOAT;
  gfloat tmax = -G_MAXFLOAT;

  GeglBufferIterator *gi;
  gi = gegl_buffer_iterator_new (buffer, NULL, 0, babl_format ("RGB float"),
                                 GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *buf = gi->data[0];

      gint i;
      for (i = 0; i < gi->length * 3; i++)
        {
          gfloat val = buf [i];

          if (val < tmin)
            tmin = val;
          if (val > tmax)
            tmax = val;
        }
    }

  if (min)
    *min = tmin;
  if (max)
    *max = tmax;
}

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  return result;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  gdouble  min, max, diff;
  GeglBufferIterator *gi;

  buffer_get_min_max (input, &min, &max);
  diff = max - min;

  /* Avoid a divide by zero error if the image is a solid color */
  if (diff == 0.0){
    gegl_buffer_copy (input, NULL, output, NULL);
    return TRUE;
  }

  gi = gegl_buffer_iterator_new (input, result, 0, babl_format ("RGBA float"),
                                 GEGL_BUFFER_READ, GEGL_ABYSS_NONE);

  gegl_buffer_iterator_add (gi, output, result, 0, babl_format ("RGBA float"),
                            GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      gfloat *in  = gi->data[0];
      gfloat *out = gi->data[1];

      gint o;
      for (o = 0; o < gi->length; o++)
        {
          out[0] = (in[0] - min) / diff;
          out[1] = (in[1] - min) / diff;
          out[2] = (in[2] - min) / diff;
          out[3] = in[3];

          in  += 4;
          out += 4;
        }
    }

  return TRUE;
}

/* This is called at the end of the gobject class_init function.
 *
 * Here we override the standard passthrough options for the rect
 * computations.
 */
static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:stretch-contrast",
    "categories" , "color:enhance",
    "description",
        _("Scales the components of the buffer to be in the 0.0-1.0 range. "
          "This improves images that make poor use of the available contrast "
          "(little contrast, very dark, or very bright images)."),
        NULL);
}

#endif
