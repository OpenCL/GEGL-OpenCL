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
 *           2013 Daniel Sabo
 */

/* OpenCL Initialization
   The API is stubbed out so we detect if OpenCL libraries are available
   in runtime.
*/

#include "config.h"

#define __GEGL_CL_INIT_MAIN__
#include "gegl-cl-init.h"
#undef __GEGL_CL_INIT_MAIN__

#include <glib.h>
#include <gmodule.h>
#include <string.h>
#include <stdio.h>

#include "gegl-cl.h"
#include "gegl-cl-color.h"
#include "opencl/random.cl.h"

#include "gegl/gegl-debug.h"

GQuark gegl_opencl_error_quark (void);

GQuark
gegl_opencl_error_quark (void)
{
  return g_quark_from_static_string ("gegl-opencl-error-quark");
}

#define GEGL_OPENCL_ERROR (gegl_opencl_error_quark ())

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

  static const int strings_len = sizeof(strings) / sizeof(strings[0]);

  if (-err < 0 || -err >= strings_len)
    return "unknown error";

  return strings[-err];
}

gboolean _gegl_cl_is_accelerated;

typedef struct
{
  gboolean         is_loaded;
  gboolean         have_opengl;
  gboolean         hard_disable;
  gboolean         enable_profiling;
  cl_context       ctx;
  cl_platform_id   platform;
  cl_device_id     device;
  cl_command_queue cq;
  cl_bool          image_support;
  size_t           iter_height;
  size_t           iter_width;
  cl_ulong         max_mem_alloc;
  cl_ulong         local_mem_size;

  char platform_name   [1024];
  char platform_version[1024];
  char platform_ext    [1024];
  char device_name     [1024];
}
GeglClState;

/* we made some performance measurements and OpenCL in the CPU is rarely worth it,
 * specially now that we got our multi-threading working */

static cl_device_type gegl_cl_default_device_type = CL_DEVICE_TYPE_GPU;
static GeglClState cl_state = { 0, };
static GHashTable *cl_program_hash = NULL;


gboolean
gegl_cl_has_gl_sharing (void)
{
  return cl_state.have_opengl && gegl_cl_is_accelerated ();
}

void
gegl_cl_disable (void)
{
  _gegl_cl_is_accelerated = FALSE;
}

