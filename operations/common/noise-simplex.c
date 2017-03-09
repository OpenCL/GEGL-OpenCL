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
 * Copyright 2014 Dimitris Papavasiliou <dpapavas@google.com>
 */

/* This plug-in generates noise textures originally presented by Ken
 * Perlin during a SIGGRAPH 2002 course and further explained in the
 * paper "Simplex noise demystified" by Stefan Gustavson.
 *
 * Comments and improvements for this code are welcome.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (scale, _("Scale"), 1.0)
    description (_("The scale of the noise function"))
    value_range (0.0, 20.0)

property_int    (iterations, _("Iterations"), 1)
    description (_("The number of noise octaves."))
    value_range (1, 20)

property_seed   (seed, _("Random seed"), rand)
    description (_("The random seed for the noise function"))

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     noise_simplex
#define GEGL_OP_C_SOURCE noise-simplex.c

#include "gegl-op.h"
#include <gegl-buffer-cl-iterator.h>
#include <gegl-debug.h>
#include <math.h>

#include "opencl/noise-simplex.cl.h"

typedef struct
{
  guint seed;
} Context;

static GeglClRunData *cl_data = NULL;

static inline gdouble
dot_2(gdouble *p,
      gdouble *q)
{
  return p[0] * q[0] + p[1] * q[1];
}

static inline void
philox (guint    s,
        guint    t,
        guint    k,
        gdouble *h)
{
  guint64 p;
  gint    i;

  /* For details regarding this hash function consult:

     Salmon, J.K.; Moraes, M.A.; Dror, R.O.; Shaw, D.E., "Parallel
     random numbers: As easy as 1, 2, 3," High Performance Computing,
     Networking, Storage and Analysis (SC), 2011 International
     Conference for , vol., no., pp.1,12, 12-18 Nov. 2011 */\

  for (i = 0 ; i < 3 ; i += 1)
    {
      p = s * (guint64)0xcd9e8d57;

      s = ((guint)(p >> 32)) ^ t ^ k;
      t = (guint)p;

      k += 0x9e3779b9;
    }

  h[0] = s / 2147483648.0 - 1.0;
  h[1] = t / 2147483648.0 - 1.0;
}

