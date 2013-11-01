/* This file is part of GEGL
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
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 */


/* Here we have implemented in OpenCL a subset of color space conversions provided by BABL
   that are commonly used. For now there is no support for conversions that need a intermediate
   representation (ex: A->B, B->C, C->D), just among two color spaces.
*/

#include "config.h"

#include "gegl.h"
#include "gegl/gegl-debug.h"
#include "gegl-cl.h"
#include "gegl-cl-color.h"

#include "opencl/colors.cl.h"

#define CL_FORMAT_N 11

static const Babl *format[CL_FORMAT_N];

static GHashTable  *color_kernels_hash = NULL;

typedef struct
{
  const Babl *from_fmt;
  const Babl *to_fmt;
  const char *kernel_name;
  cl_kernel   kernel;
} ColorConversionInfo;

static guint
color_kernels_hash_hashfunc (gconstpointer key)
{
  const ColorConversionInfo *ekey = key;
  return GPOINTER_TO_INT (ekey->from_fmt) ^ GPOINTER_TO_INT (ekey->to_fmt);
}

static gboolean
color_kernels_hash_equalfunc (gconstpointer a,
                              gconstpointer b)
{
  const ColorConversionInfo *ea = a;
  const ColorConversionInfo *eb = b;

  if (ea->from_fmt == eb->from_fmt &&
      ea->to_fmt   == eb->to_fmt)
    {
      return TRUE;
    }
  return FALSE;
}

void
gegl_cl_color_compile_kernels (void)
{
  static GeglClRunData *float_kernels = NULL;

  ColorConversionInfo float_conversions[] = {
    { babl_format ("RGBA u8"), babl_format ("RGBA float"), "rgbau8_to_rgbaf", NULL },
    { babl_format ("RGBA float"), babl_format ("RGBA u8"), "rgbaf_to_rgbau8", NULL },

    { babl_format ("RGBA float"), babl_format("RaGaBaA float"), "rgbaf_to_ragabaf", NULL },
    { babl_format ("RaGaBaA float"), babl_format("RGBA float"), "ragabaf_to_rgbaf", NULL },
    { babl_format ("RGBA u8"), babl_format("RaGaBaA float"), "rgbau8_to_ragabaf", NULL },
    { babl_format ("RaGaBaA float"), babl_format("RGBA u8"), "ragabaf_to_rgbau8", NULL },

    { babl_format ("RGBA float"), babl_format("R'G'B'A float"), "rgbaf_to_rgba_gamma_f", NULL },
    { babl_format ("R'G'B'A float"), babl_format("RGBA float"), "rgba_gamma_f_to_rgbaf", NULL },
    { babl_format ("RGBA u8"), babl_format("R'G'B'A float"), "rgbau8_to_rgba_gamma_f", NULL },
    { babl_format ("R'G'B'A float"), babl_format("RGBA u8"), "rgba_gamma_f_to_rgbau8", NULL },

    { babl_format ("RGBA float"), babl_format("Y'CbCrA float"), "rgbaf_to_ycbcraf", NULL },
    { babl_format ("Y'CbCrA float"), babl_format("RGBA float"), "ycbcraf_to_rgbaf", NULL },
    { babl_format ("RGBA u8"), babl_format("Y'CbCrA float"), "rgbau8_to_ycbcraf", NULL },
    { babl_format ("Y'CbCrA float"), babl_format("RGBA u8"), "ycbcraf_to_rgbau8", NULL },

    { babl_format ("RGB u8"), babl_format("RGBA float"), "rgbu8_to_rgbaf", NULL },
    { babl_format ("RGBA float"), babl_format("RGB u8"), "rgbaf_to_rgbu8", NULL },

    { babl_format ("Y u8"), babl_format("Y float"), "yu8_to_yf", NULL },

    { babl_format ("RGBA float"), babl_format("YA float"), "rgbaf_to_yaf", NULL },
    { babl_format ("YA float"), babl_format("RGBA float"), "yaf_to_rgbaf", NULL },
    { babl_format ("RGBA u8"), babl_format("YA float"), "rgbau8_to_yaf", NULL },
    { babl_format ("YA float"), babl_format("RGBA u8"), "yaf_to_rgbau8", NULL },

    { babl_format ("RGBA float"), babl_format("R'G'B'A u8"), "rgbaf_to_rgba_gamma_u8", NULL },
    { babl_format ("R'G'B'A u8"), babl_format("RGBA float"), "rgba_gamma_u8_to_rgbaf", NULL },

    { babl_format ("RGBA float"), babl_format("R'G'B' u8"), "rgbaf_to_rgb_gamma_u8", NULL },
    { babl_format ("R'G'B' u8"), babl_format("RGBA float"), "rgb_gamma_u8_to_rgbaf", NULL },

    { babl_format ("R'G'B'A u8"), babl_format("RaGaBaA float"), "rgba_gamma_u8_to_ragabaf", NULL },
    { babl_format ("RaGaBaA float"), babl_format("R'G'B'A u8"), "ragabaf_to_rgba_gamma_u8", NULL },
    { babl_format ("R'G'B' u8"), babl_format("RaGaBaA float"), "rgb_gamma_u8_to_ragabaf", NULL },
    { babl_format ("RaGaBaA float"), babl_format("R'G'B' u8"), "ragabaf_to_rgb_gamma_u8", NULL },

    { babl_format ("R'G'B'A u8"), babl_format("YA float"), "rgba_gamma_u8_to_yaf", NULL },
    { babl_format ("YA float"), babl_format("R'G'B'A u8"), "yaf_to_rgba_gamma_u8", NULL },
    { babl_format ("R'G'B' u8"), babl_format("YA float"), "rgb_gamma_u8_to_yaf", NULL },
    { babl_format ("YA float"), babl_format("R'G'B' u8"), "yaf_to_rgb_gamma_u8", NULL }
  };

  const char *kernel_name[G_N_ELEMENTS (float_conversions) + 1];

  for (int i = 0; i < G_N_ELEMENTS (float_conversions); ++i)
    {
      kernel_name[i] = float_conversions[i].kernel_name;
    }

  kernel_name[G_N_ELEMENTS (float_conversions)] = NULL;

  float_kernels = gegl_cl_compile_and_build (colors_cl_source, kernel_name);

  color_kernels_hash = g_hash_table_new_full (color_kernels_hash_hashfunc,
                                              color_kernels_hash_equalfunc,
                                              NULL,
                                              NULL);

  for (int i = 0; i < G_N_ELEMENTS (float_conversions); ++i)
    {
      ColorConversionInfo *info = g_new (ColorConversionInfo, 1);

      float_conversions[i].kernel = float_kernels->kernel[i];
      *info = float_conversions[i];

      g_hash_table_insert (color_kernels_hash, info, info);
    }

  /* FIXME: This concept of supported formats it not useful */
  format[0] = babl_format ("RGBA u8");
  format[1] = babl_format ("RGBA float");
  format[2] = babl_format ("RaGaBaA float");
  format[3] = babl_format ("R'G'B'A float");
  format[4] = babl_format ("Y'CbCrA float");
  format[5] = babl_format ("RGB u8");
  format[6] = babl_format ("Y u8");
  format[7] = babl_format ("Y float");
  format[8] = babl_format ("YA float");
  format[9] = babl_format ("R'G'B'A u8");
  format[10] = babl_format ("R'G'B' u8");
}

