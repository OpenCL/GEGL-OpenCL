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

#if HAVE_GIO
#include <gio/gio.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
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
#include "gegl-buffer-index.h"

typedef struct
{
  GeglBufferHeader header;
  GList           *tiles;
  gchar           *path;
#if HAVE_GIO
  GFile           *file;
  GOutputStream   *o;
#else
  int             o;
#endif

  gint             tile_size;
  gint             offset;
  gint             entry_count;
  GeglBufferBlock *last_added;
  GeglBufferBlock *in_holding; /* we need to write one block added behind
                                * to be able to recompute the forward pointing
                                * link from one entry to the next.
                                */
} SaveInfo;


GeglBufferTile *
gegl_tile_entry_new (gint x,
                     gint y,
                     gint z)
{
  GeglBufferTile *entry = g_malloc (sizeof(GeglBufferTile));
  entry->block.flags = GEGL_FLAG_TILE;
  entry->block.length = sizeof (GeglBufferTile);

  entry->x = x;
  entry->y = y;
  entry->z = z;
  return entry;
}

void
gegl_tile_entry_destroy (GeglBufferTile *entry)
{
  g_free (entry);
}

static gsize write_block (SaveInfo        *info,
                          GeglBufferBlock *block)
{
   gssize ret = 0;

   if (info->in_holding)
     {
       glong allocated_pos = info->offset + info->in_holding->length;
       info->in_holding->next = allocated_pos;

       if (block == NULL)
         info->in_holding->next = 0;

#if HAVE_GIO
       ret = g_output_stream_write (info->o, info->in_holding, info->in_holding->length, NULL, NULL);
#else
       ret = write (info->o, info->in_holding, info->in_holding->length);
	   if (ret == -1) 
         ret = 0;
#endif
       info->offset += ret;
       g_assert (allocated_pos == info->offset);
     }
  /* write block should also allocate the block and update the
   * previously added blocks next pointer
   */
   info->in_holding = block;
   return ret;
}

static void
save_info_destroy (SaveInfo *info)
{
  if (!info)
    return;
  if (info->path)
    g_free (info->path);
#if HAVE_GIO
  if (info->o)
    g_object_unref (info->o);
  if (info->file)
    g_object_unref (info->file);
#else
  if (info->o != -1)
    close (info->o);
#endif
  if (info->tiles != NULL)
    {
      GList *iter;
      for (iter = info->tiles; iter; iter = iter->next)
        gegl_tile_entry_destroy (iter->data);
      g_list_free (info->tiles);
      info->tiles = NULL;
    }
  g_slice_free (SaveInfo, info);
}



static glong z_order (const GeglBufferTile *entry)
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
  const GeglBufferTile *entryA = a;
  const GeglBufferTile *entryB = b;

  return z_order (entryB) - z_order (entryA);
}


void
gegl_buffer_header_init (GeglBufferHeader *header,
                         gint              tile_width,
                         gint              tile_height,
                         gint              bpp,
                         Babl*             format)
{
  strcpy (header->magic, "GEGL");

  header->flags = GEGL_FLAG_HEADER;
  header->tile_width  = tile_width;
  header->tile_height = tile_height;
  header->bytes_per_pixel = bpp;
  {
    gchar buf[64];
    g_snprintf (buf, 64, "%s%c\n%i×%i %ibpp\n%ix%i\n\n\n\n\n\n\n\n\n", 
          babl_get_name (format), 0,
          header->tile_width,
          header->tile_height,
          header->bytes_per_pixel,
          (gint)header->width,
          (gint)header->height);
    memcpy ((header->description), buf, 64);
  }
}

