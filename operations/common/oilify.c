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
 * Copyright 1995 Spencer Kimball and Peter Mattis
 * Copyright 1996 Torsten Martinsen
 * Copyright 2007 Daniel Richard G.
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (mask_radius, _("Mask Radius"), 1.0, 25.0, 4.0,
                   _("Radius of circle around pixel"))

gegl_chant_double (exponent, _("Exponent"), 1.0, 20.0, 8.0,
                   _("Exponent"))

gegl_chant_enum (sampler_type, _("Sampler"), GeglSamplerType, gegl_sampler_type,
                 GEGL_SAMPLER_CUBIC, _("Sampler used internally"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "oilify.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdlib.h>

#define NUM_INTENSITIES       256

static void oilify_pixel (gint x,
			  gint y,
			  gdouble radius,
			  gdouble exponent,
			  GeglSampler *sampler,
			  gfloat *dst_pixel)
{
  gint hist[4][NUM_INTENSITIES];
  gfloat temp_pixel[4];
  gint ceil_radius = ceil(radius);
  gdouble radius_sq = radius*radius;
  gint i, j, b;
  gint hist_max[4];
  gint intensity;
  gfloat sum;
  gfloat div;
  
  for (b = 0; b < 4; b++)
    {
      for (i = 0; i < NUM_INTENSITIES; i++)
	hist[b][i] = 0;
    }
  for (i = -ceil_radius; i < ceil_radius; i++)
    {
      for (j = -ceil_radius; j < ceil_radius; j++)
	{
	  if (i*i + j*j < radius_sq) {
	    gegl_sampler_get (sampler,
			      x+i,
			      y+j,
			      NULL,
			      temp_pixel);
	    for (b = 0; b < 4; b++) 
	      {
		intensity = temp_pixel[b] * NUM_INTENSITIES;
		hist[b][intensity]++;
	      }
	  }
	}
    }
  
  for (b = 0; b < 4; b++) 
    {
      hist_max[b] = 1;
      for (i = 0; i < NUM_INTENSITIES; i++) {
	hist_max[b] = MAX (hist_max[b], hist[b][i]);
      }
  }
  
  for (b = 0; b < 4; b++) 
    {
      sum = 0.0;
      div = 0.0;
      for (i = 0; i < NUM_INTENSITIES; i++)
	{
	  gfloat ratio = (gfloat) hist[b][i] / (gfloat) hist_max[b];
	  gfloat weight = pow (ratio, exponent);
	  sum += weight * (gfloat) i/(gfloat) NUM_INTENSITIES;
	  div += weight;
	}
      dst_pixel[b] = sum / div;
    }
}

static void prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   = 
  op_area->right  = 
  op_area->top    = 
  op_area->bottom = o->mask_radius;

  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);

  gint x = result->x; /* initial x                   */
  gint y = result->y; /*           and y coordinates */
  
  gfloat *dst_buf = g_slice_alloc (result->width * result->height * 4 * sizeof(gfloat));

  gfloat *out_pixel = dst_buf;

  GeglSampler *sampler = gegl_buffer_sampler_new (input,
                                                  babl_format ("RGBA float"),
                                                  o->sampler_type);

  gint n_pixels = result->width * result->height;

  while (n_pixels--)
    {
      
      oilify_pixel(x, y, o->mask_radius, o->exponent, sampler, out_pixel);

      out_pixel += 4;

      /* update x and y coordinates */
      x++;
      if (x>=result->x + result->width)
        {
          x=result->x;
          y++;
        }
    }

  gegl_buffer_set (output, result, 0, babl_format ("RGBA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (result->width * result->height * 4 * sizeof(gfloat), dst_buf);

  g_object_unref (sampler);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "categories" , "artistic",
    "name"       , "gegl:oilify",
    "description", _("Emulate an oil painting"),
    NULL);
}

#endif
