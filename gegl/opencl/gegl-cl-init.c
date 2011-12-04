#define __GEGL_CL_INIT_MAIN__
#include "gegl-cl-init.h"
#undef __GEGL_CL_INIT_MAIN__

#include <gmodule.h>
#include <string.h>
#include <stdio.h>

/* http://forums.amd.com/forum/messageview.cfm?catid=390&threadid=128536 */
char *gegl_cl_errstring(cl_int err) {
  switch (err) {
    case CL_SUCCESS:                          return strdup("Success!");
    case CL_DEVICE_NOT_FOUND:                 return strdup("Device not found.");
    case CL_DEVICE_NOT_AVAILABLE:             return strdup("Device not available");
    case CL_COMPILER_NOT_AVAILABLE:           return strdup("Compiler not available");
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:    return strdup("Memory object allocation failure");
    case CL_OUT_OF_RESOURCES:                 return strdup("Out of resources");
    case CL_OUT_OF_HOST_MEMORY:               return strdup("Out of host memory");
    case CL_PROFILING_INFO_NOT_AVAILABLE:     return strdup("Profiling information not available");
    case CL_MEM_COPY_OVERLAP:                 return strdup("Memory copy overlap");
    case CL_IMAGE_FORMAT_MISMATCH:            return strdup("Image format mismatch");
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:       return strdup("Image format not supported");
    case CL_BUILD_PROGRAM_FAILURE:            return strdup("Program build failure");
    case CL_MAP_FAILURE:                      return strdup("Map failure");
    case CL_INVALID_VALUE:                    return strdup("Invalid value");
    case CL_INVALID_DEVICE_TYPE:              return strdup("Invalid device type");
    case CL_INVALID_PLATFORM:                 return strdup("Invalid platform");
    case CL_INVALID_DEVICE:                   return strdup("Invalid device");
    case CL_INVALID_CONTEXT:                  return strdup("Invalid context");
    case CL_INVALID_QUEUE_PROPERTIES:         return strdup("Invalid queue properties");
    case CL_INVALID_COMMAND_QUEUE:            return strdup("Invalid command queue");
    case CL_INVALID_HOST_PTR:                 return strdup("Invalid host pointer");
    case CL_INVALID_MEM_OBJECT:               return strdup("Invalid memory object");
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:  return strdup("Invalid image format descriptor");
    case CL_INVALID_IMAGE_SIZE:               return strdup("Invalid image size");
    case CL_INVALID_SAMPLER:                  return strdup("Invalid sampler");
    case CL_INVALID_BINARY:                   return strdup("Invalid binary");
    case CL_INVALID_BUILD_OPTIONS:            return strdup("Invalid build options");
    case CL_INVALID_PROGRAM:                  return strdup("Invalid program");
    case CL_INVALID_PROGRAM_EXECUTABLE:       return strdup("Invalid program executable");
    case CL_INVALID_KERNEL_NAME:              return strdup("Invalid kernel name");
    case CL_INVALID_KERNEL_DEFINITION:        return strdup("Invalid kernel definition");
    case CL_INVALID_KERNEL:                   return strdup("Invalid kernel");
    case CL_INVALID_ARG_INDEX:                return strdup("Invalid argument index");
    case CL_INVALID_ARG_VALUE:                return strdup("Invalid argument value");
    case CL_INVALID_ARG_SIZE:                 return strdup("Invalid argument size");
    case CL_INVALID_KERNEL_ARGS:              return strdup("Invalid kernel arguments");
    case CL_INVALID_WORK_DIMENSION:           return strdup("Invalid work dimension");
    case CL_INVALID_WORK_GROUP_SIZE:          return strdup("Invalid work group size");
    case CL_INVALID_WORK_ITEM_SIZE:           return strdup("Invalid work item size");
    case CL_INVALID_GLOBAL_OFFSET:            return strdup("Invalid global offset");
    case CL_INVALID_EVENT_WAIT_LIST:          return strdup("Invalid event wait list");
    case CL_INVALID_EVENT:                    return strdup("Invalid event");
    case CL_INVALID_OPERATION:                return strdup("Invalid operation");
    case CL_INVALID_GL_OBJECT:                return strdup("Invalid OpenGL object");
    case CL_INVALID_BUFFER_SIZE:              return strdup("Invalid buffer size");
    case CL_INVALID_MIP_LEVEL:                return strdup("Invalid mip-map level");
    default:                                  return strdup("Unknown");
  }
}

gboolean
gegl_cl_is_accelerated (void)
{
  return cl_state.is_accelerated;
}

cl_platform_id
gegl_cl_get_platform (void)
{
  return cl_state.platform;
}

cl_device_id
gegl_cl_get_device (void)
{
  return cl_state.device;
}

cl_context
gegl_cl_get_context (void)
{
  return cl_state.ctx;
}

