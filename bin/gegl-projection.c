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

#include <stdio.h>
#include <string.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "gegl.h"
#include <babl/babl.h>
#include "gegl-projection.h"

#include <gdk/gdk.h>

#define MAX_PIXELS (128*128)   /* maximum size of area computed in one idle cycle */

enum
{
  PROP_0,
  PROP_NODE
};

enum
{
  INVALIDATED,
  COMPUTED,
  LAST_SIGNAL
};

static void            gegl_projection_class_init           (GeglProjectionClass *klass);
static void            gegl_projection_init                 (GeglProjection      *self);
static void            finalize                       (GObject       *self_object);
static void            set_property                   (GObject       *gobject,
                                                       guint          prop_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);
static void            get_property                   (GObject       *gobject,
                                                       guint          prop_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);
static gboolean        task_render                    (gpointer       foo);
static gboolean        task_monitor                   (gpointer       foo);

G_DEFINE_TYPE (GeglProjection, gegl_projection, G_TYPE_OBJECT);

static GObjectClass *parent_class = NULL;
static guint         projection_signals[LAST_SIGNAL] = {0};

static GObject *
gegl_projection_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject         *object;
  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  GeglProjection *self = GEGL_PROJECTION (object);

  self->valid_region = gdk_region_new ();
  self->queued_region = gdk_region_new ();
  self->format = babl_format ("R'G'B'A u8");
  self->buffer = g_object_new (GEGL_TYPE_BUFFER,
                               "format", self->format,
                               "x", -65536,
                               "y", -65536,
                               "width", 65536*2,
                               "height", 65536*2,
                               NULL);
 
  if (self->monitor_id == 0) 
    self->monitor_id = g_idle_add_full (
           G_PRIORITY_LOW, (GSourceFunc) task_monitor, self, NULL);

  return object;
}


static void
gegl_projection_class_init (GeglProjectionClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  gobject_class->constructor = gegl_projection_constructor;

  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                   "GeglNode",
                                   "The GeglNode to render results from",
                                   GEGL_TYPE_NODE,
                                   G_PARAM_WRITABLE |
                                   G_PARAM_CONSTRUCT));

  projection_signals[COMPUTED] =
      g_signal_new ("computed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      0    /* class offset*/,
      NULL /* accumulator */,
      NULL /* accu_data */,
      g_cclosure_marshal_VOID__POINTER,
      G_TYPE_NONE /* return type */,
      1 /* n_params */,
      G_TYPE_POINTER /* param_types */);

  projection_signals[INVALIDATED] =
      g_signal_new ("invalidated", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      0    /* class offset*/,
      NULL /* accumulator */,
      NULL /* accu_data */,
      g_cclosure_marshal_VOID__POINTER,
      G_TYPE_NONE /* return type */,
      1 /* n_params */,
      G_TYPE_POINTER /* param_types */);
  
}

static void
gegl_projection_init (GeglProjection *self)
{
  self->buffer = NULL;
  self->node = NULL;
  self->render_id = 0;
  self->monitor_id = 0;
}

