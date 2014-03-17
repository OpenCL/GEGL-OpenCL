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

#include <string.h>

#include <babl/babl.h>
#include <glib-object.h>

#include "gegl-types.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-handler.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-handler-private.h"
#include "gegl-tile-handler-zoom.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-storage.h"
#include "gegl-algorithms.h"


G_DEFINE_TYPE (GeglTileHandlerZoom, gegl_tile_handler_zoom,
               GEGL_TYPE_TILE_HANDLER)

static inline void set_blank (GeglTile   *dst_tile,
                              gint        width,
                              gint        height,
                              const Babl *format,
                              gint        i,
                              gint        j)
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

static inline void set_half (GeglTile   * dst_tile,
                             GeglTile   * src_tile,
                             gint         width,
                             gint         height,
                             const Babl * format,
                             gint i,
                             gint j)
{
  guchar     *dst_data   = gegl_tile_get_data (dst_tile);
  guchar     *src_data   = gegl_tile_get_data (src_tile);
  gint        bpp        = babl_format_get_bytes_per_pixel (format);

  if (i) dst_data += bpp * width / 2;
  if (j) dst_data += bpp * width * height / 2;

  gegl_downscale_2x2 (format, width, height, src_data, width * bpp, dst_data, width * bpp);
}

static GeglTile *
get_tile (GeglTileSource *gegl_tile_source,
          gint            x,
          gint            y,
          gint            z)
{
  GeglTileSource      *source = ((GeglTileHandler *) gegl_tile_source)->source;
  GeglTileHandlerZoom *zoom   = (GeglTileHandlerZoom *) gegl_tile_source;
  GeglTile            *tile   = NULL;
  GeglTileStorage     *tile_storage;
  gint                 tile_width;
  gint                 tile_height;

  if (source)
    tile = gegl_tile_source_get_tile (source, x, y, z);

  if (tile || (z == 0))
    return tile;

  tile_storage = _gegl_tile_handler_get_tile_storage ((GeglTileHandler *) zoom);

  if (z > tile_storage->seen_zoom)
    tile_storage->seen_zoom = z;

  tile_width = tile_storage->tile_width;
  tile_height = tile_storage->tile_height;

  {
    gint        i, j;
    const Babl *format = gegl_tile_backend_get_format (zoom->backend);
    GeglTile   *source_tile[2][2] = { { NULL, NULL }, { NULL, NULL } };

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

    tile = gegl_tile_handler_create_tile (GEGL_TILE_HANDLER (zoom), x, y, z);

    gegl_tile_lock (tile);

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
  GeglTileHandler *handler  = (void*)tile_store;

  if (command == GEGL_TILE_GET)
    return get_tile (tile_store, x, y, z);
  else
    return gegl_tile_handler_source_command (handler, command, x, y, z, data);
}

static void
gegl_tile_handler_zoom_class_init (GeglTileHandlerZoomClass *klass)
{
}

static void
gegl_tile_handler_zoom_init (GeglTileHandlerZoom *self)
{
  ((GeglTileSource *) self)->command = gegl_tile_handler_zoom_command;
}

GeglTileHandler *
gegl_tile_handler_zoom_new (GeglTileBackend *backend)
{
  GeglTileHandlerZoom *ret = g_object_new (GEGL_TYPE_TILE_HANDLER_ZOOM, NULL);

  ret->backend = backend;

  return (void*)ret;
}