cl_command_queue
gegl_cl_get_command_queue (void)
{
  return cl_state.cq;
}

#ifdef G_OS_WIN32

#include <windows.h>

#define CL_LOAD_FUNCTION(func)                                                    \
if ((gegl_##func = (t_##func) GetProcAddress(module, #func)) == NULL)             \
  {                                                                               \
    g_set_error (error, 0, 0, "symbol gegl_##func is NULL");                      \
    FreeLibrary(module);                                                          \
    return FALSE;                                                                 \
  }

#else

#define CL_LOAD_FUNCTION(func)                                                    \
if (!g_module_symbol (module, #func, (gpointer *)& gegl_##func))                  \
  {                                                                               \
    g_set_error (error, 0, 0,                                                     \
                 "%s: %s", "libOpenCL.so", g_module_error ());                    \
    if (!g_module_close (module))                                                 \
      g_warning ("%s: %s", "libOpenCL.so", g_module_error ());                    \
    return FALSE;                                                                 \
  }                                                                               \
if (gegl_##func == NULL)                                                          \
  {                                                                               \
    g_set_error (error, 0, 0, "symbol gegl_##func is NULL");                      \
    if (!g_module_close (module))                                                 \
      g_warning ("%s: %s", "libOpenCL.so", g_module_error ());                    \
    return FALSE;                                                                 \
  }

#endif

gboolean
gegl_cl_init (GError **error)
{
  cl_int err;

  if (!cl_state.is_accelerated)
    {
      #ifdef G_OS_WIN32
        HINSTANCE module;
        module = LoadLibrary ("OpenCL.dll");
      #else
        GModule *module;
        module = g_module_open ("libOpenCL.so", G_MODULE_BIND_LAZY);
      #endif

      if (!module)
        {
          g_warning ("Unable to load OpenCL library");
          return FALSE;
        }

      CL_LOAD_FUNCTION (clGetPlatformIDs)
      CL_LOAD_FUNCTION (clGetPlatformInfo)
      CL_LOAD_FUNCTION (clGetDeviceIDs)
      CL_LOAD_FUNCTION (clGetDeviceInfo)

      CL_LOAD_FUNCTION (clCreateContext)
      CL_LOAD_FUNCTION (clCreateContextFromType)
      CL_LOAD_FUNCTION (clCreateCommandQueue)
      CL_LOAD_FUNCTION (clCreateProgramWithSource)
      CL_LOAD_FUNCTION (clBuildProgram)
      CL_LOAD_FUNCTION (clGetProgramBuildInfo)

      CL_LOAD_FUNCTION (clCreateKernel)
      CL_LOAD_FUNCTION (clSetKernelArg)
      CL_LOAD_FUNCTION (clGetKernelWorkGroupInfo)
      CL_LOAD_FUNCTION (clCreateBuffer)
      CL_LOAD_FUNCTION (clEnqueueWriteBuffer)
      CL_LOAD_FUNCTION (clEnqueueReadBuffer)
      CL_LOAD_FUNCTION (clEnqueueCopyBuffer)
      CL_LOAD_FUNCTION (clCreateImage2D)
      CL_LOAD_FUNCTION (clEnqueueWriteImage)
      CL_LOAD_FUNCTION (clEnqueueReadImage)
      CL_LOAD_FUNCTION (clEnqueueCopyImage)
      CL_LOAD_FUNCTION (clEnqueueCopyBufferToImage)
      CL_LOAD_FUNCTION (clEnqueueCopyImageToBuffer)
      CL_LOAD_FUNCTION (clEnqueueNDRangeKernel)
      CL_LOAD_FUNCTION (clEnqueueBarrier)
      CL_LOAD_FUNCTION (clFinish)

      CL_LOAD_FUNCTION (clEnqueueMapBuffer)
      CL_LOAD_FUNCTION (clEnqueueMapImage)
      CL_LOAD_FUNCTION (clEnqueueUnmapMemObject)

      CL_LOAD_FUNCTION (clReleaseKernel)
      CL_LOAD_FUNCTION (clReleaseProgram)
      CL_LOAD_FUNCTION (clReleaseCommandQueue)
      CL_LOAD_FUNCTION (clReleaseContext)
      CL_LOAD_FUNCTION (clReleaseMemObject)

      gegl_clGetPlatformIDs (1, &cl_state.platform, NULL);

      gegl_clGetPlatformInfo (cl_state.platform, CL_PLATFORM_NAME,       sizeof(cl_state.platform_name),    cl_state.platform_name,    NULL);
      gegl_clGetPlatformInfo (cl_state.platform, CL_PLATFORM_VERSION,    sizeof(cl_state.platform_version), cl_state.platform_version, NULL);
      gegl_clGetPlatformInfo (cl_state.platform, CL_PLATFORM_EXTENSIONS, sizeof(cl_state.platform_ext),     cl_state.platform_ext,     NULL);

      gegl_clGetDeviceIDs (cl_state.platform, CL_DEVICE_TYPE_DEFAULT, 1, &cl_state.device, NULL);
      gegl_clGetDeviceInfo(cl_state.device, CL_DEVICE_NAME, sizeof(cl_state.device_name), cl_state.device_name, NULL);

      gegl_clGetDeviceInfo (cl_state.device, CL_DEVICE_IMAGE_SUPPORT,      sizeof(cl_bool),  &cl_state.image_support,    NULL);
      gegl_clGetDeviceInfo (cl_state.device, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(size_t),   &cl_state.max_image_height, NULL);
      gegl_clGetDeviceInfo (cl_state.device, CL_DEVICE_IMAGE2D_MAX_WIDTH,  sizeof(size_t),   &cl_state.max_image_width,  NULL);
      gegl_clGetDeviceInfo (cl_state.device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &cl_state.max_mem_alloc,    NULL);

      cl_state.max_image_width  = MIN(4096, MIN(cl_state.max_image_width, cl_state.max_image_height));
      cl_state.max_image_height = MIN(4096, MIN(cl_state.max_image_width, cl_state.max_image_height));

      g_printf("[OpenCL] Platform Name:%s\n",       cl_state.platform_name);
      g_printf("[OpenCL] Version:%s\n",             cl_state.platform_version);
      g_printf("[OpenCL] Extensions:%s\n",          cl_state.platform_ext);
      g_printf("[OpenCL] Default Device Name:%s\n", cl_state.device_name);
      g_printf("[OpenCL] Tile Dimensions: (%d, %d)\n", cl_state.max_image_width, cl_state.max_image_height);

      if (cl_state.image_support)
        {
          g_printf("[OpenCL] Image Support OK\n");
        }
      else
        {
          g_printf("[OpenCL] Image Support Error\n");
          return FALSE;
        }

      cl_state.ctx = gegl_clCreateContext(0, 1, &cl_state.device, NULL, NULL, &err);
      if(err != CL_SUCCESS)
        {
          g_printf("[OpenCL] Could not create context\n");
          return FALSE;
        }

      cl_state.cq  = gegl_clCreateCommandQueue(cl_state.ctx, cl_state.device, 0, &err);

      if(err != CL_SUCCESS)
        {
          g_printf("[OpenCL] Could not create command queue\n");
          return FALSE;
        }

    }

  cl_state.is_accelerated = TRUE;

  /* XXX: this dict is being leaked */
  cl_program_hash = g_hash_table_new (g_str_hash, g_str_equal);

  g_printf("[OpenCL] OK\n");

  return TRUE;
}

#undef CL_LOAD_FUNCTION

/* XXX: same program_source with different kernel_name[], context or device
 *      will retrieve the same key
 */
gegl_cl_run_data *
gegl_cl_compile_and_build (const char *program_source, const char *kernel_name[])
{
  gint errcode;
  gegl_cl_run_data *cl_data = NULL;

  if ((cl_data = (gegl_cl_run_data *)g_hash_table_lookup(cl_program_hash, program_source)) == NULL)
    {
      size_t length = strlen(program_source);

      gint i;
      guint kernel_n = 0;
      while (kernel_name[++kernel_n] != NULL);

      cl_data = (gegl_cl_run_data *) g_malloc(sizeof(gegl_cl_run_data)+sizeof(cl_kernel)*kernel_n);

      CL_SAFE_CALL( cl_data->program = gegl_clCreateProgramWithSource(gegl_cl_get_context(), 1, &program_source,
                                                                      &length, &errcode) );

      errcode = gegl_clBuildProgram(cl_data->program, 0, NULL, NULL, NULL, NULL);
      if (errcode != CL_SUCCESS)
        {
          char buffer[2000];
          char *err_msg;
          CL_SAFE_CALL( errcode = gegl_clGetProgramBuildInfo(cl_data->program,
                                                             gegl_cl_get_device(),
                                                             CL_PROGRAM_BUILD_LOG,
                                                             sizeof(buffer), buffer, NULL) );
          g_warning("OpenCL Build Error:%s\n%s",
                    err_msg = gegl_cl_errstring(errcode), buffer);
          free(err_msg);
          return NULL;
        }
      else
        {
          g_printf("[OpenCL] Compiling successful\n");
        }

      for (i=0; i<kernel_n; i++)
        CL_SAFE_CALL( cl_data->kernel[i] =
                      gegl_clCreateKernel(cl_data->program, kernel_name[i], &errcode) );

      g_hash_table_insert(cl_program_hash, g_strdup (program_source), (void*)cl_data);
    }

  return cl_data;
}
