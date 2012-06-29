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
 * Copyright 1997 Spencer Kimball
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (mask_radius, _("Mask Radius"), 0.0, 50.0, 10.0,
                   _("Mask Radius"))

gegl_chant_double (sharpness, _("Sharpness"), 0.0, 1.0, 0.5,
                   _("Sharpness"))

gegl_chant_double (black, _("Percent Black"), 0.0, 1.0, 0.2,
                   _("Percent Black"))

gegl_chant_double (white, _("Percent White"), 0.0, 1.0, 0.2,
                   _("Percent White"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "photocopy.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define THRESHOLD 0.75

typedef struct {
  gdouble      black;
  gdouble      white;
  gdouble      prev_mask_radius;
  gdouble      prev_sharpness;
  gdouble      prev_black;
  gdouble      prev_white;
} Ramps;

static void
grey_blur_buffer(GeglBuffer *input,
		 gdouble sharpness,
		 gdouble mask_radius,
		 GeglBuffer **dest1,
		 GeglBuffer **dest2)
{
  GeglNode *gegl, *image, *write1, *write2, *grey, *blur1, *blur2;
  gdouble radius, std_dev1, std_dev2;

  gegl = gegl_node_new();
  image = gegl_node_new_child(gegl,
			      "operation", "gegl:buffer-source",
			      "buffer", input,
			      NULL);
  grey = gegl_node_new_child(gegl,
			     "operation", "gegl:grey",
			     NULL);

  radius   = MAX (1.0, 10 * (1.0 - sharpness));
  radius   = fabs (radius) + 1.0;
  std_dev1 = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));
  
  radius   = fabs (mask_radius) + 1.0;
  std_dev2 = sqrt (-(radius * radius) / (2 * log (1.0 / 255.0)));

  blur1 =  gegl_node_new_child(gegl,
			       "operation", "gegl:gaussian-blur",
			       "std_dev_x", std_dev1,
			       "std_dev_y", std_dev1,
			       NULL);
  blur2 =  gegl_node_new_child(gegl,
			       "operation", "gegl:gaussian-blur",
			       "std_dev_x", std_dev2,
			       "std_dev_y", std_dev2,
			       NULL);

  write1 = gegl_node_new_child(gegl,
			       "operation", "gegl:buffer-sink",
			       "buffer", dest1, NULL);
  
  write2 = gegl_node_new_child(gegl,
			       "operation", "gegl:buffer-sink",
			       "buffer", dest2, NULL);
  
  gegl_node_link_many (image, grey, blur1, write1, NULL);
  gegl_node_process (write1);
  
  gegl_node_link_many (image, grey, blur2, write2, NULL);
  gegl_node_process (write2);

  g_object_unref (gegl);
}

static gdouble
calculate_threshold (gint *hist,
		     gdouble pct,
		     gint count,
		     gint under_threshold)
{
  gint    sum;
  gint    i;

  if (pct == 0.0 || count == 0)
    return (under_threshold ? 1.0 : 0.0);

  sum = 0;
  for (i = 0; i < 2000; i++)
    {
      sum += hist[i];
      if (((gdouble) sum / (gdouble) count) > pct)
        {
          if (under_threshold)
            return (THRESHOLD - (gdouble) i / 1000.0);
          else
            return ((gdouble) i / 1000.0 - THRESHOLD);
        }
    }
  
  return (under_threshold ? 0.0 : 1.0);
}

static void
compute_ramp (GeglBuffer *input,
	      GeglOperation *operation,
	      gdouble pct_black,
	      gdouble pct_white,
	      gint under_threshold,
	      gdouble *threshold_black,
	      gdouble *threshold_white)
{
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);

  GeglRectangle *whole_region;
  gint    n_pixels;
  gint    hist1[2000];
  gint    hist2[2000];
  gdouble diff;
  gint    count;
  gfloat pixel1, pixel2;
  gint x;
  gint y;
  GeglSampler *sampler1;
  GeglSampler *sampler2;
  GeglBuffer *dest1, *dest2;
  
  whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  grey_blur_buffer(input, o->sharpness, o->mask_radius, &dest1, &dest2);
      
  sampler1 = gegl_buffer_sampler_new (dest1,
				      babl_format ("Y float"),
				      GEGL_SAMPLER_LINEAR);
  
  sampler2 = gegl_buffer_sampler_new (dest2,
				      babl_format ("Y float"),
				      GEGL_SAMPLER_LINEAR);
  
  n_pixels = whole_region->width * whole_region->height;
  
  memset (hist1, 0, sizeof (int) * 2000);
  memset (hist2, 0, sizeof (int) * 2000);
  count = 0;
  x = whole_region->x;
  y = whole_region->y;
  while (n_pixels--)
    {
      gegl_sampler_get (sampler1,
			x,
                        y,
                        NULL,
                        &pixel1);

      gegl_sampler_get (sampler2,
                        x,
                        y,
                        NULL,
                        &pixel2);

      diff = pixel1/pixel2;

      if (under_threshold)
	{
	  if (diff < THRESHOLD)
	    {
	      hist2[(int) (diff * 1000)] += 1;
	      count += 1;
	    }
	}
      else 
	{
	  if (diff >= THRESHOLD && diff < 2.0)
	    {
	      hist1[(int) (diff * 1000)] += 1;
	      count += 1;
	    }
	}

      x++;
      if (x>=whole_region->x + whole_region->width)
        {
          x=whole_region->x;
          y++;
        }
    }
  
  g_object_unref (sampler1);
  g_object_unref (sampler2);
  g_object_unref (dest1);
  g_object_unref (dest2);

  *threshold_black = calculate_threshold(hist1, pct_black, count, 0);
  *threshold_white = calculate_threshold(hist2, pct_white, count, 1);
}

