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

#include <string.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#include "gegl-buffer-types.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-source.h"
#include "gegl-tile-backend.h"
#include "gegl-config.h"

G_DEFINE_TYPE (GeglTileBackend, gegl_tile_backend, GEGL_TYPE_TILE_SOURCE)

#define parent_class gegl_tile_backend_parent_class

enum
{
  PROP_0,
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_PX_SIZE,
  PROP_TILE_SIZE,
  PROP_FORMAT,
  PROP_FLUSH_ON_DESTROY
};

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (gobject);

  switch (property_id)
    {
      case PROP_TILE_WIDTH:
        g_value_set_int (value, backend->priv->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, backend->priv->tile_height);
        break;

      case PROP_TILE_SIZE:
        g_value_set_int (value, backend->priv->tile_size);
        break;

      case PROP_PX_SIZE:
        g_value_set_int (value, backend->priv->px_size);
        break;

      case PROP_FORMAT:
        g_value_set_pointer (value, (void*)backend->priv->format);
        break;

      case PROP_FLUSH_ON_DESTROY:
        g_value_set_boolean (value, backend->priv->flush_on_destroy);
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
  GeglTileBackend *backend = GEGL_TILE_BACKEND (gobject);

  switch (property_id)
    {
      case PROP_TILE_WIDTH:
        backend->priv->tile_width = g_value_get_int (value);
        return;

      case PROP_TILE_HEIGHT:
        backend->priv->tile_height = g_value_get_int (value);
        return;

      case PROP_FORMAT:
        backend->priv->format = g_value_get_pointer (value);
        break;

      case PROP_FLUSH_ON_DESTROY:
        backend->priv->flush_on_destroy = g_value_get_boolean (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
constructed (GObject *object)
{
  GeglTileBackend *backend = GEGL_TILE_BACKEND (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (backend->priv->tile_width > 0 && backend->priv->tile_height > 0);
  g_assert (backend->priv->format);

  backend->priv->px_size = babl_format_get_bytes_per_pixel (backend->priv->format);
  backend->priv->tile_size = backend->priv->tile_width * backend->priv->tile_height * backend->priv->px_size;
}

static void
gegl_tile_backend_class_init (GeglTileBackendClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->constructed  = constructed;

  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", "tile-width",
                                                     "Tile width in pixels",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", "tile-height",
                                                     "Tile height in pixels",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_TILE_SIZE,
                                   g_param_spec_int ("tile-size", "tile-size",
                                                     "Size of the tiles linear buffer in bytes",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class, PROP_PX_SIZE,
                                   g_param_spec_int ("px-size", "px-size",
                                                     "Size of a single pixel in bytes",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format", "format",
                                                         "babl format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_FLUSH_ON_DESTROY,
                                   g_param_spec_boolean ("flush-on-destroy", "flush-on-destroy",
                                                         "Cache tiles will be flushed before the backend is destroyed",
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  g_type_class_add_private (gobject_class, sizeof (GeglTileBackendPrivate));
}

static void
gegl_tile_backend_init (GeglTileBackend *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            GEGL_TYPE_TILE_BACKEND,
                                            GeglTileBackendPrivate);
  self->priv->shared = FALSE;
  self->priv->flush_on_destroy = TRUE;
}


gint
gegl_tile_backend_get_tile_size (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->tile_size;
}

gint
gegl_tile_backend_get_tile_width (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->tile_width;
}

gint
gegl_tile_backend_get_tile_height (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->tile_height;
}

const Babl *
gegl_tile_backend_get_format (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->format;
}


void
gegl_tile_backend_set_extent (GeglTileBackend     *tile_backend,
                              const GeglRectangle *rectangle)
{
  tile_backend->priv->extent = *rectangle;
}

GeglRectangle
gegl_tile_backend_get_extent (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->extent;
}

GeglTileSource *
gegl_tile_backend_peek_storage (GeglTileBackend *backend)
{
  return backend->priv->storage;
}

void
gegl_tile_backend_set_flush_on_destroy (GeglTileBackend *tile_backend,
                                        gboolean         flush_on_destroy)
{
  tile_backend->priv->flush_on_destroy = flush_on_destroy;
}

gboolean
gegl_tile_backend_get_flush_on_destroy (GeglTileBackend *tile_backend)
{
  return tile_backend->priv->flush_on_destroy;
}

void
gegl_tile_backend_unlink_swap (gchar *path)
{
  gchar *dirname = g_path_get_dirname (path);

  /* Ensure we delete only files in our known swap directory for safety. */
  if (g_file_test (path, G_FILE_TEST_EXISTS) &&
      g_strcmp0 (dirname, gegl_config()->swap) == 0)
    g_unlink (path);

  g_free (dirname);
}
