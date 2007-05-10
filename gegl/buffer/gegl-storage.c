/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>

#include "gegl-storage.h"
#include "gegl-tile.h"
#include "gegl-tile-disk.h"
#include "gegl-tile-empty.h"
#include "gegl-tile-zoom.h"
#include "gegl-tile-mem.h"
#include "gegl-tile-cache.h"
#include "gegl-tile-log.h"

G_DEFINE_TYPE (GeglStorage, gegl_storage, GEGL_TYPE_TILE_TRAITS)
#define TILE_SIZE    128

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
        if (storage->path != NULL)
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

static void
gegl_storage_dispose (GObject *object)
{
  GeglStorage   *storage;
  GeglTileTrait *trait;

  storage = (GeglStorage *) object;
  trait   = GEGL_TILE_TRAIT (object);

  (*G_OBJECT_CLASS (parent_class)->dispose)(object);
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

  gegl_tile_store_message (GEGL_TILE_STORE (storage),
                           GEGL_TILE_IDLE, 0, 0, 0, NULL);

  return TRUE;
}


static GObject *
gegl_storage_constructor (GType                  type,
                          guint                  n_params,
                          GObjectConstructParam *params)
{
  GObject        *object;
  GeglStorage    *storage;
  GeglTileTraits *traits;
  GeglTileTrait  *trait;

  object  = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  storage = GEGL_STORAGE (object);
  traits  = GEGL_TILE_TRAITS (storage);
  trait   = GEGL_TILE_TRAIT (storage);

  if (storage->path != NULL)
    {
      g_object_set (storage,
                    "source", g_object_new (GEGL_TYPE_TILE_DISK_STORE,
                                            "tile-width", storage->tile_width,
                                            "tile-height", storage->tile_height,
                                            "format", storage->format,
                                            "path", storage->path,
                                            NULL),
                    NULL);
    }
  else
    {
      g_object_set (storage,
                    "source", g_object_new (GEGL_TYPE_TILE_MEM_STORE,
                                            "tile-width", storage->tile_width,
                                            "tile-height", storage->tile_height,
                                            "format", storage->format,
                                            NULL),
                    NULL);
    }
  {
    gint tile_size;
    gint px_size;

    g_object_get (trait->source,
                  "tile-size", &tile_size,
                  "px-size", &px_size,
                  NULL);
    g_object_set (object,
                  "tile-size", tile_size,
                  "px-size", px_size,
                  NULL);
  }

  if (1) gegl_tile_traits_add (traits, g_object_new (GEGL_TYPE_TILE_CACHE,
                                                     "size", 256,
                                                     NULL));

  if (0) gegl_tile_traits_add (traits, g_object_new (GEGL_TYPE_TILE_LOG,
                                                     NULL));

  if (1) gegl_tile_traits_add (traits, g_object_new (GEGL_TYPE_TILE_ZOOM,
                                                     "backend", trait->source,
                                                     "storage", storage,
                                                     NULL));

  /* moved here to allow sharing between buffers (speeds up, but only
   * allows nulled (transparent) blank tiles,..
   */
  if (1) gegl_tile_traits_add (traits, g_object_new (GEGL_TYPE_TILE_EMPTY,
                                                     "backend", trait->source,
                                                     NULL));

  /* it doesn't really matter that empty tiles are not cached, since they
   * are Copy on Write.
   */

  storage->idle_swapper = g_timeout_add_full (G_PRIORITY_LOW,
                                              250,
                                              storage_idle,
                                              storage,
                                              NULL);

  return object;
}


static void
gegl_storage_class_init (GeglStorageClass *class)
{
  GObjectClass       *gobject_class;
  GeglTileStoreClass *tile_store_class;

  gobject_class    = (GObjectClass *) class;
  tile_store_class = (GeglTileStoreClass *) class;

  parent_class                = g_type_class_peek_parent (class);
  gobject_class->dispose      = gegl_storage_dispose;
  gobject_class->constructor  = gegl_storage_constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", "tile-width", "width of a tile in pixels",
                                                     0, G_MAXINT, TILE_SIZE,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", "tile-height", "height of a tile in pixels",
                                                     0, G_MAXINT, TILE_SIZE,
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
