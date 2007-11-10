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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#include "gegl-node-editor-plugin.h"

typedef struct _BrightnessContrast      BrightnessContrast;
typedef struct _BrightnessContrastClass BrightnessContrastClass;

struct _BrightnessContrast
{
  GeglNodeEditor  parent_instance;
  gdouble         brightness;
  gdouble         contrast;
  gdouble         x;
  gdouble         y;
  gdouble         v0;
  gdouble         v1;
};

struct _BrightnessContrastClass
{
  GeglNodeEditorClass parent_class;
};

static void construct                         (GeglNodeEditor             *editor);

EDITOR_DEFINE_TYPE (BrightnessContrast, brightness_contrast, GEGL_TYPE_NODE_EDITOR)

static void
brightness_contrast_class_init (BrightnessContrastClass *klass)
{
  GeglNodeEditorClass *node_editor_class = GEGL_NODE_EDITOR_CLASS (klass);
  gegl_node_editor_class_set_pattern (node_editor_class, "brightness-contrast");
  node_editor_class->construct = construct;
}

static void
brightness_contrast_init (BrightnessContrast *self)
{
}

static void expose (GtkWidget      *widget,
                    GdkEventExpose *eev,
                    gpointer        user_data)
{
  cairo_t *cr = gegl_widget_get_cr (widget);
  GeglNodeEditor *node_editor = user_data;
  GeglNode       *node        = node_editor->node;

  gint       i;
  gdouble    handle_radius;
  gdouble    brightness;
  gdouble    contrast;

  gdouble x[3];
  gdouble y[3];

  handle_radius = (1.0 / 3) * 0.4;

  gegl_node_get (node, "brightness", &brightness,
                       "contrast", &contrast,
                       NULL);

  for (i = 0; i < 3; i++)
  {
    x[i] = (1.0 / (3 - 1)) * i;
    y[i] = contrast * (x[i] - 0.5) + 0.5 + brightness;
    y[i] = 1.0 - y[i];
  }


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
  cairo_set_source_rgb (cr, 0.0,0.5,1.0);

  cairo_move_to (cr, x[0], y[0]);
  for (i = 1; i < 3; i++)
    {
      cairo_line_to (cr, x[i], y[i]);
    }
  cairo_stroke (cr);

  for (i = 0; i < 3; i++)
    {
      cairo_arc (cr, x[i], y[i], handle_radius, 0, 2 * 3.1415);
      cairo_save (cr);
      cairo_set_source_rgb (cr, 1.0-y[i], 1.0-y[i], 1.0-y[i]);
      cairo_fill (cr);
      cairo_restore (cr);
      cairo_stroke (cr);
    }

  cairo_destroy (cr);
}

static gboolean
event_press (GtkWidget *widget, GdkEventButton *bev, gpointer user_data)
{
  gdouble    x, y;
  GeglNodeEditor *node_editor = user_data;
  GeglNode       *node        = node_editor->node;
  cairo_t        *cr          = gegl_widget_get_cr (widget);
  BrightnessContrast *self = user_data;

  x = bev->x;
  y = bev->y;

  cairo_device_to_user (cr, &x, &y);

  self->x = x;
  self->y = y;
  gegl_node_get (node, "brightness", &self->brightness,
                       "contrast", &self->contrast,
                       NULL);
  self->v0 = self->contrast * (0.0 - 0.5) + 0.5 + self->brightness;
  self->v1 = self->contrast * (1.0 - 0.5) + 0.5 + self->brightness;

  cairo_destroy (cr);

  return TRUE;
}

static gboolean
event_motion (GtkWidget *widget, GdkEventMotion *mev, gpointer user_data)
{
  gdouble    x, y;
  GeglNodeEditor *node_editor = user_data;
  GeglNode       *node        = node_editor->node;
  cairo_t        *cr          = gegl_widget_get_cr (widget);
  BrightnessContrast *self = user_data;


  gdouble    brightness;
  gdouble    contrast;

  gegl_node_get (node, "brightness", &brightness,
                       "contrast", &contrast,
                       NULL);
  x = mev->x;
  y = mev->y;

  cairo_device_to_user (cr, &x, &y);

  if (self->x < 0.33)
    {
      y = (self->v0) - (y - self->y) - 0.5;
      contrast = (brightness - y) / 0.5;
      gegl_node_set (node, "contrast", contrast, NULL);
    }
  else if (self->x < 0.66)
    {
      brightness = self->brightness + (self->y - y);
      gegl_node_set (node, "brightness", brightness, NULL);
    }
  else
    {
      y = (self->v1) - (y - self->y) - 0.5;
      contrast = (y - brightness) / 0.5;
      gegl_node_set (node, "contrast", contrast, NULL);
    }

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
                    G_CALLBACK (event_motion), self);
  g_signal_connect (G_OBJECT (drawing_area), "button_press_event",
                    G_CALLBACK (event_press), self);

  gtk_container_add (GTK_CONTAINER (self), drawing_area);
}
