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
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gegl-tile-handler.h"
#include "gegl-tile-handler-zoom.h"
#include "gegl-tile-handler-cache.h"


G_DEFINE_TYPE (GeglTileHandlerZoom, gegl_tile_handler_zoom, GEGL_TYPE_TILE_HANDLER)

enum
{
  PROP_0,
  PROP_TILE_STORAGE,
  PROP_BACKEND
};

#include <babl/babl.h>
#include "gegl-tile-backend.h"
#include "gegl-tile-storage.h"

void gegl_tile_handler_cache_insert (GeglTileHandlerCache *cache,
                                     GeglTile             *tile,
                                     gint                  x,
                                     gint                  y,
                                     gint                  z);
static inline void set_blank (GeglTile *dst_tile,
                              gint      width,
                              gint      height,
                              Babl     *format,
                              gint      i,
                              gint      j)
{
  guchar *dst_data  = gegl_tile_get_data (dst_tile);
  gint    bpp       = babl_format_get_bytes_per_pixel (format);
  gint    rowstride = width * bpp;
  gint    scanline;

  gint    bytes = width * bpp / 2;
  guchar *dst   = dst_data + j * height / 2 * rowstride + i * rowstride / 2;

  for (scanline = 0; scanline < height / 2; scanline++)
    {
      memset (dst, 0x0, bytes);
      dst += rowstride;
    }
}

/* fixme: make the api of this, as well as blank be the
 * same as the downscale functions */
static inline void set_half_nearest (GeglTile *dst_tile,
                                     GeglTile *src_tile,
                                     gint      width,
                                     gint      height,
                                     Babl     *format,
                                     gint      i,
                                     gint      j)
{
  guchar *dst_data = gegl_tile_get_data (dst_tile);
  guchar *src_data = gegl_tile_get_data (src_tile);
  gint    bpp      = babl_format_get_bytes_per_pixel (format);
  gint    x, y;

  for (y = 0; y < height / 2; y++)
    {
      guchar *dst = dst_data +
                    (
        (
          (y + j * (height / 2)) * width
        ) + i * (width / 2)
                    ) * bpp;

      guchar *src = src_data + (y * 2 * width) * bpp;

      for (x = 0; x < width / 2; x++)
        {
          memcpy (dst, src, bpp);
          dst += bpp;
          src += bpp * 2;
        }
    }
}

static inline void
downscale_float (gint    components,
                 gint    width,
                 gint    height,
                 gint    rowstride,
                 guchar *src_data,
                 guchar *dst_data)
{
  gint y;

  if (!src_data || !dst_data)
    return;
  for (y = 0; y < height / 2; y++)
    {
      gint    x;
      gfloat *dst = (gfloat *) (dst_data + y * rowstride);
      gfloat *src = (gfloat *) (src_data + y * 2 * rowstride);

      for (x = 0; x < width / 2; x++)
        {
          int i;
          for (i = 0; i < components; i++)
            dst[i] = (src[i] +
                      src[i + components] +
                      src[i + (width * components)] +
                      src[i + (width + 1) * components]) /
                     4.0;

          dst += components;
          src += components * 2;
        }
    }
}

static inline void
downscale_u8 (gint    components,
              gint    width,
              gint    height,
              gint    rowstride,
              guchar *src_data,
              guchar *dst_data)
{
  gint y;

  if (!src_data || !dst_data)
    return;
  for (y = 0; y < height / 2; y++)
    {
      gint    x;
      guchar *dst = dst_data + y * rowstride;
      guchar *src = src_data + y * 2 * rowstride;

      for (x = 0; x < width / 2; x++)
        {
          int i;
          for (i = 0; i < components; i++)
            dst[i] = (src[i] +
                      src[i + components] +
                      src[i + rowstride] +
                      src[i + rowstride + components]) /
                     4;

          dst += components;
          src += components * 2;
        }
    }
}

static void inline set_half (GeglTile * dst_tile,
                             GeglTile * src_tile,
                             gint       width,
                             gint       height,
                             Babl     * format,
                             gint i,
                             gint j)
{
  guchar *dst_data   = gegl_tile_get_data (dst_tile);
  guchar *src_data   = gegl_tile_get_data (src_tile);
  gint    components = babl_format_get_n_components (format);
  gint    bpp        = babl_format_get_bytes_per_pixel (format);

  if (i) dst_data += bpp * width / 2;
  if (j) dst_data += bpp * width * height / 2;

  if (babl_format_get_type (format, 0) == babl_type ("float"))
    {
      downscale_float (components, width, height, width * bpp, src_data, dst_data);
    }
  else if (babl_format_get_type (format, 0) == babl_type ("u8"))
    {
      downscale_u8 (components, width, height, width * bpp, src_data, dst_data);
    }
  else
    {
      set_half_nearest (dst_tile, src_tile, width, height, format, i, j);
    }
}

