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

#include "gegl-storage.h"
#include "gegl-tile.h"
#include "gegl-tile-disk.h"
#include "gegl-tile-mem.h"
#include "gegl-tile-gio.h"
#include "gegl-handler-empty.h"
#include "gegl-handler-zoom.h"
#include "gegl-handler-cache.h"
#include "gegl-handler-log.h"


G_DEFINE_TYPE (GeglStorage, gegl_storage, GEGL_TYPE_HANDLERS)

#define TILE_WIDTH  128
#define TILE_HEIGHT 64

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

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglStorage *storage = GEGL_STORAGE (gobject);

  switch (property_id)
    {
      case PROP_WIDTH:
        g_value_set_int (value, storage->width);
        break;

      case PROP_HEIGHT:
        g_value_set_int (value, storage->height);
        break;

      case PROP_TILE_WIDTH:
        g_value_set_int (value, storage->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, storage->tile_height);
        break;

      case PROP_TILE_SIZE:
        g_value_set_int (value, storage->tile_size);
        break;

      case PROP_PX_SIZE:
        g_value_set_int (value, storage->px_size);
        break;

      case PROP_PATH:
        g_value_set_string (value, storage->path);
        break;

      case PROP_FORMAT:
        g_value_set_pointer (value, storage->format);
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
  GeglStorage *storage = GEGL_STORAGE (gobject);

  switch (property_id)
    {
      case PROP_WIDTH:
        storage->width = g_value_get_int (value);
        return;

      case PROP_HEIGHT:
        storage->height = g_value_get_int (value);
        return;

      case PROP_TILE_WIDTH:
        storage->tile_width = g_value_get_int (value);
        break;

      case PROP_TILE_HEIGHT:
        storage->tile_height = g_value_get_int (value);
        break;

      case PROP_TILE_SIZE:
        storage->tile_size = g_value_get_int (value);
        break;

      case PROP_PX_SIZE:
        storage->px_size = g_value_get_int (value);
        break;

      case PROP_PATH:
        if (storage->path)
          g_free (storage->path);
        storage->path = g_strdup (g_value_get_string (value));
        break;

      case PROP_FORMAT:
        storage->format = g_value_get_pointer (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static gboolean
storage_idle (gpointer data)
{
  GeglStorage *storage = GEGL_STORAGE (data);

  if (0 /* nothing to do*/)
    {
      storage->idle_swapper = 0;
      return FALSE;
    }

  gegl_source_idle (GEGL_SOURCE (storage));                        

  return TRUE;
}


static GObject *
gegl_storage_constructor (GType                  type,
                          guint                  n_params,
                          GObjectConstructParam *params)
{
  GObject        *object;
  GeglStorage    *storage;
  GeglHandlers   *handlers;
  GeglHandler    *handler;
  GeglHandler    *cache = NULL;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  storage  = GEGL_STORAGE (object);
  handlers = GEGL_HANDLERS (storage);
  handler  = GEGL_HANDLER (storage);

  if (storage->path != NULL)
    {
#if 0
      g_object_set (storage,
                    "source", g_object_new (GEGL_TYPE_TILE_DISK,
                                            "tile-width", storage->tile_width,
                                            "tile-height", storage->tile_height,
                                            "format", storage->format,
                                            "path", storage->path,
                                            NULL),
                    NULL);
#else
      g_object_set (storage,
                    "source", g_object_new (GEGL_TYPE_TILE_GIO,
                                            "tile-width", storage->tile_width,
                                            "tile-height", storage->tile_height,
                                            "format", storage->format,
                                            "path", storage->path,
                                            NULL),
                    NULL);
#endif
    }
  else
    {
      g_object_set (storage,
                    "source", g_object_new (GEGL_TYPE_TILE_MEM,
                                            "tile-width", storage->tile_width,
                                            "tile-height", storage->tile_height,
                                            "format", storage->format,
                                            NULL),
                    NULL);
    }

  g_object_get (handler->source,
                "tile-size", &storage->tile_size,
                "px-size",   &storage->px_size,
                NULL);

  if (g_getenv("GEGL_LOG_TILE_BACKEND"))
    gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_LOG, NULL));


  /* FIXME: the cache should be made shared between all GeglStorages,
   * to get a better gauge of memory use (ideally we would want to make
   * to adapt to an approximate number of bytes to be allocated)
   */
  if (1) 
    cache = gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_CACHE,
                                                       "size", 256,
                                                       NULL));
  if (g_getenv("GEGL_LOG_TILE_CACHE"))
    gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_LOG, NULL));


  if (1) gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_ZOOM,
                                                  "backend", handler->source,
                                                  "storage", storage,
                                                  NULL));

  if (g_getenv("GEGL_LOG_TILE_ZOOM"))
    gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_LOG, NULL));

  /* moved here to allow sharing between buffers (speeds up, but only
   * allows nulled (transparent) blank tiles, or we would need a separate
   * gegl-storage for each tile.
   */
  if (1) gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_EMPTY,
                                                  "backend", handler->source,
                                                   NULL));
  if (g_getenv("GEGL_LOG_TILE_EMPTY"))
    gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_LOG, NULL));

  /* it doesn't really matter that empty tiles are not cached, since they
   * are Copy on Write.
   */

  storage->idle_swapper = g_timeout_add_full (G_PRIORITY_LOW,
                                              500,
                                              storage_idle,
                                              storage,
                                              NULL);

  return object;
}


static void
gegl_storage_class_init (GeglStorageClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  parent_class                = g_type_class_peek_parent (class);
  gobject_class->constructor  = gegl_storage_constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", "tile-width", "width of a tile in pixels",
                                                     0, G_MAXINT, TILE_WIDTH,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", "tile-height", "height of a tile in pixels",
                                                     0, G_MAXINT, TILE_HEIGHT,
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
                                                        "The filesystem directory with swap for this sparse tile store, NULL to make this be a heap storage.",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));


  g_object_class_install_property (gobject_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format", "format", "babl format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gegl_storage_init (GeglStorage *buffer)
{
}
