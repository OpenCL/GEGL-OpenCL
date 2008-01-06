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

static GeglTile *
get_tile (GeglProvider *gegl_provider,
          gint           x,
          gint           y,
          gint           z)
{
  GeglProvider *provider = GEGL_HANDLER (gegl_provider)->provider;
  GeglHandlerEmpty *empty  = GEGL_HANDLER_EMPTY (gegl_provider);
  GeglTile      *tile   = NULL;

  if (provider)
    tile = gegl_provider_get_tile (provider, x, y, z);
  if (tile != NULL)
    return tile;

  tile = gegl_tile_dup (empty->tile);

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


static void
gegl_handler_empty_class_init (GeglHandlerEmptyClass *klass)
{
  GObjectClass      *gobject_class  = G_OBJECT_CLASS (klass);
  GeglProviderClass *provider_class = GEGL_PROVIDER_CLASS (klass);

  gobject_class->constructor  = constructor;
  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  provider_class->get_tile = get_tile;

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
