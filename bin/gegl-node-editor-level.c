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
#include "editor.h"

typedef struct _GeglNodeEditorLevel      GeglNodeEditorLevel;
typedef struct _GeglNodeEditorLevelClass GeglNodeEditorLevelClass;

struct _GeglNodeEditorLevel
{
  GeglNodeEditor  parent_instance;
};

struct _GeglNodeEditorLevelClass
{
  GeglNodeEditorClass parent_class;
};

static void gegl_node_editor_level_class_init (GeglNodeEditorLevelClass   *klass);
static void gegl_node_editor_level_init       (GeglNodeEditorLevel        *self);
static void construct                         (GeglNodeEditor             *editor);

G_DEFINE_TYPE (GeglNodeEditorLevel, gegl_node_editor_level, GEGL_TYPE_NODE_EDITOR)

static void
gegl_node_editor_level_class_init (GeglNodeEditorLevelClass *klass)
{
  GeglNodeEditorClass *node_editor_class = GEGL_NODE_EDITOR_CLASS (klass);

  node_editor_class->construct = construct;
}

static void
gegl_node_editor_level_init (GeglNodeEditorLevel *self)
{
}

static void expose (GtkWidget      *widget,
                    GdkEventExpose *eev,
                    gpointer        user_data)
{
  cairo_t *cr = gegl_widget_get_cr (widget);
  GeglNodeEditor *node_editor = user_data;
  GeglNode       *node        = node_editor->node;

  gdouble    handle_radius;
  gdouble    level;

  handle_radius = (1.0 / 3) * 0.4;

  gegl_node_get (node, "value", &level, NULL);

  {
    cairo_pattern_t *pat;
    pat = cairo_pattern_create_linear (0, 0, 1, 0);
    cairo_pattern_add_color_stop_rgba (pat, 0, 0, 0, 0, 1);
    cairo_pattern_add_color_stop_rgba (pat, 1, 1, 1, 1, 1);
    cairo_set_source (cr, pat);

    cairo_rectangle (cr, 0.0, 0.0, 1.0, 1.0);
    cairo_fill (cr);
    cairo_pattern_destroy (pat);
  }

  cairo_set_line_width (cr, 0.01);
  cairo_set_source_rgb (cr, 0,0,0);
  cairo_rectangle (cr, 0.0, 0.5, level, 0.5);
  cairo_fill (cr);
  cairo_set_source_rgb (cr, 1,1,1);
  cairo_rectangle (cr, level, 0.5, 1 - level, 0.5);
  cairo_fill (cr);

  {
    char      buf[100];
    sprintf (buf, "%2.2f", level);

    cairo_set_source_rgb (cr, 0, 0.5, 1);
    cairo_move_to (cr, 0.0, 1.0);
    cairo_show_text (cr, buf);
  }

  cairo_destroy (cr);
}

static gboolean
drag_n_motion (GtkWidget *widget, GdkEventMotion *mev, gpointer user_data)
{
  gdouble         x, y;
  GeglNodeEditor *node_editor = user_data;
  GeglNode       *node        = node_editor->node;
  cairo_t        *cr          = gegl_widget_get_cr (widget);

  x = mev->x;
  y = mev->y;

  cairo_device_to_user (cr, &x, &y);
  cairo_destroy (cr);

  gegl_node_set (node, "value", x, NULL);

  gtk_widget_queue_draw (widget);
  gdk_window_get_pointer (widget->window, NULL, NULL, NULL);

  gegl_gui_flush ();
  return TRUE;
}


static void
construct (GeglNodeEditor *self)
{
  GtkWidget *drawing_area;
  gtk_box_set_homogeneous (GTK_BOX (self), FALSE);
  gtk_box_set_spacing (GTK_BOX (self), 0);

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, 128, 128);

  gtk_widget_set_events (drawing_area,
                         GDK_EXPOSURE_MASK            |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON1_MOTION_MASK      |
                         GDK_BUTTON_PRESS_MASK        |
                         GDK_BUTTON_RELEASE_MASK);

  g_signal_connect (G_OBJECT (drawing_area), "expose-event",
                    G_CALLBACK (expose), self);
  g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event",
                    G_CALLBACK (drag_n_motion), self);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
                    G_CALLBACK (drag_n_motion), self);

  gtk_container_add (GTK_CONTAINER (self), drawing_area);
}
