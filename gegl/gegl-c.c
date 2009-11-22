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
 * Copyright 2009 Øyvind Kolås
 */

#include "config.h"
#include <gegl.h>
#include <gegl-plugin.h>
#include <glib/gprintf.h>
#include <gobject/gvaluecollector.h>
#include <stdarg.h>
#include <unistd.h>

#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"

static void
gegl_graph_adopt (GeglNode *graph, GeglNode *node)
{
  GeglNode *input, *aux;

  gegl_node_add_child (graph, node);
  g_object_unref (node);

  if ((input = gegl_node_get_producer (node, "input", NULL)))
    gegl_graph_adopt (graph, input);
  if ((aux = gegl_node_get_producer (node, "aux", NULL)))
    gegl_graph_adopt (graph, aux);
}

GeglNode *
gegl_graph (GeglNode *node)
{
  GeglNode *graph;

  graph = g_object_new (GEGL_TYPE_NODE, NULL);
  if (gegl_node_get_pad (node, "output"))
    {
      GeglNode *outproxynop = gegl_node_get_output_proxy (graph, "output");
      gegl_node_connect_to (node, "output", outproxynop, "input"); 
    }
  gegl_graph_adopt (graph, node);
  return graph;
}

GeglNode *
gegl_node (const gchar *type,
           const gchar *firstname,
           ...)
{
  GeglNode      *node;
  GeglOperation *operation;
  va_list        var_args;
  const gchar   *property_name;

  node = g_object_new (GEGL_TYPE_NODE, "operation", type, NULL);
  g_object_get (G_OBJECT (node), "gegl-operation", &operation, NULL);

  g_object_freeze_notify (G_OBJECT (node));
  va_start (var_args, firstname);
  property_name = firstname;
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
          if (operation)
            {
              pspec = g_object_class_find_property (
                G_OBJECT_GET_CLASS (G_OBJECT (operation)), property_name);
            }
          if (!pspec)
            {
              g_warning ("%s:%s has no property named: '%s'",
                         G_STRFUNC,
                         G_OBJECT_TYPE_NAME (operation), property_name);
              break;
            }
          if (!(pspec->flags & G_PARAM_WRITABLE))
            {
              g_warning ("%s: property (%s of operation class '%s' is not writable",
                         G_STRFUNC,
                         pspec->name,
                         G_OBJECT_TYPE_NAME (operation));
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
          g_object_set_property (G_OBJECT (operation), property_name, &value);
          g_value_unset (&value);
        }

      property_name = va_arg (var_args, gchar *);
    }
  g_object_thaw_notify (G_OBJECT (node));

  /* The input pads checked for are hard-coded, if no output is connected
   * pass NULL to indicate no connected node.
   */
  if (gegl_node_get_pad (node, "input"))
    {
      GeglNode *input = va_arg (var_args, GeglNode*);

      if (input)
        gegl_node_connect_from (node, "input", input, "output");
    }

  if (gegl_node_get_pad (node, "aux"))
    {
      GeglNode *aux = va_arg (var_args, GeglNode*);

      if (aux)
        gegl_node_connect_from (node, "aux", aux, "output");
    }
  va_end (var_args);
  return node;
}
