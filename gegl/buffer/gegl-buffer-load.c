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

#if HAVE_GIO
#include <gio/gio.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-tile-storage.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-handler.h"
#include "gegl-tile.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-handler-log.h"
#include "gegl-tile-handler-empty.h"
#include "gegl-types-internal.h"
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
#if HAVE_GIO
  GFile           *file;
  GInputStream    *i;
#else
  int              i;
#endif
  gint             tile_size;
  Babl            *format;
  goffset          offset;
  goffset          next_block;
  gboolean         got_header;
} LoadInfo;

static void seekto(LoadInfo *info, gint offset)
{
  info->offset = offset;
  GEGL_NOTE (GEGL_DEBUG_BUFFER_LOAD, "seek to %i", offset);
#if HAVE_GIO
  if(!g_seekable_seek (G_SEEKABLE (info->i), info->offset, G_SEEK_SET, NULL, NULL))
#else
  if(lseek (info->i, info->offset, SEEK_SET) == -1)
#endif
    {
      g_warning ("failed seeking");
    }
}

static void
load_info_destroy (LoadInfo *info)
{
  if (!info)
    return;
  if (info->path)
    g_free (info->path);
#if HAVE_GIO
  if (info->i)
    g_object_unref (info->i);
  if (info->file)
    g_object_unref (info->file);
#else
  if (info->i != -1)
    close (info->i);
#endif
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

#if HAVE_GIO
GeglBufferItem *
gegl_buffer_read_header (GInputStream *i,
                         goffset      *offset)
#else
GeglBufferItem *
gegl_buffer_read_header (int i,
                         goffset      *offset)
#endif
{
  goffset         placeholder;
  GeglBufferItem *ret;
  if (offset==0)
    offset = &placeholder;

#if HAVE_GIO
  if(!g_seekable_seek (G_SEEKABLE (i), 0, G_SEEK_SET, NULL, NULL))
#else
  if(lseek(i, 0, SEEK_SET) == -1)
#endif
    g_warning ("failed seeking to %i", 0);
  *offset = 0;

  ret = g_malloc (sizeof (GeglBufferHeader));
#if HAVE_GIO
  *offset += g_input_stream_read (i,
                   ((gchar*)ret),
                   sizeof(GeglBufferHeader),
                   NULL, NULL);
#else
  {
    ssize_t sz_read = read(i, ret, sizeof(GeglBufferHeader));
    if (sz_read != -1)
      *offset += sz_read;
  }
#endif

  GEGL_NOTE (GEGL_DEBUG_BUFFER_LOAD, "read header: tile-width: %i tile-height: %i next:%i  %ix%i\n",
                   ret->header.tile_width,
                   ret->header.tile_height,
                   (guint)ret->block.next,
                   ret->header.width,
                   ret->header.height);

  if (!(ret->header.magic[0]=='G' &&
       ret->header.magic[1]=='E' &&
       ret->header.magic[2]=='G' &&
       ret->header.magic[3]=='L'))
    {
      g_warning ("Magic is wrong! %s", ret->header.magic);
    }

  return ret;
}

/* reads a block of information from a geglbuffer that resides in an GInputStream,
 * if offset is NULL it is read from the current offsetition of the stream. If offset
 * is passed in the offset stored at the location is used as the initial seeking
 * point and will be updated with the offset after the read is completed.
 */
#if HAVE_GIO
static GeglBufferItem *read_block (GInputStream *i,
                                   goffset      *offset)
#else
static GeglBufferItem *read_block (int           i,
                                   goffset      *offset)
#endif
{
  GeglBufferBlock block;
  GeglBufferItem *ret;
  gsize           byte_read = 0;
  gint            own_size=0;

  if (*offset==0)
    return NULL;

  if (offset)
#if HAVE_GIO
    if(!g_seekable_seek (G_SEEKABLE (i), *offset, G_SEEK_SET, NULL, NULL))
#else
    if(lseek(i, *offset, SEEK_SET) == -1)
#endif
      g_warning ("failed seeking to %i", (gint)*offset);

#if HAVE_GIO
  byte_read += g_input_stream_read (i, &block, sizeof (GeglBufferBlock), NULL, NULL);
#else 
  {
	ssize_t sz_read = read (i, &block,  sizeof (GeglBufferBlock));
    if(sz_read != -1)
      byte_read += sz_read;
  }
#endif
  GEGL_NOTE (GEGL_DEBUG_BUFFER_LOAD, "read block: length:%i next:%i",
                          block.length, (guint)block.next);

  switch (block.flags)
     {
        case GEGL_FLAG_TILE:
        case GEGL_FLAG_FREE_TILE:
          own_size = sizeof (GeglBufferTile); 
          break;
        default:
          g_warning ("skipping unknown type of entry flags=%i", block.flags);
          break;
     }

  if (block.length != own_size)
    {
      GEGL_NOTE(GEGL_DEBUG_BUFFER_LOAD, "read block of size %i which is different from expected %i only using available expected",
        block.length, own_size);
    }

  if (block.length == own_size ||
      block.length > own_size )
    {
      /* we discard any excess information that might have been added in later
       * versions
       */
      ret = g_malloc (own_size);
      memcpy (ret, &block, sizeof (GeglBufferBlock));
#if HAVE_GIO
      byte_read += g_input_stream_read (i, ((gchar*)ret) + sizeof(GeglBufferBlock),
                                       own_size - sizeof(GeglBufferBlock),
                                       NULL, NULL);
#else
      {
        ssize_t sz_read = read (i, ((gchar*)ret) + sizeof(GeglBufferBlock),
                                own_size - sizeof(GeglBufferBlock));
        if(sz_read != -1)
          byte_read += sz_read;
      }
#endif
      ret->block.length = own_size;
    }
  else if (block.length < own_size)
    {
      ret = g_malloc (own_size);
      memcpy (ret, &block, sizeof (GeglBufferBlock));
#if HAVE_GIO
      byte_read += g_input_stream_read (i, ((gchar*)ret) + sizeof(GeglBufferBlock),
                                       block.length - sizeof (GeglBufferBlock),
                                       NULL, NULL);
#else
      {
        ssize_t sz_read = read (i, ret + sizeof(GeglBufferBlock),
								block.length - sizeof (GeglBufferBlock));
		if(sz_read != -1)
		  byte_read += sz_read;
      }
#endif
      ret->block.length = own_size;
    }
  else
    {
      ret = NULL;
      g_warning ("skipping block : of flags:%i\n", block.flags);
    }

  *offset += byte_read;
  return ret;
}

#if HAVE_GIO
GList *
gegl_buffer_read_index (GInputStream *i,
                        goffset      *offset)
#else
GList *
gegl_buffer_read_index (int           i,
                        goffset      *offset)
#endif
/* load the index */
{
  GList          *ret = NULL;
  GeglBufferItem *item;

  for (item = read_block (i, offset); item; item = read_block (i, offset))
    {
      g_assert (item);
      GEGL_NOTE (GEGL_DEBUG_BUFFER_LOAD,"loaded item: %i, %i, %i offset:%i next:%i", item->tile.x,
                 item->tile.y,
                 item->tile.z,
                 (guint)item->tile.offset,
                 (guint)item->block.next);
      *offset = item->block.next;
      ret = g_list_prepend (ret, item);
    }
  ret = g_list_reverse (ret);
  return ret;
}


static void sanity(void) { GEGL_BUFFER_SANITY; }


GeglBuffer *
gegl_buffer_open (const gchar *path)
{
  sanity();

  return g_object_new (GEGL_TYPE_BUFFER, "path", path, NULL);
}

GeglBuffer *
gegl_buffer_load (const gchar *path)
{
  GeglBuffer *ret;

  LoadInfo *info = g_slice_new0 (LoadInfo);
  

  info->path = g_strdup (path);
#if HAVE_GIO
  info->file = g_file_new_for_commandline_arg (info->path);
  info->i = G_INPUT_STREAM (g_file_read (info->file, NULL, NULL));
#else
  info->i = open (info->path, O_RDONLY);
#endif
  GEGL_NOTE (GEGL_DEBUG_BUFFER_LOAD, "starting to load buffer %s", path);
#if HAVE_GIO
  if (!info->i)
#else
  if (info->i == -1)
#endif
    {
      GEGL_NOTE (GEGL_DEBUG_BUFFER_LOAD, "failed top open %s for reading", path);
#if HAVE_GIO
      g_object_unref (info->file);
#endif
      return NULL;
    }

  {
    GeglBufferItem *header = gegl_buffer_read_header (info->i, &info->offset);
    g_assert (header);
    /*memcpy (&(info->header), header, sizeof (GeglBufferHeader));*/
    info->header = *(&header->header);
    info->offset = info->header.next;
    /*g_free (header);*/  /* is there a pointer to a string or something we're missing? */
  }



  info->tile_size    = info->header.tile_width *
                       info->header.tile_height *
                       info->header.bytes_per_pixel;
  info->format       = babl_format (info->header.description);

  ret = g_object_new (GEGL_TYPE_BUFFER,
                      "format", info->format,
                      "tile-width", info->header.tile_width,
                      "tile-height", info->header.tile_height,
                      "height", info->header.height,
                      "width", info->header.width,
                      "path", path,
                      NULL);

  /* sanity check, should probably report error condition and return safely instead
  */
  g_assert (babl_format_get_bytes_per_pixel (info->format) == info->header.bytes_per_pixel);

  info->tiles = gegl_buffer_read_index (info->i, &info->offset);

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

        if (info->offset != entry->offset)
          {
            seekto (info, entry->offset);
          }
        /*g_assert (info->offset == entry->offset);*/


        g_assert (tile);
        gegl_tile_lock (tile);

        data = gegl_tile_get_data (tile);
        g_assert (data);

#if HAVE_GIO
        info->offset += g_input_stream_read (info->i, data, info->tile_size,
                                          NULL, NULL);
#else
        {
          ssize_t sz_read = read (info->i, data, info->tile_size);
          if(sz_read != -1)
            info->offset += sz_read;
        }
#endif
        /*g_assert (info->offset == entry->offset + info->tile_size);*/

        gegl_tile_unlock (tile);
        gegl_tile_unref (tile);
        i++;
      }
    GEGL_NOTE (GEGL_DEBUG_BUFFER_LOAD, "%i tiles loaded",i);
  }
  GEGL_NOTE (GEGL_DEBUG_BUFFER_LOAD, "buffer loaded %s", info->path);

  load_info_destroy (info);
  return ret;
}
