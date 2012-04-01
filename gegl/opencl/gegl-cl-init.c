#include "config.h"

#define __GEGL_CL_INIT_MAIN__
#include "gegl-cl-init.h"
#undef __GEGL_CL_INIT_MAIN__

#include <gmodule.h>
#include <string.h>
#include <stdio.h>

#include "gegl-cl-color.h"

#include "gegl/gegl-debug.h"

const char *gegl_cl_errstring(cl_int err) {
  static const char* strings[] =
  {
    /* Error Codes */
      "success"                         /*  0  */
    , "device not found"                /* -1  */
    , "device not available"            /* -2  */
    , "compiler not available"          /* -3  */
    , "mem object allocation failure"   /* -4  */
    , "out of resources"                /* -5  */
    , "out of host memory"              /* -6  */
    , "profiling info not available"    /* -7  */
    , "mem copy overlap"                /* -8  */
    , "image format mismatch"           /* -9  */
    , "image format not supported"      /* -10 */
    , "build program failure"           /* -11 */
    , "map failure"                     /* -12 */
    , ""                                /* -13 */
    , ""                                /* -14 */
    , ""                                /* -15 */
    , ""                                /* -16 */
    , ""                                /* -17 */
    , ""                                /* -18 */
    , ""                                /* -19 */
    , ""                                /* -20 */
    , ""                                /* -21 */
    , ""                                /* -22 */
    , ""                                /* -23 */
    , ""                                /* -24 */
    , ""                                /* -25 */
    , ""                                /* -26 */
    , ""                                /* -27 */
    , ""                                /* -28 */
    , ""                                /* -29 */
    , "invalid value"                   /* -30 */
    , "invalid device type"             /* -31 */
    , "invalid platform"                /* -32 */
    , "invalid device"                  /* -33 */
    , "invalid context"                 /* -34 */
    , "invalid queue properties"        /* -35 */
    , "invalid command queue"           /* -36 */
    , "invalid host ptr"                /* -37 */
    , "invalid mem object"              /* -38 */
    , "invalid image format descriptor" /* -39 */
    , "invalid image size"              /* -40 */
    , "invalid sampler"                 /* -41 */
    , "invalid binary"                  /* -42 */
    , "invalid build options"           /* -43 */
    , "invalid program"                 /* -44 */
    , "invalid program executable"      /* -45 */
    , "invalid kernel name"             /* -46 */
    , "invalid kernel definition"       /* -47 */
    , "invalid kernel"                  /* -48 */
    , "invalid arg index"               /* -49 */
    , "invalid arg value"               /* -50 */
    , "invalid arg size"                /* -51 */
    , "invalid kernel args"             /* -52 */
    , "invalid work dimension"          /* -53 */
    , "invalid work group size"         /* -54 */
    , "invalid work item size"          /* -55 */
    , "invalid global offset"           /* -56 */
    , "invalid event wait list"         /* -57 */
    , "invalid event"                   /* -58 */
    , "invalid operation"               /* -59 */
    , "invalid gl object"               /* -60 */
    , "invalid buffer size"             /* -61 */
    , "invalid mip level"               /* -62 */
    , "invalid global work size"        /* -63 */
  };

  return strings[-err];
}

static gegl_cl_state cl_state = {FALSE, NULL, NULL, NULL, NULL, FALSE, 0, 0, 0, 0, "", "", "", ""};
static GHashTable *cl_program_hash = NULL;

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

cl_ulong
gegl_cl_get_local_mem_size (void)
{
  return cl_state.local_mem_size;
}

size_t
gegl_cl_get_iter_width (void)
{
  return cl_state.iter_width;
}

