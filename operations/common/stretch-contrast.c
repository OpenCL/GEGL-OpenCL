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

static gboolean
inner_process (gdouble  min,
               gdouble  max,
               gfloat  *buf,
               gint     n_pixels)
{
  gint o;

  for (o=0; o<n_pixels; o++)
    {
      buf[0] = (buf[0] - min) / (max-min);
      buf[1] = (buf[1] - min) / (max-min);
      buf[2] = (buf[2] - min) / (max-min);
      /* FIXME: really stretch the alpha channel?? */
      buf[3] = (buf[3] - min) / (max-min);

      buf += 4;
    }
  return TRUE;
}

static void
buffer_get_min_max (GeglBuffer *buffer,
                    gdouble    *min,
                    gdouble    *max)
{
  gfloat tmin = 9000000.0;
  gfloat tmax =-9000000.0;

  gfloat *buf = g_new0 (gfloat, 4 * gegl_buffer_get_pixel_count (buffer));
  gint i;
  gegl_buffer_get (buffer, 1.0, NULL, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
  for (i=0;i< gegl_buffer_get_pixel_count (buffer);i++)
    {
      gint component;
      for (component=0; component<3; component++)
        {
          gfloat val = buf[i*4+component];

          if (val<tmin)
            tmin=val;
          if (val>tmax)
            tmax=val;
        }
    }
  g_free (buf);
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
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  gdouble  min, max;

  buffer_get_min_max (input, &min, &max);
  {
    gint row;
    gfloat *buf;
    gint chunk_size=128;
    gint consumed=0;

    buf = g_new0 (gfloat, 4 * result->width  * chunk_size);

    for (row = 0; row < result->height; row = consumed)
      {
        gint chunk = consumed+chunk_size<result->height?chunk_size:result->height-consumed;
        GeglRectangle line;

        line.x = result->x;
        line.y = result->y + row;
        line.width = result->width;
        line.height = chunk;

        gegl_buffer_get (input, 1.0, &line, babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
        inner_process (min, max, buf, result->width  * chunk);
        gegl_buffer_set (output, &line, babl_format ("RGBA float"), buf,
                         GEGL_AUTO_ROWSTRIDE);
        consumed+=chunk;
      }
    g_free (buf);
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

  operation_class->name        = "gegl:stretch-contrast";
  operation_class->categories  = "color:enhance";
  operation_class->description =
        _("Scales the components of the buffer to be in the 0.0-1.0 range. "
          "This improves images that make poor use of the available contrast "
          "(little contrast, very dark, or very bright images).");
}

#endif
