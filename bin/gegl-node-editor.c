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
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#include <stdlib.h>
#include <string.h>

#include "gegl.h"
#include "gegl-node-editor.h"
#include "gegl-view.h"
#include "editor.h"

enum
{
  PROP_0,
  PROP_NODE,
  PROP_LAST
};

static void gegl_node_editor_class_init (GeglNodeEditorClass   *klass);
static void gegl_node_editor_init       (GeglNodeEditor        *self);
static void set_property                (GObject               *gobject,
                                         guint                  prop_id,
                                         const GValue          *value,
                                         GParamSpec            *pspec);

static GObject *constructor             (GType                  type,
                                         guint                  n_params,
                                         GObjectConstructParam *params);

G_DEFINE_TYPE (GeglNodeEditor, gegl_node_editor, GTK_TYPE_VBOX)

static GObjectClass *parent_class = NULL;

static void
gegl_node_editor_class_init (GeglNodeEditorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = set_property;
  gobject_class->constructor = constructor;

  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                                        "Node",
                                                        "The node to render",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_WRITABLE));
}

static void
gegl_node_editor_init (GeglNodeEditor *self)
{
  self->node = NULL;
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglNodeEditor *self = GEGL_NODE_EDITOR (gobject);

  switch (property_id)
    {
    case PROP_NODE:
      /* FIXME: reference counting? */
      self->node = GEGL_NODE (g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static GtkWidget *
property_editor_general (GeglNode *node);

static GObject *
constructor (GType                  type,
             guint                  n_params,
             GObjectConstructParam *params)
{
  GObject         *object;
  GeglNodeEditor  *self;
  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self = GEGL_NODE_EDITOR (object);

  gtk_box_set_homogeneous (GTK_BOX (object), FALSE);
  gtk_box_set_spacing (GTK_BOX (object), 0);

  gtk_container_add (GTK_CONTAINER (object), property_editor_general (self->node));

  return object;
}


GtkWidget *
gegl_node_editor_new (GeglNode *node)
{
  return g_object_new (GEGL_TYPE_NODE_EDITOR, "node", node, NULL);
}


GtkWidget *typeeditor_double (GtkSizeGroup *col1,
                              GtkSizeGroup *col2,
                              GeglNode     *node);

static void
type_editor_generic_changed (GtkWidget *entry,
                             gpointer   data)
{
  GParamSpec *param_spec = data;
  GeglNode   *node = g_object_get_data (G_OBJECT (entry), "node");
  const gchar *entry_text;
  const gchar *prop_name = param_spec->name;

  entry_text = gtk_entry_get_text (GTK_ENTRY (entry));

  if (param_spec->value_type == G_TYPE_INT)
    {
      gegl_node_set (node, prop_name, atoi (entry_text), NULL);
    }
  else if (param_spec->value_type == G_TYPE_FLOAT ||
           param_spec->value_type == G_TYPE_DOUBLE)
    {
      gegl_node_set (node, prop_name, atof (entry_text), NULL);
    }
  else if (param_spec->value_type == G_TYPE_STRING)
    {
      gegl_node_set (node, prop_name, entry_text, NULL);
    }
  else if (param_spec->value_type == G_TYPE_BOOLEAN)
    {
      gegl_node_set (node, prop_name, !strcmp(entry_text, "yes"), NULL);
    }
  else if (param_spec->value_type == GEGL_TYPE_COLOR)
    {
      GeglColor *color = g_object_new (GEGL_TYPE_COLOR, "string", entry_text, NULL);
      gegl_node_set (node, prop_name, color, NULL);
      g_object_unref (color);
    }
  gegl_view_repaint (GEGL_VIEW (editor.drawing_area));
}


static void
type_editor_color_changed (GtkColorButton *button,
                           gpointer        data)
{
  GParamSpec *param_spec = data;
  GeglNode   *node = g_object_get_data (G_OBJECT (button), "node");
  const gchar *prop_name = param_spec->name;
  GdkColor   gdkcolor;
  gint       alpha;

  gtk_color_button_get_color (button, &gdkcolor);
  alpha = gtk_color_button_get_alpha (button);

    {
      GeglColor *color = g_object_new (GEGL_TYPE_COLOR, NULL);
      gfloat r,g,b,a;
      r = gdkcolor.red/65535.0;
      g = gdkcolor.green/65535.0;
      b = gdkcolor.blue/65535.0;
      a = alpha/65535.0;
      gegl_color_set_rgba (color, r,g,b,a);
      gegl_node_set (node, prop_name, color, NULL);
      g_object_unref (color);
    }
  gegl_view_repaint (GEGL_VIEW (editor.drawing_area));
}

static GtkWidget *
type_editor_color (GtkSizeGroup *col1,
                   GtkSizeGroup *col2,
                   GeglNode     *node,
                   GParamSpec   *param_spec)
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 5);
  GtkWidget *label = gtk_label_new (param_spec->name);
  GtkWidget *button = g_object_new (GTK_TYPE_COLOR_BUTTON, "use-alpha", TRUE, NULL);

  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (col1, label);
  gtk_size_group_add_widget (col2, button);

  gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (button), "node", node);

  /*gulong handler = */g_signal_connect (G_OBJECT (button), "color-set",
                                     G_CALLBACK (type_editor_color_changed),
                                     (gpointer) param_spec);

    {
      GeglColor *color;
      GdkColor   gdkcolor;
      gfloat     r,g,b,a;

      gegl_node_get (node, param_spec->name, &color, NULL);
      gegl_color_get_rgba (color, &r, &g, &b, &a);
      gdkcolor.red   = r*65535;
      gdkcolor.green = g*65535;
      gdkcolor.blue  = b*65535;
      gtk_color_button_set_color (GTK_COLOR_BUTTON (button), &gdkcolor);
      gtk_color_button_set_alpha (GTK_COLOR_BUTTON (button), a*65535);
      g_object_unref (color);
    }
  return hbox;
}

