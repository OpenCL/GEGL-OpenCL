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
 * This plugin is used for removing the red-eye effect
 * that occurs in flash photos.
 *
 * Based on a GIMP 1.2 Perl plugin by Geoff Kuenning
 *
 * Copyright (C) 2004  Robert Merkel <robert.merkel@benambra.org>
 * Copyright (C) 2006  Andreas Røsdal <andrearo@stud.ntnu.no>
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (threshold, _("Threshold"), 0.0, 0.8, 0.4,
                   _("The value of the threshold"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "red-eye-removal.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>

#define RED_FACTOR    0.5133333
#define GREEN_FACTOR  1
#define BLUE_FACTOR   0.1933333

#define SCALE_WIDTH   100

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

static void
red_eye_reduction (gfloat *src,
                   gint    offset,
                   gfloat  threshold)
{
  const gint red   = 0;
  const gint green = 1;
  const gint blue  = 2;

  gfloat dest;

  gfloat adjusted_red       = src[offset + red] * RED_FACTOR;
  gfloat adjusted_green     = src[offset + green] * GREEN_FACTOR;
  gfloat adjusted_blue      = src[offset + blue] * BLUE_FACTOR;
  gfloat adjusted_threshold = (threshold - 0.4) * 2;

  if (adjusted_red >= adjusted_green - adjusted_threshold &&
      adjusted_red >= adjusted_blue - adjusted_threshold)
    {
      dest = CLAMP (((gdouble) (adjusted_green + adjusted_blue)
                     / (2.0  * RED_FACTOR)), 0.0, 1.0);
    }
  else
    {
      dest = src[offset + red];
    }

  src[offset] = dest;
}



static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO              *o            = GEGL_CHANT_PROPERTIES (operation);
  const Babl              *format       = babl_format ("RGBA float");

  gfloat *src_buf;
  gint    x;

  src_buf = g_new0 (gfloat, result->width * result->height * 4);

  gegl_buffer_get (input, result, 1.0, format, src_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (x = 0; x < result->width * result->height; x++)
    red_eye_reduction (src_buf, 4 * x, (float) o->threshold);

  gegl_buffer_set (output, result, 0, format, src_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);

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
    "categories"  , "enhance",
    "name"        , "gegl:red-eye-removal",
    "description" , _("Performs red-eye-removal on the image"),
    NULL);
}

#endif
