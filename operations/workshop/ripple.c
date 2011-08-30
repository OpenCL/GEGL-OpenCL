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
 * Copyright (C) 1997 Brian Degenhardt <bdegenha@ucsd.edu>
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_boolean (antialias,   _("Antialiasing"), FALSE,
                    _("Antialiasing"))
gegl_chant_boolean (tile, _("Retain tilebility"), FALSE,
                    _("Retain tilability"))
gegl_chant_string (orientation, _("Orientation"), "horizontal",
                    _("Type of orientation to choose."
                      "Choices are horizontal, vertical."
                      "Default is horizontal"))
gegl_chant_string (wave, _("Wave type"), "sine",
                   _("Type of wave to choose."
                     "Choices are sine, sawtooth."
                     "Default is sine"))
gegl_chant_string (background, _("Edges"), "smear",
                   _("Type of edges to choose."
                     "Choices are wrap, smear, blank."
                     "Default is smear"))
gegl_chant_int (amplitude, _("Amplitude"), 0, 200, 50,
                _("Amplitude of ripple"))
gegl_chant_int (period, _("Period"), 0, 200, 50,
                _("Period of ripple"))
gegl_chant_int (phi, _("Phase Shift"), 0, 360, 0,
                _("As proportion of pi"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "ripple.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>

#define SCALE_WIDTH     200
#define TILE_CACHE_SIZE  16

typedef enum
{
  BACKGROUND_TYPE_WRAP,
  BACKGROUND_TYPE_SMEAR,
  BACKGROUND_TYPE_BLANK
} BackgroundType;

typedef enum
{
  WAVE_TYPE_SINE,
  WAVE_TYPE_SAWTOOTH,
} WaveType;

typedef enum
{
  ORIENTATION_TYPE_VERTICAL,
  ORIENTATION_TYPE_HORIZONTAL
} OrientationType;

typedef struct
{
  OrientationType orie_t;
  WaveType        wave_t;
  BackgroundType  back_t;
  gint            per;
} RippleContext;


static void prepare (GeglOperation *operation)
{
  GeglChantO              *o;
  GeglOperationAreaFilter *op_area;

  op_area = GEGL_OPERATION_AREA_FILTER (operation);
  o       = GEGL_CHANT_PROPERTIES (operation);

  op_area->left   = op_area->bottom = 
  op_area->right  = op_area->top = o->amplitude + 2;

  gegl_operation_set_format (operation, "input", 
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static void 
average_two_pixels (gfloat  *dest,
                    gfloat  *temp1,
                    gfloat  *temp2,
                    gdouble  rem)
{
   gfloat x0, x1;
   gfloat alpha;
   gint   i;

   x0 = temp1[3] * (1.0 - rem);
   x1 = temp2[3] * rem;
   
   alpha = x0 + x1;
   
   for (i=0;i<3;i++)
      dest[i] = (x0 * temp1[i] + x1 * temp2[i]) / alpha;

   dest[3] = alpha;
}


static gdouble
displace_amount (gint        location,
                 GeglChantO *o,
                 WaveType    wave_t)
{
  const gdouble phi = o->phi / 360.0;
  gdouble       lambda;

  switch (wave_t)
    {
    case WAVE_TYPE_SINE:
      return (o->amplitude *
              sin (2 * G_PI * (location / (gdouble) o->period - phi)));

    case WAVE_TYPE_SAWTOOTH:
      lambda = location % o->period - phi * o->period;
      if (lambda < 0)
        lambda += o->period;
      return (o->amplitude * (fabs (((lambda / o->period) * 4) - 2) - 1));
    }

  return 0.0;
}

static void
ripple_horizontal (gfloat              *src_buf,
                   gfloat              *dst_buf,
                   const GeglRectangle *result,
                   const GeglRectangle *extended, 
                   const GeglRectangle *image,
                   gint                 y,
                   RippleContext        rip,
                   GeglChantO          *o,
                   GeglBuffer          *input)
{
  gint  x;
  gint  src_offset, dst_offset;
  Babl *format = babl_format ("RGBA float");  

  dst_offset = (y - result->y) * result->width * 4;

  for (x = result->x; x < result->x + result->width; x++)
     {
     gdouble shift = x + displace_amount(y, o, rip.wave_t);
     gint    xi    = floor (shift);
     gint    xi_a  = xi + 1;
     gdouble rem   = shift - xi;
     gint    i;
     gfloat  dest[4];

     if (rip.back_t == BACKGROUND_TYPE_WRAP)
       {
         shift = fmod (shift, image->width);
         while (shift < 0.0)
            shift += image->width;
 
         xi = (xi % image->width);
         while (xi < 0)
             xi += image->width;
      
         xi_a = (xi+1) % image->width;
       }
     else if (rip.back_t == BACKGROUND_TYPE_SMEAR)
       {
         shift = CLAMP (shift, 0, image->width - 1);
         xi    = CLAMP (xi, 0 ,image->width - 1);
         xi_a  = CLAMP (xi_a, 0, image->width - 1);
       }

     if (o->antialias)
        {
         gfloat temp1[4], temp2[4]; 
         if (xi >= 0 && xi < image->width)
           {
              if (xi >= extended->x && xi < extended->x + extended->width) 
                { 
                 src_offset = (y - extended->y) * extended->width * 4 + 
                              (xi - extended->x) * 4;
                 for (i=0;i<4;i++)
                     temp1[i] = src_buf[src_offset++];
                }
              else gegl_buffer_sample (input, xi, y, NULL, temp1, format,
                                       GEGL_INTERPOLATION_NEAREST);
           }
         else temp1[0] = temp1[1] = temp2[2] = temp1[3] = 0.0;

         if (xi_a >= 0 && xi_a < image->width)
           {
              if (xi_a >= extended->x && xi_a < extended->x + extended->width) 
                { 
                 src_offset = (y - extended->y) * extended->width * 4 + 
                              (xi_a - extended->x) * 4;
                 for (i=0;i<4;i++)
                     temp2[i] = src_buf[src_offset++];
                }
              else gegl_buffer_sample (input, xi_a, y, NULL, temp2, format,
                                       GEGL_INTERPOLATION_NEAREST);
           }
         else temp2[0] = temp2[1] = temp2[2] = temp2[3] = 0.0; 

         average_two_pixels (dest, temp1, temp2, rem);
        }
     else
        {
         if (xi >= 0 && xi < image->width)
           {
              if (xi >= extended->x && xi < extended->x + extended->width) 
                { 
                 src_offset = (y - extended->y) * extended->width * 4 + 
                              (xi - extended->x) * 4;
                 for (i=0;i<4;i++)
                     dest[i] = src_buf[src_offset++];
                }
              else gegl_buffer_sample (input, xi, y, NULL, dest, format,
                                       GEGL_INTERPOLATION_NEAREST);
           }
         else dest[0] = dest[1] = dest[2] = dest[3] = 0.0;
        }

     for (i=0;i<4;i++)
        dst_buf[dst_offset++] = dest[i];

     }
}

static void
ripple_vertical   (gfloat              *src_buf,
                   gfloat              *dst_buf,
                   const GeglRectangle *result,
                   const GeglRectangle *extended, 
                   const GeglRectangle *image,
                   gint                 y,
                   RippleContext        rip,
                   GeglChantO          *o,
                   GeglBuffer          *input)
{
  gint x;
  gint src_offset, dst_offset;
  Babl *format = babl_format ("RGBA float"); 
  dst_offset = (y - result->y) * result->width * 4;

  for (x = result->x; x < result->x + result->width; x++)
     {
     gdouble shift = y + displace_amount(x, o, rip.wave_t);
     gint    xi    = floor (shift);
     gint    xi_a  = xi + 1;
     gdouble rem   = shift - xi;
     gint    i;
     gfloat  dest[4];

     if (rip.back_t == BACKGROUND_TYPE_WRAP)
       {
         shift = fmod (shift, image->height);
         while (shift < 0.0)
            shift += image->height;
 
         xi = (xi % image->height);
         while (xi < 0)
             xi += image->height;
      
         xi_a = (xi+1) % image->height;
       }
     else if (rip.back_t == BACKGROUND_TYPE_SMEAR)
       {
         shift = CLAMP (shift, 0, image->height - 1);
         xi    = CLAMP (xi, 0 ,image->height - 1);
         xi_a  = CLAMP (xi_a, 0, image->height - 1);
       }

     if (o->antialias)
        {
         gfloat temp1[4], temp2[4]; 
         if (xi >= 0 && xi < image->width)
           {
              if (xi >= extended->y && xi < extended->y + extended->height) 
                { 
                 src_offset = (xi - extended->y) * extended->width * 4 + 
                              (x - extended->x) * 4;
                 for (i=0;i<4;i++)
                     temp1[i] = src_buf[src_offset++];
                }
              else gegl_buffer_sample (input, x, xi, NULL, temp1, format,
                                       GEGL_INTERPOLATION_NEAREST);
           }
         else temp1[0] = temp1[1] = temp2[2] = temp1[3] = 0.0;
   
         if (xi_a >= 0 && xi_a < image->width)
           {
              if (xi_a >= extended->y && xi_a < extended->y + extended->height) 
                { 
                 src_offset = (xi_a - extended->y) * extended->width * 4 + 
                              (x - extended->x) * 4;
                 for (i=0;i<4;i++)
                     temp2[i] = src_buf[src_offset++];
                }
              else gegl_buffer_sample (input, x, xi_a, NULL, temp2, format,
                                       GEGL_INTERPOLATION_NEAREST);
            }
         else temp2[0] = temp2[1] = temp2[2] = temp2[3] = 0.0; 

         average_two_pixels (dest, temp1, temp2, rem);
        }
     else
        {
         if (xi >= 0 && xi < image->width)
           {
              if (xi >= extended->y && xi < extended->y + extended->height) 
                { 
                 src_offset = (xi - extended->y) * extended->width * 4 + 
                              (x - extended->x) * 4;
                 for (i=0;i<4;i++)
                     dest[i] = src_buf[src_offset++];
                }
              else gegl_buffer_sample (input, x, xi, NULL, dest, format,
                                       GEGL_INTERPOLATION_NEAREST);
            }
         else dest[0] = dest[1] = dest[2] = dest[3] = 0.0;
        }

     for (i=0;i<4;i++)
        dst_buf[dst_offset++] = dest[i];

     }
}

static GeglRectangle
get_effective_area (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  gegl_rectangle_copy(&result, in_rect);

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO              *o            = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area      = GEGL_OPERATION_AREA_FILTER (operation);
  GeglRectangle            boundary     = get_effective_area (operation);
  GeglRectangle            extended;
  Babl                    *format       = babl_format ("RGBA float");
  RippleContext            rip;

  gfloat *dst_buf, *src_buf;
  gint    y;

  extended.x      = CLAMP (result->x - op_area->left, boundary.x, boundary.x +
                           boundary.width);
  extended.width  = CLAMP (result->width + op_area->left + op_area->right, 0,
                           boundary.width);
  extended.y      = CLAMP (result->y - op_area->top, boundary.y, boundary.y +
                           boundary.width);
  extended.height = CLAMP (result->height + op_area->top + op_area->bottom, 0,
                           boundary.height);

  dst_buf = g_new0 (gfloat, result->width * result->height * 4);
  src_buf = g_new0 (gfloat, extended.width * extended.height * 4);

  rip.per = o->period;
 
  rip.wave_t = WAVE_TYPE_SINE;
  if (!strcmp (o->wave, "sine"))
    rip.wave_t = WAVE_TYPE_SINE;
  else if (!strcmp(o->wave, "sawtooth"))
    rip.wave_t = WAVE_TYPE_SAWTOOTH;

  rip.back_t = BACKGROUND_TYPE_WRAP;
  if (!strcmp (o->background, "wrap"))
    rip.back_t = BACKGROUND_TYPE_WRAP;
  else if (!strcmp (o->background, "smear"))
    rip.back_t = BACKGROUND_TYPE_SMEAR;
  else if (!strcmp (o->background, "blank"))
    rip.back_t = BACKGROUND_TYPE_BLANK;

  rip.orie_t = ORIENTATION_TYPE_HORIZONTAL;
  if (!strcmp (o->orientation, "horizontal"))
    rip.orie_t = ORIENTATION_TYPE_HORIZONTAL;
  else if (!strcmp(o->orientation, "vertical"))
    rip.orie_t = ORIENTATION_TYPE_VERTICAL;

  if (o->tile)
     {
     rip.back_t = BACKGROUND_TYPE_WRAP;
     rip.per    = (boundary.width / (boundary.width / rip.per) *
                   (rip.orie_t == ORIENTATION_TYPE_HORIZONTAL) +
                   boundary.height / (boundary.height / rip.per) *
                   (rip.orie_t == ORIENTATION_TYPE_VERTICAL));        
     }

 
  gegl_buffer_get (input, 1.0, &extended, format,
                   src_buf, GEGL_AUTO_ROWSTRIDE);

  for (y = result->y; y < result->y + result->height; y++)
    if (rip.orie_t == ORIENTATION_TYPE_HORIZONTAL)
        ripple_horizontal (src_buf, dst_buf, result, &extended, &boundary, y,
                           rip, o, input);
    else if (rip.orie_t == ORIENTATION_TYPE_VERTICAL)
        ripple_vertical   (src_buf, dst_buf, result, &extended, &boundary, y,
                           rip, o, input);

  gegl_buffer_set (output, result, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (dst_buf);
  g_free (src_buf);

  return  TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  if (!in_rect)
    return result;

  return *in_rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_bounding_box (operation);
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
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;


  operation_class->categories  = "distorts";
  operation_class->name        = "gegl:ripple";
  operation_class->description = _("Performs ripple on the image.");
}

#endif
