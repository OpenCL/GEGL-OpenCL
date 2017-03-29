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
 * Copyright 2009 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int  (levels, _("Levels"), 8)
  description (_("number of levels per component"))
  value_range (1, 64)
  ui_gamma    (2.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     posterize
#define GEGL_OP_C_SOURCE posterize.c

#include "gegl-op.h"

#ifndef RINT
#define RINT(a) ((gint)(a+0.5))
#endif


static gboolean process (GeglOperation       *operation,
                         void                *in_buf,
                         void                *out_buf,
                         glong                samples,
                         const GeglRectangle *roi,
                         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  gfloat     *src    = in_buf;
  gfloat     *dest   = out_buf;
  gfloat      levels = o->levels;

  while (samples--)
    {
      gint i;
      for (i=0; i < 3; i++)
        dest[i] = RINT (src[i]   * levels) / levels;
      dest[3] = src[3];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}

#include "opencl/gegl-cl.h"
#include "opencl/posterize.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem              in,
            cl_mem              out,
            size_t              global_worksize,
            const GeglRectangle *roi,
            gint                level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  cl_float    levels = o->levels;

  if (!cl_data)
    {
      const char *kernel_name[] = {"cl_posterize",
                                   NULL};
      cl_data = gegl_cl_compile_and_build (posterize_cl_source, kernel_name);
    }

  if (!cl_data)
    return 1;

  {
  cl_int cl_err = 0;

  gegl_cl_set_kernel_args (cl_data->kernel[0],
                           sizeof(cl_mem),   &in,
                           sizeof(cl_mem),   &out,
                           sizeof(cl_float), &levels,
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

  operation_class->opencl_support = TRUE;
  point_filter_class->process     = process;
  point_filter_class->cl_process  = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:posterize",
    "title",       _("Posterize"),
    "reference-hash", "6c325366cad73837346ea052aea4d7dc",
    "categories" , "color",
    "description",
       _("Reduces the number of levels in each color component of the image."),
       NULL);

}

#endif
