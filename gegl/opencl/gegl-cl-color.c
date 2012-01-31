#include "gegl.h"
#include "gegl-cl-color.h"
#include "gegl-cl-init.h"

#include "gegl-cl-color-kernel.h"

static gegl_cl_run_data *kernels_color = NULL;

#define CL_FORMAT_N 6

static const Babl *format[CL_FORMAT_N];

void
gegl_cl_color_compile_kernels(void)
{
  const char *kernel_name[] = {"non_premultiplied_to_premultiplied", /* 0 */
                               "premultiplied_to_non_premultiplied", /* 1 */
                               "rgba2rgba_gamma_2_2",                /* 2 */
                               "rgba_gamma_2_22rgba",                /* 3 */
                               "rgba2rgba_gamma_2_2_premultiplied",  /* 4 */
                               "rgba_gamma_2_2_premultiplied2rgba",  /* 5 */
                               "rgbaf_to_rgbau8",                    /* 6 */
                               "rgbau8_to_rgbaf",                    /* 7 */
                               "rgba_to_ycbcra",                     /* 8 */
                               "ycbcra_to_rgba",                     /* 9 */
                               NULL};

  format[0] = babl_format ("RGBA u8"),
  format[1] = babl_format ("RGBA float"),
  format[2] = babl_format ("RaGaBaA float"),
  format[3] = babl_format ("R'G'B'A float"),
  format[4] = babl_format ("R'aG'aB'aA float"),
  format[5] = babl_format ("Y'CbCrA float"),

  kernels_color = gegl_cl_compile_and_build (kernel_color_source, kernel_name);
}


static gint
choose_kernel (const Babl *in_format, const Babl *out_format)
{
  gint kernel = -1;

  if      (in_format == babl_format ("RGBA float"))
    {
      if      (out_format == babl_format ("RaGaBaA float"))    kernel = 0;
      else if (out_format == babl_format ("R'G'B'A float"))    kernel = 2;
      else if (out_format == babl_format ("R'aG'aB'aA float")) kernel = 4;
      else if (out_format == babl_format ("RGBA u8"))          kernel = 6;
      else if (out_format == babl_format ("Y'CbCrA float"))    kernel = 8;
    }
  else if (in_format == babl_format ("RaGaBaA float"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = 1;
      else if (out_format == babl_format ("RGBA u8"))          kernel = 1;
    }
  else if (in_format == babl_format ("R'G'B'A float"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = 3;
      else if (out_format == babl_format ("RGBA u8"))          kernel = 3;
    }
  else if (in_format == babl_format ("R'aG'aB'aA float"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = 5;
      else if (out_format == babl_format ("RGBA u8"))          kernel = 5;
    }
  else if (in_format == babl_format ("RGBA u8")) /* read_imagef and write_imagef abstract texture format for us,
                                                  * so I can use the same functions as RGBA float
                                                  */
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = 7;
      else if (out_format == babl_format ("RaGaBaA float"))    kernel = 0;
      else if (out_format == babl_format ("R'G'B'A float"))    kernel = 2;
      else if (out_format == babl_format ("R'aG'aB'aA float")) kernel = 4;
      else if (out_format == babl_format ("Y'CbCrA float"))    kernel = 8;
    }
  else if (in_format == babl_format ("Y'CbCrA float"))
    {
      if      (out_format == babl_format ("RGBA float"))       kernel = 9;
      else if (out_format == babl_format ("RGBA u8"))          kernel = 9;
    }

  return kernel;
}

gboolean
gegl_cl_color_babl (const Babl *buffer_format, cl_image_format *cl_format, size_t *bytes)
{
  int i;
  gboolean supported_format = FALSE;

  for (i = 0; i < CL_FORMAT_N; i++)
    if (format[i] == buffer_format) supported_format = TRUE;

  if (!supported_format)
    return FALSE;

  if (cl_format)
    {
      if (buffer_format == babl_format ("RGBA u8"))
        {
          cl_format->image_channel_order     = CL_RGBA;
          cl_format->image_channel_data_type = CL_UNORM_INT8;
        }
      else
        {
          cl_format->image_channel_order      = CL_RGBA;
          cl_format->image_channel_data_type  = CL_FLOAT;
        }
    }

  if (bytes)
    *bytes  = (buffer_format == babl_format ("RGBA u8"))? sizeof (cl_uchar4) : sizeof (cl_float4);

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

#define CL_ERROR {g_printf("[OpenCL] Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(errcode)); return FALSE;}

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
      gegl_cl_color_babl (in_format, NULL, &s);

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
