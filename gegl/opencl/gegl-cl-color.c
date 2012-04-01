#include "config.h"
#include "gegl.h"
#include "gegl/gegl-debug.h"
#include "gegl-cl-color.h"
#include "gegl-cl-init.h"

#include "gegl-cl-color-kernel.h"

static gegl_cl_run_data *kernels_color = NULL;

#define CL_FORMAT_N 11

static const Babl *format[CL_FORMAT_N];

enum
{
CL_RGBAU8_TO_RGBAF        = 0,
CL_RGBAF_TO_RGBAU8        = 1,

CL_RGBAF_TO_RAGABAF       = 2,
CL_RAGABAF_TO_RGBAF       = 3,
CL_RGBAU8_TO_RAGABAF      = 4,
CL_RAGABAF_TO_RGBAU8      = 5,

CL_RGBAF_TO_RGBA_GAMMA_F  = 6,
CL_RGBA_GAMMA_F_TO_RGBAF  = 7,
CL_RGBAU8_TO_RGBA_GAMMA_F = 8,
CL_RGBA_GAMMA_F_TO_RGBAU8 = 9,

CL_RGBAF_TO_YCBCRAF       = 10,
CL_YCBCRAF_TO_RGBAF       = 11,
CL_RGBAU8_TO_YCBCRAF      = 12,
CL_YCBCRAF_TO_RGBAU8      = 13,

CL_RGBU8_TO_RGBAF         = 14,
CL_RGBAF_TO_RGBU8         = 15,

CL_YU8_TO_YF              = 16,

CL_RGBAF_TO_YAF           = 17,
CL_YAF_TO_RGBAF           = 18,
CL_RGBAU8_TO_YAF          = 19,
CL_YAF_TO_RGBAU8          = 20,

CL_RGBAF_TO_RGBA_GAMMA_U8 = 21,
CL_RGBA_GAMMA_U8_TO_RGBAF = 22,

CL_RGBAF_TO_RGB_GAMMA_U8  = 23,
CL_RGB_GAMMA_U8_TO_RGBAF  = 24,

CL_RGBA_GAMMA_U8_TO_RAGABAF = 25,
CL_RAGABAF_TO_RGBA_GAMMA_U8 = 26,
CL_RGB_GAMMA_U8_TO_RAGABAF  = 27,
CL_RAGABAF_TO_RGB_GAMMA_U8  = 28,

CL_RGBA_GAMMA_U8_TO_YAF = 29,
CL_YAF_TO_RGBA_GAMMA_U8 = 30,
CL_RGB_GAMMA_U8_TO_YAF  = 31,
CL_YAF_TO_RGB_GAMMA_U8  = 32,
};

void
gegl_cl_color_compile_kernels(void)
{
  const char *kernel_name[] = {"rgbau8_to_rgbaf",         /* 0  */
                               "rgbaf_to_rgbau8",         /* 1  */

                               "rgbaf_to_ragabaf",        /* 2  */
                               "ragabaf_to_rgbaf",        /* 3  */
                               "rgbau8_to_ragabaf",       /* 4  */
                               "ragabaf_to_rgbau8",       /* 5  */

                               "rgbaf_to_rgba_gamma_f",   /* 6  */
                               "rgba_gamma_f_to_rgbaf",   /* 7  */
                               "rgbau8_to_rgba_gamma_f",  /* 8  */
                               "rgba_gamma_f_to_rgbau8",  /* 9  */

                               "rgbaf_to_ycbcraf",        /* 10 */
                               "ycbcraf_to_rgbaf",        /* 11 */
                               "rgbau8_to_ycbcraf",       /* 12 */
                               "ycbcraf_to_rgbau8",       /* 13 */

                               "rgbu8_to_rgbaf",          /* 14 */
                               "rgbaf_to_rgbu8",          /* 15 */

                               "yu8_to_yf",               /* 16 */

                               "rgbaf_to_yaf",            /* 17 */
                               "yaf_to_rgbaf",            /* 18 */
                               "rgbau8_to_yaf",           /* 19 */
                               "yaf_to_rgbau8",           /* 20 */

                               "rgbaf_to_rgba_gamma_u8",  /* 21  */
                               "rgba_gamma_u8_to_rgbaf",  /* 22  */

                               "rgbaf_to_rgb_gamma_u8",   /* 23  */
                               "rgb_gamma_u8_to_rgbaf",   /* 24  */

                               "rgba_gamma_u8_to_ragabaf", /* 25 */
                               "ragabaf_to_rgba_gamma_u8", /* 26 */
                               "rgb_gamma_u8_to_ragabaf",  /* 27 */
                               "ragabaf_to_rgb_gamma_u8",  /* 28 */

                               "rgba_gamma_u8_to_yaf",     /* 29 */
                               "yaf_to_rgba_gamma_u8",     /* 30 */
                               "rgb_gamma_u8_to_yaf",      /* 31 */
                               "yaf_to_rgb_gamma_u8",      /* 32 */

                               NULL};

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

  kernels_color = gegl_cl_compile_and_build (kernel_color_source, kernel_name);
}



