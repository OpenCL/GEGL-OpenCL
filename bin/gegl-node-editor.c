/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */
#include "config.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "gegl-bin-gui-types.h"

#include "gegl.h"

#include "gegl-node-editor.h"
#include "gegl-paramspecs.h"
#include "gegl-path.h"


enum
{
  PROP_0,
  PROP_NODE,
  PROP_OPERATION_SWITCHER,
  PROP_LAST
};


static void        gegl_node_editor_class_init     (GeglNodeEditorClass   *klass);
static void        gegl_node_editor_init           (GeglNodeEditor        *self);
static void        gegl_node_editor_set_property   (GObject               *gobject,
                                                    guint                  prop_id,
                                                    const GValue          *value,
                                                    GParamSpec            *pspec);
static void        gegl_node_editor_real_construct (GeglNodeEditor        *editor);

static GObject *   gegl_node_editor_constructor    (GType                  type,
                                                    guint                  n_params,
                                                    GObjectConstructParam *params);
static GtkWidget * property_editor_general         (GeglNodeEditor        *node_editor,
                                                    GeglNode              *node);



G_DEFINE_TYPE (GeglNodeEditor, gegl_node_editor, GTK_TYPE_VBOX)


#define parent_class gegl_node_editor_parent_class 


static void
gegl_node_editor_class_init (GeglNodeEditorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property  = gegl_node_editor_set_property;
  gobject_class->constructor   = gegl_node_editor_constructor;

  klass->construct             = gegl_node_editor_real_construct;

  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                                        "Node",
                                                        "The node we're showing properties for",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_WRITABLE));
}

static void
gegl_node_editor_init (GeglNodeEditor *self)
{
  self->node               = NULL;
}

static void
gegl_node_editor_set_property (GObject      *gobject,
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

static void
gegl_node_editor_real_construct (GeglNodeEditor *self)
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
gegl_node_editor_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GObject         *object;
  GeglNodeEditor  *self;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self   = GEGL_NODE_EDITOR (object);

  self->col1 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  self->col2 = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gegl_node_editor_construct (GEGL_NODE_EDITOR (object));

  /*g_object_unref (self->col1);
  g_object_unref (self->col2);*/
  return object;
}



static void
type_editor_boolean_changed (GtkWidget *entry,
                             gpointer   data)
{
  GParamSpec  *param_spec  = data;
  GeglNode    *node        = g_object_get_data (G_OBJECT (entry), "node");
  const gchar *prop_name   = param_spec->name;

  gegl_node_set (node, prop_name,
                 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (entry)),
                 NULL);
}

static void
type_editor_numeric_changed (GtkWidget *spin_button,
                             gpointer   data)
{
  GParamSpec  *param_spec  = data;
  GeglNode    *node        = g_object_get_data (G_OBJECT (spin_button), "node");
  const gchar *prop_name   = param_spec->name;

  if (param_spec->value_type == G_TYPE_INT)
    {
      gegl_node_set (node, prop_name,
                     gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spin_button)),
                     NULL);
    }
  else
    {
      gegl_node_set (node, prop_name,
                     gtk_spin_button_get_value (GTK_SPIN_BUTTON (spin_button)),
                     NULL);
    }
}

static void
type_editor_string_changed (GtkWidget *entry,
                             gpointer   data)
{
  GParamSpec  *param_spec  = data;
  GeglNode    *node        = g_object_get_data (G_OBJECT (entry), "node");
  const gchar *prop_name   = param_spec->name;

  gegl_node_set (node, prop_name,
                 gtk_entry_get_text (GTK_ENTRY (entry)),
                 NULL);
}

static void
type_editor_color_changed (GtkColorButton *button,
                           gpointer        data)
{
  GParamSpec  *param_spec = data;
  GeglNode    *node       = g_object_get_data (G_OBJECT (button), "node");
  const gchar *prop_name  = param_spec->name;
  GdkColor     gdkcolor;
  gint         alpha;

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
}




static GtkWidget *
hbox_type_editor (const gchar* labeltext,
                  GtkWidget* entry,
                  GtkSizeGroup *col1,
                  GtkSizeGroup *col2)
{
  /* label and align the type editor widget */
  GtkWidget *hbox = gtk_hbox_new (FALSE, 5);
  GtkWidget *label = gtk_label_new (labeltext);
  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (col1, label);
  gtk_size_group_add_widget (col2, entry);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  return hbox;
}


