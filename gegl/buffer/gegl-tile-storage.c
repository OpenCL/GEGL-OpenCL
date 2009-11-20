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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-tile-storage.h"
#include "gegl-tile.h"
#include "gegl-tile-backend-file.h"
#include "gegl-tile-backend-ram.h"
#if HAVE_GIO
#include "gegl-tile-backend-tiledir.h"
#endif
#include "gegl-tile-handler-empty.h"
#include "gegl-tile-handler-zoom.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-handler-log.h"
#include "gegl-types-internal.h"
#include "gegl-utils.h"
#include "gegl-config.h"


G_DEFINE_TYPE (GeglTileStorage, gegl_tile_storage, GEGL_TYPE_TILE_HANDLER_CHAIN)

static GObjectClass * parent_class = NULL;

enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_TILE_SIZE,
  PROP_FORMAT,
  PROP_PX_SIZE,
  PROP_PATH
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

guint gegl_tile_storage_signals[LAST_SIGNAL] = { 0 };

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileStorage *tile_storage = GEGL_TILE_STORAGE (gobject);

  switch (property_id)
    {
      case PROP_WIDTH:
        g_value_set_int (value, tile_storage->width);
        break;

      case PROP_HEIGHT:
        g_value_set_int (value, tile_storage->height);
        break;

      case PROP_TILE_WIDTH:
        g_value_set_int (value, tile_storage->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, tile_storage->tile_height);
        break;

      case PROP_TILE_SIZE:
        g_value_set_int (value, tile_storage->tile_size);
        break;

      case PROP_PX_SIZE:
        g_value_set_int (value, tile_storage->px_size);
        break;

      case PROP_PATH:
        g_value_set_string (value, tile_storage->path);
        break;

      case PROP_FORMAT:
        g_value_set_pointer (value, tile_storage->format);
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
  GeglTileStorage *tile_storage = GEGL_TILE_STORAGE (gobject);

  switch (property_id)
    {
      case PROP_WIDTH:
        tile_storage->width = g_value_get_int (value);
        return;

      case PROP_HEIGHT:
        tile_storage->height = g_value_get_int (value);
        return;

      case PROP_TILE_WIDTH:
        tile_storage->tile_width = g_value_get_int (value);
        break;

      case PROP_TILE_HEIGHT:
        tile_storage->tile_height = g_value_get_int (value);
        break;

      case PROP_TILE_SIZE:
        tile_storage->tile_size = g_value_get_int (value);
        break;

      case PROP_PX_SIZE:
        tile_storage->px_size = g_value_get_int (value);
        break;

      case PROP_PATH:
        if (tile_storage->path)
          g_free (tile_storage->path);
        tile_storage->path = g_value_dup_string (value);
        break;

      case PROP_FORMAT:
        tile_storage->format = g_value_get_pointer (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static gboolean
tile_storage_idle (gpointer data)
{
  GeglTileStorage *tile_storage = GEGL_TILE_STORAGE (data);

  if (0 /* nothing to do*/)
    {
      tile_storage->idle_swapper = 0;
      return FALSE;
    }

  return gegl_tile_source_idle (GEGL_TILE_SOURCE (tile_storage));
}

GeglTileBackend *gegl_buffer_backend (GObject *buffer);

static GObject *
gegl_tile_storage_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
  GObject               *object;
  GeglTileStorage       *tile_storage;
  GeglTileHandlerChain  *tile_handler_chain;
  GeglTileHandler       *handler;
  GeglTileHandler       *empty = NULL;
  GeglTileHandler       *zoom = NULL;
  GeglTileHandler       *cache = NULL;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tile_storage  = GEGL_TILE_STORAGE (object);
  tile_handler_chain = GEGL_TILE_HANDLER_CHAIN (tile_storage);
  handler  = GEGL_HANDLER (tile_storage);


  if (tile_storage->path != NULL)
    {
#if 1
      gegl_tile_handler_set_source (handler,
                              g_object_new (GEGL_TYPE_TILE_BACKEND_FILE,
                                            "tile-width", tile_storage->tile_width,
                                            "tile-height", tile_storage->tile_height,
                                            "format", tile_storage->format,
                                            "path", tile_storage->path,
                                            NULL));
#else
      gegl_tile_handler_set_source (handler,
                    g_object_new (GEGL_TYPE_TILE_BACKEND_TILEDIR,
                                            "tile-width", tile_storage->tile_width,
                                            "tile-height", tile_storage->tile_height,
                                            "format", tile_storage->format,
                                            "path", tile_storage->path,
                                            NULL));
#endif
    }
  else
    {
      gegl_tile_handler_set_source (handler,
                              g_object_new (GEGL_TYPE_TILE_BACKEND_RAM,
                                            "tile-width", tile_storage->tile_width,
                                            "tile-height", tile_storage->tile_height,
                                            "format", tile_storage->format,
                                            NULL));
    }

  g_object_get (handler->source,
                "tile-size", &tile_storage->tile_size,
                "px-size",   &tile_storage->px_size,
                NULL);

  g_object_unref (handler->source); /* eeek */

  if (g_getenv("GEGL_LOG_TILE_BACKEND")||
      g_getenv("GEGL_TILE_LOG"))
    gegl_tile_handler_chain_add (tile_handler_chain,
                      g_object_new (GEGL_TYPE_TILE_HANDLER_LOG, NULL));


  cache = g_object_new (GEGL_TYPE_TILE_HANDLER_CACHE,
                        NULL);
  empty = g_object_new (GEGL_TYPE_TILE_HANDLER_EMPTY,
                        "backend", handler->source,
                        NULL);
  zoom = g_object_new (GEGL_TYPE_TILE_HANDLER_ZOOM,
                       "backend", handler->source,
                       "tile_storage", tile_storage,
                       NULL);

  gegl_tile_handler_chain_add (tile_handler_chain, cache);
  gegl_tile_handler_chain_add (tile_handler_chain, zoom);
  gegl_tile_handler_chain_add (tile_handler_chain, empty);


  if (g_getenv("GEGL_LOG_TILE_CACHE"))
    gegl_tile_handler_chain_add (tile_handler_chain,
                              g_object_new (GEGL_TYPE_TILE_HANDLER_LOG, NULL));
  g_object_set_data (G_OBJECT (tile_storage), "cache", cache);
  g_object_set_data (G_OBJECT (empty), "cache", cache);
  g_object_set_data (G_OBJECT (zoom), "cache", cache);

  {
    GeglTileBackend *backend;
    backend = gegl_buffer_backend (object);
    backend->storage = (gpointer)object;
  }

  tile_storage->idle_swapper = g_timeout_add_full (G_PRIORITY_LOW,
                                              250,
                                              tile_storage_idle,
                                              tile_storage,
                                              NULL);
  tile_storage->seen_zoom = FALSE;
#if ENABLE_MT
  tile_storage->mutex = g_mutex_new ();
#endif

  return object;
}

static void
gegl_tile_storage_finalize (GObject *object)
{
  GeglTileStorage *self = GEGL_TILE_STORAGE (object);

  if (self->idle_swapper)
    g_source_remove (self->idle_swapper);

  if (self->path)
    g_free (self->path);
#if ENABLE_MT
  g_mutex_free (self->mutex);
#endif

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static void
gegl_tile_storage_class_init (GeglTileStorageClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class                = g_type_class_peek_parent (class);
  gobject_class->constructor  = gegl_tile_storage_constructor;
  gobject_class->finalize     = gegl_tile_storage_finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", "tile-width", "width of a tile in pixels",
                                                     0, G_MAXINT, 128,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", "tile-height", "height of a tile in pixels",
                                                     0, G_MAXINT, 64,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_TILE_SIZE,
                                   g_param_spec_int ("tile-size", "tile-size", "size of a tile in bytes",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_PX_SIZE,
                                   g_param_spec_int ("px-size", "px-size", "size of a a pixel in bytes",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width", "width", "pixel width of buffer",
                                                     0, G_MAXINT, G_MAXINT,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", "height", "pixel height of buffer",
                                                     0, G_MAXINT, G_MAXINT,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "The filesystem directory with swap for this sparse tile store, NULL to make this be a heap tile_storage.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));


  g_object_class_install_property (gobject_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format", "format", "babl format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  gegl_tile_storage_signals[CHANGED] =
        g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_RECTANGLE);

}

static void
gegl_tile_storage_init (GeglTileStorage *buffer)
{
}
