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

#include <glib-object.h>
#include "gegl.h"
#include "gegl-view.h"

enum
{
  PROP_0,
  PROP_NODE,
  PROP_X,
  PROP_Y,
  PROP_SCALE
};

static void      gegl_view_class_init (GeglViewClass  *klass);
static void      gegl_view_init       (GeglView       *self);
static void      finalize             (GObject        *gobject);
static void      set_property         (GObject        *gobject,
                                       guint           prop_id,
                                       const GValue   *value,
                                       GParamSpec     *pspec);
static void      get_property         (GObject        *gobject,
                                       guint           prop_id,
                                       GValue         *value,
                                       GParamSpec     *pspec);
static gboolean  motion_notify_event  (GtkWidget      *widget,
                                       GdkEventMotion *event);
static GObject *
gegl_view_constructor (GType                  type,
                       guint                  n_params,
                       GObjectConstructParam *params);
static gboolean
expose_event (GtkWidget *widget, GdkEventExpose * event);

G_DEFINE_TYPE (GeglView, gegl_view, GTK_TYPE_DRAWING_AREA)

static GObjectClass *parent_class = NULL;

static void
gegl_view_class_init (GeglViewClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  
  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->constructor = gegl_view_constructor;

  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x",
                                                     "X",
                                                     "X origin",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                     "Y",
                                                     "Y origin",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_SCALE,
                                   g_param_spec_double ("scale",
                                                        "Scale",
                                                        "Zoom factor",
                                                        0.0, 100.0, 1.00,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                                        "Node",
                                                        "The node to render",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void
gegl_view_init (GeglView *self)
{
  self->node 
       = NULL;
  self->x           = 0;
  self->y           = 0;
  self->prev_x      = -1;
  self->prev_y      = -1;
  self->dirty_rects = NULL;
  self->scale       = 1.0;
}

static void
finalize (GObject *gobject)
{
  GeglView * self = GEGL_VIEW (gobject);

  if (self->projection)
    g_object_unref (self->projection);
  if (self->node)
    g_object_unref (self->node);

  G_OBJECT_CLASS (gegl_view_parent_class)->finalize (gobject);
}

static void computed_event (GeglProjection *self,
                            void           *foo,
                            void           *user_data)
{
  GeglRect  rect = *(GeglRect*)foo;
  GeglView *view = GEGL_VIEW (user_data);
  GtkWidget *widget = GTK_WIDGET (user_data);

  /* FIXME: check that the area is relevant for us */

  rect.x -= view->x;
  rect.y -= view->y;
  rect.x *= view->scale;
  rect.y *= view->scale;
  rect.w *= view->scale;
  rect.h *= view->scale;

  gtk_widget_queue_draw_area (widget, rect.x, rect.y,
                                      rect.w, rect.h);
}


static void invalidated_event (GeglProjection *self,
                               void           *foo,
                               void           *user_data)
{
  gegl_view_repaint (GEGL_VIEW (user_data));
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglView *self = GEGL_VIEW (gobject);

  switch (property_id)
    {
    case PROP_NODE:
      /* FIXME: a view should probably be made from a projection */
      if (self->node)
        g_object_unref (self->node);
      self->node = GEGL_NODE (g_value_dup_object (value));
      if (self->projection)
        g_object_unref (self->projection);
      self->projection = g_object_new (GEGL_TYPE_PROJECTION,
                                       "node", self->node,
                                       NULL);
      g_signal_connect (G_OBJECT (self->projection), "computed",
                        (GCallback)computed_event,
                        self);
      g_signal_connect (G_OBJECT (self->projection), "invalidated",
                        (GCallback)invalidated_event,
                        self);
      gegl_view_repaint (self);
      break;
    case PROP_X:
      self->x = g_value_get_int (value);
      break;
    case PROP_Y:
      self->y = g_value_get_int (value);
      break;
    case PROP_SCALE:
      self->scale = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static void
get_property (GObject      *gobject,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglView *self = GEGL_VIEW (gobject);

  switch (property_id)
    {
    case PROP_NODE:
      g_value_set_object (value, self->node);
      break;
    case PROP_X:
      g_value_set_int (value, self->x);
      break;
    case PROP_Y:
      g_value_set_int (value, self->y);
      break;
    case PROP_SCALE:
      g_value_set_double (value, self->scale);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static GObject *
gegl_view_constructor (GType                  type,
                       guint                  n_params,
                       GObjectConstructParam *params)
{
  GObject         *object;
  GtkWidget       *widget;
  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  widget = GTK_WIDGET (object);

  gtk_widget_set_events (widget, GDK_EXPOSURE_MASK
                                |GDK_BUTTON_PRESS_MASK
                                |GDK_POINTER_MOTION_MASK
                                |GDK_POINTER_MOTION_HINT_MASK);
  gtk_signal_connect (GTK_OBJECT (widget), "expose_event",
                      G_CALLBACK (expose_event), NULL);
  g_signal_connect (G_OBJECT (widget), "motion_notify_event",
		    G_CALLBACK (motion_notify_event), NULL);
  gtk_widget_set_double_buffered (widget, FALSE);

  return object;
}

static gboolean motion_notify_event (GtkWidget      *widget,
                                     GdkEventMotion *event)
{
  GeglView *view = GEGL_VIEW (widget);
  gint x, y;
  GdkModifierType state;

  if (event->is_hint)
    gdk_window_get_pointer (event->window, &x, &y, &state);
  else
    {
      x = event->x;
      y = event->y;
      state = event->state;
    }
    
  if (state & GDK_BUTTON1_MASK)
    {
      view->x += (view->prev_x-x) / view->scale;
      view->y += (view->prev_y-y) / view->scale;
      gdk_window_scroll (widget->window, -(view->prev_x-x)    ,
                                         -(view->prev_y-y)   );
    }
  else if (state & GDK_BUTTON3_MASK)
    {
      gdouble diff;
      view->x+= (x) / view->scale;
      view->y+= (y) / view->scale;

      diff = (view->prev_y-y) / 300.0;
      if (diff<0.0)
        {
          view->scale *= 1.0 + diff;
        }
      else
        {
          view->scale /= 1.0 - diff;
        }

      view->x-= (x) / view->scale;
      view->y-= (y) / view->scale;
      /*gegl_view_repaint (self);*/
      gtk_widget_queue_draw (GTK_WIDGET (view));
      g_warning ("new zoom: %f", view->scale);
    }
  view->prev_x = x;
  view->prev_y = y;
  
  return TRUE;
}

static gboolean
expose_event (GtkWidget *widget, GdkEventExpose * event)
{
  GeglView      *view;
  GeglRect roi = {0,0,0,0};
  GdkRectangle  *rectangles;
  gint           count;
  gint           i;

  view = GEGL_VIEW (widget);

  if (!view->node)
    return FALSE;
  if (!view->projection)
    return FALSE;
  
  gdk_region_get_rectangles (event->region, &rectangles, &count);
  
  for (i=0;i<count;i++)
    {
      guchar *buf;
      roi.x=view->x + rectangles[i].x / view->scale;
      roi.y=view->y + rectangles[i].y / view->scale;
      roi.w=rectangles[i].width;
      roi.h=rectangles[i].height;

      buf = g_malloc ((roi.w+1) * (roi.h+1) * 4);
      /* FIXME: this padding should not be needed, but it avoids some segfaults */
      gegl_buffer_get_rect_fmt_scale (view->projection->buffer,
                                      &roi, buf, babl_format ("R'G'B'A u8"), view->scale);

      gdk_draw_rgb_32_image (widget->window,
                             widget->style->black_gc,
                             rectangles[i].x, rectangles[i].y,
                             rectangles[i].width, rectangles[i].height,
                             GDK_RGB_DITHER_NONE,
                             buf, roi.w * 4);
      g_free (buf);
    }
  gegl_view_repaint (view);
  g_free (rectangles);
  gdk_window_get_pointer (event->window, NULL, NULL, NULL);

  return TRUE;
}

void gegl_view_repaint (GeglView *view)
{
  GtkWidget *widget = GTK_WIDGET (view);
  GeglRect roi={view->x, view->y,
                widget->allocation.width / view->scale,
                widget->allocation.height / view->scale};

  /* forget all already queued repaints */
  gegl_projection_forget_queue (view->projection, NULL);
  /* then enqueue our selves */
  gegl_projection_update_rect (view->projection, roi);
}

GeglProjection *gegl_view_get_projection (GeglView *view)
{
  return view->projection;
}
