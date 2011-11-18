#ifndef __GEGL_CL_TYPES_H__
#define __GEGL_CL_TYPES_H__

#include <glib-object.h>

#include "cl.h"
#include "cl_gl.h"
#include "cl_gl_ext.h"
#include "cl_ext.h"

typedef cl_int (*t_clGetPlatformIDs)  (cl_uint, cl_platform_id *, cl_uint *);
typedef cl_int (*t_clGetPlatformInfo) (cl_platform_id, cl_platform_info, size_t, void *, size_t *);
typedef cl_int (*t_clGetDeviceIDs)    (cl_platform_id, cl_device_type, cl_uint, cl_device_id *, cl_uint *);
typedef cl_int (*t_clGetDeviceInfo)   (cl_device_id, cl_device_info, size_t, void *, size_t *);

typedef cl_context        (*t_clCreateContext          ) (const cl_context_properties *, cl_uint, const cl_device_id *, void (CL_CALLBACK *) (const char *, const void *, size_t, void *), void *, cl_int *);
typedef cl_context        (*t_clCreateContextFromType  ) (cl_context_properties *, cl_device_type, void  (*pfn_notify) (const char *, const void *, size_t, void *), void *, cl_int  *);
typedef cl_command_queue  (*t_clCreateCommandQueue     ) (cl_context context, cl_device_id device, cl_command_queue_properties, cl_int *);
typedef cl_program        (*t_clCreateProgramWithSource) (cl_context, cl_uint, const char **, const size_t *, cl_int *);
typedef cl_int            (*t_clBuildProgram           ) (cl_program, cl_uint, const cl_device_id *, const char *, void (CL_CALLBACK *)(cl_program, void *), void *);
typedef cl_int            (*t_clGetProgramBuildInfo    ) (cl_program, cl_device_id, cl_program_build_info, size_t, void *, size_t *);
typedef cl_kernel         (*t_clCreateKernel           ) (cl_program, const char *, cl_int *);
typedef cl_int            (*t_clSetKernelArg           ) (cl_kernel, cl_uint, size_t, const void *);
typedef cl_int            (*t_clGetKernelWorkGroupInfo ) (cl_kernel, cl_device_id, cl_kernel_work_group_info, size_t, void *, size_t *);
typedef cl_mem            (*t_clCreateBuffer           ) (cl_context, cl_mem_flags, size_t, void *, cl_int *);
typedef cl_int            (*t_clEnqueueWriteBuffer     ) (cl_command_queue, cl_mem, cl_bool, size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int            (*t_clEnqueueReadBuffer      ) (cl_command_queue, cl_mem, cl_bool, size_t, size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int            (*t_clEnqueueCopyBuffer      ) (cl_command_queue, cl_mem, cl_mem,  size_t, size_t, size_t, cl_uint, const cl_event *, cl_event *);

typedef cl_mem            (*t_clCreateImage2D          ) (cl_context, cl_mem_flags, const cl_image_format *, size_t, size_t, size_t, void *, cl_int *);
typedef cl_int            (*t_clEnqueueReadImage       ) (cl_command_queue, cl_mem, cl_bool, const size_t [3], const size_t [3], size_t, size_t, void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int            (*t_clEnqueueWriteImage      ) (cl_command_queue, cl_mem, cl_bool, const size_t [3], const size_t [3], size_t, size_t, const void *, cl_uint, const cl_event *, cl_event *);
typedef cl_int            (*t_clEnqueueCopyImage       ) (cl_command_queue, cl_mem, cl_mem, const size_t [3], const size_t [3], const size_t [3], cl_uint, const cl_event *, cl_event *);
typedef cl_int            (*t_clEnqueueCopyImageToBuffer) (cl_command_queue, cl_mem, cl_mem, const size_t [3], const size_t [3], size_t, cl_uint, const cl_event *, cl_event *);
typedef cl_int            (*t_clEnqueueCopyBufferToImage) (cl_command_queue, cl_mem, cl_mem, size_t, const size_t [3], const size_t [3], cl_uint, const cl_event *, cl_event *);

typedef void *            (*t_clEnqueueMapBuffer       ) (cl_command_queue, cl_mem, cl_bool, cl_map_flags, size_t, size_t, cl_uint, const cl_event *, cl_event *, cl_int *);
typedef void *            (*t_clEnqueueMapImage        ) (cl_command_queue, cl_mem, cl_bool, cl_map_flags, const size_t [3], const size_t [3], size_t *, size_t *, cl_uint, const cl_event *, cl_event *, cl_int *);
typedef cl_int            (*t_clEnqueueUnmapMemObject  ) (cl_command_queue, cl_mem, void *, cl_uint, const cl_event *, cl_event *);


typedef cl_int            (*t_clEnqueueNDRangeKernel   ) (cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *, cl_uint, const cl_event *, cl_event *);
typedef cl_int            (*t_clEnqueueBarrier         ) (cl_command_queue);
typedef cl_int            (*t_clFinish                 ) (cl_command_queue);

typedef cl_int (*t_clReleaseKernel      ) (cl_kernel);
typedef cl_int (*t_clReleaseProgram     ) (cl_program);
typedef cl_int (*t_clReleaseCommandQueue) (cl_command_queue);
typedef cl_int (*t_clReleaseContext     ) (cl_context);
typedef cl_int (*t_clReleaseMemObject   ) (cl_mem);

#endif /* __GEGL_CL_TYPES_H__ */
