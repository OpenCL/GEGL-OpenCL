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

GtkWidget *
typeeditor_optype (GtkSizeGroup *col1,
                   GtkSizeGroup *col2,
                   GeglNodeEditor *node_editor);
enum
{
  PROP_0,
  PROP_NODE,
  PROP_OPERATION_SWITCHER,
  PROP_LAST
};

static void gegl_node_editor_class_init (GeglNodeEditorClass   *klass);
static void gegl_node_editor_init       (GeglNodeEditor        *self);
static void set_property                (GObject               *gobject,
                                         guint                  prop_id,
                                         const GValue          *value,
                                         GParamSpec            *pspec);
static void construct                   (GeglNodeEditor        *editor);

static GObject *constructor             (GType                  type,
                                         guint                  n_params,
                                         GObjectConstructParam *params);

G_DEFINE_TYPE (GeglNodeEditor, gegl_node_editor, GTK_TYPE_VBOX)

static GObjectClass *parent_class = NULL;

static void
gegl_node_editor_class_init (GeglNodeEditorClass *klass)
{
  GObjectClass        *gobject_class = G_OBJECT_CLASS (klass);
  GeglNodeEditorClass *node_editor_class = GEGL_NODE_EDITOR_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = set_property;
  gobject_class->constructor = constructor;
  node_editor_class->construct = construct;

  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                                        "Node",
                                                        "The node we're showing properties for",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_OPERATION_SWITCHER,
                                   g_param_spec_boolean ("operation-switcher",
                                                        "operation-switcher",
                                                        "Show an operation changer widget within (at the top of) this node property editor",
                                                        TRUE,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_WRITABLE));
}

static void
gegl_node_editor_init (GeglNodeEditor *self)
{
  self->node = NULL;
  self->operation_switcher = TRUE;
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
    case PROP_OPERATION_SWITCHER:
      /* FIXME: reference counting? */
      self->operation_switcher = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static GtkWidget *
property_editor_general (GeglNodeEditor *node_editor,
                         GeglNode *node);

static void
construct (GeglNodeEditor *self)
{
  gtk_box_set_homogeneous (GTK_BOX (self), FALSE);
  gtk_box_set_spacing (GTK_BOX (self), 0);

  gtk_container_add (GTK_CONTAINER (self), property_editor_general (self, self->node));
}

static void
gegl_node_editor_construct (GeglNodeEditor *self)
{
  GeglNodeEditorClass *klass;

  g_return_if_fail (GEGL_IS_NODE_EDITOR (self));
  
  klass = GEGL_NODE_EDITOR_GET_CLASS (self);
  g_assert (klass->construct);
  klass->construct(self);
}

static GObject *
constructor (GType                  type,
             guint                  n_params,
             GObjectConstructParam *params)
{
  GObject         *object;
  GeglNodeEditor  *self;
  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self = GEGL_NODE_EDITOR (object);

  self->col1 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  self->col2 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  if (self->operation_switcher)
    gtk_box_pack_start (GTK_BOX (object), typeeditor_optype (self->col1, self->col2, self), FALSE, FALSE, 0);
  gegl_node_editor_construct (GEGL_NODE_EDITOR (object));

  g_object_unref (self->col1);
  g_object_unref (self->col2);
  return object;
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
  gegl_gui_flush ();
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
  gegl_gui_flush ();
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

  g_signal_connect (G_OBJECT (entry), "activate",
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
      sprintf (str, "%3.3f", value);
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

static GtkWidget *
property_editor_general (GeglNodeEditor *node_editor,
                         GeglNode       *node)
{
  GtkSizeGroup *col1, *col2;
  GtkWidget   *vbox;
  GParamSpec **properties;
  guint        n_properties;
  gint         i;

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  
  properties = gegl_node_get_properties (node, &n_properties);
  col1 = node_editor->col1;
  col2 = node_editor->col2;

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

  return vbox;
}

/* utility method */
cairo_t *
gegl_widget_get_cr (GtkWidget *widget)
{
  gdouble    width  = widget->allocation.width;
  gdouble    height = widget->allocation.height;
  cairo_t   *cr = gdk_cairo_create (widget->window);
  gdouble    margin = 0.04;

  if (height > width)
    height = width;
  else
    width = height;

  cairo_select_font_face (cr, "Sans",
                          CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size (cr, 0.08);
  cairo_translate (cr, width * margin, height * margin);
  cairo_scale (cr, width * (1.0-margin*2), height * (1.0-margin*2));
  cairo_set_line_width (cr, 0.01);

  return cr;
}

void
gegl_node_editor_class_set_pattern (GeglNodeEditorClass *klass,
                                    const gchar         *pattern)
{
  /* FIXME: here we're most probably leaking a string */
  klass->pattern = g_strdup(pattern);/*g_strdup (pattern);*/
}

gboolean
gegl_node_editor_class_matches (GeglNodeEditorClass *klass,
                                const gchar         *operation_name)
{
  /* without a pattern it matches always */
  if (!klass->pattern)
    return TRUE;

  if (strstr (klass->pattern, operation_name))
    return TRUE;
  return FALSE;
}

static GSList *gegl_type_subtypes (GType   supertype,
                                   GSList *list)
{
  GType *types;
  guint  count;
  gint   no;

  types = g_type_children (supertype, &count);
  if (!types)
    return list;

  for (no=0; no < count; no++)
    {
      list = g_slist_prepend (list, GUINT_TO_POINTER (types[no]));
      list = gegl_type_subtypes (types[no], list);
    }
  g_free (types);
  return list;
}

static GType *gegl_type_heirs (GType  supertype,
                               guint *count)
{
  GSList *subtypes= gegl_type_subtypes (supertype, NULL);
  GSList *iter;
  GType  *heirs;
  gint    i;

  if (!subtypes)
    {
      *count = 0;
      return NULL;
    }
  *count = g_slist_length (subtypes);
  heirs = g_malloc (sizeof (GType) * (*count));
   
  for (iter = subtypes, i=0; iter; iter = g_slist_next (iter), i++) 
    {
      GType type = GPOINTER_TO_UINT (iter->data);
      heirs[i]=type;
    }
  g_assert (i==*count);

  g_slist_free (subtypes);
  return heirs;
}

GtkWidget *
gegl_node_editor_new (GeglNode *node,
                      gboolean  operation_switcher)
{
  GType editor_type;
  const gchar *operation;

  editor_type = GEGL_TYPE_NODE_EDITOR;

  operation = gegl_node_get_operation (node);

  /* iterate through all GeglNodeEditor subclasses, and check if it matches */

  {
    guint   count;
    GType  *heirs = gegl_type_heirs (GEGL_TYPE_NODE_EDITOR, &count);
    gint    i;

    for (i=0; i<count; i++)
      {
        GType type = heirs[i];
        GeglNodeEditorClass *klass = g_type_class_ref (type);
        if (gegl_node_editor_class_matches (klass, operation))
          {
            editor_type = type;
            g_type_class_unref (klass);
            break;
          }
        g_type_class_unref (klass);
      }
    g_free (heirs);
  }

  return g_object_new (editor_type, "node", node,
                                    "operation-switcher", operation_switcher,
                                    NULL);
}