size_t
gegl_cl_get_iter_height (void)
{
  return cl_state.iter_height;
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
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Unable to load OpenCL library");
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
      CL_LOAD_FUNCTION (clEnqueueCopyBuffer)
      CL_LOAD_FUNCTION (clEnqueueCopyBuffer)
      CL_LOAD_FUNCTION (clEnqueueCopyBuffer)
      CL_LOAD_FUNCTION (clEnqueueReadBufferRect)
      CL_LOAD_FUNCTION (clEnqueueWriteBufferRect)
      CL_LOAD_FUNCTION (clEnqueueCopyBufferRect)
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

      err = gegl_clGetPlatformIDs (1, &cl_state.platform, NULL);
      if(err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create platform");
          return FALSE;
        }

      gegl_clGetPlatformInfo (cl_state.platform, CL_PLATFORM_NAME,       sizeof(cl_state.platform_name),    cl_state.platform_name,    NULL);
      gegl_clGetPlatformInfo (cl_state.platform, CL_PLATFORM_VERSION,    sizeof(cl_state.platform_version), cl_state.platform_version, NULL);
      gegl_clGetPlatformInfo (cl_state.platform, CL_PLATFORM_EXTENSIONS, sizeof(cl_state.platform_ext),     cl_state.platform_ext,     NULL);

      err = gegl_clGetDeviceIDs (cl_state.platform, CL_DEVICE_TYPE_DEFAULT, 1, &cl_state.device, NULL);
      if(err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create device");
          return FALSE;
        }

      gegl_clGetDeviceInfo(cl_state.device, CL_DEVICE_NAME, sizeof(cl_state.device_name), cl_state.device_name, NULL);

      gegl_clGetDeviceInfo (cl_state.device, CL_DEVICE_IMAGE_SUPPORT,      sizeof(cl_bool),  &cl_state.image_support,    NULL);
      gegl_clGetDeviceInfo (cl_state.device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &cl_state.max_mem_alloc,    NULL);
      gegl_clGetDeviceInfo (cl_state.device, CL_DEVICE_LOCAL_MEM_SIZE,     sizeof(cl_ulong), &cl_state.local_mem_size,   NULL);

      cl_state.iter_width  = 4096;
      cl_state.iter_height = 4096;

      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Platform Name:%s",       cl_state.platform_name);
      GEGL_NOTE (GEGL_DEBUG_OPENCL, " Version:%s",            cl_state.platform_version);
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Extensions:%s",          cl_state.platform_ext);
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Default Device Name:%s", cl_state.device_name);
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Max Alloc: %lu bytes",   (unsigned long)cl_state.max_mem_alloc);
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Local Mem: %lu bytes",   (unsigned long)cl_state.local_mem_size);

      while (cl_state.iter_width * cl_state.iter_height * 16 > cl_state.max_mem_alloc)
        {
          if (cl_state.iter_height < cl_state.iter_width)
            cl_state.iter_width  /= 2;
          else
            cl_state.iter_height /= 2;
        }
      cl_state.iter_width  /= 2;

      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Iteration size: (%lu, %lu)", cl_state.iter_width, cl_state.iter_height);

      if (cl_state.image_support)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Image Support OK");
        }
      else
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Image Support Error");
          return FALSE;
        }

      cl_state.ctx = gegl_clCreateContext(0, 1, &cl_state.device, NULL, NULL, &err);
      if(err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create context");
          return FALSE;
        }

      cl_state.cq  = gegl_clCreateCommandQueue(cl_state.ctx, cl_state.device, 0, &err);

      if(err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create command queue");
          return FALSE;
        }

    }

  cl_state.is_accelerated = TRUE;

  /* XXX: this dict is being leaked */
  cl_program_hash = g_hash_table_new (g_str_hash, g_str_equal);

  if (cl_state.is_accelerated)
    gegl_cl_color_compile_kernels();

  GEGL_NOTE (GEGL_DEBUG_OPENCL, "OK");

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
          char *msg;
          size_t s;
          G_GNUC_UNUSED cl_int build_errcode = errcode;

          CL_SAFE_CALL( errcode = gegl_clGetProgramBuildInfo(cl_data->program,
                                                             gegl_cl_get_device(),
                                                             CL_PROGRAM_BUILD_LOG,
                                                             0, NULL, &s) );

          msg = g_malloc (s);
          CL_SAFE_CALL( errcode = gegl_clGetProgramBuildInfo(cl_data->program,
                                                             gegl_cl_get_device(),
                                                             CL_PROGRAM_BUILD_LOG,
                                                             s, msg, NULL) );
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Build Error:%s\n%s", gegl_cl_errstring(build_errcode), msg);
          g_free (msg);

          return NULL;
        }
      else
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Compiling successful\n");
        }

      for (i=0; i<kernel_n; i++)
        CL_SAFE_CALL( cl_data->kernel[i] =
                      gegl_clCreateKernel(cl_data->program, kernel_name[i], &errcode) );

      g_hash_table_insert(cl_program_hash, g_strdup (program_source), (void*)cl_data);
    }

  return cl_data;
}
