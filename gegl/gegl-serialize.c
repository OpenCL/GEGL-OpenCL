/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * GEGL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2016 Øyvind Kolås
 */

#include "gegl.h"
#include <string.h>
#include <stdio.h>

//#define make_rel(strv) (g_strtod (strv, NULL) * gegl_node_get_bounding_box (iter[0]).height)
#define make_rel(strv) (g_strtod (strv, NULL) * rel_dim)

void gegl_create_chain_argv (char **ops, GeglNode *start, GeglNode *proxy, double time, int rel_dim, GError **error)
{
  GeglNode   *iter[10] = {start, NULL};
  GeglNode   *new = NULL;
  gchar     **arg = ops;
  int         level = 0;
  char       *level_op[10];
  char       *level_pad[10];
  int         in_keyframes = 0;
  char       *prop = NULL;
  GHashTable *ht = NULL;
  GeglCurve  *curve = NULL;

  level_op[level] = *arg;
 
  ht = g_hash_table_new (g_str_hash, g_str_equal);

  while (*arg)
    {
      if (in_keyframes)
      {
        char *key = g_strdup (*arg);
        char *eq = strchr (key, '=');
        char *value = NULL;
        
        if (eq)
        {
          value = eq + 1;
          value[-1] = '\0';
          if (strstr (value, "rel"))
          {
            gegl_curve_add_point (curve, g_strtod (key, NULL), make_rel (value));
          }
          else
          {
            gegl_curve_add_point (curve, g_strtod (key, NULL),
                                         g_strtod (value, NULL));
          }
        }
        else
        {
          if (!strcmp (*arg, "l"))
          {
            fprintf (stderr, "linear\n");
          }
          else if (!strcmp (*arg, "s"))
          {
            fprintf (stderr, "step\n");
          }
          else if (!strcmp (*arg, "c"))
          {
            fprintf (stderr, "cubic\n");
          }
        }

        g_free (key);

        if (strchr (*arg, '}')) {

          gegl_node_set (new, prop, gegl_curve_calc_value (
              g_object_get_qdata (G_OBJECT (new),
                  g_quark_from_string(prop)), time), NULL);
          /* set interpolated values to initially passed in time */

          in_keyframes = 0;
        };
      }
      else if (strchr (*arg, ']'))
      {
        level--;
        gegl_node_connect_to (iter[level+1], "output", iter[level], level_pad[level]);
      }
      else
      {
      if (strchr (*arg, '=')) /* contains = sign, must be a property assignment */
      {
        char *match= strchr (*arg, '=');
        if (match[1] == '{')
        {
          char *key = g_strdup (*arg);
          char *value = strchr (key, '=') + 1;
          value[-1] = '\0';
          curve = gegl_curve_new (-1000.0, 1000.0);
          in_keyframes = 1;
          if (prop)
            g_free (prop);
          prop = g_strdup (key);

          g_object_set_qdata_full (G_OBJECT (new), g_quark_from_string(key), curve, g_object_unref);

          g_free (key);
        }
        else if (match[1] == '[')
        {
          char *pad = g_strdup (*arg);
          char *value = strchr (pad, '=') + 1;
          value[-1] = '\0';
          level_pad[level]=(void*)g_intern_string(pad);
          g_free (pad);
          level++;

          iter[level]=NULL;
          level_op[level]=NULL;
          level_pad[level]=NULL;
        }
        else
        {
          GType target_type = 0;
          GValue gvalue={0,};
          char *key = g_strdup (*arg);
          char *value = strchr (key, '=') + 1;
          value[-1] = '\0';

          if (!strcmp (key, "id"))
          {
            g_hash_table_insert (ht, (void*)g_intern_string (value), iter[level]);
          }
          else if (!strcmp (key, "ref"))
          {
            if (g_hash_table_lookup (ht, g_intern_string (value)))
              iter[level] = g_hash_table_lookup (ht, g_intern_string (value));
            else
              g_warning ("unknown id '%s'", value);
          }
          else
          {
            unsigned int n_props = 0;
            GParamSpec **pspecs;
            int i;

            pspecs = gegl_operation_list_properties (level_op[level], &n_props);
            for (i = 0; i < n_props; i++)
            {
              if (!strcmp (pspecs[i]->name, key))
                target_type = pspecs[i]->value_type;
            }

            if (target_type == 0)
            {
              if (error && gegl_has_operation (level_op[level]))
              {
                GString *str = g_string_new ("");
                g_string_append_printf (str, "%s has no %s property, but has ",
                  level_op[level], key);
                for (i = 0; i < n_props; i++)
                {
                  g_string_append_printf (str, "'%s', ", pspecs[i]->name);
                }
                *error = g_error_new_literal (g_quark_from_static_string ("gegl"),
                                              0, str->str);
                g_string_free (str, TRUE);
              }
            }
            else
            if (g_type_is_a (target_type, G_TYPE_DOUBLE) ||
                g_type_is_a (target_type, G_TYPE_FLOAT) ||
                g_type_is_a (target_type, G_TYPE_INT))
              {
                if (strstr (value, "rel"))
                {
                   g_object_set_qdata_full (G_OBJECT (new), g_quark_from_string(key), g_strdup (value), g_free);

                  if (g_type_is_a (target_type, G_TYPE_INT))
                    gegl_node_set (iter[level], key, (int)make_rel (value), NULL);
                  else
                    gegl_node_set (iter[level], key,  make_rel (value), NULL);

                }
                else
                {
                  if (g_type_is_a (target_type, G_TYPE_INT))
                    gegl_node_set (iter[level], key, 
                       (int)g_strtod (value, NULL), NULL);
                  else
                    gegl_node_set (iter[level], key, 
                       g_strtod (value, NULL), NULL);
                }
              }
            else if (g_type_is_a (target_type, G_TYPE_BOOLEAN))
            {
              if (!strcmp (value, "true") || !strcmp (value, "TRUE") ||
                  !strcmp (value, "YES") || !strcmp (value, "yes") ||
                  !strcmp (value, "y") || !strcmp (value, "Y") ||
                  !strcmp (value, "1") || !strcmp (value, "on"))
              {
                gegl_node_set (iter[level], key, TRUE, NULL);
              }
            else
              {
                gegl_node_set (iter[level], key, FALSE, NULL);
              }
            }
            else if (target_type == GEGL_TYPE_COLOR)
            {
              GeglColor *color = g_object_new (GEGL_TYPE_COLOR,
                                               "string", value, NULL);
              gegl_node_set (iter[level], key, color, NULL);
            }
            else if (g_type_is_a (target_type, G_TYPE_ENUM))
            {
              GEnumClass *eclass = g_type_class_peek (target_type);
              GEnumValue *evalue = g_enum_get_value_by_nick (eclass, value);
              if (evalue)
                {
                  gegl_node_set (new, key, evalue->value, NULL);
                }
              else
                {
              /* warn, but try to get a valid nick out of the old-style value
               * name
               */
                gchar *nick;
                gchar *c;
                g_printerr ("gedl (param_set %s): enum %s has no value '%s'\n",
                            key,
                            g_type_name (target_type),
                            value);
                nick = g_strdup (value);
                for (c = nick; *c; c++)
                  {
                    *c = g_ascii_tolower (*c);
                    if (*c == ' ')
                      *c = '-';
                  }
                evalue = g_enum_get_value_by_nick (eclass, nick);
                if (evalue)
                  gegl_node_set (iter[level], key, evalue->value, NULL);
                g_free (nick);
              }
            }
            else
            {
              GValue gvalue_transformed={0,};
              g_value_init (&gvalue, G_TYPE_STRING);
              g_value_set_string (&gvalue, value);
              g_value_init (&gvalue_transformed, target_type);
              g_value_transform (&gvalue, &gvalue_transformed);
              gegl_node_set_property (iter[level], key, &gvalue_transformed);
              g_value_unset (&gvalue);
              g_value_unset (&gvalue_transformed);
            }
          }
          g_free (key);
        }
      }
      else
      {
        if (strchr (*arg, ':')) /* contains : is a non-prefixed operation */
          {
            level_op[level] = *arg;
          }
          else /* default to gegl: as prefix if no : specified */
          {
            char temp[1024];
            g_snprintf (temp, 1023, "gegl:%s", *arg);
            level_op[level] = (void*)g_intern_string (temp);
          }


          if (gegl_has_operation (level_op[level]))
          {
             new = gegl_node_new_child (gegl_node_get_parent (proxy), "operation",
                                        level_op[level], NULL);

             if (iter[level])
               gegl_node_link_many (iter[level], new, proxy, NULL);
             else
               gegl_node_link_many (new, proxy, NULL);
             iter[level] = new;
          }
          else if (error)
          {
            GString *str = g_string_new ("");
            g_string_append_printf (str, "No such op '%s'", level_op[level]);
            *error = g_error_new_literal (g_quark_from_static_string ("gegl"),
                                          0, str->str);
             g_string_free (str, TRUE);
          }

      }
      }
      arg++;
    }

  if (prop)
  {
    g_free (prop);
  }
  g_hash_table_unref (ht);
  gegl_node_link_many (iter[level], proxy, NULL);
}

