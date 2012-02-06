#ifndef __GEGL_CL_TYPES_H__
#define __GEGL_CL_TYPES_H__

#include <glib-object.h>

#include "cl.h"
#include "cl_gl.h"
#include "cl_gl_ext.h"
#include "cl_ext.h"

struct _GeglClTexture
{
  cl_mem data;
  cl_image_format format;
  gint width;
  gint height;
};

typedef struct _GeglClTexture GeglClTexture;

#if defined(_WIN32)
#define CL_API_ENTRY
#define CL_API_CALL __stdcall
#else
#define CL_API_ENTRY
#define CL_API_CALL
#endif

typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clGetPlatformIDs         ) (cl_uint, cl_platform_id *, cl_uint *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clGetPlatformInfo        ) (cl_platform_id, cl_platform_info, size_t, void *, size_t *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clGetDeviceIDs           ) (cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clGetDeviceInfo          ) (cl_device_id, cl_device_info, size_t, void *, size_t *);

typedef CL_API_ENTRY cl_context        (CL_API_CALL *t_clCreateContext          ) (const cl_context_properties *, cl_uint, const cl_device_id *, void (CL_CALLBACK *) (const char *, const void *, size_t, void *), void *, cl_int *);
typedef CL_API_ENTRY cl_context        (CL_API_CALL *t_clCreateContextFromType  ) (cl_context_properties *, cl_device_type, void  (*pfn_notify) (const char *, const void *, size_t, void *), void *, cl_int  *);
typedef CL_API_ENTRY cl_command_queue  (CL_API_CALL *t_clCreateCommandQueue     ) (cl_context context, cl_device_id device, cl_command_queue_properties, cl_int *);
typedef CL_API_ENTRY cl_program        (CL_API_CALL *t_clCreateProgramWithSource) (cl_context, cl_uint, const char **, const size_t *, cl_int *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clBuildProgram           ) (cl_program, cl_uint, const cl_device_id *, const char *, void (CL_CALLBACK *)(cl_program, void *), void *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clGetProgramBuildInfo    ) (cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *);
typedef CL_API_ENTRY cl_kernel         (CL_API_CALL *t_clCreateKernel           ) (cl_program, const char *, cl_int *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clSetKernelArg           ) (cl_kernel, cl_uint, size_t, const void *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clGetKernelWorkGroupInfo ) (cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *, size_t *);
typedef CL_API_ENTRY cl_mem            (CL_API_CALL *t_clCreateBuffer           ) (cl_context, cl_mem_flags, size_t, void *, cl_int *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueWriteBuffer     ) (cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueReadBuffer      ) (cl_command_queue, cl_mem, cl_bool, size_t, size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueCopyBuffer      ) (cl_command_queue, cl_mem, cl_mem,  size_t, size_t, size_t, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueReadBufferRect  ) (cl_command_queue, cl_mem, cl_bool, const size_t [3], const size_t [3], const size_t [3], size_t, size_t, size_t, size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueWriteBufferRect ) (cl_command_queue, cl_mem, cl_bool, const size_t [3], const size_t [3], const size_t [3], size_t, size_t, size_t, size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueCopyBufferRect  ) (cl_command_queue, cl_mem, cl_mem, const size_t [3], const size_t [3], const size_t [3], size_t, size_t, size_t, size_t, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_mem            (CL_API_CALL *t_clCreateImage2D          ) (cl_context, cl_mem_flags, const cl_image_format *, size_t, size_t, size_t, void *, cl_int *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueReadImage       ) (cl_command_queue, cl_mem, cl_bool, const size_t [3], const size_t [3], size_t, size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueWriteImage      ) (cl_command_queue, cl_mem, cl_bool, const size_t [3], const size_t [3], size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueCopyImage       ) (cl_command_queue, cl_mem, cl_mem, const size_t [3], const size_t [3], const size_t [3], cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueCopyImageToBuffer) (cl_command_queue, cl_mem, cl_mem, const size_t [3], const size_t [3], size_t, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueCopyBufferToImage) (cl_command_queue, cl_mem, cl_mem, size_t, const size_t [3], const size_t [3], cl_uint, const cl_event *, cl_event *);

typedef CL_API_ENTRY void *            (CL_API_CALL *t_clEnqueueMapBuffer       ) (cl_command_queue, cl_mem, cl_bool, cl_map_flags, size_t, size_t, cl_uint, const cl_event *, cl_event *, cl_int *);
typedef CL_API_ENTRY void *            (CL_API_CALL *t_clEnqueueMapImage        ) (cl_command_queue, cl_mem, cl_bool, cl_map_flags, const size_t [3], const size_t [3], size_t *, size_t *, cl_uint, const cl_event *, cl_event *, cl_int *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueUnmapMemObject  ) (cl_command_queue, cl_mem, void *, cl_uint, const cl_event *, cl_event *);


typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueNDRangeKernel   ) (cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *, cl_uint, const cl_event *, cl_event *);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clEnqueueBarrier         ) (cl_command_queue);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clFinish                 ) (cl_command_queue);

typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clReleaseKernel          ) (cl_kernel);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clReleaseProgram         ) (cl_program);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clReleaseCommandQueue    ) (cl_command_queue);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clReleaseContext         ) (cl_context);
typedef CL_API_ENTRY cl_int            (CL_API_CALL *t_clReleaseMemObject       ) (cl_mem);

#endif /* __GEGL_CL_TYPES_H__ */