static GtkWidget *
type_editor_color (GtkSizeGroup *col1,
                   GtkSizeGroup *col2,
                   GeglNode     *node,
                   GParamSpec   *param_spec)
{
  const gchar* labeltext = param_spec->name;
  GtkWidget *button = g_object_new (GTK_TYPE_COLOR_BUTTON, "use-alpha", TRUE, NULL);
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
  return hbox_type_editor (labeltext, button, col1, col2);
}

static GQuark param_spec_quark = 0;

static void
set_param_spec (GObject    *object,
                GtkWidget  *widget,
                GParamSpec *param_spec)
{
  if (object)
    {
      if (! param_spec_quark)
        param_spec_quark = g_quark_from_static_string ("gimp-config-param-spec");

      g_object_set_qdata (object, param_spec_quark, param_spec);
    }

  if (widget)
    {
#if 0
      const gchar *blurb = g_param_spec_get_blurb (param_spec);

      if (blurb)
        {
          const gchar *domain;

          domain = gimp_type_get_translation_domain (param_spec->owner_type);
          gimp_help_set_help_data (widget, dgettext (domain, blurb), NULL);
        }
#endif
    }
}

static GParamSpec *
get_param_spec (GObject *object)
{
  if (! param_spec_quark)
    param_spec_quark = g_quark_from_static_string ("gimp-config-param-spec");

  return g_object_get_qdata (object, param_spec_quark);
}


static void
connect_notify (GObject     *config,
                const gchar *property_name,
                GCallback    callback,
                gpointer     callback_data)
{
  gchar *notify_name;

  notify_name = g_strconcat ("notify::", property_name, NULL);

  g_signal_connect_object (config, notify_name, callback, callback_data, 0);

  g_free (notify_name);
}


static void
gegl_path_chooser_button_notify (GObject        *config,
                                 GParamSpec     *param_spec,
                                 GtkFileChooser *button);

static void
gegl_path_chooser_button_callback (GtkFileChooser *button,
                                   GObject        *config)
{
  GParamSpec *param_spec;
  gchar      *value;
  gchar      *utf8;

  param_spec = get_param_spec (G_OBJECT (button));
  if (! param_spec)
    return;

  value = gtk_file_chooser_get_filename (button);
  utf8 = value ? g_filename_to_utf8 (value, -1, NULL, NULL, NULL) : NULL;
  g_free (value);


  gegl_node_get (GEGL_NODE (config),
                 param_spec->name, &value,
                NULL);

  if (! (value && utf8 && strcmp (value, utf8) == 0))
    {
      g_signal_handlers_block_by_func (config,
                                       gegl_path_chooser_button_notify,
                                       button);
      if (utf8)
      gegl_node_set (GEGL_NODE (config),
                    param_spec->name, utf8,
                    NULL);

      g_signal_handlers_unblock_by_func (config,
                                         gegl_path_chooser_button_notify,
                                         button);
    }

  g_free (value);
  g_free (utf8);
}

static void
gegl_path_chooser_button_notify (GObject        *config,
                                 GParamSpec     *param_spec,
                                 GtkFileChooser *button)
{
  gchar *filename;

  gegl_node_get (GEGL_NODE (config),
                 param_spec->name, &filename,
                 NULL);

  g_signal_handlers_block_by_func (button,
                                   gegl_path_chooser_button_callback,
                                   config);

  if (filename)
    gtk_file_chooser_set_filename (button, filename);
  else
    gtk_file_chooser_unselect_all (button);

  g_signal_handlers_unblock_by_func (button,
                                     gegl_path_chooser_button_callback,
                                     config);

  g_free (filename);
}


