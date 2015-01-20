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
 * Copyright (C) 2004 Robert Merkel <robert.merkel@benambra.org>
 * Copyright (C) 2006 Andreas Røsdal <andrearo@stud.ntnu.no>
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (threshold, _("Threshold"), 0.4)
    description (_("Red eye threshold"))
    value_range (0, 0.8)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE red-eye-removal.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>

#define RED_FACTOR    0.5133333
#define GREEN_FACTOR  1
#define BLUE_FACTOR   0.1933333

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("R'G'B'A float"));
}

static void
red_eye_reduction (gfloat *buf,
                   gfloat  threshold)
{
  gfloat adjusted_red       = buf[0] * RED_FACTOR;
  gfloat adjusted_green     = buf[1] * GREEN_FACTOR;
  gfloat adjusted_blue      = buf[2] * BLUE_FACTOR;
  gfloat adjusted_threshold = (threshold - 0.4) * 2;
  gfloat tmp;

  if (adjusted_red >= adjusted_green - adjusted_threshold &&
      adjusted_red >= adjusted_blue  - adjusted_threshold)
    {
      tmp = (gdouble) (adjusted_green + adjusted_blue) / (2.0 * RED_FACTOR);
      buf[0] = CLAMP (tmp, 0.0, 1.0);
    }
  /* Otherwise, leave the red channel alone */
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  gfloat *dest = out_buf;
  glong   i;

  /*
   * Initialize the dest buffer to the input buffer
   * (we only want to change the red component)
   */
  memcpy (out_buf, in_buf, sizeof (gfloat) * 4 * n_pixels);

  for (i = n_pixels; i > 0; i--)
    {
      red_eye_reduction (dest, o->threshold);
      dest += 4;
    }

  return TRUE;
}

#include "opencl/gegl-cl.h"
#include "opencl/red-eye-removal.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem              in,
            cl_mem              out,
            size_t              global_worksize,
            const GeglRectangle *roi,
            gint                level)
{
  GeglProperties *o           = GEGL_PROPERTIES (operation);
  cl_float   threshold    = o->threshold;


  if (!cl_data)
    {
      const char *kernel_name[] = {"cl_red_eye_removal", NULL};
      cl_data = gegl_cl_compile_and_build(red_eye_removal_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;

  {
  cl_int cl_err = 0;

  gegl_cl_set_kernel_args (cl_data->kernel[0],
                           sizeof(cl_mem),   &in,
                           sizeof(cl_mem),   &out,
                           sizeof(cl_float), &threshold,
                           NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 1,
                                        NULL, &global_worksize, NULL,
                                        0, NULL, NULL);
  CL_CHECK;
  }

  return  FALSE;

error:
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare    = prepare;
  operation_class->opencl_support = TRUE;
  point_filter_class->process = process;
  point_filter_class->cl_process  = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:red-eye-removal",
    "title",       _("Red Eye Removal"),
    "categories",  "enhance",
    "license",     "GPL3+",
    "description", _("Remove the red eye effect caused by camera flashes"),
    NULL);
}

#endif
