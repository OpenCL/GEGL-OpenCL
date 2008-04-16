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
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gegl-handler.h"
#include "gegl-handler-empty.h"
#include "gegl-handler-cache.h"

G_DEFINE_TYPE (GeglHandlerEmpty, gegl_handler_empty, GEGL_TYPE_HANDLER)

enum
{
  PROP_0,
  PROP_BACKEND
};

static void
finalize (GObject *object)
{
  GeglHandlerEmpty *empty = GEGL_HANDLER_EMPTY (object);

  if (empty->tile)
    g_object_unref (empty->tile);

  G_OBJECT_CLASS (gegl_handler_empty_parent_class)->finalize (object);
}

void gegl_handler_cache_insert (GeglHandlerCache *cache,
                                GeglTile         *tile,
                                gint              x,
                                gint              y,
                                gint              z);

static GeglTile *
get_tile (GeglSource *gegl_source,
          gint        x,
          gint        y,
          gint        z)
{
  GeglSource *source = GEGL_HANDLER (gegl_source)->source;
  GeglHandlerEmpty *empty  = GEGL_HANDLER_EMPTY (gegl_source);
  GeglTile      *tile   = NULL;

  if (source)
    tile = gegl_source_get_tile (source, x, y, z);
  if (tile != NULL)
    return tile;

  tile = gegl_tile_dup (empty->tile);
  tile->x = x;
  tile->y = y;
  tile->z = z;
  {
    GeglHandlerCache *cache = g_object_get_data (G_OBJECT (gegl_source), "cache");
    if (cache)
      gegl_handler_cache_insert (cache, tile, x, y, z);
  }

  return tile;
}


static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglHandlerEmpty *empty = GEGL_HANDLER_EMPTY (gobject);

  switch (property_id)
    {
      case PROP_BACKEND:
        g_value_set_object (value, empty->backend);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglHandlerEmpty *empty = GEGL_HANDLER_EMPTY (gobject);

  switch (property_id)
    {
      case PROP_BACKEND:
        empty->backend = g_value_get_object (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static GObject *
constructor (GType                  type,
             guint                  n_params,
             GObjectConstructParam *params)
{
  GObject       *object;
  GeglHandlerEmpty *empty;
  gint           tile_width;
  gint           tile_height;
  gint           tile_size;

  object = G_OBJECT_CLASS (gegl_handler_empty_parent_class)->constructor (type, n_params, params);

  empty  = GEGL_HANDLER_EMPTY (object);

  g_assert (empty->backend);
  g_object_get (empty->backend, "tile-width", &tile_width,
                "tile-height", &tile_height,
                "tile-size", &tile_size,
                NULL);
  /* FIXME: need babl format here */
  empty->tile = gegl_tile_new (tile_size);
  memset (gegl_tile_get_data (empty->tile), 0x00, tile_size);

  return object;
}


static gpointer
command (GeglSource     *buffer,
         GeglTileCommand command,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  if (command == GEGL_TILE_GET)
    return get_tile (buffer, x, y, z);
  return gegl_handler_chain_up (GEGL_HANDLER(buffer), command, x, y, z, data);
}


static void
gegl_handler_empty_class_init (GeglHandlerEmptyClass *klass)
{
  GObjectClass      *gobject_class  = G_OBJECT_CLASS (klass);
  GeglSourceClass *source_class = GEGL_SOURCE_CLASS (klass);

  gobject_class->constructor  = constructor;
  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  source_class->command = command;

  g_object_class_install_property (gobject_class, PROP_BACKEND,
                                   g_param_spec_object ("backend",
                                                        "backend",
                                                        "backend for this tilestore (needed for tile size data)",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gegl_handler_empty_init (GeglHandlerEmpty *self)
{
}
