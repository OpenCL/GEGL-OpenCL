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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_seed   (seed, _("Seed"),
                   _("Random seed"))

gegl_chant_double (pct_random, _("Randomization (%)"),
                   0.0, 100.0, 50.0,
                   _("Randomization"))

gegl_chant_int    (repeat, _("Repeat"),
                   1, 100, 1,
                   _("Repeat"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE "noise-hurl.c"

#include "gegl-chant.h"

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
  GeglChantO    *o  = GEGL_CHANT_PROPERTIES (operation);
  gfloat        *GEGL_ALIGNED in_pixel;
  gfloat        *GEGL_ALIGNED out_pixel;
  gfloat        *out_pix;
  GeglRectangle *whole_region;
  gint          total_size;
  gint          i, cnt;
  gint          offset;

  in_pixel  = in_buf;
  out_pixel = out_buf;
  out_pix   = out_pixel;

  for (i = 0; i < n_pixels * 4; i++)
    {
      *out_pix = *in_pixel;
      out_pix += 1;
      in_pixel += 1;
    }

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");
  total_size   = whole_region->width*whole_region->height;
  offset = 0;

  for (cnt = 0; cnt < o->repeat; cnt++)
    {
      gint x = roi->x;
      gint y = roi->y;

      out_pix = out_pixel;

      for (i = 0; i < n_pixels; i++)
        {
          gfloat red, green, blue, alpha;
          gint idx, n;

          red   = out_pix[0];
          green = out_pix[1];
          blue  = out_pix[2];
          alpha = out_pix[3];

          idx = x + whole_region->width * y;
          n = 4 * (idx + offset);

          if (gegl_random_float_range (o->seed, x, y, 0, n, 0.0, 100.0) <=
              o->pct_random)
            {
              red   = gegl_random_float (o->seed, x, y, 0, n+1);
              green = gegl_random_float (o->seed, x, y, 0, n+2);
              blue  = gegl_random_float (o->seed, x, y, 0, n+3);
            }

          out_pix[0] = red;
          out_pix[1] = green;
          out_pix[2] = blue;
          out_pix[3] = alpha;

          out_pix += 4;

          x++;
          if (x >= roi->x + roi->width)
            {
              x = roi->x;
              y++;
            }
        }
      offset += total_size;
    }

  return TRUE;
}

#include "opencl/gegl-cl.h"
#include "opencl/noise-hurl.cl.h"

GEGL_CL_STATIC

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem              in,
            cl_mem              out,
            size_t              global_worksize,
            const GeglRectangle *roi,
            gint                level)
{
  GeglChantO    *o          = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle *wr         = gegl_operation_source_get_bounding_box (operation,
                                                                      "input");
  cl_int   cl_err           = 0;
  cl_mem   cl_random_data   = NULL;
  cl_mem   cl_random_primes = NULL;
  cl_float pct_random       = o->pct_random;
  cl_int   seed             = o->seed;
  cl_int   x_offset         = roi->x;
  cl_int   y_offset         = roi->y;
  cl_int   roi_width        = roi->width;
  cl_int   wr_width         = wr->width;
  int      total_size       = wr->width*wr->height;
  cl_int   offset;
  int      it;

  GEGL_CL_BUILD(noise_hurl, "cl_noise_hurl")

  {
  cl_random_data = gegl_cl_load_random_data(&cl_err);
  CL_CHECK;
  cl_random_primes = gegl_cl_load_random_primes(&cl_err);
  CL_CHECK;

  cl_err = gegl_clEnqueueCopyBuffer(gegl_cl_get_command_queue(),
                                    in , out , 0 , 0 ,
                                    global_worksize * sizeof(cl_float4),
                                    0, NULL, NULL);
  CL_CHECK;

  GEGL_CL_ARG_START(cl_data->kernel[0])
  GEGL_CL_ARG(cl_mem,   out)
  GEGL_CL_ARG(cl_mem,   cl_random_data)
  GEGL_CL_ARG(cl_mem,   cl_random_primes)
  GEGL_CL_ARG(cl_int,   x_offset)
  GEGL_CL_ARG(cl_int,   y_offset)
  GEGL_CL_ARG(cl_int,   roi_width)
  GEGL_CL_ARG(cl_int,   wr_width)
  GEGL_CL_ARG(cl_int,   seed)
  GEGL_CL_ARG(cl_float, pct_random)
  GEGL_CL_ARG_END

  offset = 0;

  for(it = 0; it < o->repeat; ++it)
    {
      cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 9, sizeof(cl_int),
                                   (void*)&offset);
      CL_CHECK;
      cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                         cl_data->kernel[0], 1,
                                         NULL, &global_worksize, NULL,
                                         0, NULL, NULL);
      CL_CHECK;

      offset += total_size;
    }

  cl_err = gegl_clFinish(gegl_cl_get_command_queue ());
  CL_CHECK;

  GEGL_CL_RELEASE(cl_random_data)
  GEGL_CL_RELEASE(cl_random_primes)
  }

  return  FALSE;

error:
  if(cl_random_data)
    GEGL_CL_RELEASE(cl_random_data)
  if(cl_random_primes)
    GEGL_CL_RELEASE(cl_random_primes)

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
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
    "categories", "noise",
    "description", _("Completely randomize a fraction of pixels"),
    NULL);
}

#endif
