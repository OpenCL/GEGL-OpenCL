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
#include "gegl-node.h"
#include "gegl-cache.h"
#include "gegl-region.h"

enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_NODE,
  PROP_CHUNK_SIZE
};

enum
{
  INVALIDATED,
  COMPUTED,
  LAST_SIGNAL
};

static void            gegl_cache_class_init     (GeglCacheClass *klass);
static void            gegl_cache_init           (GeglCache      *self);
static void            finalize                  (GObject       *self_object);
static void            set_property              (GObject       *gobject,
                                                  guint          prop_id,
                                                  const GValue  *value,
                                                  GParamSpec    *pspec);
static void            get_property              (GObject       *gobject,
                                                  guint          prop_id,
                                                  GValue        *value,
                                                  GParamSpec    *pspec);

G_DEFINE_TYPE (GeglCache, gegl_cache, GEGL_TYPE_BUFFER);

static GObjectClass *parent_class = NULL;
static guint         cache_signals[LAST_SIGNAL] = {0};

static GObject *
gegl_cache_constructor (GType                  type,
                        guint                  n_params,
                        GObjectConstructParam *params)
{
  GObject         *object;
  GeglCache *self;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self = GEGL_CACHE (object);

  self->valid_region = gegl_region_new ();
  self->queued_region = gegl_region_new ();
  self->format = GEGL_BUFFER (self)->format;

  return object;
}


