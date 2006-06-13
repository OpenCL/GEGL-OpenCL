/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */

/* TODO:
 *   - Add a graph element to the XML, it is important that the graph can be
 *     used in place of a node anywhere in a tree (or a graph). Routing nodes
 *     should probably be used for this. The routing nodes would be OpNop
 *     nodes, or subclasses of OpNop (with just a different name).
 *   - Create code to write out an in memory graph to either a graph, or a tree
 *     with embedded graphs for things that cannot be serialized as a tree with
 *     clones.
 */

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <string.h>
#include "gegl-types.h"
#include "gegl-graph.h"
#include "gegl-node.h"

typedef struct _ParseData ParseData;

enum {
  STATE_NONE=0,
  STATE_TREE_NORMAL,
  STATE_TREE_FIRST_CHILD,
};


struct _ParseData {
  gint        state;
  GeglGraph  *gegl;
  GeglNode   *root;  /*< the top of this projection */
  GeglNode   *iter;  /*< the iterator we're either connecting to input|aux of
                         depending on context */
  GList      *parent; /*< a stack of parents, as we are recursing into aux
                         branches */

  GHashTable *ids;
  GList      *refs;
};

static const gchar *name2val (const gchar **attribute_names,
                              const gchar **attribute_values,
                              const gchar  *name)
{
  while (*attribute_names)
    {
      if (!strcmp (*attribute_names, name))
        {
          return *attribute_values;
        }

      attribute_names++;
      attribute_values++;
    }
  return NULL;
}

static void start_element (GMarkupParseContext *context,
                           const gchar         *element_name,
                           const gchar        **attribute_names,
                           const gchar        **attribute_values,
                           gpointer             user_data,
                           GError             **error)
{
  const gchar **a=attribute_names;
  const gchar **v=attribute_values;
  ParseData *pd;
 
  pd = user_data;

  if (!strcmp (element_name, "gegl"))
    {
      g_assert (pd->gegl == NULL);
      pd->gegl = g_object_new (GEGL_TYPE_GRAPH, NULL);
    }
  else if (!strcmp (element_name, "tree"))
    {
      g_assert (pd->root == NULL);
      pd->state=STATE_TREE_NORMAL;
    }
  else if (!strcmp (element_name, "node"))
    {
      GeglNode *new = gegl_graph_create_node (pd->gegl,
             "class", name2val (a, v, "class"),
             NULL);
      g_assert (new);

      while (*a)
        {
          if (!strcmp (*a, "name"))
            {
              g_object_set (new, *a, *v, NULL);
            }
          else if (!strcmp (*a, "id"))
            {
              g_hash_table_insert (pd->ids, g_strdup (*v), new);
            }
          else if (!strcmp (*a, "ref"))
            {
              pd->refs = g_list_append (pd->refs, new);
              goto set_clone_prop_as_well;
            }
          else if (strcmp (*a, "class"))
            {
              GeglOperation *operation;
              GParamSpec    *paramspec;
              set_clone_prop_as_well:

              operation = new->operation;
              paramspec = g_object_class_find_property (G_OBJECT_GET_CLASS (operation), *a);
              if (!paramspec)
                {
                  g_warning ("property %s not found for %s", *a, gegl_node_get_debug_name (new));
                }
              else if (paramspec->value_type == G_TYPE_INT)
                {
                  gegl_node_set (new, *a, atoi (*v), NULL);
                }
              else if (paramspec->value_type == G_TYPE_FLOAT ||
                       paramspec->value_type == G_TYPE_DOUBLE)
                {
                  gegl_node_set (new, *a, atof (*v), NULL);
                }
              else if (paramspec->value_type == G_TYPE_STRING)
                {
                  gegl_node_set (new, *a, *v, NULL);
                }
              else
                {
                  g_warning ("operation desired unknown parapspec type for %s", *a);
                }
            }
        a++;
        v++;
      }

      if (pd->root == NULL)
        {
          pd->root = new;
          pd->state = STATE_TREE_FIRST_CHILD;
        }
      else if (pd->state == STATE_TREE_FIRST_CHILD)
        {
          gegl_node_connect (pd->iter, "aux", new, "output");
        }
      else
        {
          gegl_node_connect (pd->iter, "input", new, "output");
        }
      pd->parent = g_list_prepend (pd->parent, new);
      pd->state = STATE_TREE_FIRST_CHILD;
      pd->iter = new;
    }
  else if (!strcmp (element_name, "graph"))
    {
      /*NYI*/
    }
}

/* Called for close tags </foo> */
static void end_element (GMarkupParseContext *context,
                         const gchar         *element_name,
                         gpointer             user_data,
                         GError             **error)
{
  ParseData *pd;
 
  pd = user_data;

  if (!strcmp (element_name, "gegl"))
    {
      /*ignored*/
    }
  else if (!strcmp (element_name, "tree"))
    {
    }
  else if (!strcmp (element_name, "node"))
    {
      pd->iter   = pd->parent->data;
      pd->parent = g_list_remove (pd->parent, pd->parent->data);
      pd->state = STATE_TREE_NORMAL;
    }
  else if (!strcmp (element_name, "graph"))
    {
      /*NYI*/
    }
}

/* Called on error, including one set by other
 * methods in the vtable. The GError should not be freed.
 */
static void error (GMarkupParseContext *context,
                   GError              *error,
                   gpointer             user_data)
{
  ParseData *pd;
 
  pd = user_data;
  gint line_number;
  gint char_number;
  g_markup_parse_context_get_position (context, &line_number, &char_number);
  g_warning ("XML Parse error %i:%i: %s", line_number, char_number, error->message);
}

static GMarkupParser parser={
  start_element,
  end_element,
  NULL,
  NULL,
  error
};

void each_ref (gpointer value,
               gpointer user_data)
{
  ParseData *pd=user_data;
  GeglNode  *dest_node = value;
  gchar     *ref;
  GeglNode  *source_node;

  gegl_node_get (dest_node, "ref", &ref, NULL);
  source_node = g_hash_table_lookup (pd->ids, ref);

  gegl_node_connect (dest_node, "input", source_node, "output");
}

GeglNode *gegl_xml_parse (const gchar *xmldata)
{
  GeglNode *ret;
  ParseData           *pd;
  GMarkupParseContext *context;
  pd = g_malloc0 (sizeof (ParseData));

  pd->ids = g_hash_table_new_full (g_str_hash,
                                   g_str_equal,
                                   g_free,
                                   NULL);
  pd->refs = NULL;

  context = g_markup_parse_context_new (&parser, 0, pd, NULL);
  g_markup_parse_context_parse (context, xmldata, strlen (xmldata), NULL);

  /* connect clones */
  g_list_foreach (pd->refs, each_ref, pd);
  
  ret = GEGL_NODE (pd->gegl);
  /* hacky redirect */
  gegl_node_add_pad (ret, gegl_node_get_pad (pd->root, "process"));
  g_free (pd);
  return ret;
}