static GtkWidget *
type_editor_file_path (GtkSizeGroup *col1,
                       GtkSizeGroup *col2,
                       GeglNode     *node,
                       GParamSpec   *param_spec)
{
  GObject   *config = G_OBJECT (node);
  GtkWidget *hbox   = gtk_hbox_new (FALSE, 5);
  GtkWidget *label  = gtk_label_new (param_spec->name);
  gchar     *filename;

  GtkWidget *button = gtk_file_chooser_button_new ("title", GTK_FILE_CHOOSER_ACTION_OPEN);

  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (col1, label);
  gtk_size_group_add_widget (col2, button);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  g_object_set_data (G_OBJECT (button), "node", node);

  gegl_node_get (node, param_spec->name, &filename, NULL);

  set_param_spec (G_OBJECT (button), button, param_spec);

  if (filename)
    {
      gchar *p = filename;
      if (!g_path_is_absolute (p))
        {
          gchar *cwd = getcwd (NULL, 0); /*XXX: linux specific */
          filename = g_build_filename (cwd, p, NULL);
          g_free (p);
          g_free (cwd);
        }
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (button), filename);
      g_free (filename);
    }
  g_signal_connect (G_OBJECT (button), "selection-changed",
                                     G_CALLBACK (gegl_path_chooser_button_callback),
                                     (gpointer) node);

  connect_notify (config, param_spec->name,
                  G_CALLBACK (gegl_path_chooser_button_notify),
                  button);
  gtk_widget_show_all (hbox);
  return hbox;
}



static gboolean
multiline_changed (GtkTextBuffer *buffer,
                   gpointer user_data)
{
  GeglNode   *node = g_object_get_data (G_OBJECT (buffer), "node");
  GParamSpec *param_spec = user_data;
  gchar      *contents;
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_bounds (buffer, &start, &end);
  contents = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);

  gegl_node_set (node, param_spec->name, contents, NULL);

  g_free (contents);
  return TRUE;
}


static GtkWidget *
type_editor_multiline (GtkSizeGroup *col1,
                       GtkSizeGroup *col2,
                       GeglNode     *node,
                       GParamSpec   *param_spec)
{
  GObject       *config    = G_OBJECT (node);
  GtkWidget     *vbox      = gtk_vbox_new (FALSE, 5);
  GtkWidget     *hbox      = gtk_hbox_new (FALSE, 5);
  GtkWidget     *label     = gtk_label_new (param_spec->name);
  GtkWidget     *scroll    = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget     *view      = gtk_text_view_new ();
  GtkTextBuffer *buffer    = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
  GtkWidget     *pad       = gtk_label_new ("");
  GtkWidget     *button    = gtk_file_chooser_button_new ("title", GTK_FILE_CHOOSER_ACTION_OPEN);
  gchar         *contents;

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (scroll), view);
  gtk_widget_set_size_request (scroll, -1, 100);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), GTK_WRAP_CHAR);

  g_object_set_data (G_OBJECT (buffer), "node", node);

  gegl_node_get (node, param_spec->name, &contents, NULL);

  set_param_spec (G_OBJECT (button), button, param_spec);

  if (contents)
    {
      gtk_text_buffer_set_text (buffer, contents, -1);
      g_free (contents);
    }

  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (col1, label);
  gtk_size_group_add_widget (col2, pad);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), pad, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scroll, TRUE, TRUE, 0);


  g_signal_connect (G_OBJECT (buffer), "changed",
                                     G_CALLBACK (multiline_changed),
                                     (gpointer) param_spec);

  connect_notify (config, param_spec->name,
                  G_CALLBACK (gegl_path_chooser_button_notify),
                  button);

  gtk_widget_show_all (vbox);

  return vbox;
}


