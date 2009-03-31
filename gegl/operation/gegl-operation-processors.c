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

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-config.h"
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
#include  "gegl-operation-composer3.h"
#include  "gegl-operation-composer.h"
#include  "gegl-operation-filter.h"
#include  "gegl-operation-meta.h"
#include  "gegl-operation-point-composer3.h"
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
  gdouble   cached_quality;
  gint      cached;
} VFuncData;

void
gegl_class_register_alternate_vfunc (GObjectClass *cclass,
                                     gpointer      vfunc_ptr2,
                                     GCallback     process,
                                     const gchar  *string);

/* this dispatcher allows overriding a callback without checking how many parameters
 * are passed and how many parameters are needed, hopefully in a compiler/archi
 * portable manner.
 */
static void
dispatch (GObject *object,
          gpointer arg1,
          gpointer arg2,
          gpointer arg3,
          gpointer arg4,
          gpointer arg5,
          gpointer arg6,
          gpointer arg7,
          gpointer arg8,
          gpointer arg9)
{
  void (*dispatch) (GObject *object,
                    gpointer arg1,
                    gpointer arg2,
                    gpointer arg3,
                    gpointer arg4,
                    gpointer arg5,
                    gpointer arg6,
                    gpointer arg7,
                    gpointer arg8,
                    gpointer arg9)=NULL;
  VFuncData *data;
  gint fast      = 0;
  gint good      = 0;
  gint reference = 0;
  gint simd      = 0;
  gint i;
  gint choice;


  data = g_type_get_qdata (G_OBJECT_TYPE(object),
                           g_quark_from_string("dispatch-data"));
  if (!data)
    {
      g_error ("dispatch called on object without dispatch-data");
    }

  if (gegl_config()->quality == data->cached_quality)
    {
      dispatch = (void*)data->callback[data->cached];
      dispatch (object, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
      return;
    }

  for (i=0;i<MAX_PROCESSOR;i++)
    {
      const gchar *string = data->string[i];
      GCallback cb = data->callback[i];

      if (string && cb!=NULL)
        {
          if (g_str_equal (string, "fast"))      fast = i;
          if (g_str_equal (string, "simd"))      simd = i;
          else if (g_str_equal (string, "good")) good = i;
          else if (g_str_equal (string, "reference"))
            reference = i;
        }
    }
  reference = 0;
  g_assert (data->callback[reference]);

  choice = reference;
  if (gegl_config()->quality <= 1.0  && simd) choice = simd;
  if (gegl_config()->quality <= 0.75 && good) choice = good;
  if (gegl_config()->quality <= 0.25 && fast) choice = fast;

  GEGL_NOTE(GEGL_DEBUG_PROCESSOR, "Using %s implementation for %s", data->string[choice], g_type_name (G_OBJECT_TYPE(object)));

  data->cached = choice;
  data->cached_quality = gegl_config()->quality;
  dispatch = (void*)data->callback[data->cached];
  dispatch (object, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
}

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
      data->cached_quality = -1.0;
      g_type_set_qdata (type, quark, data);
      g_type_set_qdata (type, g_quark_from_string("dispatch-data"), data);
    }

  /* Store the default implementation */
  if (data->callback[0]==NULL)
    {
      if (*vfunc_ptr == NULL)
        g_error ("%s: No existing default () vfunc defined for %s",
                 G_STRFUNC, g_type_name (type));
      data->callback[0]=*vfunc_ptr;
      data->string[0]=g_strdup ("reference");
    }
  *vfunc_ptr = (void*)dispatch; /* make our bootstrap replacement for the
                                 * reference implementation take over
                                 * dispatching of arguments, not sure if
                                 * this is portable C or not.
                                 */
  
  /* Find a free slot for this one */
  for (i=1; i<MAX_PROCESSOR; i++)
    {
      if (data->callback[i]==NULL)
        {
          /* store the callback and it's given name */
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
ELSE_IF( GEGL_TYPE_OPERATION_COMPOSER3)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationComposer3Class, process);
ELSE_IF( GEGL_TYPE_OPERATION_POINT_COMPOSER3)
  vfunc_offset = G_STRUCT_OFFSET (GeglOperationPointComposer3Class, process);
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