void
gegl_buffer_save (GeglBuffer          *buffer,
                  const gchar         *path,
                  const GeglRectangle *roi)
{
  SaveInfo *info = g_slice_new0 (SaveInfo);

  glong prediction = 0; 
  gint bpp;

  GEGL_BUFFER_SANITY;

  /* a header should follow the same structure as a blockdef with
   * respect to the flags and next offsets, thus this is a valid
   * cast shortcut.
   */

  info->path = g_strdup (path);
#if HAVE_GIO
  info->file = g_file_new_for_commandline_arg (info->path);
  info->o    = G_OUTPUT_STREAM (g_file_replace (info->file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL));
#else
  info->o    = open (info->path, O_RDWR|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
#endif
  g_object_get (buffer, "px-size", &bpp, NULL);
  info->header.x           = buffer->extent.x;
  info->header.y           = buffer->extent.y;
  info->header.width       = buffer->extent.width;
  info->header.height      = buffer->extent.height;
  gegl_buffer_header_init (&info->header,
                           buffer->tile_storage->tile_width,
                           buffer->tile_storage->tile_height,
                           bpp,
                           buffer->tile_storage->format
                           );
  info->header.next = (prediction += sizeof (GeglBufferHeader));


  info->tile_size = info->header.tile_width  *
                    info->header.tile_height *
                    info->header.bytes_per_pixel;

  g_assert (info->tile_size % 16 == 0);

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

    {
      gint z;
      gint factor = 1;
      for (z = 0; z < 1; z++)
        {
          bufy = y;
          while (bufy < buffer->extent.y + height)
            {
              gint tiledy  = buffer->extent.y + bufy;
              gint offsety = gegl_tile_offset (tiledy, tile_height);
              gint bufx    = x;

              while (bufx < buffer->extent.x + width)
                {
                  gint tiledx  = buffer->extent.x + bufx;
                  gint offsetx = gegl_tile_offset (tiledx, tile_width);

                  gint tx = gegl_tile_indice (tiledx / factor, tile_width);
                  gint ty = gegl_tile_indice (tiledy / factor, tile_height);

                  if (gegl_tile_source_exist (GEGL_TILE_SOURCE (buffer), tx, ty, z))
                    {
                      GeglBufferTile *entry;

                      entry = gegl_tile_entry_new (tx, ty, z);
                      info->tiles = g_list_prepend (info->tiles, entry);
                      info->entry_count++;
                    }
                  bufx += (tile_width - offsetx) * factor;
                }
              bufy += (tile_height - offsety) * factor;
            }
          factor *= 2;
        }
    }


  /* XXX:why was the list reversed here when it is just about to be sorted?
   */
/*    info->tiles = g_list_reverse (info->tiles);*/
  }

  /* sort the list of tiles into zorder */
  info->tiles = g_list_sort (info->tiles, z_order_compare);

  /* set the offset in the file each tile will be stored on */
  {
    GList *iter;
    gint   predicted_offset = sizeof (GeglBufferHeader) +
                              sizeof (GeglBufferTile) * (info->entry_count);
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglBufferTile *entry = iter->data;
        entry->block.next = iter->next?
                            (prediction += sizeof (GeglBufferTile)):0;
        entry->offset = predicted_offset;
        predicted_offset += info->tile_size;
      }
  }

  /* save the header */
#if HAVE_GIO
  info->offset += g_output_stream_write (info->o, &info->header, sizeof (GeglBufferHeader), NULL, NULL);
#else
  {
    ssize_t ret = write (info->o, &info->header, sizeof (GeglBufferHeader));
	if (ret != -1)
      info->offset += ret;
  }
#endif
  g_assert (info->offset == info->header.next);

  /* save the index */
  {
    GList *iter;
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglBufferItem *item = iter->data;

        write_block (info, &item->block);

      }
  }
  write_block (info, NULL); /* terminate the index */

  /* update header to point to start of new index (already done for
   * this serial saver, and the header is already written.
   */

  /* save each tile */
  {
    GList *iter;
    gint   i = 0;
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglBufferTile *entry = iter->data;
        guchar          *data;
        GeglTile        *tile;

        tile = gegl_tile_source_get_tile (GEGL_TILE_SOURCE (buffer),
                                          entry->x,
                                          entry->y,
                                          entry->z);
        g_assert (tile);
        data = gegl_tile_get_data (tile);
        g_assert (data);

        g_assert (info->offset == entry->offset);
#if HAVE_GIO
        info->offset += g_output_stream_write (info->o, data, info->tile_size, NULL, NULL);
#else
        { 
          ssize_t ret = write (info->o, data, info->tile_size);
	      if (ret != -1)
            info->offset += ret;
        }
#endif
        gegl_tile_unref (tile);
        i++;
      }
  }
  save_info_destroy (info);
}
