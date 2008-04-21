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
#include  "gegl-debug.h"

#include <glib/gprintf.h>

typedef struct VFuncData
{
  GCallback callback[MAX_PROCESSOR];
  gchar    *string[MAX_PROCESSOR];
} VFuncData;


void
gegl_class_register_alternate_vfunc (GObjectClass *cclass,
                                     gpointer      vfunc_ptr2,
                                     GCallback     process,
                                     const gchar  *string);

void
gegl_class_register_alternate_vfunc (GObjectClass *cclass,
                                     gpointer      vfunc_ptr2,
                                     GCallback     callback,
                                     const gchar  *string)
{
  gint i;
  GCallback *vfunc_ptr = vfunc_ptr2;
  GType      type        = G_TYPE_FROM_CLASS (cclass);
  gchar      tag[20];
  GQuark     quark;
  VFuncData *data;

  g_sprintf (tag, "%p", vfunc_ptr);
  quark = g_quark_from_string (tag);
  data = g_type_get_qdata (type, quark);
  if (!data)
    {
      data = g_new0 (VFuncData, 1);
      g_type_set_qdata (type, quark, data);
    }

  /* Store the default implementation */
  if (data->callback[0]==NULL)
    {
      if (*vfunc_ptr == NULL)
        g_error ("%s: No exsiting default () vfunc defined for %s",
                 G_STRFUNC, g_type_name (type));
      data->callback[0]=callback;
      data->string[0]=g_strdup ("reference");
    }

  /* Find a free slot for this one */
  for (i=1; i<MAX_PROCESSOR; i++)
    {
      if (data->callback[i]==NULL)
        {
          data->callback[i]=callback;
          data->string[i]=g_strdup (string);
          break;
        }
    }
  if (i>=MAX_PROCESSOR)
    {
      g_warning ("Too many callbacks added to %s",
                 g_type_name (G_TYPE_FROM_CLASS (cclass)));
    }

#ifdef USE_SSE
  /* always look for sse ops */
#else
  if (g_getenv ("GEGL_QUALITY"))
#endif
    {
      const gchar *quality  = g_getenv ("GEGL_QUALITY");
      GCallback fast        = NULL;
      GCallback good        = NULL;
      GCallback reference   = NULL;
      GCallback gcc_vectors = NULL;
#ifdef USE_SSE
      GCallback sse       = NULL;
      if (quality == NULL)
        quality = "sse";
#endif

      for (i=0;i<MAX_PROCESSOR;i++)
        {
          const gchar *string = data->string[i];
          GCallback    cb     = data->callback[i];

          if (string && cb!=NULL)
            {
              if (g_str_equal (string, "fast"))
                fast = cb;
              if (g_str_equal (string, "gcc-vectors"))
                gcc_vectors = cb;
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
          GEGL_NOTE(PROCESSOR, "Setting %s callback for %s", fast?"fast":sse?"sse":gcc_vectors?"gcc-vectors":good?"good":"reference",
          g_type_name (G_TYPE_FROM_CLASS (cclass)));
          *vfunc_ptr = fast?fast:sse?sse:gcc_vectors?gcc_vectors:good?good:reference;
#else
          GEGL_NOTE(PROCESSOR, "Setting %s callback for %s", fast?"fast":gcc_vectors?"gcc-vectors":good?"good":"reference",
          g_type_name (G_TYPE_FROM_CLASS (cclass)));
          *vfunc_ptr = fast?fast:gcc_vectors?gcc_vectors:good?good:reference;
#endif
        }
      else if (g_str_equal (quality, "good"))
        {
#ifdef USE_SSE
          GEGL_NOTE(PROCESSOR, "Setting %s callback for %s", sse?"sse":gcc_vectors?"gcc-vectors":good?"good":"reference",
           g_type_name (G_TYPE_FROM_CLASS (cclass)));
          *vfunc_ptr = sse?sse:gcc_vectors?gcc_vectors:good?good:reference;
#else
          GEGL_NOTE(PROCESSOR, "Setting %s callback for %s", gcc_vectors?"gcc-vectors":good?"good":"reference",
           g_type_name (G_TYPE_FROM_CLASS (cclass)));
          *vfunc_ptr = gcc_vectors?"gcc-vectors":good?good:reference;
#endif
        }
      else
        {
          /* best */
#ifdef USE_SSE
          if (sse && gegl_cpu_accel_get_support () & GEGL_CPU_ACCEL_X86_SSE)
            GEGL_NOTE(PROCESSOR, "Setting sse processor for %s", g_type_name (G_TYPE_FROM_CLASS (cclass)));
          *vfunc_ptr = sse?sse:reference;
#else
          *vfunc_ptr = reference;
#endif
        }
    }
}



void
gegl_operation_class_add_processor (GeglOperationClass *cclass,
                                    GCallback           process,
                                    const gchar        *string)
{
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

  gegl_class_register_alternate_vfunc (G_OBJECT_CLASS (cclass),
                                       process_vfunc_ptr,
                                       process,
                                       string);
}