#if 0
static void scalar_expose (GtkWidget      *widget,
                           GdkEventExpose *eev,
                           gpointer        user_data)
{
  gdouble     width      = widget->allocation.width;
  gdouble     height     = widget->allocation.height;
  cairo_t    *cr         = gdk_cairo_create (widget->window);
  GeglNode   *node       = g_object_get_data (G_OBJECT (widget), "node");
  GParamSpec *param_spec = user_data;
  gdouble     min, max;
  gdouble     def;
  gdouble     value;


  min = G_PARAM_SPEC_DOUBLE (param_spec)->minimum;
  max = G_PARAM_SPEC_DOUBLE (param_spec)->maximum;
  def = G_PARAM_SPEC_DOUBLE (param_spec)->default_value;

  if (min<-100000.0)
    min = -10.0;
  if (max>100000.0)
    max = 10.0;

  cairo_set_font_size (cr, 12.0);
  gegl_node_get (node, param_spec->name, &value, NULL);

  /*cairo_scale (cr, width, height);*/
  cairo_set_line_width (cr, 0.01);
  {
    cairo_pattern_t *pat;
    pat = cairo_pattern_create_linear (0, 0, width, 0);
    cairo_pattern_add_color_stop_rgba (pat, 0, 0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, 1, 1, 1, 1);
    cairo_set_source (cr, pat);
    cairo_rectangle (cr, 0.0, 0.1 * height, 1.0 * width , 0.8 * height);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);
  }

  cairo_set_source_rgb (cr, 0.3, 0.3, 0.7);
  cairo_rectangle (cr, (def - min)/(max-min) * width, 0.0, 2, 1.0 * height);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 0,0,0);
    if(0){
      gchar buf[100];
      sprintf (buf, "%2.2f (default)", def);
      cairo_move_to (cr, (def-min)/(max-min) * width, 0.8 * height);
      cairo_show_text (cr, buf);
    }


  cairo_set_source_rgb (cr, 0.4, 1.0, 0.4);
  cairo_rectangle (cr, (value - min)/(max-min) * width, 0.0, 2, 1.0 * height);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 1.0, 0.6, 0.5);
    if(1){
      gchar buf[100];
      sprintf (buf, "%2.2f", value);
      cairo_move_to (cr, (value-min)/(max-min) * width, 0.8 * height);
      cairo_show_text (cr, buf);
    }

  value=min;
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
  cairo_rectangle (cr, (value - min)/(max-min) * width, 0.0, 2, 1.0 * height);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 1.0, 0.6, 0.5);
    if(0){
      gchar buf[100];
      sprintf (buf, "%2.2f (min)", value);
      cairo_move_to (cr, (value-min)/(max-min) * width, 0.8 * height);
      cairo_show_text (cr, buf);
    }


  value=max;
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
  cairo_rectangle (cr, (value - min)/(max-min) * width, 0.0, 2, 1.0 * height);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 1.0, 0.6, 0.5);
    if(0){
      gchar buf[100];
      sprintf (buf, "%2.2f (max)", value);
      cairo_move_to (cr, (value-min)/(max-min) * width - 20, 0.8 * height);
      cairo_show_text (cr, buf);
    }

  cairo_destroy (cr);
}

static gboolean
scalar_drag_n_motion (GtkWidget *widget, GdkEventMotion *mev, gpointer user_data)
{
  GeglNode   *node = g_object_get_data (G_OBJECT (widget), "node");
  gdouble    width  = widget->allocation.width;
  gdouble    min, max;
  GParamSpec *param_spec = user_data;
  gdouble     value;

  min = G_PARAM_SPEC_DOUBLE (param_spec)->minimum;
  max = G_PARAM_SPEC_DOUBLE (param_spec)->maximum;

  if (min<-100000.0)
    min = -10.0;
  if (max>100000.0)
    max = 10.0;

  value = mev->x/width * (max-min) + min;

  gegl_node_set (node, param_spec->name, value, NULL);

  gtk_widget_queue_draw (widget);
  gdk_window_get_pointer (widget->window, NULL, NULL, NULL);
  return TRUE;
}

static GtkWidget *
type_editor_scalar (GtkSizeGroup *col1,
                    GtkSizeGroup *col2,
                    GeglNode     *node,
                    GParamSpec   *param_spec)
{
  GtkWidget *hbox = gtk_hbox_new (FALSE, 5);
  GtkWidget *drawing_area = gtk_drawing_area_new ();
  GtkWidget *label = gtk_label_new (param_spec->name);

  gtk_widget_set_size_request (drawing_area, 128, 32);

  gtk_widget_set_events (drawing_area,
                         GDK_EXPOSURE_MASK |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);

  g_object_set_data (G_OBJECT (drawing_area), "node", node);

  g_signal_connect (G_OBJECT (drawing_area), "expose-event",
                    G_CALLBACK (scalar_expose), param_spec);
  g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event",
                    G_CALLBACK (scalar_drag_n_motion), param_spec);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
                    G_CALLBACK (scalar_drag_n_motion), param_spec);


  gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
  gtk_size_group_add_widget (col1, label);
  gtk_size_group_add_widget (col2, drawing_area);

  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), drawing_area, TRUE, TRUE, 0);

  return hbox;
}
#endif




