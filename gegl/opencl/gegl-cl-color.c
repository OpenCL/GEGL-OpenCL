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
#include "opencl/colors-8bit-lut.cl.h"

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

static gboolean
gegl_cl_color_load_conversion_set (ColorConversionInfo  *conversions,
                                   gint                  num_conversions,
                                   GeglClRunData       **kernels,
                                   const gchar          *source)
{
  const char *kernel_names[num_conversions + 1];

  for (int i = 0; i < num_conversions; ++i)
    {
      kernel_names[i] = conversions[i].kernel_name;
    }

  kernel_names[num_conversions] = NULL;

  *kernels = gegl_cl_compile_and_build (source, kernel_names);

  if (!(*kernels))
    {
      return FALSE;
    }

  for (int i = 0; i < num_conversions; ++i)
    {
      ColorConversionInfo *info = g_new (ColorConversionInfo, 1);

      conversions[i].kernel = (*kernels)->kernel[i];
      *info = conversions[i];

      g_hash_table_insert (color_kernels_hash, info, info);
    }

  return TRUE;
}

gboolean
gegl_cl_color_compile_kernels (void)
{
  static GeglClRunData *float_kernels = NULL;
  static GeglClRunData *lut8_kernels = NULL;
  gboolean result = TRUE;

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
    { babl_format ("RaGaBaA float"), babl_format("YA float"), "ragabaf_to_yaf", NULL },

    { babl_format ("RGBA float"), babl_format("R'G'B'A u8"), "rgbaf_to_rgba_gamma_u8", NULL },
    { babl_format ("RGBA float"), babl_format("R'G'B' u8"), "rgbaf_to_rgb_gamma_u8", NULL },

    { babl_format ("RaGaBaA float"), babl_format("R'G'B'A u8"), "ragabaf_to_rgba_gamma_u8", NULL },
    { babl_format ("RaGaBaA float"), babl_format("R'G'B' u8"), "ragabaf_to_rgb_gamma_u8", NULL },

    { babl_format ("YA float"), babl_format("R'G'B'A u8"), "yaf_to_rgba_gamma_u8", NULL },
    { babl_format ("YA float"), babl_format("R'G'B' u8"), "yaf_to_rgb_gamma_u8", NULL },

    { babl_format ("YA float"), babl_format ("RaGaBaA float"), "yaf_to_ragabaf", NULL },

    { babl_format ("Y float"), babl_format ("RaGaBaA float"), "yf_to_ragabaf", NULL },

    { babl_format ("RGBA float"), babl_format ("RGB float"), "rgbaf_to_rgbf", NULL },

    { babl_format ("R'G'B' float"), babl_format ("RGBA float"), "rgb_gamma_f_to_rgbaf", NULL },

    /* Reuse some conversions */
    { babl_format ("R'G'B' u8"), babl_format ("R'G'B'A float"), "rgbu8_to_rgbaf", NULL },
    { babl_format ("R'G'B'A u8"), babl_format ("R'G'B'A float"), "rgbau8_to_rgbaf", NULL },

    { babl_format ("R'G'B' float"), babl_format ("RaGaBaA float"), "rgb_gamma_f_to_rgbaf", NULL },
  };

  ColorConversionInfo lut8_conversions[] = {
    { babl_format ("R'G'B'A u8"), babl_format("RGBA float"), "rgba_gamma_u8_to_rgbaf", NULL },
    { babl_format ("R'G'B'A u8"), babl_format("RaGaBaA float"), "rgba_gamma_u8_to_ragabaf", NULL },
    { babl_format ("R'G'B'A u8"), babl_format("YA float"), "rgba_gamma_u8_to_yaf", NULL },
    { babl_format ("R'G'B' u8"), babl_format("RGBA float"), "rgb_gamma_u8_to_rgbaf", NULL },
    { babl_format ("R'G'B' u8"), babl_format("RaGaBaA float"), "rgb_gamma_u8_to_ragabaf", NULL },
    { babl_format ("R'G'B' u8"), babl_format("YA float"), "rgb_gamma_u8_to_yaf", NULL },
  };

  /* Fail if the kernels are already compiled, meaning this must have been called twice */
  g_return_val_if_fail (!float_kernels, FALSE);
  g_return_val_if_fail (!color_kernels_hash, FALSE);

  color_kernels_hash = g_hash_table_new_full (color_kernels_hash_hashfunc,
                                              color_kernels_hash_equalfunc,
                                              NULL,
                                              NULL);

  gegl_cl_color_load_conversion_set (float_conversions,
                                     G_N_ELEMENTS (float_conversions),
                                     &float_kernels,
                                     colors_cl_source);

  if (!float_kernels)
    {
      g_warning ("Failed to compile color conversions (float_kernels)");
      result = FALSE;
    }

  gegl_cl_color_load_conversion_set (lut8_conversions,
                                     G_N_ELEMENTS (lut8_conversions),
                                     &lut8_kernels,
                                     colors_8bit_lut_cl_source);

  if (!lut8_kernels)
    {
      g_warning ("Failed to compile color conversions (lut8_kernels)");
      result = FALSE;
    }

  return result;
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
  if (bytes)
    *bytes = babl_format_get_bytes_per_pixel (buffer_format);

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

  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Missing OpenCL conversion for %s -> %s",
             babl_get_name (in_format),
             babl_get_name (out_format));

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
