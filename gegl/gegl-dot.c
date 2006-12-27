/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2006 Øyvind Kolås
 */

#include <stdio.h>
#include "gegl-plugin.h"

gchar *
gegl_to_dot (GeglNode *node)
{
  GeglGraph *graph  = GEGL_GRAPH (node);
  GString   *string = g_string_new ("digraph gegl {\n");

    {
      GList *nodes = gegl_graph_get_children (graph);
      GList *entry = nodes;

      while (entry)
        {
          GeglNode *node = entry->data;
          gchar buf[512];
          sprintf (buf, "op_%p [label=\"%s\"];\n", node, gegl_node_get_debug_name (node));
          g_string_append (string, buf);
          entry = g_list_next (entry);
        }

      g_list_free (nodes);
    }

    {
      GList *nodes = gegl_graph_get_children (graph);
      GList *entry = nodes;

      while (entry)
        {
          GeglNode *node = entry->data;
          {
             GList *connections = gegl_node_get_sinks (node);
             GList *entry;
             entry = connections;

             while (entry)
               {
                GeglConnection *connection = entry->data;
                gchar buf[512];
                GeglNode *source;
                GeglNode *sink;

                source = gegl_connection_get_source_node (connection);
                sink = gegl_connection_get_sink_node (connection);

                sprintf (buf, "op_%p -> op_%p;\n", source, sink);
                g_string_append (string, buf);
                entry = g_list_next (entry);
               }

             /* FIXME: should have needed a free, but we're working directly on
              * the list,..
              */
          }
          entry = g_list_next (entry);
        }

      g_list_free (nodes);
    }

  g_string_append (string, "}");
  return g_string_free (string, FALSE);
}