static void
gegl_cache_class_init (GeglCacheClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  gobject_class->constructor = gegl_cache_constructor;

  g_object_class_install_property (gobject_class, PROP_NODE,
                                   g_param_spec_object ("node",
                                   "GeglNode",
                                   "The GeglNode to render results from",
                                   GEGL_TYPE_NODE,
                                   G_PARAM_WRITABLE |
                                   G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_CHUNK_SIZE,
                                   g_param_spec_int ("chunk-size", "chunk-size", "Size of chunks being rendered (larger chunks need more memory to do the processing).",
                                                     8*8, 2048*2048, 128*128,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  /* overriding pspecs for properties in parent class */
  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x", "x", "local origin's offset relative to source origin",
                                                     G_MININT, G_MAXINT, -4096,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y", "y", "local origin's offset relative to source origin",
                                                     G_MININT, G_MAXINT, -4096,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width", "width", "pixel width of buffer",
                                                     -1, G_MAXINT, 10240*4,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", "height", "pixel height of buffer",
                                                     -1, G_MAXINT, 10240*4,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));


  cache_signals[COMPUTED] =
      g_signal_new ("computed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      0    /* class offset*/,
      NULL /* accumulator */,
      NULL /* accu_data */,
      g_cclosure_marshal_VOID__BOXED,
      G_TYPE_NONE /* return type */,
      1 /* n_params */,
      GEGL_TYPE_RECTANGLE /* param_types */);

  cache_signals[INVALIDATED] =
      g_signal_new ("invalidated", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
      0    /* class offset*/,
      NULL /* accumulator */,
      NULL /* accu_data */,
      g_cclosure_marshal_VOID__BOXED,
      G_TYPE_NONE /* return type */,
      1 /* n_params */,
      GEGL_TYPE_RECTANGLE /* param_types */);
  
}

static void
gegl_cache_init (GeglCache *self)
{
  self->node = NULL;
  self->chunk_size = 128*128;

  /* thus providing a default value for GeglCache, that overrides the NULL
   * from GeglBuffer */
  GEGL_BUFFER (self)->format = (gpointer)babl_format ("R'G'B'A u8");
}

static void
finalize (GObject *gobject)
{
  GeglCache *self = GEGL_CACHE (gobject);


  while (g_idle_remove_by_data (gobject));


  if (self->node)
    {
      gint handler = g_signal_handler_find (self->node, G_SIGNAL_MATCH_DATA,
                                                 gegl_node_signals[GEGL_NODE_INVALIDATED],
                                                 0, NULL, NULL, self);
      if (handler)
        {
          g_signal_handler_disconnect (self->node, handler);
        }
      g_object_unref (self->node);
    }
  if (self->valid_region)
    gegl_region_destroy (self->valid_region);
  if (self->queued_region)
    gegl_region_destroy (self->queued_region);
  G_OBJECT_CLASS (gegl_cache_parent_class)->finalize (gobject);
}

gboolean
gegl_cache_is_rendered (GeglCache *cache)
{
  if (gegl_region_empty (cache->queued_region) &&
      cache->dirty_rectangles == NULL)
    return TRUE;
  return FALSE;
}

static void
node_invalidated (GeglNode      *source,
                  GeglRectangle *rect,
                  gpointer       data)
{
  GeglCache *cache = GEGL_CACHE (data);

  {
      GeglRegion     *region;
      region = gegl_region_rectangle (rect);
      gegl_region_subtract (cache->valid_region, region);
      gegl_region_destroy (region);
  }

  g_signal_emit (cache, cache_signals[INVALIDATED], 0, rect, NULL);
  gegl_region_subtract (cache->queued_region, cache->valid_region);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglCache *self = GEGL_CACHE (gobject);
  switch (property_id)
    {
      case PROP_NODE:
        if (self->node)
          {
            /* disconnecting dirt propagation */
            gulong handler;
            handler = g_signal_handler_find (self->node, G_SIGNAL_MATCH_DATA,
                                             gegl_node_signals[GEGL_NODE_INVALIDATED],
                                             0, NULL, NULL, self);
            if (handler)
              {
                g_signal_handler_disconnect (self->node, handler);
              }
            g_object_unref (self->node);
          }
        self->node = GEGL_NODE (g_value_dup_object (value));
        g_signal_connect (G_OBJECT (self->node), "invalidated",
                          G_CALLBACK (node_invalidated), self);
        break;

      /* For the rest, upchaining to the property implementation in GeglBuffer */
      case PROP_CHUNK_SIZE:
        self->chunk_size = g_value_get_int (value);
        break;
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
  GeglCache *self = GEGL_CACHE (gobject);
  switch (property_id)
    {
      case PROP_NODE:
        g_value_set_object (value, self->node);
        break;

      /* For the rest, upchaining to the property implementation in GeglBuffer */
      case PROP_X:
        g_object_get_property (gobject, "GeglBuffer::x", value);
        break;
      case PROP_CHUNK_SIZE:
        g_value_set_int (value, self->chunk_size);
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


void
gegl_cache_dequeue (GeglCache     *self,
                    GeglRectangle *roi)
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

      while (self->dirty_rectangles)
         self->dirty_rectangles = g_list_remove (self->dirty_rectangles,
            self->dirty_rectangles->data);
    }
}

void
gegl_cache_invalidate (GeglCache     *self,
                       GeglRectangle *roi)
{
  gegl_cache_dequeue (self, roi);

  if (roi)
    {
      GeglRegion *temp_region;
      temp_region = gegl_region_rectangle (roi);
      gegl_region_subtract (self->valid_region, temp_region);
      gegl_region_destroy (temp_region);
      g_signal_emit (self, cache_signals[INVALIDATED], 0, roi, NULL);
    }
  else
    {
      GeglRectangle rect = {0,0,0,0}; /* should probably be the extent of the cache */
      if (self->valid_region)
        gegl_region_destroy (self->valid_region);
      self->valid_region = gegl_region_new ();
      g_signal_emit (self, cache_signals[INVALIDATED], 0, &rect, NULL);
    }
}

void
gegl_cache_enqueue (GeglCache     *self,
                    GeglRectangle  roi)
{
  GeglRegion *temp_region;

  temp_region = gegl_region_rectangle (&roi);

  gegl_region_union (self->queued_region, temp_region);
  gegl_region_subtract (self->queued_region, self->valid_region);

  gegl_region_destroy (temp_region);
}

/* renders a single queued rectangle */
static gboolean render_rectangle (GeglCache *cache)
{
  gint max_area = cache->chunk_size;

  GeglRectangle *dr;

  if (cache->dirty_rectangles)
    {
      guchar *buf;

      dr = cache->dirty_rectangles->data;

      if (dr->h * dr->w > max_area && 1)
        {
          gint band_size;
         
          if (dr->h > dr->w)
            {
              band_size = dr->h / 2;

              if (band_size<1)
                band_size=1;

              GeglRectangle *fragment = g_malloc (sizeof (GeglRectangle));
              *fragment = *dr;

              fragment->h = band_size;
              dr->h-=band_size;
              dr->y+=band_size;

              cache->dirty_rectangles = g_list_prepend (cache->dirty_rectangles, fragment);
              return TRUE;
            }
          else 
            {
              band_size = dr->w / 2;

              if (band_size<1)
                band_size=1;

              GeglRectangle *fragment = g_malloc (sizeof (GeglRectangle));
              *fragment = *dr;

              fragment->w = band_size;
              dr->w-=band_size;
              dr->x+=band_size;

              cache->dirty_rectangles = g_list_prepend (cache->dirty_rectangles, fragment);
              return TRUE;
            }
        }
      
      cache->dirty_rectangles = g_list_remove (cache->dirty_rectangles, dr);

      if (!dr->w || !dr->h)
        return TRUE;
      
      buf = g_malloc (dr->w * dr->h * gegl_buffer_px_size (GEGL_BUFFER (cache)));
      g_assert (buf);

      gegl_node_blit (cache->node, dr, 1.0, cache->format, 0, (gpointer*) buf, GEGL_BLIT_DEFAULT);
      gegl_buffer_set (GEGL_BUFFER (cache), dr, cache->format, buf);
      
      gegl_region_union_with_rect (cache->valid_region, (GeglRectangle*)dr);

      g_signal_emit (cache, cache_signals[COMPUTED], 0, dr, NULL, NULL);

      g_free (buf);
      g_free (dr);
    }
  return cache->dirty_rectangles != NULL;
}

static gint rect_area (GeglRectangle *rectangle)
{
  return rectangle->w*rectangle->h;
}

static gint region_area (GeglRegion    *region)
{
  GeglRectangle *rectangles;
  gint           n_rectangles;
  gint           i;
  gint           sum=0;
  gegl_region_get_rectangles (region, &rectangles, &n_rectangles);

  for (i=0; i<n_rectangles; i++)
    {
      sum+=rect_area(&rectangles[i]);
    }
  g_free (rectangles);
  return sum;
}

static gint area_left (GeglCache     *cache,
                       GeglRectangle *rectangle)
{
  GeglRegion    *region;
  gint           sum=0;

  region = gegl_region_rectangle (rectangle);
  gegl_region_subtract (region, cache->valid_region);
  sum += region_area (region);
  gegl_region_destroy (region);
  return sum;
}

gboolean
gegl_cache_render (GeglCache     *cache,
                   GeglRectangle *rectangle,
                   gdouble       *progress)
{
  g_assert (GEGL_IS_CACHE (cache));
  {
    gboolean more_work = render_rectangle (cache);
    if (more_work == TRUE)
      {
        if (progress)
          {
            gint valid;
            gint wanted;
            if (rectangle)
              {
                wanted = rect_area (rectangle);   
                valid  = wanted - area_left (cache, rectangle);
              }
            else
              {
                valid = region_area(cache->valid_region);
                wanted = region_area(cache->queued_region);
              }
            if (wanted == 0)
              {
                *progress = 1.0;
              }
            else
              {
                *progress = (double)valid/wanted;
              }
              wanted = 1;
          }
        return more_work;
      }
  }

  if (rectangle)
    { /* we're asked to work on a specific rectangle thus we only focus
         on it */
      GeglRegion *region = gegl_region_rectangle (rectangle);
      gegl_region_subtract (region, cache->valid_region);
      GeglRectangle *rectangles;
      gint           n_rectangles;
      gint           i;
      
      gegl_region_get_rectangles (region, &rectangles, &n_rectangles);

      for (i=0; i<n_rectangles && i<1; i++)
        {
          GeglRectangle roi = *((GeglRectangle*)&rectangles[i]);
          GeglRectangle *dr;
          GeglRegion *tr = gegl_region_rectangle ((void*)&roi);
          gegl_region_subtract (cache->queued_region, tr);
          gegl_region_destroy (tr);
         
          dr = g_malloc(sizeof (GeglRectangle));
          *dr = roi;
          cache->dirty_rectangles = g_list_append (cache->dirty_rectangles, dr);
        }
      g_free (rectangles);
      if (n_rectangles!=0)
        {
          if (progress)
            *progress = 1.0-((double)area_left (cache, rectangle)/rect_area(rectangle));
          return TRUE;
        }
      return FALSE;
    }
  else if (!gegl_region_empty (cache->queued_region) &&
           !cache->dirty_rectangles)
    {
      GeglRectangle *rectangles;
      gint           n_rectangles;
      gint           i;

      gegl_region_get_rectangles (cache->queued_region, &rectangles, &n_rectangles);

      for (i=0; i<n_rectangles && i<1; i++)
        {
          GeglRectangle roi = *((GeglRectangle*)&rectangles[i]);
          GeglRectangle *dr;
          GeglRegion *tr = gegl_region_rectangle ((void*)&roi);
          gegl_region_subtract (cache->queued_region, tr);
          gegl_region_destroy (tr);
         
          dr = g_malloc(sizeof (GeglRectangle));
          *dr = roi;
          cache->dirty_rectangles = g_list_append (cache->dirty_rectangles, dr);
        }
      g_free (rectangles);
    }
 if (progress)
   *progress = 0.69;

  return !gegl_cache_is_rendered (cache);
}
