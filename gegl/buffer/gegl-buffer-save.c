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

#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#endif

#include "gegl-types.h"

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

typedef struct
{
  GeglBufferFileHeader header;
  GList               *tiles;
  gchar               *path;
  gint                 fd;
  gint                 tile_size;
  gint                 x_tile_shift;
  gint                 y_tile_shift;
  gint                 offset;
} SaveInfo;


static GeglTileEntry *
tile_entry_new (gint x,
                gint y,
                gint z)
{
  GeglTileEntry *entry = g_slice_new0 (GeglTileEntry);

  entry->x = x;
  entry->y = y;
  entry->z = z;
  return entry;
}

static void
tile_entry_destroy (GeglTileEntry *entry)
{
  g_slice_free (GeglTileEntry, entry);
}

static void
save_info_destroy (SaveInfo *info)
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
        tile_entry_destroy (iter->data);
      g_list_free (info->tiles);
      info->tiles = NULL;
    }
  g_slice_free (SaveInfo, info);
}



static glong z_order (const GeglTileEntry *entry)
{
  glong value;

  gint  i;
  gint  srcA = entry->x;
  gint  srcB = entry->y;
  gint  srcC = entry->z;

  /* interleave the 10 least significant bits of all coordinates,
   * this gives us Z-order / morton order of the space and should
   * work well as a hash
   */
  value = 0;
  for (i = 20; i >= 0; i--)
    {
#define ADD_BIT(bit)    do { value |= (((bit) != 0) ? 1 : 0); value <<= 1; \
    } \
  while (0)
      ADD_BIT (srcA & (1 << i));
      ADD_BIT (srcB & (1 << i));
      ADD_BIT (srcC & (1 << i));
#undef ADD_BIT
    }
  return value;
}

static gint z_order_compare (gconstpointer a,
                             gconstpointer b)
{
  const GeglTileEntry *entryA = a;
  const GeglTileEntry *entryB = b;

  return z_order (entryB) - z_order (entryA);
}

void
gegl_buffer_save (GeglBuffer          *buffer,
                  const gchar         *path,
                  const GeglRectangle *roi)
{
  SaveInfo *info = g_slice_new0 (SaveInfo);

  if (sizeof (GeglBufferFileHeader) != 256)
    {
      g_warning ("GeglBufferFileHeader is %i bytes, should be 256 padding is off by: %i bytes %i ints", (int) sizeof (GeglBufferFileHeader), (int) sizeof (GeglBufferFileHeader) - 256, (int) (sizeof (GeglBufferFileHeader) - 256) / 4);
      return;
    }

  strcpy (info->header.magic, "_G_E_G_L");
  info->path = g_strdup (path);
  info->fd   = g_open (info->path,
                       O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (info->fd == -1)
    {
      gchar *name = g_filename_display_name (info->path);

      g_message ("Unable to open '%s' when saving a buffer: %s",
                 name, g_strerror (errno));
      g_free (name);

      save_info_destroy (info);
      return;
    }

  info->header.width       = buffer->extent.width;
  info->header.height      = buffer->extent.height;
  info->header.x           = buffer->extent.x;
  info->header.y           = buffer->extent.y;
  info->header.tile_width  = buffer->storage->tile_width;
  info->header.tile_height = buffer->storage->tile_height;

  g_object_get (buffer, "px-size", &(info->header.bpp), NULL);
/*  = gegl_buffer_px_size (buffer);*/

  info->tile_size = info->header.tile_width * info->header.tile_height * info->header.bpp;
  strcpy (info->header.format, ((Babl *) (buffer->storage->format))->instance.name);

  /* collect list of tiles to be written */
  {
    gint width       = buffer->extent.width;
    gint height      = buffer->extent.height;
    gint tile_width  = info->header.tile_width;
    gint tile_height = info->header.tile_height;
    gint x           = 0;
    gint y           = 0;

    int  bufy;

    if (roi)
      {
        x      = roi->x;
        y      = roi->y;
        width  = roi->width;
        height = roi->height;
      }

    info->x_tile_shift = -buffer->shift_x / tile_width;
    info->y_tile_shift = -buffer->shift_y / tile_height;


    {
      gint z;
      gint factor = 1;
      for (z = 0; z < 10; z++)
        {
          bufy = y;
          while (bufy < buffer->extent.y + height)
            {
              gint tiledy  = buffer->extent.y + buffer->shift_y + bufy;
              gint offsety = gegl_tile_offset (tiledy, tile_height);
              gint bufx    = x;

              while (bufx < buffer->extent.x + width)
                {
                  gint tiledx  = buffer->extent.x + bufx + buffer->shift_x;
                  gint offsetx = gegl_tile_offset (tiledx, tile_width);

                  gint tx = gegl_tile_indice (tiledx / factor, tile_width);
                  gint ty = gegl_tile_indice (tiledy / factor, tile_height);

                  if (gegl_provider_exist (GEGL_PROVIDER (buffer), tx, ty, z))
                    {
                      tx += info->x_tile_shift / factor;
                      ty += info->y_tile_shift / factor;

                      info->tiles = g_list_prepend (info->tiles,
                                                    tile_entry_new (tx, ty, z));
                      info->header.tile_count++;
                    }
                  bufx += (tile_width - offsetx) * factor;
                }
              bufy += (tile_height - offsety) * factor;
            }
          factor *= 2;
        }
    }

    info->tiles = g_list_reverse (info->tiles);
  }

  /* FIXME: sort the index into Z-order */

  info->tiles = g_list_sort (info->tiles, z_order_compare);

  /* set the offset in the file each tile will be stored on */
  {
    GList *iter;
    gint   offset = sizeof (GeglBufferFileHeader) + sizeof (GeglTileEntry) * info->header.tile_count;

    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglTileEntry *entry = iter->data;
        entry->offset = offset;
        offset       += info->tile_size;
      }
  }

  /* save the header */
  info->offset += write (info->fd, &info->header, sizeof (GeglBufferFileHeader));

  /* save the index */

  {
    GList *iter;
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglTileEntry *entry = iter->data;
        info->offset += write (info->fd, entry, sizeof (GeglTileEntry));
      }
  }

  /* save each tile */
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
                                         entry->x - info->x_tile_shift / factor,
                                         entry->y - info->y_tile_shift / factor,
                                         entry->z);
        g_assert (tile);
        data = gegl_tile_get_data (tile);
        g_assert (data);

        info->offset += write (info->fd, data, info->tile_size);
        g_object_unref (G_OBJECT (tile));
        /*fprintf (stderr, "\r%f%% (%ikb)", (1.0*i)/info->header.tile_count, info->offset /1024);*/
        i++;
      }
    /*fprintf (stderr, "done      \n");*/
  }


  save_info_destroy (info);
}

