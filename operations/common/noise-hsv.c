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
 * Copyright 1995 Spencer Kimball and Peter Mattis
 * Copyright 1996 Torsten Martinsen
 * Copyright 2000 Tim Copperfield <timecop@japan.co.jp>
 * Copyright 2012 Maxime Nicco <maxime.nicco@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int  (holdness, _("Holdness"), 2)
  value_range (1, 8)

property_double (hue_distance, _("Hue"), 3.0)
  value_range   (0.0, 180.0)

property_double (saturation_distance, _("Saturation"), 0.04)
  value_range   (0.0, 1.0)

property_double (value_distance, _("Value"), 0.04)
  value_range   (0.0, 1.0)

property_seed   (seed, _("Random seed"), rand)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     noise_hsv
#define GEGL_OP_C_SOURCE noise-hsv.c

#include "gegl-op.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static gfloat
randomize_value (gfloat      now,
                 gfloat      min,
                 gfloat      max,
                 gboolean    wraps_around,
                 gfloat      rand_max,
                 gint        holdness,
                 gint        x,
                 gint        y,
                 gint        n,
                 GeglRandom *rand)
{
  gint    flag, i;
  gfloat rand_val, new_val, steps;

  steps = max - min;
  rand_val = gegl_random_float (rand, x, y, 0, n++);

  for (i = 1; i < holdness; i++)
  {
    gfloat tmp = gegl_random_float (rand, x, y, 0, n++);
    if (tmp < rand_val)
      rand_val = tmp;
  }

  flag = (gegl_random_float (rand, x, y, 0, n) < 0.5) ? -1 : 1;
  new_val = now + flag * fmod (rand_max * rand_val, steps);

  if (new_val < min)
  {
    if (wraps_around)
      new_val += steps;
    else
      new_val = min;
  }

  if (max < new_val)
  {
    if (wraps_around)
      new_val -= steps;
    else
      new_val = max;
  }

  return new_val;
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("HSVA float"));
  gegl_operation_set_format (operation, "output", babl_format ("HSVA float"));
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o  = GEGL_PROPERTIES (operation);
  GeglRectangle whole_region;
  gint i;
  gint x, y;

  gfloat   * GEGL_ALIGNED in_pixel;
  gfloat   * GEGL_ALIGNED out_pixel;

  gfloat    hue, saturation, value, alpha;

  in_pixel      = in_buf;
  out_pixel     = out_buf;

  x = roi->x;
  y = roi->y;

  whole_region = *(gegl_operation_source_get_bounding_box (operation, "input"));

  for (i = 0; i < n_pixels; i++)
  {
    /* n is independent from the roi, but from the whole image */
    gint n = (3 * o->holdness + 4) * (x + whole_region.width * y);

    hue        = in_pixel[0];
    saturation = in_pixel[1];
    value      = in_pixel[2];
    alpha      = in_pixel[3];

    /* there is no need for scattering hue of desaturated pixels here */
    if ((o->hue_distance > 0) && (saturation > 0))
      hue = randomize_value (hue, 0.0, 1.0, TRUE, o->hue_distance / 360.0,
                             o->holdness, x, y, n, o->rand);

    n += o->holdness + 1;
    /* desaturated pixels get random hue before increasing saturation */
    if (o->saturation_distance > 0) {
      if (saturation == 0)
        hue = gegl_random_float_range (o->rand, x, y, 0, n, 0.0, 1.0);
      saturation = randomize_value (saturation, 0.0, 1.0, FALSE,
                                    o->saturation_distance, o->holdness,
                                    x, y, n+1, o->rand);
    }

    n += o->holdness + 2;
    if (o->value_distance > 0)
      value = randomize_value (value, 0.0, 1.0, FALSE, o->value_distance,
                               o->holdness, x, y, n, o->rand);

    out_pixel[0] = hue;
    out_pixel[1] = saturation;
    out_pixel[2] = value;
    out_pixel[3] = alpha;

    in_pixel  += 4;
    out_pixel += 4;

    x++;
    if (x >= roi->x + roi->width)
      {
        x = roi->x;
        y++;
      }
  }

  return TRUE;
}

#include "opencl/gegl-cl.h"
#include "opencl/noise-hsv.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem               in,
            cl_mem               out,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle *wr = gegl_operation_source_get_bounding_box (operation,
                                                              "input");
  cl_int      cl_err           = 0;
  cl_mem      cl_random_data   = NULL;
  cl_int      x_offset         = roi->x;
  cl_int      y_offset         = roi->y;
  cl_int      roi_width        = roi->width;
  cl_int      wr_width         = wr->width;
  cl_ushort4  rand;
  cl_int      holdness;
  cl_float    hue_distance;
  cl_float    saturation_distance;
  cl_float    value_distance;

  gegl_cl_random_get_ushort4 (o->rand, &rand);

  if (!cl_data)
    {
      const char *kernel_name[] = { "cl_noise_hsv", NULL };
      cl_data = gegl_cl_compile_and_build (noise_hsv_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;

  cl_random_data = gegl_cl_load_random_data (&cl_err);
  CL_CHECK;

  holdness = o->holdness;
  hue_distance = o->hue_distance / 360.0;
  saturation_distance = o->saturation_distance;
  value_distance = o->value_distance;

  gegl_cl_set_kernel_args (cl_data->kernel[0],
                           sizeof(cl_mem),     &in,
                           sizeof(cl_mem),     &out,
                           sizeof(cl_mem),     &cl_random_data,
                           sizeof(cl_ushort4), &rand,
                           sizeof(cl_int),     &x_offset,
                           sizeof(cl_int),     &y_offset,
                           sizeof(cl_int),     &roi_width,
                           sizeof(cl_int),     &wr_width,
                           sizeof(cl_int),     &holdness,
                           sizeof(cl_float),   &hue_distance,
                           sizeof(cl_float),   &saturation_distance,
                           sizeof(cl_float),   &value_distance,
                           NULL);

  CL_CHECK;
  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 1,
                                        NULL, &global_worksize, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
  CL_CHECK;

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

  operation_class->prepare = prepare;
  operation_class->opencl_support = TRUE;
  point_filter_class->process = process;
  point_filter_class->cl_process = cl_process;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:noise-hsv",
    "title",      _("Add HSV Noise"),
    "categories", "noise",
    "reference-hash", "5754cd0bcadabd01bbc253b5c41cbd74",
    "description", _("Randomize hue, saturation and value independently"),
      NULL);
}

#endif
