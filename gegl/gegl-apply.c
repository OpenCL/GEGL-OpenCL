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
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "gegl-apply.h"

#include "graph/gegl-node.h"

#include "operation/gegl-operation.h"
#include "operation/gegl-operations.h"
#include "operation/gegl-operation-meta.h"
#include "operation/gegl-operation-point-filter.h"

#include "process/gegl-eval-mgr.h"
#include "process/gegl-have-visitor.h"
#include "process/gegl-prepare-visitor.h"
#include "process/gegl-finish-visitor.h"
#include "process/gegl-processor.h"


static void
gegl_node_set_props (GeglNode *node,
                     va_list   var_args)
{
  const char *property_name;

  g_object_freeze_notify (G_OBJECT (node));

  property_name = va_arg (var_args, gchar *);
  while (property_name)
    {
      GValue      value = { 0, };
      GParamSpec *pspec = NULL;
      gchar      *error = NULL;

      if (!strcmp (property_name, "name"))
        {
          pspec = g_object_class_find_property (
            G_OBJECT_GET_CLASS (G_OBJECT (node)), property_name);

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          G_VALUE_COLLECT (&value, var_args, 0, &error);
          if (error)
            {
              g_warning ("%s: %s", G_STRFUNC, error);
              g_free (error);
              g_value_unset (&value);
              break;
            }
          g_object_set_property (G_OBJECT (node), property_name, &value);
          g_value_unset (&value);
        }
      else
        {
          if (node->operation)
            {
              pspec = g_object_class_find_property (
                G_OBJECT_GET_CLASS (G_OBJECT (node->operation)), property_name);
            }
          if (!pspec)
            {
              g_warning ("%s:%s has no property named: '%s'",
                         G_STRFUNC,
                         gegl_node_get_debug_name (node), property_name);
              break;
            }
          if (!(pspec->flags & G_PARAM_WRITABLE))
            {
              g_warning ("%s: property (%s of operation class '%s' is not writable",
                         G_STRFUNC,
                         pspec->name,
                         G_OBJECT_TYPE_NAME (node->operation));
              break;
            }

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          G_VALUE_COLLECT (&value, var_args, 0, &error);
          if (error)
            {
              g_warning ("%s: %s", G_STRFUNC, error);
              g_free (error);
              g_value_unset (&value);
              break;
            }
          g_object_set_property (G_OBJECT (node->operation), property_name, &value);
          g_value_unset (&value);
        }
      property_name = va_arg (var_args, gchar *);
    }
  g_object_thaw_notify (G_OBJECT (node));
}


void
gegl_apply_op (GeglBuffer *buffer,
               const gchar *first_property_name,
               ...)
{
  va_list var_args;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  va_start (var_args, first_property_name);
  gegl_apply_op_valist (buffer, first_property_name, var_args);
  va_end (var_args);
}

void
gegl_apply_op_valist (GeglBuffer  *buffer,
                      const gchar *first_property_name,
                      va_list      var_args)
{
  GeglBuffer  *tempbuf = NULL;
  GeglNode    *node;
  GeglNode    *source;
  GeglNode    *sink;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  g_object_ref (buffer);

  source  = gegl_node_new_child (NULL, "operation", "gegl:buffer-source",
                                    "buffer", buffer,
                                    NULL);
  node   = gegl_node_new_child (NULL, "operation", first_property_name, NULL);

  if (!GEGL_IS_OPERATION_POINT_FILTER (node->operation))
    {
      tempbuf = gegl_buffer_new (gegl_buffer_get_extent (buffer), gegl_buffer_get_format (buffer));

      sink = gegl_node_new_child (NULL, "operation", "gegl:write-buffer",
                                        "buffer", tempbuf,
                                        NULL);
    }
  else
    {
      sink = gegl_node_new_child (NULL, "operation", "gegl:write-buffer",
                                        "buffer", buffer,
                                        NULL);
    }

  gegl_node_link_many (source, node, sink, NULL);

  gegl_node_set_props (node, var_args);

  gegl_node_process (sink);

  g_object_unref (source);
  g_object_unref (node);
  g_object_unref (sink);

  if (tempbuf)
    {
      gegl_buffer_copy (tempbuf, NULL, buffer, NULL);
      g_object_unref (tempbuf);
    }
  g_object_unref (buffer);
}

GeglBuffer *gegl_filter_op_valist (GeglBuffer    *buffer,
                                   const gchar   *first_property_name,
                                   va_list        var_args)
{
  GeglBuffer  *tempbuf = NULL;
  GeglNode    *node;
  GeglNode    *source;
  GeglNode    *sink;

  //g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  if (buffer)
    {
      g_object_ref (buffer);
      source  = gegl_node_new_child (NULL, "operation", "gegl:buffer-source",
                                        "buffer", buffer,
                                        NULL);
    }
  node   = gegl_node_new_child (NULL, "operation", first_property_name, NULL);

  sink = gegl_node_new_child (NULL, "operation", "gegl:buffer-sink",
                                    "buffer", &tempbuf,
                                    NULL);

  if (buffer)
    gegl_node_link_many (source, node, sink, NULL);
  else
    gegl_node_link_many (node, sink, NULL);

  gegl_node_set_props (node, var_args);

  gegl_node_process (sink);

  if (buffer)
    {
      g_object_unref (source);
      g_object_unref (buffer);
    }
  g_object_unref (node);
  g_object_unref (sink);

  return tempbuf;
}

GeglBuffer *gegl_filter_op     (GeglBuffer    *buffer,
                                const gchar   *first_property_name,
                                ...)
{
  GeglBuffer *ret;
  va_list var_args;

  //g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  va_start (var_args, first_property_name);
  ret = gegl_filter_op_valist (buffer, first_property_name, var_args);
  va_end (var_args);
  return ret;
}



void
gegl_render_op (GeglBuffer *source_buffer,
                GeglBuffer *target_buffer,
                const gchar *first_property_name,
                ...)
{
  va_list var_args;

  g_return_if_fail (GEGL_IS_BUFFER (source_buffer));
  g_return_if_fail (GEGL_IS_BUFFER (target_buffer));

  va_start (var_args, first_property_name);
  gegl_render_op_valist (source_buffer, target_buffer,
                         first_property_name, var_args);
  va_end (var_args);
}

void
gegl_render_op_valist (GeglBuffer  *source_buffer,
                       GeglBuffer  *target_buffer,
                       const gchar *first_property_name,
                       va_list      var_args)
{
  GeglNode    *node;
  GeglNode    *source;
  GeglNode    *sink;

  g_return_if_fail (GEGL_IS_BUFFER (source_buffer));
  g_return_if_fail (GEGL_IS_BUFFER (target_buffer));

  g_object_ref (source_buffer);
  g_object_ref (target_buffer);

  source  = gegl_node_new_child (NULL, "operation", "gegl:buffer-source",
                                       "buffer", source_buffer,
                                       NULL);
  node   = gegl_node_new_child (NULL, "operation", first_property_name, NULL);

  sink   = gegl_node_new_child (NULL, "operation", "gegl:write-buffer",
                                      "buffer", target_buffer,
                                      NULL);

  gegl_node_link_many (source, node, sink, NULL);
  gegl_node_set_props (node, var_args);
  gegl_node_process (sink);

  g_object_unref (source);
  g_object_unref (node);
  g_object_unref (sink);

  g_object_unref (source_buffer);
  g_object_unref (target_buffer);
}
