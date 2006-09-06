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
#define GEGL_CHANT_DESCRIPTION     "Scales the components of the buffer to be in the 0.0-1.0 range"

#define GEGL_CHANT_SELF            "stretch-contrast.c"
#define GEGL_CHANT_CLASS_INIT /*< we need to modify the standard class init
                                  of the super class */
#define GEGL_CHANT_CATEGORIES      "color"
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
  gegl_buffer_get_fmt (buffer, buf, babl_format ("RGBA float"));
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
         const gchar   *output_prop)
{
  GeglOperationFilter *filter = GEGL_OPERATION_FILTER (operation);
  GeglRect            *result;
  GeglBuffer          *input,
                      *output;
  gdouble              min, max;

  input = filter->input;
  result = gegl_operation_get_requested_region (operation);

  
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
        GeglBuffer *in_line = g_object_new (GEGL_TYPE_BUFFER,
                                            "source", input,
                                            "x",      result->x,
                                            "y",      result->y+row,
                                            "width",  result->w,
                                            "height", chunk,
                                            NULL);
        GeglBuffer *out_line = g_object_new (GEGL_TYPE_BUFFER,
                         "source", output,
                         "x",      result->x,
                         "y",      result->y+row,
                         "width",  result->w,
                         "height", chunk,
                         NULL);
        gegl_buffer_get_fmt (in_line, buf, babl_format ("RGBA float"));
        inner_process (min, max, buf, result->w * chunk);
        gegl_buffer_set_fmt (out_line, buf, babl_format ("RGBA float"));
        g_object_unref (in_line);
        g_object_unref (out_line);
        consumed+=chunk;
      }
    g_free (buf);
  }

  filter->output = output;

  return TRUE;
}

/* computes the bounding rectangle of the raster data needed to
 * compute the scale op.
 */
static gboolean
calc_source_regions (GeglOperation *self)
{
  gegl_operation_set_source_region (self, "input",
                                    gegl_operation_get_requested_region (self));
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
