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

#include <math.h>
#include <babl/babl.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "gegl.h"
#include "gegl-view.h"

enum
{
  PROP_0,
  PROP_NODE,
  PROP_X,
  PROP_Y,
  PROP_SCALE,
  PROP_BLOCK
};


typedef struct _GeglViewPrivate
{
  GeglNode      *node;
  gint           x;
  gint           y;
  gdouble        scale;
  gboolean       block;    /* blocking render */

  guint          monitor_id;
  GeglProcessor *processor;
} GeglViewPrivate;


G_DEFINE_TYPE (GeglView, gegl_view, GTK_TYPE_DRAWING_AREA)
#define GEGL_VIEW_GET_PRIVATE(obj) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GEGL_TYPE_VIEW, GeglViewPrivate))

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
#ifdef HAVE_GTK2
static gboolean  expose_event         (GtkWidget      *widget,
                                       GdkEventExpose *event);
#endif
#ifdef HAVE_GTK3
static gboolean  draw                 (GtkWidget * widget,
                                       cairo_t *cr);
#endif

static void      gegl_view_repaint       (GeglView *view);

static void
gegl_view_class_init (GeglViewClass * klass)
{
  GObjectClass   *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class  = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

#ifdef HAVE_GTK2
  widget_class->expose_event        = expose_event;
#endif

#ifdef HAVE_GTK3
  widget_class->draw                = draw;
#endif

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
  g_object_class_install_property (gobject_class, PROP_BLOCK,
                                   g_param_spec_boolean ("block",
                                                        "Blocking render",
                                                        "Make sure all data requested to blit is generated.",
                                                        FALSE,
                                                        G_PARAM_READWRITE));


   g_type_class_add_private (klass, sizeof (GeglViewPrivate));
}

static void
gegl_view_init (GeglView *self)
{
  GeglViewPrivate *priv = GEGL_VIEW_GET_PRIVATE (self);
  priv->node        = NULL;
  priv->x           = 0;
  priv->y           = 0;
  priv->scale       = 1.0;
  priv->monitor_id  = 0;
  priv->processor   = NULL;
}

static void
finalize (GObject *gobject)
{
  GeglView * self = GEGL_VIEW (gobject);
  GeglViewPrivate *priv = GEGL_VIEW_GET_PRIVATE (self);

  if (priv->monitor_id)
    {  
      g_source_remove (priv->monitor_id);
      priv->monitor_id = 0;
    }

  if (priv->node)
    g_object_unref (priv->node);

  if (priv->processor)
    g_object_unref (priv->processor);

  G_OBJECT_CLASS (gegl_view_parent_class)->finalize (gobject);
}

static void
computed_event (GeglNode      *self,
                GeglRectangle *rect,
                GeglView      *view)
{
  GeglViewPrivate *priv = GEGL_VIEW_GET_PRIVATE (view);
  gint x = priv->scale * (rect->x) - priv->x;
  gint y = priv->scale * (rect->y) - priv->y;
  gint w = ceil (priv->scale * rect->width  + 1);
  gint h = ceil (priv->scale * rect->height + 1);

  gtk_widget_queue_draw_area (GTK_WIDGET (view), x, y, w, h);
}


