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
 * Copyright 2003      Calvin Williamson
 *           2005-2008 Øyvind Kolås
 */

#define GEGL_INTERNAL

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-operation.h"
#include "gegl-utils.h"
#include "gegl-cpuaccel.h"
#include "graph/gegl-node.h"
#include "graph/gegl-connection.h"
#include "graph/gegl-pad.h"
#include "buffer/gegl-region.h"
#include "buffer/gegl-buffer.h"
#include "gegl-operations.h"

#include  "gegl-operation-area-filter.h"
#include  "gegl-operation-composer.h"
#include  "gegl-operation-filter.h"
#include  "gegl-operation-meta.h"
#include  "gegl-operation-point-composer.h"
#include  "gegl-operation-point-filter.h"
#include  "gegl-operation-sink.h"
#include  "gegl-operation-source.h"

void
gegl_operation_class_add_processor (GeglOperationClass *cclass,
                                    GCallback           process,
                                    const gchar        *string)
{
  gint i;
  GType    type        = G_TYPE_FROM_CLASS (cclass);
  GType    parent_type = g_type_parent (type);
  gint     vfunc_offset;
  gpointer process_vfunc_ptr;

#define ELSE_IF(type) else if(parent_type==type)
if(parent_type == GEGL_TYPE_OPERATION)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationClass, process);
ELSE_IF( GEGL_TYPE_OPERATION_SOURCE)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationSourceClass, process);
ELSE_IF( GEGL_TYPE_OPERATION_SINK)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationSinkClass, process);
ELSE_IF( GEGL_TYPE_OPERATION_FILTER)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationFilterClass, process);
ELSE_IF( GEGL_TYPE_OPERATION_AREA_FILTER)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationFilterClass, process);
ELSE_IF( GEGL_TYPE_OPERATION_POINT_FILTER)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationPointFilterClass, process);
ELSE_IF( GEGL_TYPE_OPERATION_COMPOSER)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationComposerClass, process);
ELSE_IF( GEGL_TYPE_OPERATION_POINT_COMPOSER)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationPointComposerClass, process);
#undef ELSE_IF
else
  {
     g_error ("%s unable to use %s as parent_type for %s",
              G_STRFUNC, g_type_name (parent_type), g_type_name(G_TYPE_FROM_CLASS (cclass)));
  }
  process_vfunc_ptr = G_STRUCT_MEMBER_P(cclass, vfunc_offset);
#define PROCESS_VFUNC (*(GCallback*) G_STRUCT_MEMBER_P ((cclass), (vfunc_offset)))


  /* Store the default implementation */
  if (cclass->processor[0]==NULL)
    {
      if (cclass->process == NULL)
        g_error ("No process() vfunc defined for %s",
                 g_type_name (G_TYPE_FROM_CLASS (cclass)));
      cclass->processor[0]=process;
      cclass->processor_string[0]=g_strdup ("reference");
    }

  /* Find a free slot for this one */
  for (i=1; i<MAX_PROCESSOR; i++)
    {
      if (cclass->processor[i]==NULL)
        {
          cclass->processor[i]=process;
          cclass->processor_string[i]=g_strdup (string);
          break;
        }
    }
  if (i>=MAX_PROCESSOR)
    {
      g_warning ("Too many processors added to %s",
                 g_type_name (G_TYPE_FROM_CLASS (cclass)));
    }

#ifdef USE_SSE
  /* always look for sse ops */
#else
  if (g_getenv ("GEGL_QUALITY"))
#endif
    {
      const gchar *quality = g_getenv ("GEGL_QUALITY");
      GCallback fast      = NULL;
      GCallback good      = NULL;
      GCallback reference = NULL;
#ifdef USE_SSE
      GCallback sse       = NULL;
      if (quality == NULL)
        quality = "sse";
#endif

      for (i=0;i<MAX_PROCESSOR;i++)
        {
          const gchar *string = cclass->processor_string[i];
          GCallback    cb     = cclass->processor[i];

          if (string && cb!=NULL)
            {
              if (g_str_equal (string, "fast"))
                fast = cb;
              else if (g_str_equal (string, "good"))
                good = cb;
#ifdef USE_SSE
              else if (g_str_equal (string, "sse"))
                sse = cb;
#endif
              else if (g_str_equal (string, "reference"))
                reference = cb;
            }
        }

      g_assert (reference);
      if (g_str_equal (quality, "fast"))
        {
#ifdef USE_SSE
          g_print ("Setting %s processor for %s\n", fast?"fast":sse?"sse":good?"good":"reference",
          g_type_name (G_TYPE_FROM_CLASS (cclass)));
          PROCESS_VFUNC = fast?fast:sse?sse:good?good:reference;
#else
          g_print ("Setting %s processor for %s\n", fast?"fast":good?"good":"reference",
          g_type_name (G_TYPE_FROM_CLASS (cclass)));
          PROCESS_VFUNC = fast?fast:good?good:reference;
#endif
        }
      else if (g_str_equal (quality, "good"))
        {
#ifdef USE_SSE
          g_print ("Setting %s processor for %s\n", sse?"sse":good?"good":"reference",
           g_type_name (G_TYPE_FROM_CLASS (cclass)));
#else
          g_print ("Setting %s processor for %s\n", good?"good":"reference",
           g_type_name (G_TYPE_FROM_CLASS (cclass)));
          PROCESS_VFUNC = good?good:reference;
#endif
        }
      else
        {
          /* best */
#ifdef USE_SSE
          if (sse && gegl_cpu_accel_get_support () & GEGL_CPU_ACCEL_X86_SSE)
            g_print ("Setting sse processor for %s\n", g_type_name (G_TYPE_FROM_CLASS (cclass)));
          PROCESS_VFUNC = sse?sse:reference;
#else
          PROCESS_VFUNC = reference;
#endif
        }
    }
}
