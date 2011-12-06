#include "gegl.h"
#include "gegl-cl-color.h"
#include "gegl-cl-init.h"

#include "gegl-cl-color-kernel.h"

static gegl_cl_run_data *kernels_color = NULL;

static const Babl *format[6];

void
gegl_cl_color_compile_kernels(void)
{
  const char *kernel_name[] = {"non_premultiplied_to_premultiplied", /* 0 */
                               "premultiplied_to_non_premultiplied", /* 1 */
                               "rgba2rgba_gamma_2_2",                /* 2 */
                               "rgba_gamma_2_22rgba",                /* 3 */
                               "rgba2rgba_gamma_2_2_premultiplied",  /* 4 */
                               "rgba_gamma_2_2_premultiplied2rgba",  /* 5 */
                               NULL};

  format[0] = babl_format ("RaGaBaA float"),
  format[1] = babl_format ("RGBA float"),
  format[2] = babl_format ("R'G'B'A float"),
  format[3] = babl_format ("RGBA float"),
  format[4] = babl_format ("R'aG'aB'aA float"),
  format[5] = babl_format ("RGBA float"),

  kernels_color = gegl_cl_compile_and_build (kernel_color_source, kernel_name);
}

gboolean
gegl_cl_color_supported (const Babl *in_format, const Babl *out_format)
{
  int i;
  gboolean supported_format_in  = FALSE;
  gboolean supported_format_out = FALSE;

  for (i = 0; i < 6; i++)
    {
      if (format[i] == in_format)  supported_format_in  = TRUE;
      if (format[i] == out_format) supported_format_out = TRUE;
    }

  return (supported_format_in && supported_format_out);
}

#define CONV_1(x)   {conv[0] = x; conv[1] = -1;}
#define CONV_2(x,y) {conv[0] = x; conv[1] =  y;}

//#define CL_ERROR {g_assert(0);}
#define CL_ERROR {g_printf("[OpenCL] Error in %s:%d@%s - %s\n", __FILE__, __LINE__, __func__, gegl_cl_errstring(errcode)); return FALSE;}

gboolean
gegl_cl_color_conv (cl_mem in_tex, cl_mem out_tex, const size_t size[2],
                    const Babl *in_format, const Babl *out_format)
{
  int i;
  int errcode;
  int conv[2] = {-1, -1};

  if (!gegl_cl_color_supported (in_format, out_format))
    CL_ERROR

  if (in_format == out_format)
    {
      const size_t origin[3] = {0, 0, 0};
      const size_t region[3] = {size[0], size[1], 1};

      /* just copy in_tex to out_tex */
      errcode = gegl_clEnqueueCopyImage (gegl_cl_get_command_queue(),
                                         in_tex, out_tex, origin, origin, region,
                                         0, NULL, NULL);
      if (errcode != CL_SUCCESS) CL_ERROR

      errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
      if (errcode != CL_SUCCESS) CL_ERROR
    }
  else
    {
      if      (in_format == babl_format ("RGBA float"))
        {
          if      (out_format == babl_format ("RaGaBaA float"))    CONV_1(0)
          else if (out_format == babl_format ("R'G'B'A float"))    CONV_1(2)
          else if (out_format == babl_format ("R'aG'aB'aA float")) CONV_1(4)
        }
      else if (in_format == babl_format ("RaGaBaA float"))
        {
          if      (out_format == babl_format ("RGBA float"))       CONV_1(1)
          else if (out_format == babl_format ("R'G'B'A float"))    CONV_2(1, 2)
          else if (out_format == babl_format ("R'aG'aB'aA float")) CONV_2(1, 4)
        }
      else if (in_format == babl_format ("R'G'B'A float"))
        {
          if      (out_format == babl_format ("RGBA float"))       CONV_1(3)
          else if (out_format == babl_format ("RaGaBaA float"))    CONV_2(3, 0)
          else if (out_format == babl_format ("R'aG'aB'aA float")) CONV_2(3, 4)
        }
      else if (in_format == babl_format ("R'aG'aB'aA float"))
        {
          if      (out_format == babl_format ("RGBA float"))       CONV_1(5)
          else if (out_format == babl_format ("RaGaBaA float"))    CONV_2(5, 0)
          else if (out_format == babl_format ("R'G'B'A float"))    CONV_2(5, 2)
        }

      for (i=0; i<2; i++)
        {
          if (conv[i] >= 0)
            {
              errcode = gegl_clSetKernelArg(kernels_color->kernel[conv[i]], 0, sizeof(cl_mem), (void*)&in_tex);
              if (errcode != CL_SUCCESS) CL_ERROR

              errcode = gegl_clSetKernelArg(kernels_color->kernel[conv[i]], 1, sizeof(cl_mem), (void*)&out_tex);
              if (errcode != CL_SUCCESS) CL_ERROR

              errcode = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                                    kernels_color->kernel[conv[i]], 2,
                                                    NULL, size, NULL,
                                                    0, NULL, NULL);
              if (errcode != CL_SUCCESS) CL_ERROR

              errcode = gegl_clEnqueueBarrier(gegl_cl_get_command_queue());
              if (errcode != CL_SUCCESS) CL_ERROR
            }
        }
    }

  return TRUE;
}

#undef CL_ERROR