void gegl_create_chain (const char *str, GeglNode *op_start, GeglNode *op_end, double time, int rel_dim, GError **error)
{
  gchar **argv = NULL;
  gint    argc = 0;
  g_shell_parse_argv (str, &argc, &argv, NULL);
  if (argv)
  {
    gegl_create_chain_argv (argv, op_start, op_end, time, rel_dim, error);
    g_strfreev (argv);
  }
}

gchar *gegl_serialize (GeglNode *start, GeglNode *end)
{
  char *ret = NULL;
  GeglNode *iter;
  GString *str = g_string_new ("");
  if (start == NULL && 0)
  {
    start = end;
    while (gegl_node_get_producer (start, "input", NULL))
      start = gegl_node_get_producer (start, "input", NULL);
  }

  iter = end;
  while (iter)
  {
    if (iter == start)
    {
      iter = NULL;
    }
    else
    {
      GString *s2 = g_string_new ("");
      g_string_append_printf (s2, " %s", gegl_node_get_operation (iter));
      {
        gint i;
        guint n_properties;
        GParamSpec **properties;

        properties = gegl_operation_list_properties (gegl_node_get_operation (iter), &n_properties);
        for (i = 0; i < n_properties; i++)
        {
          const gchar *property_name = g_param_spec_get_name (properties[i]);
          GType        property_type = G_PARAM_SPEC_VALUE_TYPE (properties[i]);
          const GValue*default_value = g_param_spec_get_default_value (properties[i]);

          if (property_type == G_TYPE_FLOAT)
            {
              gfloat defval = g_value_get_float (default_value);
              gfloat value;
              gchar  str[G_ASCII_DTOSTR_BUF_SIZE];
              gegl_node_get (iter, properties[i]->name, &value, NULL);
              if (value != defval)
              {
                g_ascii_dtostr (str, sizeof(str), value);
                g_string_append_printf (s2, " %s=%s", property_name, str);
              }
            }
          else if (property_type == G_TYPE_DOUBLE)
            {
              gdouble  defval = g_value_get_double (default_value);
              gdouble value;
              gchar   str[G_ASCII_DTOSTR_BUF_SIZE];
              gegl_node_get (iter, property_name, &value, NULL);
              if (value != defval)
              {
                g_ascii_dtostr (str, sizeof(str), value);
                g_string_append_printf (s2, " %s=%s", property_name, str);
              }
            }
          else if (property_type == G_TYPE_INT)
            {
              gint  defval = g_value_get_int (default_value);
              gint  value;
              gchar str[64];
              gegl_node_get (iter, properties[i]->name, &value, NULL);
              if (value != defval)
              {
                g_snprintf (str, sizeof (str), "%i", value);
                g_string_append_printf (s2, " %s=%s", property_name, str);
              }
            }
          else if (property_type == G_TYPE_BOOLEAN)
            {
              gboolean value;
              gboolean defval = g_value_get_boolean (default_value);
              gegl_node_get (iter, properties[i]->name, &value, NULL);
              if (value != defval)
              {
                if (value)
                  g_string_append_printf (s2, " %s=true", property_name);
                else
                  g_string_append_printf (s2, " %s=false", property_name);
              }
            }
         else if (property_type == G_TYPE_STRING)
            {
              gchar *value;
              const gchar *defval = g_value_get_string (default_value);
              gegl_node_get (iter, properties[i]->name, &value, NULL);
              if (!g_str_equal (defval, value))
              {
                g_string_append_printf (s2, " %s='%s'", property_name, value);
              }
              g_free (value);
            }
          else if (g_type_is_a (property_type, G_TYPE_ENUM))
            {
              GEnumClass *eclass = g_type_class_peek (property_type);
              gint defval = g_value_get_enum (default_value);
              gint value;

              gegl_node_get (iter, properties[i]->name, &value, NULL);
              if (value != defval)
              {
                GEnumValue *evalue = g_enum_get_value (eclass, value);
                g_string_append_printf (s2, " %s=%s", property_name, evalue->value_nick);
              }
            }
          else if (property_type == GEGL_TYPE_COLOR)
            {
              GeglColor *color;
              GeglColor *defcolor = g_value_get_object (default_value);
              gchar     *value;
              gchar     *defvalue = NULL;
              gegl_node_get (iter, properties[i]->name, &color, NULL);
              g_object_get (color, "string", &value, NULL);
              if (defcolor)
              {
                g_object_get (defcolor, "string", &defvalue, NULL);
              }
              g_object_unref (color);
              if (defvalue && !g_str_equal (defvalue, value))
                g_string_append_printf (s2, " %s='%s'", property_name, value);
              g_free (value);
            }
          else
            {
              g_warning ("%s: serialization of %s properties not implemented",
                         property_name, g_type_name (property_type));

            }

          {
            GeglNode *aux = gegl_node_get_producer (iter, "aux", NULL);
            if (aux)
            {
              char *str = gegl_serialize (NULL, aux);
              g_string_append_printf (s2, " aux=[%s ]", str);
              g_free (str);
            }
          }
        }
      }

      g_string_prepend (str, s2->str);
      g_string_free (s2, TRUE);
      iter = gegl_node_get_producer (iter, "input", NULL);
    }
  }
  ret = str->str;
  g_string_free (str, FALSE);

  return ret;
}