static gdouble
noise2 (gdouble  x,
        gdouble  y,
        Context *context)
{
  gdouble s, t, n;
  gdouble g[3][2], u[3][2];
  gint i, j, di, k;

  /* Skew the input point and find the lowest corner of the containing
     simplex. */

  s = (x + y) * (sqrt(3) - 1) / 2;
  i = (gint)floor(x + s);
  j = (gint)floor(y + s);

  /* Calculate the (unskewed) distance between the input point and all
     simplex corners. */

  s = (i + j) * (3 - sqrt(3)) / 6;
  u[0][0] = x - i + s;
  u[0][1] = y - j + s;

  di = (u[0][0] >= u[0][1]);

  u[1][0] = u[0][0] - di + (3 - sqrt(3)) / 6;
  u[1][1] = u[0][1] - !di + (3 - sqrt(3)) / 6;

  u[2][0] = u[0][0] - 1 + (3 - sqrt(3)) / 3;
  u[2][1] = u[0][1] - 1 + (3 - sqrt(3)) / 3;

  /* Calculate gradients for each corner vertex. */

  philox(i, j, context->seed, g[0]);
  philox(i + di, j + !di, context->seed, g[1]);
  philox(i + 1, j + 1, context->seed, g[2]);

  /* Finally accumulate the contribution of each vertex to the current
   * pixel. */

  for (k = 0, n = 0 ; k < 3 ; k += 1)
    {
      t = 0.5 - dot_2(u[k], u[k]);

      if (t > 0)
        {
          t *= t;
          n += t * t * dot_2(g[k], u[k]);
        }
    }

  /* Normalize result to lie in [-1, 1]. */

  return 70 * n;
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("Y float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
cl_process (GeglOperation       *operation,
            cl_mem               out_tex,
            const GeglRectangle *roi)
{
  GeglProperties   *o      = GEGL_PROPERTIES (operation);
  const size_t  gbl_size[] = {roi->width, roi->height};
  size_t        work_group_size;
  cl_uint       cl_iterations   = o->iterations;
  cl_int        cl_err          = 0;
  cl_int        cl_x_0          = roi->x;
  cl_int        cl_y_0          = roi->y;
  cl_float      cl_scale        = o->scale / 50.0;
  cl_uint       cl_seed         = o->seed;

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_noise", NULL};
      cl_data = gegl_cl_compile_and_build (noise_simplex_cl_source,
                                           kernel_name);

      if (!cl_data)
        return TRUE;
    }

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof(cl_mem), &out_tex,
                                    sizeof(cl_int), &cl_x_0,
                                    sizeof(cl_int), &cl_y_0,
                                    sizeof(cl_uint), &cl_iterations,
                                    sizeof(cl_float), &cl_scale,
                                    sizeof(cl_uint), &cl_seed,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clGetKernelWorkGroupInfo (cl_data->kernel[0],
                                          gegl_cl_get_device (),
                                          CL_KERNEL_WORK_GROUP_SIZE,
                                          sizeof (size_t), &work_group_size,
                                          NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, gbl_size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  cl_err = gegl_clFinish (gegl_cl_get_command_queue ());
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
c_process (GeglOperation       *operation,
           void                *out_buf,
           glong                n_pixels,
           const GeglRectangle *roi,
           gint                 level)
{
  gint factor = 1 << level;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Context     context;
  gfloat     *pixel = out_buf;

  gint x = roi->x;
  gint y = roi->y;

  context.seed = o->seed;
    
  while (n_pixels --)
  {
    gint    i;
    gdouble c, d;

    /* Pile up noise octaves onto the output value. */

    for (i = 0, c = 1, d = o->scale / 50.0, *pixel = 0;
         i < o->iterations;
         c *= 2, d *= 2, i += 1)
      {
        *pixel += noise2 ((double) (x) * d * factor, (double) (y) * d * factor, &context) / c;
      }

    pixel += 1;

    x++;
    if (x>=roi->x + roi->width)
      {
        x=roi->x;
        y++;
      }
  }

  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *out_buf,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglBufferIterator *iter;
  const Babl         *out_format = gegl_operation_get_format (operation,
                                                              "output");

  g_assert(babl_format_get_n_components (out_format) == 1 &&
           babl_format_get_type (out_format, 0) == babl_type ("float"));

  if (gegl_operation_use_opencl (operation))
    {
      GeglBufferClIterator *cl_iter;
      gboolean              err;

      GEGL_NOTE (GEGL_DEBUG_OPENCL, "GEGL_OPERATION_POINT_RENDER: %s", GEGL_OPERATION_GET_CLASS (operation)->name);

      cl_iter = gegl_buffer_cl_iterator_new (out_buf, roi, out_format, GEGL_CL_BUFFER_WRITE);

      while (gegl_buffer_cl_iterator_next (cl_iter, &err) && !err)
        {
          err = cl_process (operation, cl_iter->tex[0], cl_iter->roi);

          if (err)
            {
              gegl_buffer_cl_iterator_stop (cl_iter);
              break;
            }
        }

      if (err)
        GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error: %s", GEGL_OPERATION_GET_CLASS (operation)->name);
      else
        return TRUE;
    }

  iter = gegl_buffer_iterator_new (out_buf, roi, level, out_format,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    c_process (operation, iter->data[0], iter->length, &iter->roi[0], level);

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:simplex-noise",
    "title",              _("Simplex Noise"),
    "categories",         "render",
    "reference-hash",     "d6c535d254ebf7cb3213fdb26527f16b",
    "position-dependent", "true",
    "description", _("Generates a solid noise texture."),
    NULL);
}

#endif