static void prepare (GeglOperation *operation)
{
  GeglChantO              *o;
    
  o       = GEGL_CHANT_PROPERTIES (operation);
  
  gegl_operation_set_format (operation, "input",
                             babl_format ("Y float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("Y float"));
  if(o->chant_data)
    {
      Ramps* ramps = (Ramps*) o->chant_data;

      /* hack so that thresholds are only calculated once */
      if(ramps->prev_mask_radius != o->mask_radius ||
	 ramps->prev_sharpness   != o->sharpness   ||
	 ramps->prev_black       != o->black       ||
	 ramps->prev_white       != o->white)
	{
	  g_slice_free (Ramps, o->chant_data);
	  o->chant_data = NULL;
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
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);

  GeglBuffer *dest1;
  GeglBuffer *dest2;
  GeglSampler *sampler1;
  GeglSampler *sampler2;

  Ramps* ramps;

  gint n_pixels;
  gfloat pixel1, pixel2;
  gfloat *out_pixel;
  gint x;
  gint y;
  
  gdouble diff;
  Ramps *get_ramps;
  gdouble ramp_down;
  gdouble ramp_up;
  gdouble mult;
  
  gfloat *dst_buf;
  GeglRectangle *whole_region;
  
  static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

  dst_buf = g_slice_alloc (result->width * result->height * 1 * sizeof(gfloat));

  g_static_mutex_lock (&mutex);
  if(o->chant_data == NULL) 
    {
      whole_region = gegl_operation_source_get_bounding_box (operation, "input");
      o->chant_data = g_slice_new (Ramps);
      ramps = (Ramps*) o->chant_data;
      compute_ramp(input,operation,o->black,o->white,1,&ramps->black,&ramps->white);
      ramps->prev_mask_radius = o->mask_radius;
      ramps->prev_sharpness   = o->sharpness;
      ramps->prev_black       = o->black;
      ramps->prev_white       = o->white;
    }
  g_static_mutex_unlock (&mutex);

  gegl_buffer_set_extent(input, result);
  grey_blur_buffer(input, o->sharpness, o->mask_radius, &dest1, &dest2);
  
  sampler1 = gegl_buffer_sampler_new (dest1,
				      babl_format ("Y float"),
				      GEGL_SAMPLER_LINEAR);
  
  sampler2 = gegl_buffer_sampler_new (dest2,
				      babl_format ("Y float"),
				      GEGL_SAMPLER_LINEAR);
  out_pixel = dst_buf;
  x = result->x;
  y = result->y;
  n_pixels = result->width * result->height;
  
  get_ramps = (Ramps*) o->chant_data;
  ramp_down = get_ramps->black;
  ramp_up = get_ramps->white;

  while (n_pixels--)
    {
      gegl_sampler_get (sampler1,
			x,
                        y,
                        NULL,
                        &pixel1);

      gegl_sampler_get (sampler2,
                        x,
                        y,
                        NULL,
                        &pixel2);
      diff = pixel1/pixel2;
      if (diff < THRESHOLD)
	{
	  if (ramp_down == 0.0)
	    mult = 0.0;
	  else
	    mult = (ramp_down - MIN (ramp_down,
				     (THRESHOLD - diff))) / ramp_down;
	  *out_pixel = pixel1 * mult;
	} 
      else
	{
	  if (ramp_up == 0.0)
	    mult = 1.0;
	  else
	    mult = MIN (ramp_up,
			(diff - THRESHOLD)) / ramp_up;

	  *out_pixel = 1.0 - (1.0 - mult) * (1.0 - pixel1);
	}
      out_pixel += 1;
      
      x++;
      if (x>=result->x + result->width)
        {
          x=result->x;
          y++;
        }
    }
  
  gegl_buffer_set (output, result, 0, babl_format ("Y float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (result->width * result->height * 1 * sizeof(gfloat), dst_buf);
 
  g_object_unref (sampler1);
  g_object_unref (sampler2);
  g_object_unref (dest1);
  g_object_unref (dest2);
 
  whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  gegl_buffer_set_extent(input, whole_region);
  return  TRUE;
}

static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      g_slice_free (Ramps, o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass               *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  
  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize = finalize;
  operation_class->prepare = prepare;
  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
				 "categories" , "artistic",
				 "name"       , "gegl:photocopy",
				 "description", _("Photocopy effect"),
				 NULL);
}

#endif
