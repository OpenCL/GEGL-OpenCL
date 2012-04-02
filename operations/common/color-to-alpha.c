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
 * Color To Alpha plug-in v1.0 by Seth Burgess, sjburges@gimp.org 1999/05/14
 *  with algorithm by clahey
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_color (color, _("Color"), "black",
                  _("The color to render (defaults to 'black')"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "color-to-alpha.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}

/*
 * An excerpt from a discussion on #gimp that sheds some light on the ideas
 * behind the algorithm that is being used here.
 *
 <clahey>   so if a1 > c1, a2 > c2, and a3 > c2 and a1 - c1 > a2-c2, a3-c3,
 then a1 = b1 * alpha + c1 * (1-alpha)
 So, maximizing alpha without taking b1 above 1 gives
 a1 = alpha + c1(1-alpha) and therefore alpha = (a1-c1) / (1-c1).
 <sjburges> clahey: btw, the ordering of that a2, a3 in the white->alpha didn't
 matter
 <clahey>   sjburges: You mean that it could be either a1, a2, a3 or
 a1, a3, a2?
 <sjburges> yeah
 <sjburges> because neither one uses the other
 <clahey>   sjburges: That's exactly as it should be.  They are both just
 getting reduced to the same amount, limited by the the darkest
 color.
 <clahey>   Then a2 = b2 * alpha + c2 * (1- alpha).  Solving for b2 gives
 b2 = (a1-c2)/alpha + c2.
 <sjburges> yeah
 <clahey>   That gives us are formula for if the background is darker than the
 foreground? Yep.
 <clahey>   Next if a1 < c1, a2 < c2, a3 < c3, and c1-a1 > c2-a2, c3-a3, and
 by our desired result a1 = b1 * alpha + c1 * (1-alpha),
 we maximize alpha without taking b1 negative gives
 alpha = 1 - a1 / c1.
 <clahey>   And then again, b2 = (a2-c2) / alpha + c2 by the same formula.
 (Actually, I think we can use that formula for all cases, though
 it may possibly introduce rounding error.
 <clahey>   sjburges: I like the idea of using floats to avoid rounding error.
 Good call.
*/

static void
color_to_alpha (gfloat     *color,
                gfloat     *src,
                gint        offset)
{
  gint i;
  gfloat temp[4];

  temp[3] = src[offset + 3];

  for (i=0; i<3; i++)
    {
      if (color[i] < 1.e-4f)
        temp[i] = src[offset+i];
      else if (src[offset+i] > color[i])
        temp[i] = (src[offset+i] - color[i]) / (1.0f - color[i]);
      else if (src[offset+i] < color[i])
        temp[i] = (color[i] - src[offset+i]) / color[i];
      else
        temp[i] = 0.0f;
    }

  if (temp[0] > temp[1])
    {
      if (temp[0] > temp[2])
        src[offset+3] = temp[0];
      else
        src[offset+3] = temp[2];
    }
  else if (temp[1] > temp[2])
    src[offset+3] = temp[1];
  else
    src[offset+3] = temp[2];

  if (src[offset+3] < 1.e-4f)
    return;

  for (i=0; i<3; i++)
    src[offset+i] = (src[offset+i] - color[i]) / src[offset+3] + color[i];

  src[offset+3] *= temp[3];
}



static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o      = GEGL_CHANT_PROPERTIES (operation);
  const Babl *format = babl_format ("RGBA float");
  gfloat     *src_buf, color[4];
  gint        x;

  src_buf = g_new0 (gfloat, result->width * result->height * 4);

  gegl_buffer_get (input, result, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  gegl_color_get_pixel (o->color, babl_format ("RGBA float"), color);

  for (x = 0; x < result->width * result->height; x++)
    color_to_alpha (color, src_buf, 4 * x);

  gegl_buffer_set (output, result, 0, format, src_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);

  return TRUE;
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
    "name"       , "gegl:color-to-alpha",
    "categories" , "color",
    "description", _("Performs color-to-alpha on the image."),
    NULL);
}

#endif