static GeglTile *
get_tile (GeglTileSource *gegl_tile_source,
          gint            x,
          gint            y,
          gint            z)
{
  GeglTileSource      *source = GEGL_HANDLER (gegl_tile_source)->source;
  GeglTileHandlerZoom *zoom   = GEGL_TILE_HANDLER_ZOOM (gegl_tile_source);
  GeglTile            *tile   = NULL;
  Babl                *format = (Babl *) (zoom->backend->format);
  gint                 tile_width;
  gint                 tile_height;
  gint                 tile_size;

  if (source)
    {
      tile = gegl_tile_source_get_tile (source, x, y, z);
    }

  if (tile)
    return tile;

  if (z == 0)/* at base level with no tile found->send null, and shared empty
               tile will be used instead */
    {
      return NULL;
    }

  if (z>zoom->tile_storage->seen_zoom)
    zoom->tile_storage->seen_zoom = z;

  g_assert (zoom->backend);
  g_object_get (zoom->backend, "tile-width", &tile_width,
                "tile-height", &tile_height,
                "tile-size", &tile_size,
                NULL);

  {
    gint      i, j;
    guchar   *data;
    GeglTile *source_tile[2][2] = { { NULL, NULL }, { NULL, NULL } };

    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++)
        {
          /* we get the tile from ourselves, to make successive rescales work
           * correctly */
            source_tile[i][j] = gegl_tile_source_get_tile (gegl_tile_source,
                                                          x * 2 + i, y * 2 + j, z - 1);
        }

    if (source_tile[0][0] == NULL &&
        source_tile[0][1] == NULL &&
        source_tile[1][0] == NULL &&
        source_tile[1][1] == NULL)
      {
        return NULL;   /* no data from level below, return NULL and let GeglTileHandlerEmpty
                          fill in the shared empty tile */
      }

    g_assert (tile == NULL);
    if (tile == NULL)
      {
        tile = gegl_tile_new (tile_size);

        tile->x          = x;
        tile->y          = y;
        tile->z          = z;
        tile->tile_storage = zoom->tile_storage;
        tile->stored_rev = 1;
        tile->rev        = 1;

        {
          GeglTileHandlerCache *cache;
          cache = g_object_get_data (G_OBJECT (gegl_tile_source), "cache");
          if (cache)
            {
              gegl_tile_handler_cache_insert (cache, tile, x, y, z);
            }
        }
      }
    gegl_tile_lock (tile);
    data = gegl_tile_get_data (tile);

    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++)
        {
          if (source_tile[i][j])
            {
              set_half (tile, source_tile[i][j], tile_width, tile_height, format, i, j);
              gegl_tile_unref (source_tile[i][j]);
            }
          else 
            {
              set_blank (tile, tile_width, tile_height, format, i, j);
            }
        }
    gegl_tile_unlock (tile);
  }

  return tile;
}

static gpointer
gegl_tile_handler_zoom_command (GeglTileSource  *tile_store,
                                GeglTileCommand  command,
                                gint             x,
                                gint             y,
                                gint             z,
                                gpointer         data)
  {
  GeglTileHandler *handler  = GEGL_HANDLER (tile_store);
  /*GeglTileSource *source = handler->source;*/

  switch (command)
    {
      case GEGL_TILE_GET:
        return get_tile (tile_store, x, y, z);
      default:
        /* pass the command on */
        return gegl_tile_handler_chain_up (handler, command, x, y, z, data);
    }
}


static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileHandlerZoom *zoom = GEGL_TILE_HANDLER_ZOOM (gobject);

  switch (property_id)
    {
      case PROP_TILE_STORAGE:
        g_value_set_object (value, zoom->tile_storage);
        break;

      case PROP_BACKEND:
        g_value_set_object (value, zoom->backend);
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
  GeglTileHandlerZoom *zoom = GEGL_TILE_HANDLER_ZOOM (gobject);

  switch (property_id)
    {
      case PROP_TILE_STORAGE:
        zoom->tile_storage = g_value_get_object (value);
        break;

      case PROP_BACKEND:
        zoom->backend = g_value_get_object (value);
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
  GObject      *object;
  GeglTileHandlerZoom *zoom;

  object = G_OBJECT_CLASS (gegl_tile_handler_zoom_parent_class)->constructor (type, n_params, params);

  zoom   = GEGL_TILE_HANDLER_ZOOM (object);

  return object;
}


static void
gegl_tile_handler_zoom_class_init (GeglTileHandlerZoomClass *klass)
{
  GObjectClass      *gobject_class  = G_OBJECT_CLASS (klass);
  GeglTileSourceClass *source_class = GEGL_TILE_SOURCE_CLASS (klass);

  gobject_class->constructor  = constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  source_class->command = gegl_tile_handler_zoom_command;

  g_object_class_install_property (gobject_class, PROP_TILE_STORAGE,
                                   g_param_spec_object ("tile_storage",
                                                        "tile_storage",
                                                        "tile_storage for this tilestore (needed for tile size data)",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));


  g_object_class_install_property (gobject_class, PROP_BACKEND,
                                   g_param_spec_object ("backend",
                                                        "backend",
                                                        "backend for this tilestore (needed for tile size data)",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gegl_tile_handler_zoom_init (GeglTileHandlerZoom *self)
{
  self->backend = NULL;
  self->tile_storage = NULL;
}
