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

#include <string.h>
#include <errno.h>

#include <gio/gio.h>
#include <glib-object.h>

#include "gegl-types.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-load.h"
#include "gegl-tile-storage.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-handler.h"
#include "gegl-tile.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-handler-log.h"
#include "gegl-tile-handler-empty.h"
#include "gegl-types.h"
#include "gegl-utils.h"
#include "gegl-buffer-save.h"
#include "gegl-cache.h"
#include "gegl-region.h"
#include "gegl-buffer-index.h"
#include "gegl-debug.h"

#include <glib/gprintf.h>

typedef struct
{
  GeglBufferHeader header;
  GList           *tiles;
  gchar           *path;
  GFile           *file;
  GInputStream    *i;
  gint             tile_size;
  Babl            *format;
  gint             pos;
  glong            next_block;
  gboolean         got_header;
} LoadInfo;

static void seekto(LoadInfo *info, gint pos)
{
  info->pos = pos;
  GEGL_NOTE (BUFFER_LOAD, "seek to %i", pos);
  if(!g_seekable_seek (G_SEEKABLE (info->i), info->pos, G_SEEK_SET, NULL, NULL))
    {
      g_warning ("failed seeking");
    }
}

static GeglBufferItem *read_header (LoadInfo *info)
{
  GeglBufferItem *ret;

  /* XXX: initialize synchronize buffer state */

  if (info->pos != 0)
    {
      seekto (info, 0);
    }

  ret = g_malloc (sizeof (GeglBufferHeader));
  info->pos+= g_input_stream_read (info->i,
                   ((gchar*)ret),
                   sizeof(GeglBufferHeader),
                   NULL, NULL);

  info->next_block = ret->block.next;

  GEGL_NOTE (BUFFER_LOAD, "read header: tile-width: %i tile-height: %i next:%i\n",
                   ret->header.tile_width,
                   ret->header.tile_height,
                   (guint)ret->block.next);
  return ret;
}


static GeglBufferItem *read_block (LoadInfo *info)
{
  GeglBufferBlock block;
  GeglBufferItem *ret;

  if (!info->next_block)
    return NULL;
  if (info->pos != info->next_block)
    {
      seekto (info, info->next_block);
    }

  info->pos+= g_input_stream_read (info->i, &block, sizeof (GeglBufferBlock),
                                   NULL, NULL);
  GEGL_NOTE (BUFFER_LOAD, "read block: length:%i next:%i",
                   block.length,
                   (guint)block.next);

  ret = g_malloc (block.length);
  memcpy (ret, &block, sizeof (GeglBufferBlock));
  info->pos+= g_input_stream_read (info->i,
                   ((gchar*)ret) + sizeof(GeglBufferBlock),
                   block.length - sizeof(GeglBufferBlock),
                   NULL, NULL);
  info->next_block = ret->block.next;
  return ret;
}

static void
load_info_destroy (LoadInfo *info)
{
  if (!info)
    return;
  if (info->path)
    g_free (info->path);
  if (info->i)
    g_object_unref (info->i);
  if (info->file)
    g_object_unref (info->file);

  if (info->tiles != NULL)
    {
      GList *iter;
      for (iter = info->tiles; iter; iter = iter->next)
        {
          g_free (iter->data);
        }
      g_list_free (info->tiles);
      info->tiles = NULL;
    }
  g_slice_free (LoadInfo, info);
}

static void sanity(void) { GEGL_BUFFER_SANITY; }

GeglBuffer *
gegl_buffer_open (const gchar *path)
{
  GeglBuffer *ret;
  LoadInfo *info = g_slice_new0 (LoadInfo);

  sanity();

  info->path = g_strdup (path);
  info->file = g_file_new_for_commandline_arg (info->path);
  info->i = G_INPUT_STREAM (g_file_read (info->file, NULL, NULL));

  GEGL_NOTE (BUFFER_LOAD, "starting to load buffer %s", path);
#if 0
  if (info->fd == -1)
    {
      gchar *name = g_filename_display_name (info->path);

      g_message ("Unable to open '%s' for loading a buffer: %s",
                 name, g_strerror (errno));
      g_free (name);

      load_info_destroy (info);
      return;
    }
#endif

  {
    GeglBufferItem *header= read_header (info);
    g_assert (header);
    memcpy (&(info->header), header, sizeof (GeglBufferHeader));
    /*g_free (header);*/  /* is there a pointer to a string or something we're missing? */
  }


  if (!(info->header.magic[0]=='G' &&
       info->header.magic[1]=='E' &&
       info->header.magic[2]=='G' &&
       info->header.magic[3]=='L'))
    {
      g_warning ("Magic is wrong! %s", info->header.magic);
    }

  info->tile_size    = info->header.tile_width *
                       info->header.tile_height *
                       info->header.bytes_per_pixel;
  info->format       = babl_format (info->header.description);

  ret = g_object_new (GEGL_TYPE_BUFFER, "format", info->format, NULL);

  /* load the index */
  {
    GeglBufferItem *item; /* = read_block (info);*/
    for (item = read_block (info); item; item = read_block (info))
      {
        g_assert (item);
        GEGL_NOTE (BUFFER_LOAD,"loaded item: %i, %i, %i offset:%i next:%i", item->tile.x,
                                    item->tile.y,
                                    item->tile.z,
                                    (guint)item->tile.offset,
                                    (guint)item->block.next);
        info->tiles = g_list_prepend (info->tiles, item);
      }
    info->tiles = g_list_reverse (info->tiles);
  }

  /* load each tile */
  {
    GList *iter;
    gint   i = 0;
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglBufferTile *entry = iter->data;
        guchar         *data;
        GeglTile       *tile;


        tile = gegl_tile_source_get_tile (GEGL_TILE_SOURCE (ret),
                                          entry->x,
                                          entry->y,
                                          entry->z);

        if (info->pos != entry->offset)
          {
            seekto (info, entry->offset);
          }
        g_assert (info->pos == entry->offset);


        g_assert (tile);
        gegl_tile_lock (tile);

        data = gegl_tile_get_data (tile);
        g_assert (data);

        info->pos += g_input_stream_read (info->i, data, info->tile_size,
                                          NULL, NULL);

        g_assert (info->pos == entry->offset + info->tile_size);

        gegl_tile_unlock (tile);
        g_object_unref (G_OBJECT (tile));
        i++;
      }
    GEGL_NOTE (BUFFER_LOAD, "buffer loaded %s tiles loaded: %i", info->path, i);
  }

  load_info_destroy (info);
  return ret;
}