static void
finalize (GObject *gobject)
{
  GeglProjection *self = GEGL_PROJECTION (gobject);
  while (g_idle_remove_by_data (gobject));
  if (self->buffer)
    g_object_unref (self->buffer);
  if (self->node)
    g_object_unref (self->node);
  if (self->valid_region)
    gdk_region_destroy (self->valid_region);
  if (self->queued_region)
    gdk_region_destroy (self->queued_region);
  G_OBJECT_CLASS (gegl_projection_parent_class)->finalize (gobject);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglProjection *self = GEGL_PROJECTION (gobject);
  switch (property_id)
    {
      case PROP_NODE:
        if (self->node)
          g_object_unref (self->node);
        self->node = GEGL_NODE (g_value_dup_object (value));
        
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglProjection *self = GEGL_PROJECTION (gobject);
  switch (property_id)
    {
      case PROP_NODE:
        g_value_set_object (value, self->node);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}


void gegl_projection_forget_queue (GeglProjection *self,
                             GeglRect       *roi)
{

  if (roi)
    {
      GdkRectangle gdk_rect;
      GdkRegion *temp_region;
      memcpy (&gdk_rect, roi, sizeof (GdkRectangle));
      temp_region = gdk_region_rectangle (&gdk_rect);
      gdk_region_subtract (self->queued_region, temp_region);
    }
  else
    {
      if (self->queued_region)
        gdk_region_destroy (self->queued_region);
      self->queued_region = gdk_region_new ();

      while (self->dirty_rects)
         self->dirty_rects = g_list_remove (self->dirty_rects,
            self->dirty_rects->data);
    }
}


void gegl_projection_forget (GeglProjection *self,
                             GeglRect       *roi)
{
  gegl_projection_forget_queue (self, roi);

  if (roi)
    {
      GdkRectangle gdk_rect;
      GdkRegion *temp_region;
      memcpy (&gdk_rect, roi, sizeof (GdkRectangle));
      temp_region = gdk_region_rectangle (&gdk_rect);
      gdk_region_subtract (self->valid_region, temp_region);
    }
  else
    {
      if (self->valid_region)
        gdk_region_destroy (self->valid_region);
      self->valid_region = gdk_region_new ();
    }
}

void gegl_projection_update_rect (GeglProjection *self,
                                  GeglRect        roi)
{
  GdkRectangle gdk_rect;
  GdkRegion *temp_region;

  g_assert (sizeof (GeglRect) == sizeof (GdkRectangle));
  memcpy (&gdk_rect, &roi, sizeof (GdkRectangle));
  temp_region = gdk_region_rectangle (&gdk_rect);

  gdk_region_union (self->queued_region, temp_region);
  gdk_region_subtract (self->queued_region, self->valid_region);

  gdk_region_destroy (temp_region);

  /* start a monitor if the monitor of the projection is dormant */
  if (self->monitor_id == 0) 
    self->monitor_id = g_idle_add_full (
           G_PRIORITY_LOW, (GSourceFunc) task_monitor, self, NULL);
}

/* renders a rectangle, in chunks as an idle function */
static gboolean task_render (gpointer foo)
{
  GeglProjection  *projection = GEGL_PROJECTION (foo);
  gint max_area = MAX_PIXELS;

  GeglRect *dr;

  if (projection->dirty_rects)
    {
      guchar *buf;

      dr = projection->dirty_rects->data;

      if (dr->h * dr->w > max_area && 1)
        {
          gint band_size;
         
          if (dr->h > dr->w)
            {
              band_size = dr->h / 2;

              if (band_size<1)
                band_size=1;

              GeglRect *fragment = g_malloc (sizeof (GeglRect));
              *fragment = *dr;

              fragment->h = band_size;
              dr->h-=band_size;
              dr->y+=band_size;

              projection->dirty_rects = g_list_prepend (projection->dirty_rects, fragment);
              return TRUE;
            }
          else 
            {
              band_size = dr->w / 2;

              if (band_size<1)
                band_size=1;

              GeglRect *fragment = g_malloc (sizeof (GeglRect));
              *fragment = *dr;

              fragment->w = band_size;
              dr->w-=band_size;
              dr->x+=band_size;

              projection->dirty_rects = g_list_prepend (projection->dirty_rects, fragment);
              return TRUE;
            }
        }
      
      projection->dirty_rects = g_list_remove (projection->dirty_rects, dr);

      if (!dr->w || !dr->h)
        return TRUE;
      
      buf = g_malloc ((dr->w) * (dr->h) * 4);
      g_assert (buf);

      gegl_node_blit_buf (projection->node, dr, babl_format ("R'G'B'A u8"), 0, (gpointer*) buf);
      gegl_buffer_set_rect_fmt (projection->buffer, dr, buf, babl_format ("R'G'B'A u8"));
      
      gdk_region_union_with_rect (projection->valid_region, (GdkRectangle*)dr);

      g_signal_emit (projection, projection_signals[COMPUTED], 0, dr, NULL, NULL);

      g_free (buf);
      g_free (dr);
    }
  
  if (!projection->dirty_rects)
    { /* job done */
      projection->render_id = 0;
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}

static gboolean task_monitor (gpointer foo)
{
  GeglProjection  *projection = GEGL_PROJECTION (foo);
  GeglRect dirty_rect = gegl_node_get_dirty_rect (projection->node);
  if (dirty_rect.w != 0 &&
      dirty_rect.h != 0)
    {
      GdkRectangle   rectangle;
      GdkRegion     *region;

      rectangle.width = dirty_rect.w;
      rectangle.height = dirty_rect.h;
      rectangle.x = dirty_rect.x;
      rectangle.y = dirty_rect.y;

      region = gdk_region_rectangle (&rectangle);
      gdk_region_subtract (projection->valid_region, region);

      if (!gdk_region_empty (region))
        {
          g_signal_emit (projection, projection_signals[INVALIDATED], 0, NULL, NULL, NULL);
        }
      gdk_region_destroy (region);
    }

  gegl_node_clear_dirt (projection->node);

  gdk_region_subtract (projection->queued_region, projection->valid_region);

  if (!gdk_region_empty (projection->queued_region) &&
      !projection->dirty_rects &&
      projection->render_id == 0) 
    {
      GdkRectangle  *rectangles;
      gint           n_rectangles;
      gint           i;

      gdk_region_get_rectangles (projection->queued_region, &rectangles, &n_rectangles);

      for (i=0; i<n_rectangles && i<1; i++)
        {
          GeglRect roi = *((GeglRect*)&rectangles[i]);
          GeglRect *dr;
          GdkRegion *tr = gdk_region_rectangle ((void*)&roi);
          gdk_region_subtract (projection->queued_region, tr);
          gdk_region_destroy (tr);
         
          dr = g_malloc(sizeof (GeglRect));
          *dr = roi;
          projection->dirty_rects = g_list_append (projection->dirty_rects, dr);

          /* start a renderer if it isn't already running */
          if (projection->render_id == 0)
            projection->render_id = g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc) task_render, projection, NULL);
        }
      g_free (rectangles);
    }
  if (gdk_region_empty (projection->queued_region))
    { /* job done */
      projection->monitor_id = 0;
      return FALSE;
    }
  else
    {
      return TRUE;
    }
}

gboolean gegl_projection_render (GeglProjection *self)
{
     while (self->dirty_rects)
      {
        task_render (self);
      }
     if (!gdk_region_empty (self->queued_region))
       task_monitor (self);
  return (self->dirty_rects != NULL);
}

GeglBuffer *gegl_projection_get_buffer (GeglProjection *self)
{
  return self->buffer;
}