static void
invalidated_event (GeglNode      *self,
                   GeglRectangle *rect,
                   GeglView      *view)
{
  gegl_view_repaint (view);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglView *self = GEGL_VIEW (gobject);
  GeglViewPrivate *priv = GEGL_VIEW_GET_PRIVATE (self);

  switch (property_id)
    {
    case PROP_NODE:
      if (priv->node)
        {
          g_object_unref (priv->node);
        }

      if (g_value_get_object (value))
        {
          priv->node = GEGL_NODE (g_value_dup_object (value));

          g_signal_connect_object (priv->node, "computed",
                                   G_CALLBACK (computed_event),
                                   self, 0);
          g_signal_connect_object (priv->node, "invalidated",
                                   G_CALLBACK (invalidated_event),
                                   self, 0);
          gegl_view_repaint (self);
        }
      else
        {
          priv->node = NULL;
        }
      break;
    case PROP_X:
      priv->x = g_value_get_int (value);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;
    case PROP_BLOCK:
      priv->block = g_value_get_boolean (value);
      break;
    case PROP_Y:
      priv->y = g_value_get_int (value);
      gtk_widget_queue_draw (GTK_WIDGET (self));
      break;
    case PROP_SCALE:
      priv->scale = g_value_get_double (value);
      gtk_widget_queue_draw (GTK_WIDGET (self));
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
  GeglViewPrivate *priv = GEGL_VIEW_GET_PRIVATE (self);

  switch (property_id)
    {
    case PROP_NODE:
      g_value_set_object (value, priv->node);
      break;
    case PROP_X:
      g_value_set_int (value, priv->x);
      break;
    case PROP_BLOCK:
      g_value_set_boolean (value, priv->block);
      break;
    case PROP_Y:
      g_value_set_int (value, priv->y);
      break;
    case PROP_SCALE:
      g_value_set_double (value, priv->scale);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static void
draw_implementation (GeglViewPrivate *priv, cairo_t *cr, GdkRectangle *rect)
{
  cairo_surface_t *surface = NULL;
  guchar          *buf = NULL;
  Babl            *format = NULL;
  GeglRectangle   roi;

  roi.x = priv->x + rect->x;
  roi.y = priv->y + rect->y;
  roi.width  = rect->width;
  roi.height = rect->height;

  buf = g_malloc ((roi.width) * (roi.height) * 4);

  format = babl_format_new (babl_model ("RGBA"), babl_type ("u8"),
                            babl_component ("B"),
                            babl_component ("G"),
                            babl_component ("R"),
                            babl_component ("A"),
                            NULL);
  gegl_node_blit (priv->node,
                  priv->scale,
                  &roi,
                  format,
                  (gpointer)buf,
                  GEGL_AUTO_ROWSTRIDE,
                  GEGL_BLIT_CACHE | (priv->block ? 0 : GEGL_BLIT_DIRTY));

  surface = cairo_image_surface_create_for_data (buf, 
                                                 CAIRO_FORMAT_ARGB32, 
                                                 roi.width, roi.height, 
                                                 roi.width*4);
  cairo_set_source_surface (cr, surface, rect->x, rect->y);
  cairo_paint (cr);

  cairo_surface_finish (surface);
  g_free (buf);

}

#ifdef HAVE_GTK3
static gboolean
draw (GtkWidget * widget, cairo_t *cr)
{
  GeglView      *view = GEGL_VIEW (widget);
  GeglViewPrivate *priv = GEGL_VIEW_GET_PRIVATE (view);
  GdkRectangle rect;

  if (!priv->node)
    return FALSE;

  gdk_cairo_get_clip_rectangle (cr, &rect);

  draw_implementation (priv, cr, &rect);

  gegl_view_repaint (view);

  return FALSE;
}
#endif

#ifdef HAVE_GTK2
static gboolean
expose_event (GtkWidget      *widget,
              GdkEventExpose *event)
{
  GeglView      *view = GEGL_VIEW (widget);
  GeglViewPrivate *priv = GEGL_VIEW_GET_PRIVATE (view);
  cairo_t      *cr;
  GdkRectangle rect;

  if (!priv->node)
    return FALSE;

  cr = gdk_cairo_create (widget->window);
  gdk_cairo_region (cr, event->region);
  cairo_clip (cr);
  gdk_region_get_clipbox (event->region, &rect);

  draw_implementation (priv, cr, &rect);
  cairo_destroy (cr);

  gegl_view_repaint (view);

  return FALSE;
}
#endif

static gboolean
task_monitor (GeglView *view)
{
  GeglViewPrivate *priv = GEGL_VIEW_GET_PRIVATE (view);
  if (priv->processor==NULL)
    return FALSE;
  if (gegl_processor_work (priv->processor, NULL))
    return TRUE;

  priv->monitor_id = 0;

  return FALSE;
}

void
gegl_view_repaint (GeglView *view)
{
  GtkWidget       *widget = GTK_WIDGET (view);
  GeglViewPrivate *priv   = GEGL_VIEW_GET_PRIVATE (view);
  GeglRectangle    roi;
  GtkAllocation    allocation;

  roi.x = priv->x / priv->scale;
  roi.y = priv->y / priv->scale;
  gtk_widget_get_allocation (widget, &allocation);
  roi.width = ceil(allocation.width / priv->scale+1);
  roi.height = ceil(allocation.height / priv->scale+1);

  if (priv->monitor_id == 0)
    {
      priv->monitor_id = g_idle_add_full (G_PRIORITY_LOW,
                                          (GSourceFunc) task_monitor, view,
                                          NULL);

      if (priv->processor == NULL)
        {
          if (priv->node)
            priv->processor = gegl_node_new_processor (priv->node, &roi);
        }
    }

  if (priv->processor)
    gegl_processor_set_rectangle (priv->processor, &roi);
}