static GtkWidget *
type_editor_boolean (GtkSizeGroup *col1,
                     GtkSizeGroup *col2,
                     GeglNode     *node,
                     GParamSpec   *param_spec)
{
  gboolean value;
  const gchar* labeltext = param_spec->name;
  GtkWidget *entry = gtk_check_button_new ();
  g_object_set_data (G_OBJECT (entry), "node", node);
  g_signal_connect (G_OBJECT (entry), "toggled",
                      G_CALLBACK (type_editor_boolean_changed),
                      (gpointer) param_spec);

  gegl_node_get (node, param_spec->name, &value, NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (entry), value);

  return hbox_type_editor (labeltext, entry, col1, col2);
}


static GtkAdjustment *
adjustment_from_param_spec (GParamSpec *param_spec, gdouble value)
{
  gdouble   lower, upper, step_inc, page_inc;
  gdouble   intwidth;

  switch (param_spec->value_type)
    {
      case G_TYPE_INT:
          lower = G_PARAM_SPEC_INT (param_spec)->minimum;
          upper = G_PARAM_SPEC_INT (param_spec)->maximum;
          break;
      case G_TYPE_FLOAT:
          lower = G_PARAM_SPEC_FLOAT (param_spec)->minimum;
          upper = G_PARAM_SPEC_FLOAT (param_spec)->maximum;
      case G_TYPE_DOUBLE:          
          lower = G_PARAM_SPEC_DOUBLE (param_spec)->minimum;
          upper = G_PARAM_SPEC_DOUBLE (param_spec)->maximum;
          break;
      default:          
#if 0
          g_warn_if_reached();  /* broken type dispatch */
#endif
          lower = -10000000;
          upper =  10000000;
    }

  intwidth = upper - lower;
  step_inc = intwidth <= 50 ? 0.1 : intwidth <= 500 ? 1 : 10;
  if (param_spec->value_type == G_TYPE_INT && step_inc < 1)
      step_inc = 1;

  page_inc = 10 * step_inc;

  return (GtkAdjustment*) gtk_adjustment_new (value, lower, upper,
                                              step_inc, page_inc, 0);
}

static GtkWidget *
type_editor_numeric (GtkSizeGroup *col1,
                     GtkSizeGroup *col2,
                     GeglNode     *node,
                     GParamSpec   *param_spec)
{
  const gchar* labeltext = param_spec->name;
  GtkWidget *spin_button;
 
  gdouble value;
  gint n_digits = 2;
  if (param_spec->value_type == G_TYPE_INT)
    {
      gint intval;
      gegl_node_get (node, param_spec->name, &intval, NULL);
      value = intval;
      n_digits = 0;
    }
  else
    {
      gegl_node_get (node, param_spec->name, &value, NULL);
    }

  spin_button = gtk_spin_button_new (adjustment_from_param_spec (param_spec, value),
                                     1, n_digits );
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (spin_button), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spin_button), 6);

  g_object_set_data (G_OBJECT (spin_button), "node", node);
  g_signal_connect (G_OBJECT (spin_button), "value-changed",
                    G_CALLBACK (type_editor_numeric_changed),
                    (gpointer) param_spec);

  return hbox_type_editor(labeltext, spin_button, col1, col2);
}


static GtkWidget *
type_editor_string (GtkSizeGroup *col1,
                    GtkSizeGroup *col2,
                    GeglNode     *node,
                    GParamSpec   *param_spec)
{
  const gchar* labeltext = param_spec->name;
  GtkWidget *entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 6);

  g_object_set_data (G_OBJECT (entry), "node", node);
  g_signal_connect (G_OBJECT (entry), "changed",
                    G_CALLBACK (type_editor_string_changed),
                    (gpointer) param_spec);
    {
      gchar *value;
      gegl_node_get (node, param_spec->name, &value, NULL);
      gtk_entry_set_text (GTK_ENTRY (entry), value);
      g_free (value);
    }

  return hbox_type_editor(labeltext, entry, col1, col2);
}

