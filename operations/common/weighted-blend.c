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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 *           2008 James Legg
 */
#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_C_SOURCE weighted-blend.c
#define GEGL_OP_NAME     weighted_blend

#include "gegl-op.h"

static void prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", format);
  gegl_operation_set_format (operation, "output", format);
}

#include "opencl/gegl-cl.h"
#include "opencl/weighted-blend.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *self,
            cl_mem               in_tex,
            cl_mem               aux_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  gint cl_err = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"cl_copy_weigthed_blend",
                                   "cl_weighted_blend",
                                   NULL};
      cl_data = gegl_cl_compile_and_build (weighted_blend_cl_source,
                                           kernel_name);
    }
  if (!cl_data) return TRUE;


  if (!aux_tex)
    {
      cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem), (void*)&in_tex);
      CL_CHECK;
      cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem), (void*)&out_tex);
      CL_CHECK;

      cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                           cl_data->kernel[0], 1,
                                           NULL, &global_worksize, NULL,
                                           0, NULL, NULL);
      CL_CHECK;
    }
  else
    {
      cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 0, sizeof(cl_mem), (void*)&in_tex);
      CL_CHECK;
      cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 1, sizeof(cl_mem), (void*)&aux_tex);
      CL_CHECK;
      cl_err = gegl_clSetKernelArg(cl_data->kernel[1], 2, sizeof(cl_mem), (void*)&out_tex);
      CL_CHECK;

      cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                           cl_data->kernel[1], 1,
                                           NULL, &global_worksize, NULL,
                                           0, NULL, NULL);
      CL_CHECK;
    }

  return FALSE;

error:
  return TRUE;
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat *in  = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  gint    i;

  if (aux == NULL)
    {
      /* there is no auxilary buffer.
       * output the input buffer.
       */
      for (i = 0; i < n_pixels; i++)
        {
          gint j;
          for (j = 0; j < 4; j++)
            {
              out[j] = in[j];
            }
          in  += 4;
          out += 4;
        }
    }
  else
    {
      for (i=0; i<n_pixels; i++)
        {
          gint   j;
          gfloat total_alpha;
          /* find the proportion between alpha values */
          total_alpha = in[3] + aux[3];
          if (!total_alpha)
            {
              /* no coverage from any source pixel */
              for (j = 0; j < 4; j++)
                {
                  out[j] = 0.0;
                }
            }
          else
            {
              /* the total alpha is non-zero, therefore we may find a colour from a weighted blend */
              gfloat in_weight = in[3] / total_alpha;
              gfloat aux_weight = 1.0 - in_weight;
              for (j = 0; j < 3; j++)
                {
                  out[j] = in_weight * in[j] + aux_weight * aux[j];
                }
              out[3] = total_alpha;
            }
          in  += 4;
          aux += 4;
          out += 4;
        }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  point_composer_class->process    = process;
  point_composer_class->cl_process = cl_process;
  operation_class->prepare         = prepare;
  operation_class->opencl_support  = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name" ,       "gegl:weighted-blend",
    "title",       _("Weighted Blend"),
    "categories" , "compositors:blend",
    "reference-hash", "577a40b1a4c8fbe3e48407bc0c51304d",
    "description",
      _("blend two images using alpha values as weights"),
    NULL);
}
#endif