static gint
choose_kernel (const Babl *in_format, const Babl *out_format)
{
  gint kernel = -1;

  if      (in_format == babl_format ("RGBA float"))
    {
      if      (out_format == babl_format ("RGBA u8"))          kernel = CL_RGBAF_TO_RGBAU8;
      else if (out_format == babl_format ("RaGaBaA float"))    kernel = CL_RGBAF_TO_RAGABAF;
      else if (out_format == babl_format ("R'G'B'A u8"))       kernel = CL_RGBAF_TO_RGBA_GAMMA_U8;
      else if (out_format == babl_format ("R'G'B' u8"))        kernel = CL_RGBAF_TO_RGB_GAMMA_U8;
      else if (out_format == babl_format ("R'G'B'A float"))    kernel = CL_RGBAF_TO_RGBA_GAMMA_F;
      else if (out_format == babl_format ("Y'CbCrA float"))    kernel = CL_RGBAF_TO_YCBCRAF;
      else if (out_format == babl_format ("RGB u8"))           kernel = CL_RGBAF_TO_RGBU8;
      else if (out_format == babl_format ("YA float"))         kernel = CL_RGBAF_TO_YAF;
    }
  else if (in_format == babl_format ("RGBA u8"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = CL_RGBAU8_TO_RGBAF;
      else if (out_format == babl_format ("RaGaBaA float"))    kernel = CL_RGBAU8_TO_RAGABAF;
      else if (out_format == babl_format ("R'G'B'A float"))    kernel = CL_RGBAU8_TO_RGBA_GAMMA_F;
      else if (out_format == babl_format ("Y'CbCrA float"))    kernel = CL_RGBAU8_TO_YCBCRAF;
      else if (out_format == babl_format ("YA float"))         kernel = CL_RGBAU8_TO_YAF;
    }
  else if (in_format == babl_format ("RaGaBaA float"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = CL_RAGABAF_TO_RGBAF;
      else if (out_format == babl_format ("RGBA u8"))          kernel = CL_RAGABAF_TO_RGBAU8;
      else if (out_format == babl_format ("R'G'B'A u8"))       kernel = CL_RAGABAF_TO_RGBA_GAMMA_U8;
      else if (out_format == babl_format ("R'G'B' u8"))        kernel = CL_RAGABAF_TO_RGB_GAMMA_U8;
    }
  else if (in_format == babl_format ("R'G'B'A float"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = CL_RGBA_GAMMA_F_TO_RGBAF;
      else if (out_format == babl_format ("RGBA u8"))          kernel = CL_RGBA_GAMMA_F_TO_RGBAU8;
    }
  else if (in_format == babl_format ("Y'CbCrA float"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = CL_YCBCRAF_TO_RGBAF;
      else if (out_format == babl_format ("RGBA u8"))          kernel = CL_YCBCRAF_TO_RGBAU8;
    }
  else if (in_format == babl_format ("RGB u8"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = CL_RGBU8_TO_RGBAF;
    }
  else if (in_format == babl_format ("Y u8"))
    {
      if      (out_format == babl_format ("Y float"))          kernel = CL_YU8_TO_YF;
    }
  else if (in_format == babl_format ("YA float"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = CL_YAF_TO_RGBAF;
      else if (out_format == babl_format ("RGBA u8"))          kernel = CL_YAF_TO_RGBAU8;
      else if (out_format == babl_format ("R'G'B'A u8"))       kernel = CL_YAF_TO_RGBA_GAMMA_U8;
      else if (out_format == babl_format ("R'G'B' u8"))        kernel = CL_YAF_TO_RGB_GAMMA_U8;
    }
  else if (in_format == babl_format ("R'G'B'A u8"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = CL_RGBA_GAMMA_U8_TO_RGBAF;
      else if (out_format == babl_format ("RaGaBaA float"))    kernel = CL_RGBA_GAMMA_U8_TO_RAGABAF;
      else if (out_format == babl_format ("YA float"))         kernel = CL_RGBA_GAMMA_U8_TO_YAF;
    }
  else if (in_format == babl_format ("R'G'B' u8"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = CL_RGB_GAMMA_U8_TO_RGBAF;
      else if (out_format == babl_format ("RaGaBaA float"))    kernel = CL_RGB_GAMMA_U8_TO_RAGABAF;
      else if (out_format == babl_format ("YA float"))         kernel = CL_RGB_GAMMA_U8_TO_YAF;
    }

  return kernel;
}

gboolean
gegl_cl_color_babl (const Babl *buffer_format, size_t *bytes)
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

gegl_cl_color_op
gegl_cl_color_supported (const Babl *in_format, const Babl *out_format)
{
  if (in_format == out_format)
    return GEGL_CL_COLOR_EQUAL;

  if (choose_kernel (in_format, out_format) >= 0)
    return GEGL_CL_COLOR_CONVERT;
  else
    return GEGL_CL_COLOR_NOT_SUPPORTED;
}

#define CL_ERROR {GEGL_NOTE (GEGL_DEBUG_OPENCL, "Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(errcode)); return FALSE;}

gboolean
gegl_cl_color_conv (cl_mem in_tex, cl_mem out_tex, const size_t size,
                    const Babl *in_format, const Babl *out_format)
{
  int errcode;

  if (gegl_cl_color_supported (in_format, out_format) == GEGL_CL_COLOR_NOT_SUPPORTED)
    return FALSE;

  if (in_format == out_format)
    {
      size_t s;
      gegl_cl_color_babl (in_format, &s);

      /* just copy in_tex to out_tex */
      errcode = gegl_clEnqueueCopyBuffer (gegl_cl_get_command_queue(),
                                          in_tex, out_tex, 0, 0, size * s,
                                          0, NULL, NULL);
      if (errcode != CL_SUCCESS) CL_ERROR

      errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
      if (errcode != CL_SUCCESS) CL_ERROR
    }
  else
    {
      gint k = choose_kernel (in_format, out_format);

      errcode = gegl_clSetKernelArg(kernels_color->kernel[k], 0, sizeof(cl_mem), (void*)&in_tex);
      if (errcode != CL_SUCCESS) CL_ERROR

      errcode = gegl_clSetKernelArg(kernels_color->kernel[k], 1, sizeof(cl_mem), (void*)&out_tex);
      if (errcode != CL_SUCCESS) CL_ERROR

      errcode = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                            kernels_color->kernel[k], 1,
                                            NULL, &size, NULL,
                                            0, NULL, NULL);
      if (errcode != CL_SUCCESS) CL_ERROR

      errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
      if (errcode != CL_SUCCESS) CL_ERROR
    }

  return TRUE;
}

#undef CL_ERROR
