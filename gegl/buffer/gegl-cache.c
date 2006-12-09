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

#include "gegl-plugin.h"
#include "gegl-types.h"
#include <babl/babl.h>
#include "gegl-projection.h"
#include "gegl-region.h"


#define MAX_PIXELS (128*128)   /* maximum size of area computed in one idle cycle */

enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_NODE
};

enum
{
  INVALIDATED,
  COMPUTED,
  LAST_SIGNAL
};

static void            gegl_projection_class_init     (GeglProjectionClass *klass);
static void            gegl_projection_init           (GeglProjection      *self);
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

G_DEFINE_TYPE (GeglProjection, gegl_projection, GEGL_TYPE_BUFFER);

static GObjectClass *parent_class = NULL;
static guint         projection_signals[LAST_SIGNAL] = {0};

static GObject *
gegl_projection_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject         *object;
  GeglProjection *self;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self = GEGL_PROJECTION (object);

  self->valid_region = gegl_region_new ();
  self->queued_region = gegl_region_new ();
  self->buffer = GEGL_BUFFER (self);
  self->format = self->buffer->format;
 
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

  /* overriding pspecs for properties in parent class */
  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x", "x", "local origin's offset relative to source origin",
                                                     G_MININT, G_MAXINT, -10240,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
 g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y", "y", "local origin's offset relative to source origin",
                                                     G_MININT, G_MAXINT, -10240,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width", "width", "pixel width of buffer",
                                                     -1, G_MAXINT, 10240*2,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", "height", "pixel height of buffer",
                                                     -1, G_MAXINT, 10240*2,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));


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

  /* thus providing a default value for GeglProjection, that overrides the NULL
   * from GeglBuffer */
  GEGL_BUFFER (self)->format = (gpointer)babl_format ("R'G'B'A u8");
}

static void
finalize (GObject *gobject)
{
  GeglProjection *self = GEGL_PROJECTION (gobject);
  while (g_idle_remove_by_data (gobject));
  if (self->node)
    g_object_unref (self->node);
  if (self->valid_region)
    gegl_region_destroy (self->valid_region);
  if (self->queued_region)
    gegl_region_destroy (self->queued_region);
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

      /* For the rest, upchaining to the property implementation in GeglBuffer */
      case PROP_X:
        g_object_set_property (gobject, "GeglBuffer::x", value);
        break;
      case PROP_Y:
        g_object_set_property (gobject, "GeglBuffer::y", value);
        break;
      case PROP_WIDTH:
        g_object_set_property (gobject, "GeglBuffer::width", value);
        break;
      case PROP_HEIGHT:
        g_object_set_property (gobject, "GeglBuffer::height", value);
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

      /* For the rest, upchaining to the property implementation in GeglBuffer */
      case PROP_X:
        g_object_get_property (gobject, "GeglBuffer::x", value);
        break;
      case PROP_Y:
        g_object_get_property (gobject, "GeglBuffer::y", value);
        break;
      case PROP_WIDTH:
        g_object_get_property (gobject, "GeglBuffer::width", value);
        break;
      case PROP_HEIGHT:
        g_object_get_property (gobject, "GeglBuffer::height", value);
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
      GeglRegion *temp_region;
      temp_region = gegl_region_rectangle (roi);
      gegl_region_subtract (self->queued_region, temp_region);
      gegl_region_destroy (temp_region);
    }
  else
    {
      if (self->queued_region)
        gegl_region_destroy (self->queued_region);
      self->queued_region = gegl_region_new ();

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
      GeglRegion *temp_region;
      temp_region = gegl_region_rectangle (roi);
      gegl_region_subtract (self->valid_region, temp_region);
      gegl_region_destroy (temp_region);
    }
  else
    {
      if (self->valid_region)
        gegl_region_destroy (self->valid_region);
      self->valid_region = gegl_region_new ();
    }
}

void gegl_projection_update_rect (GeglProjection *self,
                                  GeglRect        roi)
{
  GeglRegion *temp_region;

  temp_region = gegl_region_rectangle (&roi);

  gegl_region_union (self->queued_region, temp_region);
  gegl_region_subtract (self->queued_region, self->valid_region);

  gegl_region_destroy (temp_region);

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
      
      buf = g_malloc (dr->w * dr->h * gegl_buffer_px_size (GEGL_BUFFER (projection)));
      g_assert (buf);

      gegl_node_blit (projection->node, dr, projection->format, 0, (gpointer*) buf);
      gegl_buffer_set (projection->buffer, dr, buf, projection->format);
      
      gegl_region_union_with_rect (projection->valid_region, (GeglRect*)dr);

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
      GeglRegion     *region;

      region = gegl_region_rectangle (&dirty_rect);
      gegl_region_subtract (projection->valid_region, region);

      if (!gegl_region_empty (region))
        {
          g_signal_emit (projection, projection_signals[INVALIDATED], 0, NULL, NULL, NULL);
        }
      gegl_region_destroy (region);
    }

  gegl_node_clear_dirt (projection->node);

  gegl_region_subtract (projection->queued_region, projection->valid_region);

  if (!gegl_region_empty (projection->queued_region) &&
      !projection->dirty_rects &&
      projection->render_id == 0) 
    {
      GeglRect  *rectangles;
      gint           n_rectangles;
      gint           i;

      gegl_region_get_rectangles (projection->queued_region, &rectangles, &n_rectangles);

      for (i=0; i<n_rectangles && i<1; i++)
        {
          GeglRect roi = *((GeglRect*)&rectangles[i]);
          GeglRect *dr;
          GeglRegion *tr = gegl_region_rectangle ((void*)&roi);
          gegl_region_subtract (projection->queued_region, tr);
          gegl_region_destroy (tr);
         
          dr = g_malloc(sizeof (GeglRect));
          *dr = roi;
          projection->dirty_rects = g_list_append (projection->dirty_rects, dr);

          /* start a renderer if it isn't already running */
          if (projection->render_id == 0)
            projection->render_id = g_idle_add_full (G_PRIORITY_LOW, (GSourceFunc) task_render, projection, NULL);
        }
      g_free (rectangles);
    }
  if (gegl_region_empty (projection->queued_region))
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
     if (!gegl_region_empty (self->queued_region))
       task_monitor (self);
  return (self->dirty_rects != NULL);
}

GeglBuffer *gegl_projection_get_buffer (GeglProjection *self)
{
  return self->buffer;
}
