/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
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

  gfloat *buf = g_malloc0 (sizeof (gfloat) * 4 * buffer->width * buffer->height);
  gint i;
  gegl_buffer_get (buffer, NULL, buf, babl_format ("RGBA float"), 1.0);
  for (i=0;i<gegl_buffer_pixels (buffer);i++)
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
  GeglRectangle *result;
  GeglBuffer    *input,
                *output;
  gdouble        min, max;

  input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));

  result = gegl_operation_get_requested_region (operation, context_id);
  
  if (result->w==0 ||
      result->h==0)
    {
      output = g_object_ref (input);
      return TRUE;
    }

  buffer_get_min_max (input, &min, &max);

  output = g_object_new (GEGL_TYPE_BUFFER,
                         "format", babl_format ("RGBA float"),
                         "x",      result->x,
                         "y",      result->y,
                         "width",  result->w,
                         "height", result->h,
                         NULL);

  {
    gint row;
    guchar *buf;
    gint chunk_size=128;
    gint consumed=0;

    buf = g_malloc0 (sizeof (gfloat) * 4 * result->w * chunk_size);

    for (row=0;row<result->h;row=consumed)
      {
        gint chunk = consumed+chunk_size<result->h?chunk_size:result->h-consumed;
        GeglRectangle line = {result->x,
                         result->y+row,
                         result->w,
                         chunk};

        gegl_buffer_get (input, &line, buf, babl_format ("RGBA float"), 1.0);
        inner_process (min, max, buf, result->w * chunk);
        gegl_buffer_set (output, &line, buf, babl_format ("RGBA float"));
        consumed+=chunk;
      }
    g_free (buf);
  }

  gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));

  return TRUE;
}

static gboolean
calc_source_regions (GeglOperation *self,
                     gpointer       context_id)
{
  /*gegl_operation_set_source_region (self, context_id, "input",
                                    gegl_operation_get_requested_region (self, context_id));*/
  gegl_operation_set_source_region (self, context_id, "input",
                                    gegl_operation_source_get_defined_region (self, "input"));
  return TRUE;
}

/* This is called at the end of the gobject class_init function, the
 * GEGL_CHANT_CLASS_INIT was needed to make this happen
 *
 * Here we override the standard passthrough options for the rect
 * computations.
 */
static void class_init (GeglOperationClass *operation_class)
{
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
