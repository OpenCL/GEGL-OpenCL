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
#if GEGL_CHANT_PROPERTIES
 
#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            stretch_contrast
#define GEGL_CHANT_DESCRIPTION     "Scales the components of the buffer to be in the 0.0-1.0 range. This improves images that makes poor use of the available contrast (little contrast, very dark, or very bright images)."

#define GEGL_CHANT_SELF            "stretch-contrast.c"
#define GEGL_CHANT_CLASS_INIT /*< we need to modify the standard class init
                                  of the super class */
#define GEGL_CHANT_CATEGORIES      "color:enhance"
#define GEGL_CHANT_PREPARE
#include "gegl-chant.h"

static gboolean
inner_process (gdouble        min,
                gdouble        max,
                guchar        *buf,
                gint           n_pixels)
{
  gint o;
  gfloat *p = (gfloat*) (buf);

  for (o=0; o<n_pixels; o++)
    {
      gint i;
      for (i=0;i<3;i++)
        p[i] = (p[i] - min) / (max-min);
      p+=4;
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

  gfloat *buf = g_malloc0 (sizeof (gfloat) * 4 * gegl_buffer_get_pixel_count (buffer));
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

static gboolean
process (GeglOperation *operation,
         gpointer       context_id)
{
  const GeglRectangle *result;
  GeglBuffer          *input;
  GeglBuffer          *output;
  gdouble              min, max;

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));

  result = gegl_operation_get_requested_region (operation, context_id);

  buffer_get_min_max (input, &min, &max);

  output = gegl_operation_get_target (operation, context_id, "output");
  {
    gint row;
    guchar *buf;
    gint chunk_size=128;
    gint consumed=0;

    buf = g_malloc0 (sizeof (gfloat) * 4 * result->width  * chunk_size);

    for (row=0;row<result->height;row=consumed)
      {
        gint chunk = consumed+chunk_size<result->height?chunk_size:result->height-consumed;
        GeglRectangle line = { result->x,
                               result->y + row,
                               result->width,
                               chunk};

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

static GeglRectangle
compute_input_request (GeglOperation       *operation,
                       const gchar         *input_pad,
                       const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_defined_region (operation, "input");
  return result;
}

static void prepare (GeglOperation *operation,
                     gpointer       context_id)
{
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

/* This is called at the end of the gobject class_init function, the
 * GEGL_CHANT_CLASS_INIT was needed to make this happen
 *
 * Here we override the standard passthrough options for the rect
 * computations.
 */
static void class_init (GeglOperationClass *operation_class)
{
  operation_class->compute_input_request = compute_input_request;
}

#endif