static void
type_editor_vector_changed (GtkWidget *entry,
                            gpointer   data)
{
  GParamSpec  *param_spec  = data;
  GeglNode    *node        = g_object_get_data (G_OBJECT (entry), "node");
  const gchar *prop_name   = param_spec->name;
  GeglPath *vector;

  gegl_node_get (node, prop_name, &vector, NULL);
  gegl_path_clear (vector);
  gegl_path_parse_string (vector, gtk_entry_get_text (GTK_ENTRY (entry)));
  g_object_unref (vector);
}


static GtkWidget *
type_editor_path (GtkSizeGroup *col1,
                    GtkSizeGroup *col2,
                    GeglNode     *node,
                    GParamSpec   *param_spec)
{
  const gchar* labeltext = param_spec->name;
  GtkWidget *entry = gtk_entry_new ();
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 6);

  g_object_set_data (G_OBJECT (entry), "node", node);
  g_signal_connect (G_OBJECT (entry), "changed",
                    G_CALLBACK (type_editor_vector_changed),
                    (gpointer) param_spec);
    {
      gchar *value;
      GeglPath *vector;

      gegl_node_get (node, param_spec->name, &vector, NULL);
      if (vector)
        {
          value = gegl_path_to_string (vector);
          gtk_entry_set_text (GTK_ENTRY (entry), value);
          g_object_unref (vector);
        }
      g_free (value);
    }

  return hbox_type_editor(labeltext, entry, col1, col2);
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

  properties = gegl_list_properties (gegl_node_get_operation (node), &n_properties);
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

          if (g_type_is_a (G_PARAM_SPEC_TYPE (properties[i]), GEGL_TYPE_PARAM_FILE_PATH))
            {
              prop_editor = type_editor_file_path (col1, col2, node, properties[i]);
            }
          else if (g_type_is_a (G_PARAM_SPEC_TYPE (properties[i]), GEGL_TYPE_PARAM_MULTILINE))
            {
              prop_editor = type_editor_multiline (col1, col2, node, properties[i]);
            }
          else if (properties[i]->value_type == GEGL_TYPE_COLOR)
            {
              prop_editor = type_editor_color (col1, col2, node, properties[i]);
            }
#if 0
          else if (properties[i]->value_type == G_TYPE_DOUBLE)
            {
              prop_editor = type_editor_scalar (col1, col2, node, properties[i]);
            }
#endif
          else if (properties[i]->value_type == G_TYPE_BOOLEAN)
            {
              prop_editor = type_editor_boolean (col1, col2, node, properties[i]);
            }
          else if (properties[i]->value_type == G_TYPE_INT    ||
                   properties[i]->value_type == G_TYPE_FLOAT  ||
                   properties[i]->value_type == G_TYPE_DOUBLE)
            {
              prop_editor = type_editor_numeric (col1, col2, node, properties[i]);
            }
          else if (properties[i]->value_type == G_TYPE_STRING)
            {
              prop_editor = type_editor_string (col1, col2, node, properties[i]);
            }
          else if (properties[i]->value_type == GEGL_TYPE_PATH)
            {
              prop_editor = type_editor_path (col1, col2, node, properties[i]);
            }
          else 
            {
              GParamSpec *param_spec = properties[i];
              const gchar *labeltext = param_spec->name;
              GtkWidget *label2 = gtk_label_new ("unsupported type");
              prop_editor = hbox_type_editor(labeltext, label2, col1, col2);
            }
          gtk_box_pack_start (GTK_BOX (vbox), prop_editor, FALSE, FALSE, 0);
        }
    }

  return vbox;
}

/**
 * gegl_widget_get_cr:
 *
 * Utility method.
 */
cairo_t *
gegl_widget_get_cr (GtkWidget *widget)
{
  gdouble    width  = widget->allocation.width;
  gdouble    height = widget->allocation.height;
  cairo_t   *cr     = gdk_cairo_create (widget->window);
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

/**
 * gegl_node_editor_class_set_pattern:
 *
 * Set the pattern that this matches on.
 */
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
  GSList *subtypes = gegl_type_subtypes (supertype, NULL);
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
gegl_node_editor_new (GeglNode *node)
{
  GType        editor_type;
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

  return g_object_new (editor_type,
                       "node", node,
                       NULL);
}
