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
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */
#include "config.h"

#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "../gegl-types.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-storage.h"
#include "gegl-tile-backend.h"
#include "gegl-handler.h"
#include "gegl-tile.h"
#include "gegl-handler-cache.h"
#include "gegl-handler-log.h"
#include "gegl-handler-empty.h"
#include "gegl-buffer-allocator.h"
#include "gegl-types.h"
#include "gegl-utils.h"
#include "gegl-buffer-save.h"
#include "gegl-cache.h"
#include "gegl-region.h"

typedef struct
{
  GeglBufferFileHeader header;
  GList               *tiles;
  gchar               *path;
  gint                 fd;
  gint                 tile_size;
  gint                 x_tile_shift;
  gint                 y_tile_shift;
  Babl                *format;
} LoadInfo;


static void
load_info_destroy (LoadInfo *info)
{
  if (!info)
    return;
  if (info->path)
    g_free (info->path);
  if (info->fd != 0 &&
      info->fd != -1)
    close (info->fd);
  if (info->tiles != NULL)
    {
      GList *iter;
      for (iter = info->tiles; iter; iter = iter->next)
        {
          g_slice_free (GeglTileEntry, iter->data);
        }
      g_list_free (info->tiles);
      info->tiles = NULL;
    }
  g_slice_free (LoadInfo, info);
}

void
gegl_buffer_load (GeglBuffer  *buffer,
                  const gchar *path)
{
  LoadInfo *info = g_slice_new0 (LoadInfo);

  if (sizeof (GeglBufferFileHeader) != 256)
    {
      g_warning ("GeglBufferFileHeader is %i bytes, should be 256 padding is off by: %i bytes %i ints", (int) sizeof (GeglBufferFileHeader), (int) sizeof (GeglBufferFileHeader) - 256, (int) (sizeof (GeglBufferFileHeader) - 256) / 4);
      return;
    }

  if (!strcmp (info->header.magic, "_G_E_G_L"))
    {
      g_warning ("Magic is wrong!");
    }

  info->path = g_strdup (path);
  info->fd   = g_open (info->path, O_RDONLY, 0);
  if (info->fd == -1)
    {
      g_message ("Unable to open '%s' for loading a buffer", info->path);
      load_info_destroy (info);
      return;
    }

  read (info->fd, &(info->header), sizeof (GeglBufferFileHeader));

  info->tile_size    = info->header.tile_width * info->header.tile_height * info->header.bpp;
  info->format       = babl_format (info->header.format);
  info->x_tile_shift = buffer->shift_x / info->header.tile_width;
  info->y_tile_shift = buffer->shift_y / info->header.tile_height;

  /* load the index */
  {
    gint i;
    for (i = 0; i < info->header.tile_count; i++)
      {
        GeglTileEntry *entry = g_slice_new0 (GeglTileEntry);

        read (info->fd, entry, sizeof (GeglTileEntry));

        info->tiles = g_list_append (info->tiles, entry);
      }
  }

  /* load each tile */
  {
    GList *iter;
    gint   i = 0;
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglTileEntry *entry = iter->data;
        guchar        *data;
        GeglTile      *tile;
        gint           factor = 1 << entry->z;


        tile = gegl_provider_get_tile (GEGL_PROVIDER (buffer),
                                         entry->x + info->x_tile_shift / factor,
                                         entry->y + info->y_tile_shift / factor,
                                         entry->z);
        g_assert (tile);
        gegl_tile_lock (tile);

        data = gegl_tile_get_data (tile);
        g_assert (data);

        read (info->fd, data, info->tile_size);

        gegl_tile_unlock (tile);
        g_object_unref (G_OBJECT (tile));
        /*fprintf (stderr, "\r%f  %i %i %i  ", (1.0*i)/info->header.tile_count, entry->x - info->x_tile_shift, entry->y, entry->z);*/
        i++;

        if (GEGL_IS_CACHE (buffer) && entry->z == 0)
          {
            GeglRectangle rect;
            gegl_rectangle_set (&rect, entry->x * info->header.tile_width,
                                entry->y * info->header.tile_height,
                                info->header.tile_width,
                                info->header.tile_height);
            gegl_region_union_with_rect (GEGL_CACHE (buffer)->valid_region, &rect);
          }
      }
    /*fprintf (stderr, "done      \n");*/
  }


  load_info_destroy (info);
}
