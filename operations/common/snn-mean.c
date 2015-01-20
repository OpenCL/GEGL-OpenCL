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
 * Copyright 2005 Øyvind Kolås <pippin@gimp.org>,
 *           2007 Øyvind Kolås <oeyvindk@hig.no>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_int (radius, _("Radius"), 8)
    description(_("Radius of square pixel region, (width and height will be radius*2+1)"))
    value_range (0, 100)
    ui_range    (0, 40)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")

property_int (pairs, _("Pairs"), 2)
  description(_("Number of pairs; higher number preserves more acute features"))
  value_range (1, 2)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_C_SOURCE snn-mean.c

#include "gegl-op.h"
#include <math.h>

static void
snn_mean (GeglBuffer          *src,
          GeglBuffer          *dst,
          const GeglRectangle *dst_rect,
          gdouble              radius,
          gint                 pairs);


static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o    = GEGL_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = ceil (o->radius);
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result);

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties      *o = GEGL_PROPERTIES (operation);
  GeglBuffer          *temp_in;
  GeglRectangle        compute;

  if (gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, result))
      return TRUE;

  compute = gegl_operation_get_required_for_output (operation, "input", result);

  if (o->radius < 1.0)
    {
      output = g_object_ref (input);
    }
  else
    {
      temp_in = gegl_buffer_create_sub_buffer (input, &compute);

      snn_mean (temp_in, output, result, o->radius, o->pairs);
      g_object_unref (temp_in);
    }

  return  TRUE;
}

#define POW2(a)((a)*(a))

static inline gfloat colordiff (gfloat *pixA,
                                gfloat *pixB)
{
  return POW2(pixA[0]-pixB[0])+
         POW2(pixA[1]-pixB[1])+
         POW2(pixA[2]-pixB[2]);
}


static void
snn_mean (GeglBuffer          *src,
          GeglBuffer          *dst,
          const GeglRectangle *dst_rect,
          gdouble              dradius,
          gint                 pairs)
{
  gint x,y;
  gint offset;
  gfloat *src_buf;
  gfloat *dst_buf;
  gint radius = dradius;
  gint src_width = gegl_buffer_get_width (src);
  gint src_height = gegl_buffer_get_height (src);

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_new0 (gfloat, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, NULL, 1.0, babl_format ("RGBA float"), src_buf, 
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  offset = 0;

  for (y=0; y<dst_rect->height; y++)
    {
      gfloat *center_pix;

      center_pix = src_buf + ((radius) + (y+radius)* src_width)*4;

      for (x=0; x<dst_rect->width; x++)
        {
          gint u,v;

          gfloat  accumulated[4]={0.0f, 0.0f, 0.0f, 0.0f};
          gint    count=0;

          /* iterate through the upper left quater of pixels */
          for (v=-radius;v<=0;v++)
            for (u=-radius;u<= (pairs==1?radius:0);u++)
              {
                gfloat *selected_pix = center_pix;
                gfloat  best_diff = 1000.0;
                gint    i;

                /* skip computations for the center pixel */
                if (u != 0 &&
                    v != 0)
                  {
                    /* compute the coordinates of the symmetric pairs for
                     * this locaiton in the quadrant
                     */
                    gint xs[4], ys[4];

                    xs[0] = x+u+radius;
                    xs[1] = x-u+radius;
                    xs[2] = x-u+radius;
                    xs[3] = x+u+radius;
                    ys[0] = y+v+radius;
                    ys[1] = y-v+radius;
                    ys[2] = y+v+radius;
                    ys[3] = y-v+radius;

                    /* check which member of the symmetric quadruple to use */
                    for (i=0;i<pairs*2;i++)
                      {
                        if (xs[i] >= 0 && xs[i] < src_width &&
                            ys[i] >= 0 && ys[i] < src_height)
                          {
                            gfloat *tpix = src_buf + (xs[i]+ys[i]* src_width)*4;
                            gfloat diff = colordiff (tpix, center_pix);
                            if (diff < best_diff)
                              {
                                best_diff = diff;
                                selected_pix = tpix;
                              }
                          }
                      }
                  }

                /* accumulate the components of the best sample from
                 * the symmetric quadruple
                 */
                for (i=0;i<4;i++)
                  {
                    accumulated[i] += selected_pix[i];
                  }
                count++;

                if (u==0 && v==0)
                  break; /* to avoid doubly processing when using only 1 pair */
              }
          for (u=0; u<4; u++)
            dst_buf[offset*4+u] = accumulated[u]/count;
          offset++;

          center_pix += 4;
        }
    }
  gegl_buffer_set (dst, dst_rect, 0, babl_format ("RGBA float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}


#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

#include "opencl/snn-mean.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_snn_mean (cl_mem                in_tex,
             cl_mem                out_tex,
             const GeglRectangle  *src_rect,
             const GeglRectangle  *roi,
             gint                  radius,
             gint                  pairs)
{
  cl_int cl_err = 0;
  size_t global_ws[2];

  if (!cl_data)
    {
      const char *kernel_name[] = {"snn_mean", NULL};
      cl_data = gegl_cl_compile_and_build (snn_mean_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;


  global_ws[0] = roi->width;
  global_ws[1] = roi->height;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_int),   (void*)&src_rect->width);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int),   (void*)&src_rect->height);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_int),   (void*)&radius);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_int),   (void*)&pairs);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        NULL, global_ws, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint err;

  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties *o = GEGL_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,
                                                         result,
                                                         out_format,
                                                         GEGL_CL_BUFFER_WRITE);

  gint read = gegl_buffer_cl_iterator_add_2 (i,
                                             input,
                                             result,
                                             in_format,
                                             GEGL_CL_BUFFER_READ,
                                             op_area->left,
                                             op_area->right,
                                             op_area->top,
                                             op_area->bottom,
                                             GEGL_ABYSS_NONE);

  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err) return FALSE;

      err = cl_snn_mean(i->tex[read],
                        i->tex[0],
                        &i->roi[read],
                        &i->roi[0],
                        ceil(o->radius),
                        o->pairs);

      if (err) return FALSE;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:snn-mean",
    "categories" , "enhance:noise-reduction",
    "title",       _("Symmetric Nearest Neighbour"),
    "description",
        _("Noise reducing edge preserving blur filter based "
          " on Symmetric Nearest Neighbours"),
        NULL);
}

#endif
