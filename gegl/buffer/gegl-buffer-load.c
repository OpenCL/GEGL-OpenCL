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
  g_printf ("seek to %i\n", pos);
  g_seekable_seek (G_SEEKABLE (info->i), info->pos, G_SEEK_SET, NULL, NULL);
}

static GeglBufferItem *read_header (LoadInfo *info)
{
  GeglBufferBlock block;
  GeglBufferItem *ret;

  /* XXX: initialize synchronize buffer state */

  if (info->pos != 0)
    {
      seekto (info, 0);
    }

  info->pos += g_input_stream_read (info->i, &block, sizeof (GeglBufferBlock), NULL, NULL);

  if (info->got_header)
    {
      ret = g_malloc (block.length);
      memcpy (ret, &block, sizeof (GeglBufferBlock));
      info->pos+= g_input_stream_read (info->i,
                       ((gchar*)ret) + sizeof(GeglBufferBlock),
                       block.length - sizeof(GeglBufferBlock),
                       NULL, NULL);
    }
  else
    {
      info->got_header = TRUE;
      ret = g_malloc (sizeof (GeglBufferHeader));
      memcpy (ret, &block, sizeof (GeglBufferBlock));
      info->pos+= g_input_stream_read (info->i,
                       ((gchar*)ret) + sizeof(GeglBufferBlock),
                       sizeof(GeglBufferHeader) - sizeof(GeglBufferBlock),
                       NULL, NULL);
    }
  info->next_block = ret->block.next;

  g_printf ("header: next:%i\n", (guint)ret->block.next);
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
  if (info->got_header)
    {
      ret = g_malloc (block.length);
      memcpy (ret, &block, sizeof (GeglBufferBlock));
      info->pos+= g_input_stream_read (info->i,
                       ((gchar*)ret) + sizeof(GeglBufferBlock),
                       block.length - sizeof(GeglBufferBlock),
                       NULL, NULL);
    }
  else
    {
      info->got_header = TRUE;
      ret = g_malloc (sizeof (GeglBufferHeader));
      memcpy (ret, &block, sizeof (GeglBufferBlock));
      info->pos+= g_input_stream_read (info->i,  ((gchar*)ret) + sizeof(GeglBufferBlock),
                       sizeof(GeglBufferHeader) - sizeof(GeglBufferBlock),
                       NULL, NULL);
    }
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


void
gegl_buffer_load (GeglBuffer  *buffer,
                  const gchar *path)
{
  LoadInfo *info = g_slice_new0 (LoadInfo);

  GEGL_BUFFER_SANITY;

  info->path = g_strdup (path);
  info->file = g_file_new_for_commandline_arg (info->path);
  info->i = G_INPUT_STREAM (g_file_read (info->file, NULL, NULL));
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

  /* load the index */
  {
    GeglBufferItem *item; /* = read_block (info);*/
    for (item = read_block (info); item; item = read_block (info))
      {
        g_assert (item);
        g_print ("%i, %i, %i offset:%i next:%i\n", item->tile.x,
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


        tile = gegl_tile_source_get_tile (GEGL_TILE_SOURCE (buffer),
                                          entry->x,
                                          entry->y,
                                          entry->z);
        g_assert (tile);
        gegl_tile_lock (tile);

        data = gegl_tile_get_data (tile);
        g_assert (data);

        if (info->pos != entry->offset)
          {
            seekto (info, entry->offset);
          }
        g_assert (info->pos == entry->offset);

        info->pos += g_input_stream_read (info->i, data, info->tile_size,
                                          NULL, NULL);

        g_assert (info->pos == entry->offset + info->tile_size);

        gegl_tile_unlock (tile);
        g_object_unref (G_OBJECT (tile));
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
