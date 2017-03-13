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

/* This plug-in generates cellular noise textures based on the
 * function described in the paper
 *
 *    Steven Worley. 1996. A cellular texture basis function.
 *    In Proceedings of the 23rd annual conference on Computer
 *    graphics and interactive techniques (SIGGRAPH '96).
 *
 * Comments and improvements for this code are welcome.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#define MAX_RANK 3

#ifdef GEGL_PROPERTIES

property_double  (scale, _("Scale"), 1.0)
    description  (_("The scale of the noise function"))
    value_range  (0, 20.0)

property_double  (shape, _("Shape"), 2.0)
    description  (_("Interpolate between Manhattan and Euclidean distance."))
    value_range  (1.0, 2.0)

property_int     (rank, _("Rank"), 1)
    description  (_("Select the n-th closest point"))
    value_range  (1, MAX_RANK)

property_int     (iterations, _("Iterations"), 1)
    description  (_("The number of noise octaves."))
    value_range  (1, 20)

property_boolean (palettize, _("Palettize"), FALSE)
    description  (_("Fill each cell with a random color"))

property_seed    (seed, _("Random seed"), rand)
    description  (_("The random seed for the noise function"))

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     noise_cell
#define GEGL_OP_C_SOURCE noise-cell.c

#include "gegl-op.h"
#include <gegl-buffer-cl-iterator.h>
#include <gegl-debug.h>
#include <math.h>

#include "opencl/noise-cell.cl.h"

/* Random feature counts following the Poisson distribution with
   lambda equal to 7. */

static const gint poisson[256] = {
  7, 9, 12, 12, 8, 7, 5, 5, 6, 7, 8, 6, 10, 7, 6, 2, 8, 3, 9, 5, 13, 10, 9,
  8, 8, 9, 3, 8, 9, 6, 8, 7, 4, 9, 6, 3, 10, 7, 7, 7, 6, 7, 4, 14, 7, 6, 11,
  7, 7, 7, 12, 7, 10, 6, 8, 11, 3, 5, 7, 7, 8, 7, 9, 8, 5, 8, 11, 3, 4, 5, 8,
  8, 7, 8, 9, 2, 7, 8, 12, 4, 8, 2, 11, 8, 14, 7, 8, 2, 3, 10, 4, 6, 9, 5, 8,
  7, 10, 10, 10, 14, 5, 7, 6, 4, 5, 6, 11, 8, 7, 3, 11, 5, 5, 2, 9, 7, 7, 7,
  9, 2, 7, 6, 9, 7, 6, 5, 12, 5, 3, 11, 9, 12, 8, 6, 8, 6, 8, 5, 5, 7, 5, 2,
  9, 5, 5, 8, 11, 8, 8, 10, 6, 4, 7, 14, 7, 3, 10, 7, 7, 4, 9, 10, 10, 9, 8,
  8, 7, 6, 5, 10, 10, 5, 10, 7, 7, 10, 7, 4, 9, 9, 6, 8, 5, 10, 7, 3, 9, 9,
  7, 8, 9, 7, 5, 7, 6, 5, 5, 12, 4, 7, 5, 5, 4, 5, 7, 10, 8, 7, 9, 4, 6, 11,
  6, 3, 7, 8, 9, 5, 8, 6, 7, 8, 7, 7, 3, 7, 7, 9, 4, 5, 5, 6, 9, 7, 6, 12, 4,
  9, 10, 8, 8, 6, 4, 9, 9, 8, 11, 6, 8, 13, 8, 9, 12, 6, 9, 8
};

typedef struct
{
  gdouble  shape;
  gdouble  closest[MAX_RANK];
  guint    feature, rank, seed;
  gboolean palettize;
} Context;

static GeglClRunData *cl_data = NULL;

static inline guint
philox (guint s,
        guint t,
        guint k)
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

  return s;
}

static inline gdouble
lcg (guint *hash)
{
  return (*hash = *hash * (guint)1664525 + (guint)1013904223) / (gdouble)4294967296.0;
}

static void
search_box (gint     s,
            gint     t,
            gdouble  x,
            gdouble  y,
            Context *context)
{
  guint hash;
  gint i, n;

  hash = philox ((guint)s, (guint)t, context->seed);
  n = poisson[hash >> 24];

  for (i = 0 ; i < n ; i += 1)
    {
      gdouble delta_x, delta_y, d;
      gint    j, k;

      /* Calculate the distance to each feature point. */

      delta_x = s + lcg (&hash) - x;
      delta_y = t + lcg (&hash) - y;

      if (context->shape == 2)
        d = delta_x * delta_x + delta_y * delta_y;
      else if (context->shape == 1)
        d = fabs (delta_x) + fabs (delta_y);
      else
        d = pow (fabs (delta_x), context->shape) +
          pow (fabs (delta_y), context->shape);

      /* Insert it into the list of n closest distances if needed. */

      for (j = 0 ; j < context->rank && d > context->closest[j] ; j += 1);

      if (j < context->rank)
        {
          for (k = context->rank - 1 ; k > j ; k -= 1)
            {
              context->closest[k] = context->closest[k - 1];
            }

          context->closest[j] = d;

          if (j == context->rank - 1)
            context->feature = hash;
        }
    }
}

