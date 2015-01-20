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
 * Copyright 1997 Miles O'Neal <meo@rru.com>  http://www.rru.com/~meo/
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 */

/*
 *  HURL Operation
 *      We just assign a random value at the current pixel
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (pct_random, _("Randomization (%)"), 50.0)
    value_range (0.0, 100.0)

property_int   (repeat, _("Repeat"), 1)
   value_range (1, 100)

property_seed (seed, _("Random seed"), rand) 

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE noise-hurl.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input" , babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o       = GEGL_PROPERTIES (operation);
  gfloat         *in_pix  = in_buf;
  gfloat         *out_pix = out_buf;
  GeglRectangle  *whole_region;
  gint            total_size, cnt;
  gint            x, y;

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  total_size   = whole_region->width * whole_region->height;

  for (y = roi->y; y < roi->y + roi->height; y++)
    for (x = roi->x; x < roi->x + roi->width; x++)
      {
        gfloat red, green, blue, alpha;
        gint   idx = x + whole_region->width * y;

        red   = in_pix[0];
        green = in_pix[1];
        blue  = in_pix[2];
        alpha = in_pix[3];

        for (cnt = o->repeat - 1; cnt >= 0; cnt--)
          {
            gint n = 4 * (idx + cnt * total_size);

            if (gegl_random_float_range (o->rand, x, y, 0, n, 0.0, 100.0) <=
                o->pct_random)
              {
                red   = gegl_random_float (o->rand, x, y, 0, n+1);
                green = gegl_random_float (o->rand, x, y, 0, n+2);
                blue  = gegl_random_float (o->rand, x, y, 0, n+3);
                break;
              }
          }

        out_pix[0] = red;
        out_pix[1] = green;
        out_pix[2] = blue;
        out_pix[3] = alpha;

        out_pix += 4;
        in_pix  += 4;
      }

  return TRUE;
}

#include "opencl/gegl-cl.h"
#include "opencl/noise-hurl.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem              in,
            cl_mem              out,
            size_t              global_worksize,
            const GeglRectangle *roi,
            gint                level)
{
  GeglProperties    *o          = GEGL_PROPERTIES (operation);
  GeglRectangle *wr         = gegl_operation_source_get_bounding_box (operation,
                                                                      "input");
  cl_int      cl_err           = 0;
  cl_mem      cl_random_data   = NULL;
  cl_float    pct_random       = o->pct_random;
  cl_int      x_offset         = roi->x;
  cl_int      y_offset         = roi->y;
  cl_int      roi_width        = roi->width;
  cl_int      wr_width         = wr->width;
  int         total_size       = wr->width*wr->height;
  cl_int      offset;
  int         it;
  cl_ushort4  rand;

  gegl_cl_random_get_ushort4 (o->rand, &rand);

  if (!cl_data)
  {
    const char *kernel_name[] ={"cl_noise_hurl", NULL};
    cl_data = gegl_cl_compile_and_build(noise_hurl_cl_source, kernel_name);
  }

  if (!cl_data)
    return TRUE;

  {
  cl_random_data = gegl_cl_load_random_data (&cl_err);
  CL_CHECK;

  cl_err = gegl_clEnqueueCopyBuffer (gegl_cl_get_command_queue(),
                                     in , out , 0 , 0 ,
                                     global_worksize * sizeof(cl_float4),
                                     0, NULL, NULL);
  CL_CHECK;

  gegl_cl_set_kernel_args (cl_data->kernel[0],
                           sizeof(cl_mem),   &out,
                           sizeof(cl_mem),   &cl_random_data,
                           sizeof(cl_int),   &x_offset,
                           sizeof(cl_int),   &y_offset,
                           sizeof(cl_int),   &roi_width,
                           sizeof(cl_int),   &wr_width,
                           sizeof(cl_ushort4), &rand,
                           sizeof(cl_float), &pct_random,
                           NULL);
  CL_CHECK;

  offset = 0;

  for(it = 0; it < o->repeat; ++it)
    {
      cl_err = gegl_clSetKernelArg (cl_data->kernel[0], 8, sizeof(cl_int),
                                    (void*)&offset);
      CL_CHECK;
      cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                            cl_data->kernel[0], 1,
                                            NULL, &global_worksize, NULL,
                                            0, NULL, NULL);
      CL_CHECK;

      offset += total_size;
    }

  cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
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
  point_filter_class->cl_process = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:noise-hurl",
    "title",      _("Randomly Shuffle Pixels"),
    "categories", "noise",
    "description", _("Completely randomize a fraction of pixels"),
    NULL);
}

#endif
