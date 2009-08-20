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
 * Copyright 2006 Øyvind Kolås
 */
#include "config.h"

#include <glib.h>
#include <glib-object.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include "operation/gegl-operation.h"
#include "property-types/gegl-color.h"
#include "property-types/gegl-curve.h"
#include "property-types/gegl-path.h"
#include "property-types/gegl-paramspecs.h"
#include "gegl-instrument.h"
#include "gegl-xml.h"

#ifdef G_OS_WIN32
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#define realpath(a, b)    _fullpath (b, a, _MAX_PATH)
#endif

typedef struct _ParseData ParseData;

enum
{
  STATE_NONE = 0,
  STATE_TREE_NORMAL,
  STATE_TREE_FIRST_CHILD
};

struct _ParseData
{
  gint         state;
  const gchar *path_root;
  GeglNode    *gegl;
  gchar       *param;/*< the param we are setting (NULL when not in <param></param>) */
  GeglNode    *iter; /*< the iterator we're either connecting to input|aux of
                         depending on context */
  GList       *parent;/*< a stack of parents, as we are recursing into aux
                         branches */
  GeglCurve   *curve;/*< the curve whose points we are parsing */

  GHashTable  *ids;
  GList       *refs;
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

static void param_set (ParseData   *pd,
                       GeglNode    *new,
                       const gchar *param_name,
                       const gchar *param_value)
{
  if (!strcmp (param_name, "name"))
    {
      g_object_set (new, param_name, param_value, NULL);
    }
  else if (!strcmp (param_name, "id"))
    {
      g_hash_table_insert (pd->ids, g_strdup (param_value), new);
    }
  else if (!strcmp (param_name, "ref"))
    {
      pd->refs = g_list_append (pd->refs, new);
      goto set_clone_prop_as_well;
    }
  else if (strcmp (param_name, "operation") &&
           strcmp (param_name, "type"))
    {
      GeglOperation *operation;
      GParamSpec    *paramspec;
set_clone_prop_as_well:

      operation = new->operation;
      paramspec = g_object_class_find_property (G_OBJECT_GET_CLASS (operation), param_name);

      if (!paramspec)
        {
          g_warning ("property %s not found for %s",
                     param_name, gegl_node_get_debug_name (new));
        }
      else if (g_type_is_a (G_PARAM_SPEC_TYPE (paramspec),
                            GEGL_TYPE_PARAM_FILE_PATH))
        {
          gchar buf[PATH_MAX];

          if (g_path_is_absolute (param_value))
            {
              gegl_node_set (new, param_name, param_value, NULL);
            }
          else if (pd->path_root)
            {
              gchar absolute_path[PATH_MAX];
              g_snprintf (buf, sizeof (buf),
                          "%s/%s", pd->path_root, param_value);
              realpath (buf, absolute_path);
              gegl_node_set (new, param_name, absolute_path, NULL);
            }
          else
            {
              gchar absolute_path[PATH_MAX];
              g_snprintf (buf, sizeof (buf), "./%s", param_value);
              realpath (buf, absolute_path);
              gegl_node_set (new, param_name, absolute_path, NULL);
            }
        }
      else if (paramspec->value_type == G_TYPE_INT)
        {
          gegl_node_set (new, param_name, atoi (param_value), NULL);
        }
      else if (paramspec->value_type == G_TYPE_FLOAT ||
               paramspec->value_type == G_TYPE_DOUBLE)
        {
          gegl_node_set (new, param_name, g_ascii_strtod (param_value, NULL), NULL);
        }
      else if (paramspec->value_type == G_TYPE_STRING)
        {
          gegl_node_set (new, param_name, param_value, NULL);
        }
      else if (paramspec->value_type == G_TYPE_BOOLEAN)
        {
          if (!strcmp (param_value, "true") ||
              !strcmp (param_value, "TRUE") ||
              !strcmp (param_value, "YES") ||
              !strcmp (param_value, "yes") ||
              !strcmp (param_value, "y") ||
              !strcmp (param_value, "Y") ||
              !strcmp (param_value, "1") ||
              !strcmp (param_value, "on")
          )
            {
              gegl_node_set (new, param_name, TRUE, NULL);
            }
          else
            {
              gegl_node_set (new, param_name, FALSE, NULL);
            }
        }
      else if (paramspec->value_type == GEGL_TYPE_COLOR)
        {
          GeglColor *color = g_object_new (GEGL_TYPE_COLOR,
                                           "string", param_value,
                                           NULL);

          gegl_node_set (new, param_name, color, NULL);

          g_object_unref (color);
        }
      else if (paramspec->value_type == GEGL_TYPE_CURVE)
        {
	  if (pd->curve)
	    {
	      gegl_node_set (new, param_name, pd->curve, NULL);

	      g_object_unref (pd->curve);
	      pd->curve = NULL;
	    }
	}
      else if (paramspec->value_type == GEGL_TYPE_PATH)
        {
          GeglPath *path = gegl_path_new ();
          gegl_path_parse_string (path, param_value);
	  gegl_node_set (new, param_name, path, NULL);
	}
      else
        {
          g_warning ("operation desired unknown parapspec type for %s",
                     param_name);
        }
    }
}

static void start_element (GMarkupParseContext *context,
                           const gchar         *element_name,
                           const gchar        **attribute_names,
                           const gchar        **attribute_values,
                           gpointer             user_data,
                           GError             **error)
{
  const gchar **a  = attribute_names;
  const gchar **v  = attribute_values;
  ParseData    *pd = user_data;

  if (!strcmp (element_name, "gegl") ||
      !strcmp (element_name, "image"))
    {
      GeglNode *new = g_object_new (GEGL_TYPE_NODE, "operation", "gegl:nop", NULL);
      if (pd->gegl == NULL)
        {
          pd->gegl = new;
        }
      else
        {
        }
      pd->state  = STATE_TREE_NORMAL;
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
  else if (!strcmp (element_name, "params"))
    {
      /*g_warning ("go a param set");*/
    }
  else if (!strcmp (element_name, "param"))
    {
      if (pd->param != NULL)
        g_warning ("eek, haven't cleared previous param");
      g_assert (name2val (a, v, "name"));
      pd->param = g_strdup (name2val (a, v, "name"));
    }
  else if (!strcmp (element_name, "curve"))
    {
      if (pd->curve != NULL)
	g_warning ("we haven't cleared previous curve");
      g_assert (name2val (a, v, "ymin"));
      g_assert (name2val (a, v, "ymax"));
      pd->curve = gegl_curve_new (g_ascii_strtod (name2val (a, v, "ymin"), NULL),
				  g_ascii_strtod (name2val (a, v, "ymax"), NULL));
    }
  else if (!strcmp (element_name, "curve-point"))
    {
      if (!pd->curve)
	g_warning ("curve not instantiated");
      else
        {
	  g_assert (name2val (a, v, "x"));
	  g_assert (name2val (a, v, "y"));

	  gegl_curve_add_point (pd->curve,
				g_ascii_strtod (name2val (a, v, "x"), NULL),
				g_ascii_strtod (name2val (a, v, "y"), NULL));
	}
    }
  else if (!strcmp (element_name, "link") ||
           !strcmp (element_name, "links") ||
           !strcmp (element_name, "stack") ||
           !strcmp (element_name, "launcher") ||
           !strcmp (element_name, "launchers") ||
           !strcmp (element_name, "source") ||
           !strcmp (element_name, "destination"))
    {
      /* ignore */
    }
  else
    {
      GeglNode *new;

      if (!strcmp (element_name, "clone"))
        {
          new = gegl_node_new_child (pd->gegl,
                                     "operation", "gegl:clone",
                                     NULL);
        }
      else if (!strcmp (element_name, "layer"))
        {
          new = gegl_node_new_child (pd->gegl,
                                     "operation", "gegl:layer",
                                     NULL);
        }
      else if (!strcmp (element_name, "node"))
        {
          new = gegl_node_new_child (pd->gegl,
                                     "operation", name2val (a, v, "operation"),
                                     NULL);
        }
      else if (!strcmp (element_name, "filter"))
        {
          new = gegl_node_new_child (pd->gegl,
                                     "operation", name2val (a, v, "type"),
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
          param_set (pd, new, *a, *v);
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
      pd->state  = STATE_TREE_FIRST_CHILD;
      pd->iter   = new;
    }
}

static void text (GMarkupParseContext *context,
                  const gchar         *text,
                  gsize                text_len,
                  gpointer             user_data,
                  GError             **error)
{
  ParseData *pd = user_data;

  if (pd->param && pd->iter && !pd->curve)
    {
      param_set (pd, pd->iter, pd->param, text);
    }
}

/* Called for close tags </foo> */
static void end_element (GMarkupParseContext *context,
                         const gchar         *element_name,
                         gpointer             user_data,
                         GError             **error)
{
  ParseData *pd = user_data;

  if (!strcmp (element_name, "gegl") ||
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
      pd->parent = g_list_delete_link (pd->parent, pd->parent);
      pd->state  = STATE_TREE_NORMAL;
    }
  else if (!strcmp (element_name, "graph"))
    {
      /*NYI*/
    }
  else if (!strcmp (element_name, "param"))
    {
      g_free (pd->param);
      pd->param = NULL;
    }
  else if (!strcmp (element_name, "curve"))
    {
      g_assert (pd->param && pd->iter);
      param_set (pd, pd->iter, pd->param, NULL);
    }
  else if (!strcmp (element_name, "link") ||
           !strcmp (element_name, "links") ||
           !strcmp (element_name, "launcher") ||
           !strcmp (element_name, "launchers") ||
           !strcmp (element_name, "source") ||
           !strcmp (element_name, "destination") ||
           !strcmp (element_name, "stack") ||
           !strcmp (element_name, "params") ||
	   !strcmp (element_name, "curve-point"))
    {
      /* ignore */
    }
  else if (1 ||
           !strcmp (element_name, "node") ||
           !strcmp (element_name, "filter"))
    {
      pd->iter   = pd->parent->data;
      pd->parent = g_list_delete_link (pd->parent, pd->parent);
      pd->state  = STATE_TREE_NORMAL;
    }
}

/* Called on error, including one set by other
 * methods in the vtable. The GError should not be freed.
 */
static void error (GMarkupParseContext *context,
                   GError              *error,
                   gpointer             user_data)
{
  gint  line_number;
  gint  char_number;

  g_markup_parse_context_get_position (context, &line_number, &char_number);
  g_warning ("XML Parse error %i:%i: %s",
             line_number, char_number, error->message);
}

static GMarkupParser parser = {
  start_element,
  end_element,
  text,
  NULL,
  error
};

static void each_ref (gpointer value,
                      gpointer user_data)
{
  ParseData *pd        = user_data;
  GeglNode  *dest_node = value;
  gchar     *ref;
  GeglNode  *source_node;

  gegl_node_get (dest_node, "ref", &ref, NULL);
  source_node = g_hash_table_lookup (pd->ids, ref);

  gegl_node_connect_from (dest_node, "input", source_node, "output");
}

GeglNode *gegl_node_new_from_xml (const gchar *xmldata,
                                  const gchar *path_root)
{
  glong                time = gegl_ticks ();
  ParseData            pd   = { 0, };
  GMarkupParseContext *context;

  pd.ids       = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  pd.refs      = NULL;
  pd.path_root = path_root;

  context = g_markup_parse_context_new (&parser, 0, &pd, NULL);
  g_markup_parse_context_parse (context, xmldata, strlen (xmldata), NULL);

  /* connect clones */
  g_list_foreach (pd.refs, each_ref, &pd);

  g_markup_parse_context_free (context);
  g_hash_table_destroy (pd.ids);

  time = gegl_ticks () - time;
  gegl_instrument ("gegl", "gegl_parse_xml", time);

  return GEGL_NODE (pd.gegl);
}

GeglNode *
gegl_node_new_from_file (const gchar   *path)
{
  GeglNode *node;
  GError   *err = NULL;
  gchar    *script;
  gchar    *temp1;
  gchar    *temp2;
  gchar    *path_root;

  g_assert (path);

  temp1 = g_strdup (path);
  temp2 = g_strdup (g_path_get_dirname (temp1));
  path_root = g_strdup (realpath (temp2, NULL));

  g_file_get_contents (path, &script, NULL, &err);
  if (err != NULL)
    {
      g_warning ("Unable to read file: %s", err->message);
      g_error_free (err);
      return NULL;
    }

  node = gegl_node_new_from_xml (script, path_root);

  g_free (temp1);
  g_free (temp2);
  g_free (path_root);
  return node;
}

/****/


#define ind    do { gint i; for (i = 0; i < indent; i++) g_string_append (ss->buf, " "); } while (0)

typedef struct _SerializeState SerializeState;
struct _SerializeState
{
  GString     *buf;
  const gchar *path_root;
  gint         clone_count;
  GHashTable  *clones;
  gboolean     terse;
};

static void
free_clone_id (gpointer key,
               gpointer value,
               gpointer user_data)
{
  g_free (value);
}

static void
xml_attr (GString     *buf,
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
      for (p = text; *p; p++)
        {
          if (*p == '\n')
            g_string_append (buf, "&#10;");
          else
            g_string_append_c (buf, *p);
        }
      g_string_append_c (buf, '\'');

      g_free (text);
    }
}

static void
xml_param_start (SerializeState *ss,
                 gint            indent,
                 const gchar    *key)
{
  g_assert (key);
  ind; g_string_append (ss->buf, "<param name='");
  g_string_append (ss->buf, key);
  g_string_append (ss->buf, "'>");
}

static void
xml_param_text (SerializeState *ss,
                const gchar    *value)
{
  gchar *text;
  /*gchar *p;*/

  g_assert (value);

  /* Why isn't this used here??? */
  text = g_markup_escape_text (value, -1);

  g_string_append (ss->buf, value);
  /*for (p=text;*p;p++)
    {
    if (*p=='\n')
    g_string_append (ss->buf, "&#10;");
    else
    g_string_append_c (ss->buf, *p);

    }*/

  g_free (text);
}

static void
xml_param_end (SerializeState *ss)
{
  g_string_append (ss->buf, "</param>\n");
}

static void
xml_param (SerializeState *ss,
           gint            indent,
           const gchar    *key,
           const gchar    *value)
{
  g_assert (key);

  if (value)
  {
    xml_param_start (ss, indent, key);
    xml_param_text (ss, value);
    xml_param_end (ss);
  }
}

static void
xml_curve_point (SerializeState *ss,
                 gint            indent,
                 gfloat          x,
                 gfloat          y)
{
  gchar str[64];
  ind; g_string_append (ss->buf, "<curve-point x='");
  g_ascii_dtostr (str, sizeof(str), x);
  g_string_append (ss->buf, str);
  g_string_append (ss->buf, "' y='");
  g_ascii_dtostr (str, sizeof(str), y);
  g_string_append (ss->buf, str);
  g_string_append (ss->buf, "'/>\n");
}

static void
xml_curve (SerializeState *ss,
           gint            indent,
           GeglCurve      *curve)
{
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];
  gdouble min_y, max_y;
  guint num_points = gegl_curve_num_points (curve);
  guint i;

  gegl_curve_get_y_bounds (curve, &min_y, &max_y);

  ind; g_string_append (ss->buf, "<curve ymin='");
  g_ascii_dtostr (str, sizeof(str), min_y);
  g_string_append (ss->buf, str);
  g_string_append (ss->buf, "' ymax='");
  g_ascii_dtostr (str, sizeof(str), max_y);
  g_string_append (ss->buf, str);
  g_string_append (ss->buf, "'>\n");
  for (i = 0; i < num_points; ++i)
    {
      gdouble x, y;
      gegl_curve_get_point (curve, i, &x, &y);
      xml_curve_point (ss, indent + 2, x, y);
    }
  ind; g_string_append (ss->buf, "</curve>\n");
}

static void
serialize_properties (SerializeState *ss,
                      gint            indent,
                      GeglNode       *node)
{
  GParamSpec **properties;
  guint        n_properties;
  gboolean     got_a_param = FALSE;
  gint         i;

  properties = gegl_list_properties (gegl_node_get_operation (node),
                                     &n_properties);

  for (i = 0; i < n_properties; i++)
    {
      if (strcmp (properties[i]->name, "input") &&
          strcmp (properties[i]->name, "output") &&
          strcmp (properties[i]->name, "aux"))
        {
          if (!got_a_param)
            {
              ind; g_string_append (ss->buf, "<params>\n");
              got_a_param = TRUE;
            }

          if (g_type_is_a (G_PARAM_SPEC_TYPE (properties[i]),
                           GEGL_TYPE_PARAM_FILE_PATH))
            {
              gchar *value;
              gegl_node_get (node, properties[i]->name, &value, NULL);

              if (value)
                {
                  if (ss->path_root &&
                      !strncmp (ss->path_root, value, strlen (ss->path_root)))
                    {
                      xml_param (ss, indent + 2, properties[i]->name, &value[strlen (ss->path_root) + 1]);
                    }
                  else
                    {
                      xml_param (ss, indent + 2, properties[i]->name, value);
                    }
                }

              g_free (value);
            }
          else if (properties[i]->value_type == G_TYPE_FLOAT)
            {
              gfloat value;
              gchar  str[G_ASCII_DTOSTR_BUF_SIZE];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              g_ascii_dtostr (str, sizeof(str), value);
              xml_param (ss, indent + 2, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_DOUBLE)
            {
              gdouble value;
              gchar   str[G_ASCII_DTOSTR_BUF_SIZE];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              g_ascii_dtostr (str, sizeof(str), value);
              xml_param (ss, indent + 2, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_INT)
            {
              gint  value;
              gchar str[64];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              g_snprintf (str, sizeof (str), "%i", value);
              xml_param (ss, indent + 2, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_BOOLEAN)
            {
              gboolean value;
              gegl_node_get (node, properties[i]->name, &value, NULL);
              if (value)
                {
                  xml_param (ss, indent + 2, properties[i]->name, "true");
                }
              else
                {
                  xml_param (ss, indent + 2, properties[i]->name, "false");
                }
            }
          else if (properties[i]->value_type == G_TYPE_STRING)
            {
              gchar *value;
              gegl_node_get (node, properties[i]->name, &value, NULL);
              xml_param (ss, indent + 2, properties[i]->name, value);
              g_free (value);
            }
          else if (properties[i]->value_type == GEGL_TYPE_COLOR)
            {
              GeglColor *color;
              gchar     *value;
              gegl_node_get (node, properties[i]->name, &color, NULL);
              g_object_get (color, "string", &value, NULL);
              g_object_unref (color);
              xml_param (ss, indent + 2, properties[i]->name, value);
              g_free (value);
            }
	  else if (properties[i]->value_type == GEGL_TYPE_CURVE)
	    {
	      GeglCurve *curve;
	      guint num_points;
	      gegl_node_get (node, properties[i]->name, &curve, NULL);
	      num_points = gegl_curve_num_points (curve);
	      xml_param_start (ss, indent + 2, properties[i]->name);
	      g_string_append (ss->buf, "\n");
	      xml_curve (ss, indent + 4, curve);
	      indent += 2; ind; indent -= 2; xml_param_end (ss);
	      g_object_unref (curve);
	    }
	  else if (properties[i]->value_type == GEGL_TYPE_PATH)
	    {
          gchar *svg_path;
          GeglPath *path;
	      gegl_node_get (node, properties[i]->name, &path, NULL);
	      xml_param_start (ss, indent + 2, properties[i]->name);
          svg_path = gegl_path_to_string (path);	      
	      g_string_append (ss->buf, svg_path);
	      xml_param_end (ss);

          g_object_unref (path);
	    }
          else
            {
              g_warning ("%s: serialization of %s properties not implemented",
                         properties[i]->name, g_type_name (properties[i]->value_type));
            }
        }
    }

  if (got_a_param)
    {
      ind; g_string_append (ss->buf, "</params>\n");
    }
  g_free (properties);
}

static void
serialize_layer (SerializeState *ss,
                 gint            indent,
                 GeglNode       *layer)
{
  gchar  *name;
  gchar  *src;
  gchar  *composite_op;
  gdouble x;
  gdouble y;
  gdouble opacity;

  gegl_node_get (layer, "name", &name,
                 "src", &src,
                 "x", &x,
                 "y", &y,
                 "opacity", &opacity,
                 "composite_op", &composite_op,
                 NULL);
  ind; g_string_append (ss->buf, "<layer");
  if (name[0])
    g_string_append_printf (ss->buf, " name='%s'", name);
  if (x != 0.0)
    {
      gchar str[G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_dtostr (str, sizeof(str), x);
      g_string_append_printf (ss->buf, " x='%s'", str);
    }
  if (y != 0.0)
    {
      gchar str[G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_dtostr (str, sizeof(str), y);
      g_string_append_printf (ss->buf, " y='%s'", str);
    }
  if (opacity != 1.0)
    {
      gchar str[G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_dtostr (str, sizeof(str), opacity);
      g_string_append_printf (ss->buf, " opacity='%s'", str);
    }
  if (src[0])
    {
      if (ss->path_root &&
          !strncmp (ss->path_root, src, strlen (ss->path_root)))
        {
          g_string_append_printf (ss->buf, " src='%s'", &src[strlen (ss->path_root) + 1]);
        }
      else
        {
          g_string_append_printf (ss->buf, " src='%s'", src);
        }
    }
  g_string_append (ss->buf, "/>\n");
}

static void
add_stack (SerializeState *ss,
           gint            indent,
           GeglNode       *head)
{
  /*ind; g_string_append (ss->buf, "<stack>\n");
     indent+=2;*/

  if (GEGL_IS_NODE (head))
    {
      GeglNode *iter = head;

      while (iter)
        {
          GeglPad     *input, *aux;
          const gchar *id = NULL;
          gchar       *class;
          gegl_node_get (iter, "operation", &class, NULL);

          input = gegl_node_get_pad (iter, "input");
          aux   = gegl_node_get_pad (iter, "aux");

          if (gegl_node_get_num_sinks (iter) > 1)
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
                  g_snprintf (temp_id, sizeof (temp_id),
                              "clone%i", ss->clone_count++);
                  id = g_strdup (temp_id);
                  g_hash_table_insert (ss->clones, iter, (gchar *) id);
                  /* the allocation is freed by the hash table */
                }
            }

          if (!strcmp (class, "layer"))
            {
              serialize_layer (ss, indent, iter);
            }
          else
            {
              if (aux &&
                  gegl_pad_get_connected_to (aux))
                {
                  GeglPad  *source_pad;
                  GeglNode *source_node;
                  source_pad  = gegl_pad_get_connected_to (aux);
                  source_node = gegl_pad_get_node (source_pad);
                  {
                    GeglNode *graph = g_object_get_data (G_OBJECT (source_node),
                                                         "graph");
                    if (graph)
                      source_node = graph;
                  }
                  ind; g_string_append (ss->buf, "<node");

                  {
                    gchar *class;
                    gchar *name;
                    gegl_node_get (iter, "operation", &class,
                                   "name", &name,
                                   NULL);
                    if (name[0])
                      {
                        xml_attr (ss->buf, "name", name);
                      }
                    xml_attr (ss->buf, "operation", class);
                    if (id != NULL)
                      xml_attr (ss->buf, "id", id);
                    g_free (name);
                    g_free (class);
                  }

                  g_string_append (ss->buf, ">\n");
                  serialize_properties (ss, indent + 4, iter);
                  add_stack (ss, indent + 4, source_node);

                  ind; g_string_append (ss->buf, "</node>\n");
                }
              else
                {
                  if (class)
                    {
                      if (strcmp (class, "gegl:nop") &&
                          strcmp (class, "gegl:clone"))
                        {
                          ind; g_string_append (ss->buf, "<node");

                          {
                            gchar *class;
                            gchar *name;
                            gegl_node_get (iter, "operation", &class,
                                           "name", &name,
                                           NULL);
                            if (name[0])
                              {
                                xml_attr (ss->buf, "name", name);
                              }
                            xml_attr (ss->buf, "operation", class);
                            if (id != NULL)
                              xml_attr (ss->buf, "id", id);
                            g_free (name);
                            g_free (class);
                          }

                          g_string_append (ss->buf, ">\n");
                          serialize_properties (ss, indent + 4, iter);
                          ind; g_string_append (ss->buf, "</node>\n");
                        }
                    }
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
                  GeglNode *graph       = g_object_get_data (G_OBJECT (source_node),
                                                             "graph");
                  if (graph)
                    source_node = graph;
                  iter = source_node;
                }
              else
                iter = NULL;
            }
          else
            {
              iter = NULL;
            }

          g_free (class);
        }
    }
  /*indent-=2;
     ind; g_string_append (ss->buf, "<stack>\n");*/
}

gchar *
gegl_node_to_xml (GeglNode    *gegl,
                  const gchar *path_root)
{
  SerializeState  ss;

  ss.buf         = g_string_new ("");
  ss.path_root   = path_root;
  ss.clone_count = 0;
  ss.clones      = g_hash_table_new (NULL, NULL);
  ss.terse       = FALSE;

  gegl = gegl_node_get_output_proxy (gegl, "output");

  g_string_append (ss.buf, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (ss.buf, "<gegl>\n");

  add_stack (&ss, 2, gegl);

  g_string_append (ss.buf, "</gegl>\n");

  g_hash_table_foreach (ss.clones, free_clone_id, NULL);
  g_hash_table_destroy (ss.clones);

  return g_string_free (ss.buf, FALSE);
}
