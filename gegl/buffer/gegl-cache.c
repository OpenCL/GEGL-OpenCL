/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include <babl/babl.h>

#include "gegl.h"
#include "gegl-types-internal.h"

#include "gegl-cache.h"
#include "gegl-region.h"

enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT
};

enum
{
  INVALIDATED,
  COMPUTED,
  LAST_SIGNAL
};

static void            finalize     (GObject      *object);
static void            dispose      (GObject      *object);
static void            set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec);
static void            get_property (GObject      *object,
                                     guint         prop_id,
                                     GValue       *value,
                                     GParamSpec   *pspec);

G_DEFINE_TYPE (GeglCache, gegl_cache, GEGL_TYPE_BUFFER)

guint gegl_cache_signals[LAST_SIGNAL] = { 0 };

static void
gegl_cache_constructed (GObject *object)
{
  GeglCache *self = GEGL_CACHE (object);
  gint i;

  G_OBJECT_CLASS (gegl_cache_parent_class)->constructed (object);

  for (i = 0; i < GEGL_CACHE_VALID_MIPMAPS; i++)
    self->valid_region[i] = gegl_region_new ();
}

/* expand invalidated regions to be align with coordinates divisible by 8 in both
 * directions. This improves improves the performance of GeglProcessor when it
 * iterates the resulting dirtied rectangles in the GeglRegion.
 */
static GeglRectangle gegl_rectangle_expand (const GeglRectangle *rectangle)
{
  gint align = 8;
  GeglRectangle expanded = *rectangle;
  gint xdiff;
  gint ydiff;

  if (gegl_rectangle_is_infinite_plane (rectangle))
    return *rectangle;

  xdiff = expanded.x % align;
  if (xdiff < 0)
    xdiff = align + xdiff;
  expanded.width += xdiff;
  expanded.x -= xdiff;
  xdiff = align -(expanded.width % align);
  expanded.width += xdiff;

  ydiff = expanded.y % align;
  if (ydiff < 0)
    ydiff = align + ydiff;
  expanded.height += ydiff;
  expanded.y -= ydiff;
  ydiff = align -(expanded.height % align);
  expanded.height += ydiff;

  return expanded;
}

static void
gegl_cache_class_init (GeglCacheClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed  = gegl_cache_constructed;
  gobject_class->finalize     = finalize;
  gobject_class->dispose      = dispose;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  /* overriding pspecs for properties in parent class */
  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x", "x",
                                                     "local origin's offset relative to source origin",
                                                     G_MININT / 2, G_MAXINT / 2, -4096,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y", "y",
                                                     "local origin's offset relative to source origin",
                                                     G_MININT / 2, G_MAXINT / 2, -4096,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width", "width",
                                                     "pixel width of buffer",
                                                     -1, G_MAXINT, 10240 * 4,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", "height",
                                                     "pixel height of buffer",
                                                     -1, G_MAXINT, 10240 * 4,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));


  gegl_cache_signals[COMPUTED] =
    g_signal_new ("computed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_RECTANGLE);

  gegl_cache_signals[INVALIDATED] =
    g_signal_new ("invalidated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_RECTANGLE);
}

static void
gegl_cache_init (GeglCache *self)
{
  g_mutex_init (&self->mutex);
}

static void
dispose (GObject *gobject)
{
  while (g_idle_remove_by_data (gobject)) ;

  G_OBJECT_CLASS (gegl_cache_parent_class)->dispose (gobject);
}

static void
finalize (GObject *gobject)
{
  GeglCache *self = GEGL_CACHE (gobject);
  gint i;

  g_mutex_clear (&self->mutex);
  for (i = 0; i < GEGL_CACHE_VALID_MIPMAPS; i++)
    if (self->valid_region[i])
      gegl_region_destroy (self->valid_region[i]);
  G_OBJECT_CLASS (gegl_cache_parent_class)->finalize (gobject);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (property_id)
    {
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
  switch (property_id)
    {
      /* Upchain to the property implementation in GeglBuffer */
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

void
gegl_cache_invalidate (GeglCache           *self,
                       const GeglRectangle *roi)
{
  gint i;
  g_mutex_lock (&self->mutex);

  if (roi)
    {
      GeglRectangle expanded = gegl_rectangle_expand (roi);

      GeglRegion *temp_region;
      temp_region = gegl_region_rectangle (&expanded);
      for (i = 0; i < GEGL_CACHE_VALID_MIPMAPS; i++)
        gegl_region_subtract (self->valid_region[i], temp_region);
      gegl_region_destroy (temp_region);
      g_signal_emit (self, gegl_cache_signals[INVALIDATED], 0,
                     roi, NULL);
    }
  else
    {
      GeglRectangle rect = { 0, 0, 0, 0 }; /* should probably be the extent of the cache */
      for (i = 0; i < GEGL_CACHE_VALID_MIPMAPS; i++)
      {
        if (self->valid_region[i])
          gegl_region_destroy (self->valid_region[i]);
        self->valid_region[i] = gegl_region_new ();
      }
      g_signal_emit (self, gegl_cache_signals[INVALIDATED], 0,
                     &rect, NULL);
    }
  g_mutex_unlock (&self->mutex);
}

void
gegl_cache_computed (GeglCache           *self,
                     const GeglRectangle *rect,
                     gint                 level)
{
  g_return_if_fail (GEGL_IS_CACHE (self));
  g_return_if_fail (rect != NULL);

  g_mutex_lock (&self->mutex);

  if (level < GEGL_CACHE_VALID_MIPMAPS)
    gegl_region_union_with_rect (self->valid_region[level], rect);

  g_signal_emit (self, gegl_cache_signals[COMPUTED], 0, rect, NULL);
  g_mutex_unlock (&self->mutex);
}

gboolean
gegl_buffer_list_valid_rectangles (GeglBuffer     *buffer,
                                   GeglRectangle **rectangles,
                                   gint           *n_rectangles);

gboolean
gegl_buffer_list_valid_rectangles (GeglBuffer     *buffer,
                                   GeglRectangle **rectangles,
                                   gint           *n_rectangles)
{
  GeglCache *cache;
  gint level = 0; /* should be an argument */
  g_return_val_if_fail (GEGL_IS_CACHE (buffer), FALSE);
  cache = GEGL_CACHE (buffer);

  if (level < 0)
    level = 0;
  if (level >= GEGL_CACHE_VALID_MIPMAPS)
    level = GEGL_CACHE_VALID_MIPMAPS-1;

  gegl_region_get_rectangles (cache->valid_region[level],
                              rectangles, n_rectangles);

  return TRUE;
}
