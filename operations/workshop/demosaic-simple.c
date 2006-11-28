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

gegl_chant_int (pattern, 0, 3, 0, "Bayer pattern used, 0 seems to work for some nikon files, 2 for some Fuji files.")
 
#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            demosaic_simple
#define GEGL_CHANT_DESCRIPTION     "Performs a naive grayscale2color demosaicing of an image, no interpolation."
#define GEGL_CHANT_SELF            "demosaic-simple.c"
#define GEGL_CHANT_CATEGORIES      "blur"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

static GeglRect get_source_rect (GeglOperation *self);

#include <stdio.h>
static void
demosaic (GeglChantOperation *op,
          GeglBuffer *src,
          GeglBuffer *dst);

static gboolean
process (GeglOperation *operation)
{
  GeglOperationFilter *filter;
  GeglChantOperation  *self;
  GeglBuffer          *input;
  GeglBuffer          *output;

  filter = GEGL_OPERATION_FILTER (operation);
  self   = GEGL_CHANT_OPERATION (operation);


  input  = filter->input;

    {
      GeglRect   *result = gegl_operation_result_rect (operation);
      GeglRect    need   = get_source_rect (operation);
      GeglBuffer *temp_in;

      if (result->w==0 ||
          result->h==0)
        {
          output = g_object_ref (input);
        }
      else
        {
          temp_in = g_object_new (GEGL_TYPE_BUFFER,
                                 "source", input,
                                 "x",      need.x,
                                 "y",      need.y,
                                 "width",  need.w,
                                 "height", need.h,
                                 NULL);

          output = g_object_new (GEGL_TYPE_BUFFER,
                                 "format", babl_format ("RGB float"),
                                 "x",      need.x,
                                 "y",      need.y,
                                 "width",  need.w,
                                 "height", need.h,
                                 NULL);

          demosaic (self, temp_in, output);
          g_object_unref (temp_in);
        }

      {
        GeglBuffer *cropped = g_object_new (GEGL_TYPE_BUFFER,
                                              "source", output,
                                              "x",      result->x,
                                              "y",      result->y,
                                              "width",  result->w,
                                              "height", result->h,
                                              NULL);
        filter->output = cropped;
        g_object_unref (output);
      }
    }
  return  TRUE;
}

static void
demosaic (GeglChantOperation *op,
          GeglBuffer *src,
          GeglBuffer *dst)
{
  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;

  src_buf = g_malloc0 (src->width * src->height * 4);
  dst_buf = g_malloc0 (dst->width * dst->height * 4 * 3);
  
  gegl_buffer_get (src, NULL, src_buf, babl_format ("Y float"), 1.0);

  offset=0;
  for (y=src->y; y<dst->height + src->y; y++)
    for (x=src->x; x<dst->width + src->x; x++)
      {
        gfloat red=0.0;
        gfloat green=0.0;
        gfloat blue=0.0;

        if (y<dst->height+src->y-1 &&
            x<dst->width+src->x-1)
          {
        if ((y + op->pattern%2)%2==0)
          {
            if ((x+op->pattern/2)%2==1)
              {
                blue=src_buf[offset+1];
                green=src_buf[offset];
                red=src_buf[offset + src->width];
              }
            else
              {
                blue=src_buf[offset];
                green=src_buf[offset+1];
                red=src_buf[offset+1+src->width];
              }
          }
        else
          {
            if ((x+op->pattern/2)%2==1)
              {
                blue=src_buf[offset + src->width + 1];
                green=src_buf[offset + 1];
                red=src_buf[offset];
              }
            else
              {
                blue=src_buf[offset + src->width];
                green=src_buf[offset];
                red=src_buf[offset + 1];
              }
          }
        }
        
        dst_buf [offset*3+0] = red;
        dst_buf [offset*3+1] = green;
        dst_buf [offset*3+2] = blue;

        offset++;
      }

  gegl_buffer_set (dst, NULL, dst_buf, babl_format ("RGB float"));
  g_free (src_buf);
  g_free (dst_buf);
}

#include <math.h>
static GeglRect 
get_defined_region (GeglOperation *operation)
{
  GeglRect  result = {0,0,0,0};
  GeglRect *in_rect = gegl_operation_source_get_defined_region (operation,
                                                                "input");
  if (!in_rect)
    return result;

  result = *in_rect;
  if (result.w != 0 &&
      result.h != 0)
    {
      result.w+=1;
      result.h+=1;
    }
  
  return result;
}

static GeglRect get_source_rect (GeglOperation *self)
{
  GeglRect            rect;

  rect  = *gegl_operation_get_requested_region (self);
  if (rect.w != 0 &&
      rect.h != 0)
    {
      rect.w += 1;
      rect.h += 1;
    }

  return rect;
}

static gboolean
calc_source_regions (GeglOperation *self)
{
  GeglRect need = get_source_rect (self);

  gegl_operation_set_source_region (self, "input", &need);

  return TRUE;
}

static GeglRect
get_affected_region (GeglOperation *self,
                     const gchar   *input_pad,
                     GeglRect       region)
{
  gint                radius;
 
  radius = ceil(1);

  region.w += 1;
  region.h += 1;
  return region;
}

static void class_init (GeglOperationClass *operation_class)
{
  operation_class->get_defined_region  = get_defined_region;
  operation_class->get_affected_region = get_affected_region;
  operation_class->calc_source_regions = calc_source_regions;
}

#endif