static GtkWidget *
type_editor_generic (GtkSizeGroup *col1,
                     GtkSizeGroup *col2,
                     GeglNode     *node,
                     GParamSpec   *param_spec)
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 5);
  GtkWidget *label = gtk_label_new (param_spec->name);
  GtkWidget *entry = gtk_entry_new ();

  gtk_entry_set_width_chars (GTK_ENTRY (entry), 6);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (col1, label);
  gtk_size_group_add_widget (col2, entry);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (entry), "node", node);

  /*gulong handler = */g_signal_connect (G_OBJECT (entry), "activate",
                                     G_CALLBACK (type_editor_generic_changed),
                                     (gpointer) param_spec);

  if (param_spec->value_type == G_TYPE_BOOLEAN)
    {
      gboolean value;
      gegl_node_get (node, param_spec->name, &value, NULL);
      if (value)
        gtk_entry_set_text (GTK_ENTRY (entry), "yes");
      else
        gtk_entry_set_text (GTK_ENTRY (entry), "no");
    }
  else if (param_spec->value_type == G_TYPE_FLOAT)
    {
      gfloat value;
      gchar str[64];
      gegl_node_get (node, param_spec->name, &value, NULL);
      sprintf (str, "!!!float!!!%f", value);
      gtk_entry_set_text (GTK_ENTRY (entry), str);
    }
  else if (param_spec->value_type == G_TYPE_DOUBLE)
    {
      gdouble value;
      gchar str[64];
      gegl_node_get (node, param_spec->name, &value, NULL);
      sprintf (str, "%f", value);
      gtk_entry_set_text (GTK_ENTRY (entry), str);
    }
  else if (param_spec->value_type == G_TYPE_INT)
    {
      gint value;
      gchar str[64];
      gegl_node_get (node, param_spec->name, &value, NULL);
      sprintf (str, "%i", value);
      gtk_entry_set_text (GTK_ENTRY (entry), str);
    }
  else if (param_spec->value_type == G_TYPE_STRING)
    {
      gchar *value;
      gegl_node_get (node, param_spec->name, &value, NULL);
      gtk_entry_set_text (GTK_ENTRY (entry), value);
      g_free (value);
    }
  else if (param_spec->value_type == GEGL_TYPE_COLOR)
    {
      GeglColor *color;
      gchar     *color_string;
      gegl_node_get (node, param_spec->name, &color, NULL);
      g_object_get (color, "string", &color_string, NULL);
      gtk_entry_set_text (GTK_ENTRY (entry), color_string);
      g_free (color_string);
      g_object_unref (color);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (entry), "hm");
    }
  return hbox;
}

GtkWidget *
typeeditor_optype (GtkSizeGroup *col1,
                   GtkSizeGroup *col2,
                   GeglNode     *item);


static GtkWidget *
property_editor_general (GeglNode *node)
{
  GtkSizeGroup *col1, *col2;
  GtkWidget   *vbox;
  GParamSpec **properties;
  guint        n_properties;
  gint         i;

  col1 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  col2 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  
  properties = gegl_node_get_properties (node, &n_properties);

  gtk_box_pack_start (GTK_BOX (vbox), typeeditor_optype (col1, col2, node), FALSE, FALSE, 0);
  /*gtk_box_pack_start (GTK_BOX (vbox), prop_editor, FALSE, FALSE, 0);*/
  for (i=0; i<n_properties; i++)
    {
      if (strcmp (properties[i]->name, "input") &&
          strcmp (properties[i]->name, "output") &&
          strcmp (properties[i]->name, "aux"))
        {
          GtkWidget *prop_editor;
       
          if (properties[i]->value_type == GEGL_TYPE_COLOR)
            {
              prop_editor = type_editor_color (col1, col2, node, properties[i]);
            }
          else
            { 
              prop_editor = type_editor_generic (col1, col2, node, properties[i]);
            }
          gtk_box_pack_start (GTK_BOX (vbox), prop_editor, FALSE, FALSE, 0);
        }
    }

  g_object_unref (G_OBJECT (col1));
  g_object_unref (G_OBJECT (col2));
  return vbox;
}
