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
 * Copyright 2006 Mark Probst <mark.probst@gmail.com>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double_ui (red,   _("Red"),   -10.0, 10.0, 0.5, -1.0, 1.0, 1.0,
                      _("Amount of red"))
gegl_chant_double_ui (green, _("Green"), -10.0, 10.0, 0.25, -1.0, 1.0, 1.0,
                      _("Amount of green"))
gegl_chant_double_ui (blue,  _("Blue"),  -10.0, 10.0, 0.25, -1.0, 1.0, 1.0,
                      _("Amount of blue"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE      "mono-mixer.c"

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  /* set the babl format this operation prefers to work on */
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "output", babl_format ("YA float"));
}

#include "opencl/gegl-cl.h"
#include "buffer/gegl-buffer-cl-iterator.h"

static const char* kernel_source =
"__kernel void Mono_mixer_cl(__global const float4 *src_buf,              \n"
"                            float4                 color,                \n"
"                            __global float2       *dst_buf)              \n"
"{                                                                        \n"
"  int gid = get_global_id(0);                                            \n"
"  float4 tmp = src_buf[gid] * color;                                     \n"
"  dst_buf[gid].x = tmp.x + tmp.y + tmp.z;                                \n"
"  dst_buf[gid].y = tmp.w;                                                \n"
"}                                                                        \n";

static gegl_cl_run_data * cl_data = NULL;

static cl_int
cl_mono_mixer(cl_mem                in_tex,
              cl_mem                out_tex,
              size_t                global_worksize,
              const GeglRectangle  *roi,
              gfloat                red,
              gfloat                green,
              gfloat                blue)
{
  cl_int cl_err = 0;
  if (!cl_data)
  {
    const char *kernel_name[] = {"Mono_mixer_cl", NULL};
    cl_data = gegl_cl_compile_and_build(kernel_source, kernel_name);
  }
  if (!cl_data) return 0;

  {
  cl_float4 color;
  color.s[0] = red;
  color.s[1] = green;
  color.s[2] = blue;
  color.s[3] = 1.0f;

  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),    (void*)&in_tex);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_float4), (void*)&color);
  cl_err |= gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_mem),    (void*)&out_tex);
  if (cl_err != CL_SUCCESS) return cl_err;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue(), cl_data->kernel[0],
                                       1, NULL,
                                       &global_worksize, NULL,
                                       0, NULL, NULL);
  }

  return cl_err;
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
  gint j;
  cl_int cl_err;

  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,   result, out_format, GEGL_CL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  gint read = gegl_buffer_cl_iterator_add (i, input, result, in_format,  GEGL_CL_BUFFER_READ, GEGL_ABYSS_NONE);
  while (gegl_buffer_cl_iterator_next (i, &err))
  {
    if (err) return FALSE;
    for (j=0; j < i->n; j++)
    {
      cl_err = cl_mono_mixer(i->tex[read][j], i->tex[0][j], i->size[0][j], &i->roi[0][j], o->red ,o->green , o->blue);

      if (cl_err != CL_SUCCESS)
      {
        g_warning("[OpenCL] Error in gegl:mono-mixer: %s", gegl_cl_errstring(cl_err));
        return FALSE;
      }
    }
  }
  return TRUE;
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat      red   = o->red;
  gfloat      green = o->green;
  gfloat      blue  = o->blue;
  gfloat     *in_buf;
  gfloat     *out_buf;

  if (gegl_cl_is_accelerated ())
    if (cl_process (operation, input, output, result))
      return TRUE;

 if ((result->width > 0) && (result->height > 0))
 {
     gint num_pixels = result->width * result->height;
     gint i;
     gfloat *in_pixel, *out_pixel;

     in_buf = g_new (gfloat, 4 * num_pixels);
     out_buf = g_new (gfloat, 2 * num_pixels);

     gegl_buffer_get (input, result, 1.0, babl_format ("RGBA float"), in_buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

     in_pixel = in_buf;
     out_pixel = out_buf;
     for (i = 0; i < num_pixels; ++i)
     {
         out_pixel[0] = in_pixel[0] * red + in_pixel[1] * green + in_pixel[2] * blue;
         out_pixel[1] = in_pixel[3];

         in_pixel += 4;
         out_pixel += 2;
     }

     gegl_buffer_set (output, result, 0, babl_format ("YA float"), out_buf,
                      GEGL_AUTO_ROWSTRIDE);

     g_free (in_buf);
     g_free (out_buf);
 }

 return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:mono-mixer",
    "categories" , "color",
    "description", _("Monochrome channel mixer"),
    NULL);
}

#endif
