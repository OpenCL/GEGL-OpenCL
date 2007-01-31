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
#include <string.h>
#include "gegl-plugin.h"

void
gegl_add_graph (GString     *string,
                GeglNode    *node,
                const gchar *label)
{
  GeglNode *graph = node;

  g_string_append_printf (string, "subgraph cluster_%s%p { graph [ label=\"%s %p\" fontsize=\"10\" ranksep=\"0.3\" nodesep=\"0.3\"]; node [ fontsize=\"10\" ];\n", label, node, label, node);

    {
      GSList *nodes = gegl_node_get_children (graph);
      GSList *entry = nodes;

      while (entry)
        {
          GeglNode *node = entry->data;

          if (node->is_graph)
            {
              gchar *name = g_strdup (gegl_node_get_debug_name (node));
              gchar *p = name;
              while (*p)
                {
                  if (*p == ' ' ||
                      *p == '-')
                    *p='_';
                  p++;
                }
              gegl_add_graph (string, node, name);
              g_free (name);
            }

          g_string_append_printf (string, "op_%p [label=\"", node);


          g_string_append_printf (string, "{{");

	  
	    {
	      GSList *pads = gegl_node_get_pads (node);
	      GSList *entry = pads;
              gboolean got_output = FALSE;
	      while (entry)
		{
		  GeglPad *pad = entry->data;
                  if (gegl_pad_is_output (pad))
                    {
                      if (got_output)
                        {
                          g_string_append (string, "|");
                        }
                      got_output = TRUE;
                      g_string_append_printf (string, "<%s>%s",
                                              gegl_pad_get_name (pad),
                                              gegl_pad_get_name (pad));
                    }
		  entry = g_slist_next (entry);
		}
	    }
          g_string_append_printf (string, "}|{%s|{", gegl_node_get_debug_name (node));

	  if(1)
            {
              guint n_properties;
              GParamSpec **properties = gegl_node_get_properties (node, &n_properties);
              gint i;
              for (i=0;i<n_properties;i++)
                {
                  const gchar *name = properties[i]->name;
                  GValue tvalue={0,};
                  GValue svalue={0,};

                  if (properties[i]->value_type == GEGL_TYPE_BUFFER)
                    continue;

                  g_value_init (&svalue, G_TYPE_STRING);
                  g_value_init (&tvalue, properties[i]->value_type);

                  gegl_node_get_property (node, name, &tvalue);

                  if (g_value_transform (&tvalue, &svalue))
                    { 
                      gchar *sval = g_value_dup_string (&svalue);
                      if (sval && strlen (sval) > 30)
                        {
                          sval[28]='.';
                          sval[29]='.';
                          sval[30]='\0';
                        }
                      if (sval)
                        {
                          g_string_append_printf (string, "%s=%s\\n", name, sval);
                          g_free (sval);
                        }
                      g_value_unset (&svalue);
                    }
                  g_value_unset (&tvalue);

                }
              g_free (properties);
            }

          g_string_append_printf (string, "}}|{");

	    {
	      GSList *pads = gegl_node_get_pads (node);
	      GSList *entry = pads;
              gboolean got_input = FALSE;
	      while (entry)
		{
		  GeglPad *pad = entry->data;
                  if (gegl_pad_is_input (pad))
                    {
                      if (got_input)
                        {
                          g_string_append (string, "|");
                        }
                      got_input = TRUE;
                      g_string_append_printf (string, "<%s>%s",
                                              gegl_pad_get_name (pad),
                                              gegl_pad_get_name (pad));
                    }
		  entry = g_slist_next (entry);
		}
	    }

          g_string_append_printf (string, "}}\"\n shape=\"record\"];\n");


          entry = g_slist_next (entry);
        }

      g_slist_free (nodes);
    }

    {
      GSList *nodes = gegl_node_get_children (graph);
      GSList *entry = nodes;

      while (entry)
        {
          GeglNode *node = entry->data;
          {
             GSList *connections = gegl_node_get_sinks (node);
             GSList *entry;
             entry = connections;

             while (entry)
               {
                GeglConnection *connection = entry->data;
                gchar buf[512];
                GeglNode *source;
                GeglNode *sink;
		GeglPad  *source_pad;
		GeglPad  *sink_pad;

                source = gegl_connection_get_source_node (connection);
                sink = gegl_connection_get_sink_node (connection);
                source_pad = gegl_connection_get_source_pad (connection);
                sink_pad = gegl_connection_get_sink_pad (connection);

                sprintf (buf, "op_%p:%s -> op_%p:%s;\n", source,
		   gegl_pad_get_name (source_pad), sink,
		   gegl_pad_get_name (sink_pad));
                g_string_append (string, buf);
                entry = g_slist_next (entry);
               }
          }
          entry = g_slist_next (entry);
        }
      g_slist_free (nodes);
    }
  g_string_append_printf (string, "}\n");
}

gchar *
gegl_to_dot (GeglNode *node)
{
  GString   *string = g_string_new ("digraph gegl { graph [ rankdir = \"BT\"];\n");

  gegl_add_graph (string, node, "GEGL");
  g_string_append (string, "}");
  return g_string_free (string, FALSE);
}
