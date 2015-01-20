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
 * Copyright (C) 2012 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_color (color, _("Color"), "white")
    description(_("The color to make transparent."))

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE color-to-alpha.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("R'G'B'A float"));
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
color_to_alpha (const gfloat *color,
                const gfloat *src,
                gfloat       *dst)
{
  gint i;
  gfloat alpha[4];

  for (i=0; i<4; i++)
    dst[i] = src[i];

  alpha[3] = dst[3];

  for (i=0; i<3; i++)
    {
      if (color[i] < 0.00001)
        alpha[i] = dst[i];
      else if (dst[i] > color[i] + 0.00001)
        alpha[i] = (dst[i] - color[i]) / (1.0f - color[i]);
      else if (dst[i] < color[i] - 0.00001)
        alpha[i] = (color[i] - dst[i]) / (color[i]);
      else
        alpha[i] = 0.0f;
    }

  if (alpha[0] > alpha[1])
    {
      if (alpha[0] > alpha[2])
        dst[3] = alpha[0];
      else
        dst[3] = alpha[2];
    }
  else if (alpha[1] > alpha[2])
    {
      dst[3] = alpha[1];
    }
  else
    {
      dst[3] = alpha[2];
    }

  if (dst[3] < 0.00001)
    return;

  for (i=0; i<3; i++)
    dst[i] = (dst[i] - color[i]) / dst[3] + color[i];

  dst[3] *= alpha[3];
}

#include "opencl/gegl-cl.h"
#include "opencl/color-to-alpha.cl.h"

static GeglClRunData * cl_data = NULL;

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem              in,
            cl_mem              out,
            size_t              global_worksize,
            const GeglRectangle *roi,
            gint                level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat      color[4];
  gegl_color_get_pixel (o->color, babl_format ("R'G'B'A float"), color);

  if (!cl_data)
    {
      const char *kernel_name[] = {"cl_color_to_alpha",NULL};
      cl_data = gegl_cl_compile_and_build (color_to_alpha_cl_source, kernel_name);
    }
  if (!cl_data)
    return TRUE;
  else
  {
    cl_int cl_err = 0;
    cl_float4 f_color;
    f_color.s[0] = color[0];
    f_color.s[1] = color[1];
    f_color.s[2] = color[2];
    f_color.s[3] = color[3];

    cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0,  sizeof(cl_mem),   (void*)&in);
    CL_CHECK;
    cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1,  sizeof(cl_mem),   (void*)&out);
    CL_CHECK;
    cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2,  sizeof(cl_float4),(void*)&f_color);
    CL_CHECK;

    cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                         cl_data->kernel[0], 1,
                                         NULL, &global_worksize, NULL,
                                         0, NULL, NULL);
    CL_CHECK;
  }

  return  FALSE;

 error:
  return TRUE;
}


static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl *format = babl_format ("R'G'B'A float");
  gfloat      color[4];
  gint        x;

  gfloat *in_buff = in_buf;
  gfloat *out_buff = out_buf;

  gegl_color_get_pixel (o->color, format, color);

  for (x = 0; x < n_pixels; x++)
    {
      color_to_alpha (color, in_buff, out_buff);
      in_buff  += 4;
      out_buff += 4;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;
  gchar                         *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='svg:dst-over'>"
    "  <node operation='gegl:crop'>"
    "    <params>"
    "      <param name='width'>200.0</param>"
    "      <param name='height'>200.0</param>"
    "    </params>"
    "  </node>"
    "  <node operation='gegl:checkerboard'>"
    "    <params><param name='color1'>rgb(0.5, 0.5, 0.5)</param></params>"
    "  </node>"
    "</node>"
    "<node operation='gegl:color-to-alpha'>"
    "</node>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-input.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  filter_class->process    = process;
  filter_class->cl_process = cl_process;

  operation_class->prepare = prepare;
  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:color-to-alpha",
    "title",       _("Color to Alpha"),
    "categories",  "color",
    "license",     "GPL3+",
    "description", _("Convert a specified color to transparency, works best with white."),
    "reference-composition", composition,
    NULL);
}

#endif
