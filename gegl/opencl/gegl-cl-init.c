#define __GEGL_CL_INIT_MAIN__
#include "gegl-cl-init.h"
#undef __GEGL_CL_INIT_MAIN__

#include <gmodule.h>
#include <stdio.h>

static gboolean cl_is_accelerated  = FALSE;

gboolean
gegl_cl_is_accelerated (void)
{
  return cl_is_accelerated;
}

#define CL_LOAD_FUNCTION(func)                                                    \
if (!g_module_symbol (module, #func, (gpointer *)& gegl_##func))                  \
  {                                                                               \
    g_set_error (error, 0, 0,                                \
                 "%s: %s", "libOpenCL.so", g_module_error ());                    \
    if (!g_module_close (module))                                                 \
      g_warning ("%s: %s", "libOpenCL.so", g_module_error ());                    \
    return FALSE;                                                                 \
  }                                                                               \
if (gegl_##func == NULL)                                                          \
  {                                                                               \
    g_set_error (error, 0, 0, "symbol gegl_##func is NULL"); \
    if (!g_module_close (module))                                                 \
      g_warning ("%s: %s", "libOpenCL.so", g_module_error ());                    \
    return FALSE;                                                                 \
  }

gboolean
gegl_cl_init (GError **error)
{
  GModule *module;

  char buffer[65536];

  if (!cl_is_accelerated)
    {
      module = g_module_open ("libOpenCL.so", G_MODULE_BIND_LAZY);

      if (!module)
        {
          g_set_error (error, 0, 0,
                       "%s", g_module_error ());
          return FALSE;
        }

      CL_LOAD_FUNCTION (clGetPlatformIDs)
      CL_LOAD_FUNCTION (clGetPlatformInfo)

      cl_is_accelerated = TRUE;
    }

  return TRUE;
}

#undef CL_LOAD_FUNCTION