static double
noise2 (double x,
        double y,
        Context *context)
{
  gdouble d_l, d_r, d_t, d_b, *d_0;
  gint s, t, i;

  for (i = 0 ; i < context->rank ; context->closest[i] = INFINITY, i += 1);

  s = (gint)floor(x);
  t = (gint)floor(y);

  /* Search the box the point is in. */

  search_box (s, t, x, y, context);

  d_0 = &context->closest[context->rank - 1];
  d_l = x - s; d_l *= d_l;
  d_r = 1.0 - x + s; d_r *= d_r;
  d_b = y - t; d_b *= d_b;
  d_t = 1.0 - y + t; d_t *= d_t;

  /* Search adjacent boxes if it is possible for them to contain a
   * nearby feature point. */

  if (d_l < *d_0)
    {
      if (d_l + d_b < *d_0)
        search_box (s - 1, t - 1, x, y, context);

      search_box (s - 1, t, x, y, context);

      if (d_l + d_t < *d_0)
        search_box (s - 1, t + 1, x, y, context);
    }

  if (d_b < *d_0)
    search_box (s, t - 1, x, y, context);

  if (d_t < *d_0)
    search_box (s, t + 1, x, y, context);

  if (d_r < *d_0)
    {
      if (d_r + d_b < *d_0)
        search_box (s + 1, t - 1, x, y, context);

      search_box (s + 1, t, x, y, context);

      if (d_r + d_t < *d_0)
        search_box (s + 1, t + 1, x, y, context);
    }

  /* If palettized output is requested return the normalized hash of
   * the closest feature point, otherwise return the closest
   * distance. */

  if (context->palettize)
    return (gdouble)context->feature / 4294967295.0;
  else
    return pow (context->closest[context->rank - 1], 1 / context->shape);
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
  GeglProperties *o        = GEGL_PROPERTIES (operation);
  const size_t  gbl_size[] = {roi->width, roi->height};
  size_t        work_group_size;
  cl_uint       cl_iterations   = o->iterations;
  cl_int        cl_err          = 0;
  cl_int        cl_x_0          = roi->x;
  cl_int        cl_y_0          = roi->y;
  cl_float      cl_scale        = o->scale / 50.0;
  cl_float      cl_shape        = o->shape;
  cl_uint       cl_rank         = o->rank;
  cl_uint       cl_seed         = o->seed;
  cl_int        cl_palettize    = (cl_int)o->palettize;

  if (!cl_data)
  {
    const char *kernel_name[] = {"kernel_noise", NULL};
    cl_data = gegl_cl_compile_and_build (noise_cell_cl_source, kernel_name);

    if (!cl_data)
      return TRUE;
  }

  cl_err = gegl_cl_set_kernel_args (cl_data->kernel[0],
                                    sizeof(cl_mem), &out_tex,
                                    sizeof(cl_int), &cl_x_0,
                                    sizeof(cl_int), &cl_y_0,
                                    sizeof(cl_uint), &cl_iterations,
                                    sizeof(cl_float), &cl_scale,
                                    sizeof(cl_float), &cl_shape,
                                    sizeof(cl_uint), &cl_rank,
                                    sizeof(cl_uint), &cl_seed,
                                    sizeof(cl_int), &cl_palettize,
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
  gint factor = (1 << level);
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Context     context;
  gfloat     *pixel = out_buf;

  gint x = roi->x;
  gint y = roi->y;

  context.seed = o->seed;
  context.rank = o->rank;
  context.shape = o->shape;
  context.palettize = o->palettize;
    
  while (n_pixels --)
  {
    gint    i;
    gdouble c, d;

    /* Pile up noise octaves onto the output value. */

    for (i = 0, c = 1, d = o->scale / 50.0, *pixel = 0;
         i < o->iterations;
         c *= 2, d *= 2, i += 1) {
      *pixel += noise2 ((double) (x * d * factor),
                        (double) (y * d * factor),
                        &context) / c;
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
    "name",               "gegl:cell-noise",
    "title",              _("Cell Noise"),
    "categories",         "render",
    "position-dependent", "true",

    "description",        _("Generates a cellular texture."),
    "reference-hash",     "24a97f60de37791a65ef8931c3ca3342",

    "reference",  "Steven Worley. 1996. A cellular texture basis function. In Proceedings of the 23rd annual conference on Computer graphics and interactive techniques (SIGGRAPH '96).",

    NULL);
}

#endif
