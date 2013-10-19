#include "gegl-cl.h"

cl_int
gegl_cl_set_kernel_args (cl_kernel kernel, ...)
{
  cl_int error = 0;
  int i = 0;
  va_list var_args;

  va_start (var_args, kernel);
  while (1)
    {
      size_t  size;
      void   *value;
      size  = va_arg (var_args, size_t);
      if (!size)
        break;
      value = va_arg (var_args, void *);

      error = gegl_clSetKernelArg (kernel, i++, size, value);
      if (error)
        break;
    }
  va_end (var_args);

  return error;
}