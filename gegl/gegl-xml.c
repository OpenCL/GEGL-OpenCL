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

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "gegl-types.h"
#include "gegl-graph.h"
#include "gegl-node.h"
#include "gegl-operation.h"
#include "gegl-pad.h"
#include "gegl-color.h"
#include "gegl-instrument.h"
#include "gegl-xml.h"
#include "gegl-paramspecs.h"

typedef struct _ParseData ParseData;

enum {
  STATE_NONE=0,
  STATE_TREE_NORMAL,
  STATE_TREE_FIRST_CHILD
};

struct _ParseData {
  gint        state;
  const gchar      *path_root;
  GeglNode   *gegl;
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

  if (!strcmp (element_name, "gegl")||
      !strcmp (element_name, "image"))
    {
      GeglNode *new = g_object_new (GEGL_TYPE_NODE, NULL);
      if (pd->gegl == NULL)
        {
          pd->gegl = new;
        }
      else
        {
        }
      pd->state=STATE_TREE_NORMAL;
      pd->parent = g_list_prepend (pd->parent, new);

      gegl_node_get_output_proxy (new, "output"); /* creates the pad if it doesn't exist */
      if (pd->iter)
        gegl_node_connect_from (pd->iter, "input", new, "output");
      pd->iter = gegl_node_get_output_proxy (new, "output");
    }
  else if (!strcmp (element_name, "graph"))
    {
      /*NYI*/
    }
  else
    {
      GeglNode *new;

      if (!strcmp (element_name, "node"))
        {
          new = gegl_node_new_child (pd->gegl,
                                     "operation", name2val (a, v, "operation"),
                                     NULL);
        }
      else
        {
          new = gegl_node_new_child (pd->gegl,
                                     "operation", element_name,
                                     NULL);
        }
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
          else if (strcmp (*a, "operation"))
            {
              GeglOperation *operation;
              GParamSpec    *paramspec;
              set_clone_prop_as_well:

              operation = new->operation;
              paramspec = g_object_class_find_property (G_OBJECT_GET_CLASS (operation), *a);

              if (!paramspec)
                {
                   g_warning ("property %s not found for %s",
                              *a, gegl_node_get_debug_name (new));
                }
              else if (g_type_is_a (G_PARAM_SPEC_TYPE (paramspec),
                                    GEGL_TYPE_PARAM_PATH))
                {
                  gchar buf[PATH_MAX];

                  if (g_path_is_absolute (*v))
                    {
                      gegl_node_set (new, *a, *v, NULL);
                    }
                  else if (pd->path_root)
                    {
                      gchar absolute_path[PATH_MAX];
                      sprintf (buf, "%s/%s", pd->path_root, *v);
                      realpath (buf, absolute_path);
                      gegl_node_set (new, *a, absolute_path, NULL);
                    }
                  else
                    {
                      gchar absolute_path[PATH_MAX];
                      sprintf (buf, "./%s", *v);
                      realpath (buf, absolute_path);
                      gegl_node_set (new, *a, absolute_path, NULL);
                    }
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
              else if (paramspec->value_type == G_TYPE_BOOLEAN)
                {
                  if (!strcmp (*v, "true") ||
                      !strcmp (*v, "TRUE") ||
                      !strcmp (*v, "YES") ||
                      !strcmp (*v, "yes") ||
                      !strcmp (*v, "y") ||
                      !strcmp (*v, "Y") ||
                      !strcmp (*v, "1") ||
                      !strcmp (*v, "on")
                      )
                    {
                      gegl_node_set (new, *a, TRUE, NULL);
                    }
                  else
                    {
                      gegl_node_set (new, *a, FALSE, NULL);
                    }
                }
              else if (paramspec->value_type == GEGL_TYPE_COLOR)
                {
                  GeglColor *color = g_object_new (GEGL_TYPE_COLOR,
                                                   "string", *v,
                                                   NULL);

                  gegl_node_set (new, *a, color, NULL);

                  g_object_unref (color);
                }
              else
                {
                  g_warning ("operation desired unknown parapspec type for %s",
                             *a);
                }
            }
        a++;
        v++;
      }

      if (pd->state == STATE_TREE_FIRST_CHILD)
        {
          gegl_node_connect_from (pd->iter, "aux", new, "output");
        }
      else
        {
          if (pd->iter)
            gegl_node_connect_from (pd->iter, "input", new, "output");
        }
      pd->parent = g_list_prepend (pd->parent, new);
      pd->state = STATE_TREE_FIRST_CHILD;
      pd->iter = new;
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

  if (!strcmp (element_name, "gegl")||
      !strcmp (element_name, "image"))
    {
      /*ignored*/
    }
  else if (!strcmp (element_name, "tree") ||
           !strcmp (element_name, "layers"))
    {
      if (gegl_node_get_pad (pd->iter, "input"))
        {
          gegl_node_connect_from (pd->iter, "input",
                                  gegl_node_get_input_proxy (GEGL_NODE (pd->parent->data), "input"),
                                  "output");
          pd->iter = gegl_node_get_input_proxy (GEGL_NODE (pd->parent->data),
                                                "input");
        }
      else
        {
          pd->iter = NULL;
        }
      pd->parent = g_list_remove (pd->parent, pd->parent->data);
      pd->state = STATE_TREE_NORMAL;
    }
  else if (!strcmp (element_name, "graph"))
    {
      /*NYI*/
    }
  else if (1 || !strcmp (element_name, "node"))
    {
      pd->iter   = pd->parent->data;
      pd->parent = g_list_remove (pd->parent, pd->parent->data);
      pd->state = STATE_TREE_NORMAL;
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
  gint       line_number;
  gint       char_number;

  pd = user_data;
  g_markup_parse_context_get_position (context, &line_number, &char_number);
  g_warning ("XML Parse error %i:%i: %s",
             line_number, char_number, error->message);
}

static GMarkupParser parser={
  start_element,
  end_element,
  NULL,
  NULL,
  error
};

static void each_ref (gpointer value,
                      gpointer user_data)
{
  ParseData *pd=user_data;
  GeglNode  *dest_node = value;
  gchar     *ref;
  GeglNode  *source_node;

  gegl_node_get (dest_node, "ref", &ref, NULL);
  source_node = g_hash_table_lookup (pd->ids, ref);

  gegl_node_connect_from (dest_node, "input", source_node, "output");
}

GeglNode *gegl_parse_xml (const gchar *xmldata,
                          const gchar *path_root)
{
  glong     time = gegl_ticks ();
  GeglNode *ret;
  ParseData           *pd;
  GMarkupParseContext *context;
  pd = g_new0 (ParseData, 1);

  pd->ids = g_hash_table_new_full (g_str_hash,
                                   g_str_equal,
                                   g_free,
                                   NULL);
  pd->refs = NULL;
  pd->path_root = path_root;

  context = g_markup_parse_context_new (&parser, 0, pd, NULL);
  g_markup_parse_context_parse (context, xmldata, strlen (xmldata), NULL);

  /* connect clones */
  g_list_foreach (pd->refs, each_ref, pd);

  ret = GEGL_NODE (pd->gegl);
  g_free (pd);

  time = gegl_ticks () - time;
  gegl_instrument ("gegl", "gegl_parse_xml", time);
  return ret;
}

/****/


#define ind do{gint i;for(i=0;i<indent;i++)g_string_append(ss->buf, " ");}while(0)

typedef struct _SerializeState SerializeState;
struct _SerializeState
{
  GString     *buf;
  const gchar *path_root;
  gint         clone_count;
  GHashTable  *clones;
  gboolean     terse;
};

static void tuple (GString     *buf,
                   const gchar *key,
                   const gchar *value)
{
  g_assert (key);

  if (value)
    {
      gchar *text = g_markup_escape_text (value, -1);
      gchar *p;

      g_string_append_c (buf, ' ');
      g_string_append (buf, key);
      g_string_append_c (buf, '=');
      g_string_append_c (buf, '\'');
      for (p=text;*p;p++)
        {
          if (*p=='\n')
            g_string_append (buf, "&#10;");
          else
            g_string_append_c (buf, *p);

        }
      g_string_append_c (buf, '\'');

      g_free (text);
    }
}

static void encode_node_attributes (SerializeState *ss,
                                    GeglNode       *node,
                                    const           gchar *id)
{
  GParamSpec ** properties;
  guint n_properties;
  gchar *class;
  gint i;

  properties = gegl_list_properties (gegl_node_get_operation (node), &n_properties);

  if (!ss->terse)
    {
      gegl_node_get (node, "operation", &class, NULL);
      tuple (ss->buf, "operation", class);
      g_free (class);
    }

  for (i=0; i<n_properties; i++)
    {
      if (strcmp (properties[i]->name, "input") &&
          strcmp (properties[i]->name, "output") &&
          strcmp (properties[i]->name, "aux"))
        {

          if (g_type_is_a (G_PARAM_SPEC_TYPE (properties[i]),
                           GEGL_TYPE_PARAM_PATH))
            {
              gchar *value;
              gegl_node_get (node, properties[i]->name, &value, NULL);

              if (value)
                {
                  if (ss->path_root &&
                   !strncmp (ss->path_root, value, strlen (ss->path_root)))
                    {
                      tuple (ss->buf, properties[i]->name, &value[strlen(ss->path_root)+1]);
                    }
                  else
                    {
                      tuple (ss->buf, properties[i]->name, value);
                    }
                }

              g_free (value);
            }
          else if (properties[i]->value_type == G_TYPE_FLOAT)
            {
              gfloat value;
              gchar str[64];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              sprintf (str, "%f", value);
              tuple (ss->buf, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_DOUBLE)
            {
              gdouble value;
              gchar str[64];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              sprintf (str, "%f", value);
              tuple (ss->buf, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_INT)
            {
              gint value;
              gchar str[64];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              sprintf (str, "%i", value);
              tuple (ss->buf, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_BOOLEAN)
            {
              gboolean value;
              gegl_node_get (node, properties[i]->name, &value, NULL);
              if (value)
                {
                  tuple (ss->buf, properties[i]->name, "true");
                }
              else
                {
                  tuple (ss->buf, properties[i]->name, "false");
                }
            }
          else if (properties[i]->value_type == G_TYPE_STRING)
            {
              gchar *value;
              gegl_node_get (node, properties[i]->name, &value, NULL);
              tuple (ss->buf, properties[i]->name, value);
              g_free (value);
            }
          else if (properties[i]->value_type == GEGL_TYPE_COLOR)
            {
              GeglColor *color;
              gchar *value;
              gegl_node_get (node, properties[i]->name, &color, NULL);
              g_object_get (color, "string", &value, NULL);
              g_object_unref (color);
              tuple (ss->buf, properties[i]->name, value);
              g_free (value);
            }
          else
            {
              g_warning ("%s: serialization of %s proeprties not implemented",
                properties[i]->name, g_type_name (properties[i]->value_type));
            }
        }
    }
    if (id != NULL)
      tuple (ss->buf, "id", id);

  g_free (properties);
}

static void add_stack (SerializeState *ss,
                       gint            indent,
                       GeglNode       *head)
{

  if (GEGL_IS_NODE (head))
    {
      GeglNode *iter = head;

      while (iter)
        {
          GeglPad *input, *aux;
          const gchar *id=NULL;

          input = gegl_node_get_pad (iter, "input");
          aux = gegl_node_get_pad (iter, "aux");

          if (gegl_node_get_num_sinks (iter)>1)
            {
              const gchar *new_id = g_hash_table_lookup (ss->clones, iter);
              if (new_id)
                {
                  ind; g_string_append (ss->buf, "<clone ref='");
                  g_string_append (ss->buf, new_id);
                  g_string_append (ss->buf, "'/>\n");
                  return; /* terminate the stack, the cloned part is already
                             serialized */
                }
              else
                {
                  gchar temp_id[64];
                  sprintf (temp_id, "clone%i", ss->clone_count++);
                  id = g_strdup (temp_id);
                  g_hash_table_insert (ss->clones, iter, (gchar*)id);
                  /* the allocation is freed by the hash table */
                }
            }

          if (aux &&
              gegl_pad_get_connected_to (aux))
            {
              GeglPad *source_pad;
              GeglNode *source_node;
              source_pad = gegl_pad_get_connected_to (aux);
              source_node = gegl_pad_get_node (source_pad);
              {
                GeglNode *graph = g_object_get_data (G_OBJECT (source_node),
                                                     "graph");
                if (graph)
                  source_node = graph;
              }
              ind;if (ss->terse)
                {
                  g_string_append (ss->buf, "<");
                  g_string_append (ss->buf, gegl_node_get_operation (iter));
                }
              else
                {
                  g_string_append (ss->buf, "<node");
                }
              encode_node_attributes (ss, iter, id);
              g_string_append (ss->buf, ">\n");
              add_stack (ss, indent+4, source_node);

              ind;if (ss->terse)
                {
                  g_string_append (ss->buf, "</");
                  g_string_append (ss->buf, gegl_node_get_operation (iter));
                  g_string_append (ss->buf, ">\n");
                }
              else
                {
                  g_string_append (ss->buf, "</node\n>");
                }
            }
          else
            {
              gchar *class;
              gegl_node_get (iter, "operation", &class, NULL);

              if (class)
                {
                  if (strcmp (class, "nop") &&
                      strcmp (class, "clone"))
                    {
                      ind;if (ss->terse)
                        {
                          g_string_append (ss->buf, "<");
                          g_string_append (ss->buf, class);
                        }
                      else
                        {
                          g_string_append (ss->buf, "<node");
                      }

                      encode_node_attributes (ss, iter, id);
                      g_string_append (ss->buf, "/>\n");
                    }
                  g_free (class);
                }
            }
          id = NULL;
          if (input)
            {
              GeglPad *source_pad;
              source_pad = gegl_pad_get_connected_to (input);
              if (source_pad)
                {
                  GeglNode *source_node = gegl_pad_get_node (source_pad);
                  GeglNode *graph = g_object_get_data (G_OBJECT (source_node),
                                                       "graph");
                  if (graph)
                    source_node = graph;
                  iter = source_node;
                }
              else
                iter = NULL;
            }
          else
            iter = NULL;
        }
    }
}

static void free_clone_id (gpointer key,
                           gpointer value,
                           gpointer user_data)
{
  g_free (value);
}

gchar *
gegl_to_xml (GeglNode    *gegl,
             const gchar *path_root)
{
  gchar *ret;
  SerializeState *ss = g_new0 (SerializeState, 1);
  ss->buf = g_string_new ("");
  ss->clones = g_hash_table_new (NULL, NULL);
  ss->terse = TRUE;
  ss->path_root = path_root;

  gegl = gegl_node_get_output_proxy (gegl, "output");

  g_string_append (ss->buf, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (ss->buf, "<gegl>\n");

  add_stack (ss, 2, gegl);

  g_string_append (ss->buf, "</gegl>\n");

  g_hash_table_foreach (ss->clones, free_clone_id, NULL);
  g_hash_table_destroy (ss->clones);

  ret=g_string_free (ss->buf, FALSE);
  g_free (ss);
  return ret;
}
