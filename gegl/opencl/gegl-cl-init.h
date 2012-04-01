#ifndef __GEGL_CL_INIT_H__
#define __GEGL_CL_INIT_H__

#include "gegl-cl-types.h"

#define CL_SAFE_CALL(func)                                          \
func;                                                               \
if (errcode != CL_SUCCESS)                                          \
{                                                                   \
  g_warning("OpenCL error in %s, Line %u in file %s\nError:%s",     \
            #func, __LINE__, __FILE__, gegl_cl_errstring(errcode)); \
}

typedef struct
  {
    gboolean is_accelerated;
    cl_context ctx;
    cl_platform_id platform;
    cl_device_id device;
    cl_command_queue cq;
    cl_bool image_support;
    size_t iter_height;
    size_t iter_width;
    cl_ulong max_mem_alloc;
    cl_ulong local_mem_size;

    char platform_name   [1024];
    char platform_version[1024];
    char platform_ext    [1024];
    char device_name     [1024];
  }
gegl_cl_state;

const char *gegl_cl_errstring(cl_int err);

gboolean gegl_cl_init (GError **error);

gboolean gegl_cl_is_accelerated (void);

cl_platform_id gegl_cl_get_platform (void);

cl_device_id gegl_cl_get_device (void);

cl_context gegl_cl_get_context (void);

cl_command_queue gegl_cl_get_command_queue (void);

cl_ulong gegl_cl_get_local_mem_size (void);

size_t gegl_cl_get_iter_width (void);

size_t gegl_cl_get_iter_height (void);

typedef struct
{
  cl_program program;
  cl_kernel  kernel[];
} gegl_cl_run_data;

gegl_cl_run_data *gegl_cl_compile_and_build (const char *program_source,
                                             const char *kernel_name[]);

#define GEGL_CL_CHUNK_SIZE 1024 * 1024

#ifdef __GEGL_CL_INIT_MAIN__

t_clGetPlatformIDs  gegl_clGetPlatformIDs  = NULL;
t_clGetPlatformInfo gegl_clGetPlatformInfo = NULL;
t_clGetDeviceIDs    gegl_clGetDeviceIDs    = NULL;
t_clGetDeviceInfo   gegl_clGetDeviceInfo   = NULL;

t_clCreateContext           gegl_clCreateContext           = NULL;
t_clCreateContextFromType   gegl_clCreateContextFromType   = NULL;
t_clCreateCommandQueue      gegl_clCreateCommandQueue      = NULL;
t_clCreateProgramWithSource gegl_clCreateProgramWithSource = NULL;
t_clBuildProgram            gegl_clBuildProgram            = NULL;
t_clGetProgramBuildInfo     gegl_clGetProgramBuildInfo     = NULL;
t_clCreateKernel            gegl_clCreateKernel            = NULL;
t_clSetKernelArg            gegl_clSetKernelArg            = NULL;
t_clGetKernelWorkGroupInfo  gegl_clGetKernelWorkGroupInfo  = NULL;
t_clCreateBuffer            gegl_clCreateBuffer            = NULL;
t_clEnqueueWriteBuffer      gegl_clEnqueueWriteBuffer      = NULL;
t_clEnqueueReadBuffer       gegl_clEnqueueReadBuffer       = NULL;
t_clEnqueueCopyBuffer       gegl_clEnqueueCopyBuffer       = NULL;
t_clEnqueueReadBufferRect   gegl_clEnqueueReadBufferRect   = NULL;
t_clEnqueueWriteBufferRect  gegl_clEnqueueWriteBufferRect  = NULL;
t_clEnqueueCopyBufferRect   gegl_clEnqueueCopyBufferRect   = NULL;
t_clCreateImage2D           gegl_clCreateImage2D           = NULL;
t_clEnqueueWriteImage       gegl_clEnqueueWriteImage       = NULL;
t_clEnqueueReadImage        gegl_clEnqueueReadImage        = NULL;
t_clEnqueueCopyImage        gegl_clEnqueueCopyImage        = NULL;
t_clEnqueueCopyBufferToImage gegl_clEnqueueCopyBufferToImage = NULL;
t_clEnqueueCopyImageToBuffer gegl_clEnqueueCopyImageToBuffer = NULL;
t_clEnqueueNDRangeKernel    gegl_clEnqueueNDRangeKernel    = NULL;
t_clEnqueueBarrier          gegl_clEnqueueBarrier          = NULL;
t_clFinish                  gegl_clFinish                  = NULL;

t_clEnqueueMapBuffer        gegl_clEnqueueMapBuffer        = NULL;
t_clEnqueueMapImage         gegl_clEnqueueMapImage         = NULL;
t_clEnqueueUnmapMemObject   gegl_clEnqueueUnmapMemObject   = NULL;

t_clReleaseKernel       gegl_clReleaseKernel       = NULL;
t_clReleaseProgram      gegl_clReleaseProgram      = NULL;
t_clReleaseCommandQueue gegl_clReleaseCommandQueue = NULL;
t_clReleaseContext      gegl_clReleaseContext      = NULL;
t_clReleaseMemObject    gegl_clReleaseMemObject    = NULL;

#else

extern t_clGetPlatformIDs  gegl_clGetPlatformIDs;
extern t_clGetPlatformInfo gegl_clGetPlatformInfo;
extern t_clGetDeviceIDs    gegl_clGetDeviceIDs;
extern t_clGetDeviceInfo   gegl_clGetDeviceInfo;

extern t_clCreateContext           gegl_clCreateContext;
extern t_clCreateContextFromType   gegl_clCreateContextFromType;
extern t_clCreateCommandQueue      gegl_clCreateCommandQueue;
extern t_clCreateProgramWithSource gegl_clCreateProgramWithSource;
extern t_clBuildProgram            gegl_clBuildProgram;
extern t_clGetProgramBuildInfo     gegl_clGetProgramBuildInfo;
extern t_clCreateKernel            gegl_clCreateKernel;
extern t_clSetKernelArg            gegl_clSetKernelArg;
extern t_clGetKernelWorkGroupInfo  gegl_clGetKernelWorkGroupInfo;
extern t_clCreateBuffer            gegl_clCreateBuffer;
extern t_clEnqueueWriteBuffer      gegl_clEnqueueWriteBuffer;
extern t_clEnqueueReadBuffer       gegl_clEnqueueReadBuffer;
extern t_clEnqueueReadBufferRect   gegl_clEnqueueReadBufferRect;
extern t_clEnqueueWriteBufferRect  gegl_clEnqueueWriteBufferRect;
extern t_clEnqueueCopyBufferRect   gegl_clEnqueueCopyBufferRect;
extern t_clCreateImage2D           gegl_clCreateImage2D;
extern t_clEnqueueWriteImage       gegl_clEnqueueWriteImage;
extern t_clEnqueueReadImage        gegl_clEnqueueReadImage;
extern t_clEnqueueCopyImage        gegl_clEnqueueCopyImage;
extern t_clEnqueueCopyBuffer       gegl_clEnqueueCopyBuffer;
extern t_clEnqueueCopyBufferToImage gegl_clEnqueueCopyBufferToImage;
extern t_clEnqueueCopyImageToBuffer gegl_clEnqueueCopyImageToBuffer;
extern t_clEnqueueNDRangeKernel    gegl_clEnqueueNDRangeKernel;
extern t_clEnqueueBarrier          gegl_clEnqueueBarrier;
extern t_clFinish                  gegl_clFinish;

extern t_clEnqueueMapBuffer        gegl_clEnqueueMapBuffer;
extern t_clEnqueueMapImage         gegl_clEnqueueMapImage;
extern t_clEnqueueUnmapMemObject   gegl_clEnqueueUnmapMemObject;

extern t_clReleaseKernel       gegl_clReleaseKernel;
extern t_clReleaseProgram      gegl_clReleaseProgram;
extern t_clReleaseCommandQueue gegl_clReleaseCommandQueue;
extern t_clReleaseContext      gegl_clReleaseContext;
extern t_clReleaseMemObject    gegl_clReleaseMemObject;

#endif

#endif  /* __GEGL_CL_INIT_H__ */
