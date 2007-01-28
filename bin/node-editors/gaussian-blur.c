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

#include "gegl-node-editor-plugin.h"
#include <math.h>

#define SCALE 20

typedef struct _GaussianBlur
{
  GeglNodeEditor  parent_instance;
} GaussianBlur;

typedef struct _GaussianBlurClass
{
  GeglNodeEditorClass parent_class;
} GaussianBlurClass;

EDITOR_DEFINE_TYPE (GaussianBlur, gaussian_blur, GEGL_TYPE_NODE_EDITOR)

static void construct (GeglNodeEditor             *editor);

static void
gaussian_blur_class_init (GaussianBlurClass *klass)
{
  GeglNodeEditorClass *node_editor_class = GEGL_NODE_EDITOR_CLASS (klass);
  gegl_node_editor_class_set_pattern (node_editor_class, "gaussian-blur");
  node_editor_class->construct = construct;
}

static void
gaussian_blur_init (GaussianBlur *self)
{
}

static void expose (GtkWidget      *widget,
                    GdkEventExpose *eev,
                    gpointer        user_data)
{
  cairo_t *cr = gegl_widget_get_cr (widget);
  GeglNodeEditor *node_editor = user_data;
  GeglNode       *node        = node_editor->node;

  gdouble    radius;

  gegl_node_get (node, "std-dev-x", &radius, NULL);

  cairo_set_line_width (cr, 0.01);
  cairo_set_source_rgb (cr, 0,0,0);
  cairo_rectangle (cr, 0.0, 0.0, 1.0, 1.0);
  cairo_fill (cr);

    {
      cairo_arc (cr, 0.5, 0.5, radius/SCALE, 0, 2 * 3.1415);
      cairo_save (cr);
      cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      cairo_fill (cr);
      cairo_restore (cr);
      cairo_stroke (cr);
    }

  {
    char      buf[100];
    sprintf (buf, "%2.2f", radius);

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
  gdouble         radius;
  GeglNodeEditor *node_editor = user_data;
  GeglNode       *node        = node_editor->node;
  cairo_t        *cr          = gegl_widget_get_cr (widget);

  x = mev->x;
  y = mev->y;

  cairo_device_to_user (cr, &x, &y);
  cairo_destroy (cr);

  radius = sqrt( (x-0.5)*(x-0.5) + (y-0.5) * (y-0.5)) * SCALE;

  gegl_node_set (node, "std-dev-x", radius, NULL);
  gegl_node_set (node, "std-dev-y", radius, NULL);

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