static cl_kernel
find_color_kernel (const Babl *in_format,
                   const Babl *out_format)
{
  ColorConversionInfo *info;
  ColorConversionInfo search = { in_format, out_format, NULL, NULL };
  info = g_hash_table_lookup (color_kernels_hash, &search);
  if (info)
    return info->kernel;
  return NULL;
}

gboolean
gegl_cl_color_babl (const Babl *buffer_format,
                    size_t     *bytes)
{
  int i;
  gboolean supported_format = FALSE;

  if (bytes)
    *bytes = SIZE_MAX;

  for (i = 0; i < CL_FORMAT_N; i++)
    if (format[i] == buffer_format) supported_format = TRUE;

  if (!supported_format)
    return FALSE;

  if (bytes)
    {
      if (buffer_format == babl_format ("RGBA u8"))
        *bytes = sizeof (cl_uchar4);
      else if (buffer_format == babl_format ("RGB u8"))
        *bytes = sizeof (cl_uchar3);
      else if (buffer_format == babl_format ("Y u8"))
        *bytes = sizeof (cl_uchar);
      else if (buffer_format == babl_format ("Y float"))
        *bytes = sizeof (cl_float);
      else if (buffer_format == babl_format ("YA float"))
        *bytes = sizeof (cl_float2);
      else if (buffer_format == babl_format("R'G'B'A u8"))
        *bytes = sizeof (cl_uchar4);
      else if (buffer_format == babl_format("R'G'B' u8"))
        *bytes = sizeof (cl_uchar3);
      else
        *bytes = sizeof (cl_float4);
    }

  return TRUE;
}

GeglClColorOp
gegl_cl_color_supported (const Babl *in_format,
                         const Babl *out_format)
{
  if (in_format == out_format)
    return GEGL_CL_COLOR_EQUAL;

  if (color_kernels_hash && find_color_kernel (in_format, out_format))
    return GEGL_CL_COLOR_CONVERT;

  return GEGL_CL_COLOR_NOT_SUPPORTED;
}

gboolean
gegl_cl_color_conv (cl_mem         in_tex,
                    cl_mem         out_tex,
                    const size_t   size,
                    const Babl    *in_format,
                    const Babl    *out_format)
{
  cl_kernel kernel = NULL;
  cl_int cl_err = 0;

  if (in_format == out_format)
    {
      size_t s = babl_format_get_bytes_per_pixel (in_format);

      /* just copy in_tex to out_tex */
      cl_err = gegl_clEnqueueCopyBuffer (gegl_cl_get_command_queue(),
                                         in_tex, out_tex, 0, 0, size * s,
                                         0, NULL, NULL);
      CL_CHECK;
      return FALSE;
    }

  kernel = find_color_kernel (in_format, out_format);
  if (!kernel)
    return FALSE;

  cl_err = gegl_cl_set_kernel_args (kernel,
                                    sizeof(cl_mem), &in_tex,
                                    sizeof(cl_mem), &out_tex,
                                    NULL);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue (),
                                        kernel, 1,
                                        NULL, &size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;
  return FALSE;

error:
  return TRUE;
}