void
gegl_cl_hard_disable (void)
{
  cl_state.hard_disable = TRUE;
  _gegl_cl_is_accelerated = FALSE;
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

void
gegl_cl_set_profiling (gboolean enable)
{
  g_return_if_fail (!cl_state.is_loaded);

  cl_state.enable_profiling = enable;
}

void
gegl_cl_set_default_device_type (cl_device_type default_device_type)
{
  g_return_if_fail (!cl_state.is_loaded);

  gegl_cl_default_device_type = default_device_type;
}

static gboolean
gegl_cl_device_has_extension (cl_device_id device, const char *extension_name)
{
  cl_int     cl_err;
  size_t     string_len = 0;
  gchar     *device_ext_string = NULL;
  gchar    **extensions;
  gboolean   found = FALSE;

  if (!extension_name)
    return FALSE;

  cl_err= gegl_clGetDeviceInfo (device, CL_DEVICE_EXTENSIONS,
                                0, NULL, &string_len);
  CL_CHECK_ONLY (cl_err);

  if (!string_len)
    return FALSE;

  device_ext_string = g_malloc0 (string_len);

  cl_err = gegl_clGetDeviceInfo (device, CL_DEVICE_EXTENSIONS,
                                 string_len, device_ext_string, NULL);
  CL_CHECK_ONLY (cl_err);

  extensions = g_strsplit (device_ext_string, " ", 0);

  for (gint i = 0; extensions[i] && !found; ++i)
    {
      if (!strcmp (extensions[i], extension_name))
        found = TRUE;
    }

  g_free (device_ext_string);
  g_strfreev (extensions);

  return found;
}

gboolean
gegl_cl_has_extension (const char *extension_name)
{
  if (!gegl_cl_is_accelerated () || !extension_name)
    return FALSE;

  return gegl_cl_device_has_extension (cl_state.device, extension_name);
}

#ifdef G_OS_WIN32

#include <windows.h>

#define CL_LOAD_FUNCTION(func)                                                    \
if ((gegl_##func = (t_##func) GetProcAddress(module, #func)) == NULL)             \
  {                                                                               \
    g_set_error (error, GEGL_OPENCL_ERROR, 0, "symbol gegl_##func is NULL");      \
    FreeLibrary(module);                                                          \
    return FALSE;                                                                 \
  }

#else

#ifdef __APPLE__
#define GL_LIBRARY_NAME "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
#define CL_LIBRARY_NAME "/System/Library/Frameworks/OpenCL.framework/Versions/Current/OpenCL"
#else
#define GL_LIBRARY_NAME "libGL.so.1"
#define CL_LIBRARY_NAME "libOpenCL.so"
#endif

#define CL_LOAD_FUNCTION(func)                                                    \
if (!g_module_symbol (module, #func, (gpointer *)& gegl_##func))                  \
  {                                                                               \
    GEGL_NOTE (GEGL_DEBUG_OPENCL, "%s: %s", CL_LIBRARY_NAME, g_module_error ());  \
    g_set_error (error, GEGL_OPENCL_ERROR, 0, "%s: %s", CL_LIBRARY_NAME, g_module_error ()); \
    if (!g_module_close (module))                                                 \
      g_warning ("%s: %s", CL_LIBRARY_NAME, g_module_error ());                   \
    return FALSE;                                                                 \
  }                                                                               \
if (gegl_##func == NULL)                                                          \
  {                                                                               \
    GEGL_NOTE (GEGL_DEBUG_OPENCL, "symbol gegl_##func is NULL");                  \
    g_set_error (error, GEGL_OPENCL_ERROR, 0, "symbol gegl_##func is NULL");      \
    if (!g_module_close (module))                                                 \
      g_warning ("%s: %s", CL_LIBRARY_NAME, g_module_error ());                   \
    return FALSE;                                                                 \
  }

#endif

#define CL_LOAD_EXTENSION_FUNCTION(func)                                          \
g_assert(gegl_clGetExtensionFunctionAddress);                                     \
gegl_##func = gegl_clGetExtensionFunctionAddress(#func);                          \
if (gegl_##func == NULL)                                                          \
  {                                                                               \
    GEGL_NOTE (GEGL_DEBUG_OPENCL, "symbol gegl_##func is NULL");                  \
    g_set_error (error, GEGL_OPENCL_ERROR, 0, "symbol gegl_##func is NULL");      \
    return FALSE;                                                                 \
  }

#if defined(__APPLE__)
typedef struct _CGLContextObject *CGLContextObj;
typedef struct CGLShareGroupRec  *CGLShareGroupObj;

typedef CGLContextObj (*t_CGLGetCurrentContext) (void);
typedef CGLShareGroupObj (*t_CGLGetShareGroup) (CGLContextObj);

t_CGLGetCurrentContext gegl_CGLGetCurrentContext;
t_CGLGetShareGroup gegl_CGLGetShareGroup;

/* FIXME: Move this to cl_gl_ext.h */
#define CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE        0x10000000
#elif defined(G_OS_WIN32)
/* pass */
#else
typedef struct _XDisplay Display;
typedef struct __GLXcontextRec *GLXContext;


typedef GLXContext (*t_glXGetCurrentContext) (void);
typedef Display * (*t_glXGetCurrentDisplay) (void);

t_glXGetCurrentContext gegl_glXGetCurrentContext;
t_glXGetCurrentDisplay gegl_glXGetCurrentDisplay;
#endif

static gboolean
gegl_cl_init_get_gl_sharing_props (cl_context_properties   gl_contex_props[64],
                                   GError                **error)
{
  static gboolean gl_loaded = FALSE;

  #if defined(__APPLE__)
  CGLContextObj kCGLContext;
  CGLShareGroupObj kCGLShareGroup;

  if (!gl_loaded)
    {
      GModule *module = g_module_open (GL_LIBRARY_NAME, G_MODULE_BIND_LAZY);

      if (!g_module_symbol (module, "CGLGetCurrentContext", (gpointer *)&gegl_CGLGetCurrentContext))
        printf ("Failed to load CGLGetCurrentContext");
      if (!g_module_symbol (module, "CGLGetShareGroup", (gpointer *)&gegl_CGLGetShareGroup))
        printf ("Failed to load CGLGetShareGroup");

      gl_loaded = TRUE;
    }

  kCGLContext = gegl_CGLGetCurrentContext ();
  kCGLShareGroup = gegl_CGLGetShareGroup (kCGLContext);

  gl_contex_props[0] = CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE;
  gl_contex_props[1] = (cl_context_properties)kCGLShareGroup;
  gl_contex_props[2] = 0;
  return TRUE;

  #elif defined(G_OS_WIN32)

  GEGL_NOTE (GEGL_DEBUG_OPENCL, "GL sharing not supported on WIN32");
  g_set_error (error, GEGL_OPENCL_ERROR, 0, "GL sharing not supported on WIN32");

  return FALSE;

  #else /* Some kind of unix */
  GLXContext  context;
  Display    *display;

  if (!gl_loaded)
    {
      GModule *module = g_module_open (GL_LIBRARY_NAME, G_MODULE_BIND_LAZY);

      if (!g_module_symbol (module, "glXGetCurrentContext", (gpointer *)&gegl_glXGetCurrentContext))
        printf ("Failed to load glXGetCurrentContext");
      if (!g_module_symbol (module, "glXGetCurrentDisplay", (gpointer *)&gegl_glXGetCurrentDisplay))
        printf ("Failed to load glXGetCurrentDisplay");

      gl_loaded = TRUE;
    }

  context = gegl_glXGetCurrentContext();
  display = gegl_glXGetCurrentDisplay();
  if (!context || !display)
    {
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not get a valid OpenGL context");
      g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not get a valid OpenGL context");
      return FALSE;
    }

  gl_contex_props[0] = CL_GL_CONTEXT_KHR;
  gl_contex_props[1] = (cl_context_properties)context;
  gl_contex_props[2] = CL_GLX_DISPLAY_KHR;
  gl_contex_props[3] = (cl_context_properties)display;
  gl_contex_props[4] = 0;
  return TRUE;

  #endif
}

static gboolean
gegl_cl_init_common (cl_device_type          requested_device_type,
                     gboolean                gl_sharing,
                     GError                **error);

gboolean
gegl_cl_init_with_opengl  (GError **error)
{
  return gegl_cl_init_common (gegl_cl_default_device_type, TRUE, error);
}

gboolean
gegl_cl_init (GError **error)
{
  return gegl_cl_init_common (gegl_cl_default_device_type, FALSE, error);
}

static gboolean
gegl_cl_init_load_functions (GError **error)
{
#ifdef G_OS_WIN32
  HINSTANCE module = LoadLibrary ("OpenCL.dll");
#else
  GModule *module = g_module_open (CL_LIBRARY_NAME, G_MODULE_BIND_LAZY);
#endif

  if (!module)
    {
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "Unable to load OpenCL library");
      g_set_error (error, GEGL_OPENCL_ERROR, 0, "Unable to load OpenCL library");
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
  CL_LOAD_FUNCTION (clEnqueueReadBufferRect)
  CL_LOAD_FUNCTION (clEnqueueWriteBufferRect)
  CL_LOAD_FUNCTION (clEnqueueCopyBufferRect)
  CL_LOAD_FUNCTION (clCreateImage2D)
  CL_LOAD_FUNCTION (clCreateImage3D)
  CL_LOAD_FUNCTION (clEnqueueReadImage)
  CL_LOAD_FUNCTION (clEnqueueWriteImage)
  CL_LOAD_FUNCTION (clEnqueueCopyImage)
  CL_LOAD_FUNCTION (clEnqueueCopyImageToBuffer)
  CL_LOAD_FUNCTION (clEnqueueCopyBufferToImage)

  CL_LOAD_FUNCTION (clEnqueueMapBuffer)
  CL_LOAD_FUNCTION (clEnqueueMapImage)
  CL_LOAD_FUNCTION (clEnqueueUnmapMemObject)

  CL_LOAD_FUNCTION (clEnqueueNDRangeKernel)
  CL_LOAD_FUNCTION (clEnqueueBarrier)
  CL_LOAD_FUNCTION (clFinish)

  CL_LOAD_FUNCTION (clGetEventProfilingInfo)

  CL_LOAD_FUNCTION (clReleaseKernel)
  CL_LOAD_FUNCTION (clReleaseProgram)
  CL_LOAD_FUNCTION (clReleaseCommandQueue)
  CL_LOAD_FUNCTION (clReleaseContext)
  CL_LOAD_FUNCTION (clReleaseMemObject)

  CL_LOAD_FUNCTION (clGetExtensionFunctionAddress);

  return TRUE;
}

static gboolean
gegl_cl_gl_init_load_functions (GError **error)
{
  CL_LOAD_EXTENSION_FUNCTION (clCreateFromGLTexture2D)
  CL_LOAD_EXTENSION_FUNCTION (clEnqueueAcquireGLObjects)
  CL_LOAD_EXTENSION_FUNCTION (clEnqueueReleaseGLObjects)

  return TRUE;
}

static gboolean
gegl_cl_init_load_device_info (cl_platform_id   platform,
                               cl_device_id     device,
                               cl_device_type   requested_device_type,
                               GError         **error)
{
  cl_int err = CL_SUCCESS;

  if (device)
    {
      /* Get platform from device */
      err = gegl_clGetDeviceInfo (device, CL_DEVICE_PLATFORM, sizeof (cl_platform_id), &platform, NULL);
      if (err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create platform");
          g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not create platform");
          return FALSE;
        }
    }
  else
    {
      cl_platform_id *platforms = NULL;
      cl_uint num_platforms = 0;

      if (!requested_device_type)
        requested_device_type = CL_DEVICE_TYPE_DEFAULT;

      err = gegl_clGetPlatformIDs (0, NULL, &num_platforms);
      if (err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create platform");
          g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not create platform");
          return FALSE;
        }

      if (platform)
        {
          platforms = g_new (cl_platform_id, 1);
          num_platforms = 1;
          platforms[0] = platform;
        }
      else
        {
          platforms = g_new (cl_platform_id, num_platforms);
          err = gegl_clGetPlatformIDs (num_platforms, platforms, NULL);
        }

      if (err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create platform");
          g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not create platform");
          g_free (platforms);
          return FALSE;
        }

      for (int platform_idx = 0; platform_idx < num_platforms; platform_idx++)
        {
          platform = platforms[platform_idx];
          err = gegl_clGetDeviceIDs (platform, requested_device_type, 1, &device, NULL);
          if (err == CL_SUCCESS)
            break;
        }

      g_free (platforms);

      if (err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create device: %s", gegl_cl_errstring (err));
          g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not create device: %s", gegl_cl_errstring (err));
          return FALSE;
        }
    }

  cl_state.platform = platform;
  cl_state.device = device;

  gegl_clGetPlatformInfo (platform, CL_PLATFORM_NAME,       sizeof(cl_state.platform_name),    cl_state.platform_name,    NULL);
  gegl_clGetPlatformInfo (platform, CL_PLATFORM_VERSION,    sizeof(cl_state.platform_version), cl_state.platform_version, NULL);
  gegl_clGetPlatformInfo (platform, CL_PLATFORM_EXTENSIONS, sizeof(cl_state.platform_ext),     cl_state.platform_ext,     NULL);

  gegl_clGetDeviceInfo (device, CL_DEVICE_NAME, sizeof(cl_state.device_name), cl_state.device_name, NULL);

  gegl_clGetDeviceInfo (device, CL_DEVICE_IMAGE_SUPPORT,      sizeof(cl_bool),  &cl_state.image_support,    NULL);
  gegl_clGetDeviceInfo (device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &cl_state.max_mem_alloc,    NULL);
  gegl_clGetDeviceInfo (device, CL_DEVICE_LOCAL_MEM_SIZE,     sizeof(cl_ulong), &cl_state.local_mem_size,   NULL);

  cl_state.iter_width  = 4096;
  cl_state.iter_height = 4096;

  while (cl_state.iter_width * cl_state.iter_height * 16 > cl_state.max_mem_alloc)
    {
      if (cl_state.iter_height < cl_state.iter_width)
        cl_state.iter_width  /= 2;
      else
        cl_state.iter_height /= 2;
    }

  cl_state.iter_width  /= 2;

  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Platform Name: %s",       cl_state.platform_name);
  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Version: %s",             cl_state.platform_version);
  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Extensions: %s",          cl_state.platform_ext);
  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Default Device Name: %s", cl_state.device_name);
  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Max Alloc: %lu bytes",    (unsigned long)cl_state.max_mem_alloc);
  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Local Mem: %lu bytes",    (unsigned long)cl_state.local_mem_size);
  GEGL_NOTE (GEGL_DEBUG_OPENCL, "Iteration size: (%lu, %lu)",
                                (long unsigned int)cl_state.iter_width,
                                (long unsigned int)cl_state.iter_height);

  return TRUE;
}

static gboolean
gegl_cl_init_common (cl_device_type          requested_device_type,
                     gboolean                gl_sharing,
                     GError                **error)
{
  cl_int err;

  if (cl_state.hard_disable)
    {
      GEGL_NOTE (GEGL_DEBUG_OPENCL, "OpenCL is disabled");
      g_set_error (error, GEGL_OPENCL_ERROR, 0, "OpenCL is disabled");
      return FALSE;
    }

  if (!cl_state.is_loaded)
    {
      cl_command_queue_properties command_queue_flags = 0;
      cl_context ctx = NULL;

      if (!gegl_cl_init_load_functions (error))
        return FALSE;

      if (gl_sharing)
        {
#ifdef __APPLE__
          cl_device_id sharing_device;
#endif
          cl_context_properties gl_contex_props[64];

          if (!gegl_cl_init_get_gl_sharing_props (gl_contex_props, error))
            return FALSE;

#ifdef __APPLE__
          /* Create context */
          ctx = gegl_clCreateContext (gl_contex_props, 0, 0, NULL, 0, &err);

          if (err != CL_SUCCESS)
            {
              GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create context: %s", gegl_cl_errstring (err));
              g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not create context: %s", gegl_cl_errstring (err));
              return FALSE;
            }

          /* Get device */
          clGetContextInfo (ctx, CL_CONTEXT_DEVICES, sizeof(cl_device_id), &sharing_device, NULL);

          if (err != CL_SUCCESS)
            {
              clReleaseContext (ctx);
              GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not get context's device: %s", gegl_cl_errstring (err));
              g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not get context's device: %s", gegl_cl_errstring (err));
              return FALSE;
            }

          if (!gegl_cl_init_load_device_info (NULL, sharing_device, 0, error))
            {
              clReleaseContext (ctx);
              return FALSE;
            }
#else
          /* Get default GPU device */
          if (!gegl_cl_init_load_device_info (NULL, NULL, CL_DEVICE_TYPE_GPU, error))
            return FALSE;

          if (!gegl_cl_device_has_extension (cl_state.device, "cl_khr_gl_sharing"))
            {
              GEGL_NOTE (GEGL_DEBUG_OPENCL, "Device does not support cl_khr_gl_sharing");
              g_set_error (error, GEGL_OPENCL_ERROR, 0, "Device does not support cl_khr_gl_sharing");
              return FALSE;
            }

          /* Load extension functions */
          if (!gegl_cl_gl_init_load_functions (error))
            return FALSE;

          /* Create context */
          ctx = gegl_clCreateContext (gl_contex_props, 1, &cl_state.device, NULL, NULL, &err);

          if (err != CL_SUCCESS)
            {
              GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create context: %s", gegl_cl_errstring (err));
              g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not create context: %s", gegl_cl_errstring (err));
              return FALSE;
            }
#endif
        }
      else
        {
          if (!gegl_cl_init_load_device_info (NULL, NULL, requested_device_type, error))
            return FALSE;
          ctx = gegl_clCreateContext (NULL, 1, &cl_state.device, NULL, NULL, &err);
        }

      if (cl_state.image_support)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Image Support OK");
        }
      else
        {
          if (ctx)
            gegl_clReleaseContext (ctx);

          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Image Support Error");
          g_set_error (error, GEGL_OPENCL_ERROR, 0, "Image Support Error");
          return FALSE;
        }

      cl_state.ctx = ctx;

      command_queue_flags = 0;
      if (cl_state.enable_profiling)
        command_queue_flags |= CL_QUEUE_PROFILING_ENABLE;

      cl_state.cq = gegl_clCreateCommandQueue (cl_state.ctx, cl_state.device, command_queue_flags, &err);

      if (err != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not create command queue");
          g_set_error (error, GEGL_OPENCL_ERROR, 0, "Could not create command queue");
          return FALSE;
        }

      if (gl_sharing)
        cl_state.have_opengl = TRUE;
      _gegl_cl_is_accelerated = TRUE;
      cl_state.is_loaded = TRUE;

      /* XXX: this dict is being leaked */
      cl_program_hash = g_hash_table_new (g_str_hash, g_str_equal);

      gegl_cl_color_compile_kernels ();

      GEGL_NOTE (GEGL_DEBUG_OPENCL, "OK");
    }

  if (cl_state.is_loaded)
    _gegl_cl_is_accelerated = TRUE;

  return TRUE;
}

#undef CL_LOAD_FUNCTION

/* XXX: same program_source with different kernel_name[], context or device
 *      will retrieve the same key
 */
GeglClRunData *
gegl_cl_compile_and_build (const char *program_source, const char *kernel_name[])
{
  gint errcode;
  GeglClRunData *cl_data = NULL;
  if (!gegl_cl_is_accelerated ())
    return NULL;

  cl_data = (GeglClRunData *)g_hash_table_lookup (cl_program_hash, program_source);

  if (cl_data == NULL)
    {
      const size_t lengths[] = {strlen(random_cl_source), strlen(program_source)};
      const char *sources[] = {random_cl_source, program_source};

      gint    i;
      char   *msg;
      size_t  s = 0;
      cl_int  build_errcode;
      guint   kernel_n = 0;

      while (kernel_name[++kernel_n] != NULL);

      cl_data = (GeglClRunData *) g_new (GeglClRunData, 1);

      cl_data->program = gegl_clCreateProgramWithSource (gegl_cl_get_context (), 2, sources,
                                                         lengths, &errcode);
      CL_CHECK_ONLY (errcode);

      build_errcode = gegl_clBuildProgram (cl_data->program, 0, NULL, NULL, NULL, NULL);

      errcode = gegl_clGetProgramBuildInfo (cl_data->program,
                                            gegl_cl_get_device (),
                                            CL_PROGRAM_BUILD_LOG,
                                            0, NULL, &s);
      CL_CHECK_ONLY (errcode);

      if (s)
        {
          msg = g_malloc (s);
          errcode = gegl_clGetProgramBuildInfo (cl_data->program,
                                                gegl_cl_get_device (),
                                                CL_PROGRAM_BUILD_LOG,
                                                s, msg, NULL);
          CL_CHECK_ONLY (errcode);
        }
      else
        {
          msg = strdup ("");
        }

      if (build_errcode != CL_SUCCESS)
        {
          GEGL_NOTE (GEGL_DEBUG_OPENCL, "Build Error: %s\n%s",
                                        gegl_cl_errstring (build_errcode),
                                        msg);
          g_free (msg);
          return NULL;
        }
      else
        {
          g_strchug (msg);
          if (strlen (msg))
            GEGL_NOTE (GEGL_DEBUG_OPENCL, "Compiling successful\n%s", msg);
          else
            GEGL_NOTE (GEGL_DEBUG_OPENCL, "Compiling successful");
          g_free (msg);
        }

      cl_data->kernel = g_new (cl_kernel, kernel_n);
      cl_data->work_group_size = g_new (size_t, kernel_n);

      for (i = 0; i < kernel_n; i++)
        {
          cl_data->kernel[i] = gegl_clCreateKernel (cl_data->program,
                                                    kernel_name[i],
                                                    &errcode);
          CL_CHECK_ONLY (errcode);

          errcode = gegl_clGetKernelWorkGroupInfo (cl_data->kernel[i],
                                                   gegl_cl_get_device (),
                                                   CL_KERNEL_WORK_GROUP_SIZE,
                                                   sizeof(size_t),
                                                   &cl_data->work_group_size[i],
                                                   NULL);
          CL_CHECK_ONLY (errcode);
        }

      g_hash_table_insert (cl_program_hash, g_strdup (program_source), (void*)cl_data);
    }

  return cl_data;
}

void
gegl_cl_cleanup (void)
{
  cl_int err;
  err = gegl_cl_random_cleanup ();
  if (err != CL_SUCCESS)
    GEGL_NOTE (GEGL_DEBUG_OPENCL, "Could not free cl_random_data: %s", gegl_cl_errstring (err));
}
