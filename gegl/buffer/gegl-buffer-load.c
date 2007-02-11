#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "../gegl-types.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-storage.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-trait.h"
#include "gegl-tile.h"
#include "gegl-tile-cache.h"
#include "gegl-tile-log.h"
#include "gegl-tile-empty.h"
#include "gegl-buffer-allocator.h"
#include "gegl-types.h"
#include "gegl-utils.h"
#include "gegl-buffer-save.h"
#include "gegl-cache.h"
#include "gegl-region.h"

typedef struct {
  GeglBufferFileHeader  header;
  GList      *tiles;
  gchar      *path;
  gint        fd;
  gint        tile_size;
  gint        x_tile_shift;
  gint        y_tile_shift;
  Babl       *format;
} LoadInfo;


static void load_info_destroy (LoadInfo *info)
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
      for (iter=info->tiles;iter;iter=iter->next)
        {
          GeglTileEntry *entry = iter->data;
          g_free (entry);
        }
      g_list_free (info->tiles);
      info->tiles = NULL;
    }
  g_free (info);
}

void
gegl_buffer_load (GeglBuffer    *buffer,
                  const gchar   *path)
{
  LoadInfo *info = g_malloc0 (sizeof (LoadInfo));

  if (sizeof (GeglBufferFileHeader)!=256)
    {
      g_warning ("GeglBufferFileHeader is %i bytes, should be 256 padding is off by: %i bytes %i ints", (int)sizeof (GeglBufferFileHeader), (int)sizeof(GeglBufferFileHeader)-256, (int)(sizeof(GeglBufferFileHeader)-256)/4);
      return;
    }

  if (!strcmp (info->header.magic, "_G_E_G_L"))
    {
      g_warning ("Magic is wrong!");
    }

  info->path = g_strdup (path);
  info->fd = g_open (info->path, O_RDONLY, S_IRWXU | S_IRWXO);
  if (info->fd == -1)
    {
      g_message ("Unable to open '%s' for loading a buffer", info->path);
      load_info_destroy (info);
      return;
    }

  read (info->fd, &(info->header), sizeof (GeglBufferFileHeader));

  info->tile_size = info->header.tile_width * info->header.tile_height * info->header.bpp;
  info->format = babl_format (info->header.format);
  info->x_tile_shift = buffer->total_shift_x/info->header.tile_width;
  info->y_tile_shift = buffer->total_shift_y/info->header.tile_height;

  /* load the index */
  {
    gint i;
    for (i=0;i<info->header.tile_count;i++)
      {
        GeglTile *entry = g_malloc0 (sizeof (GeglTileEntry));

        read (info->fd, entry, sizeof (GeglTileEntry));

        info->tiles = g_list_append (info->tiles, entry);
      }
  }

  /* load each tile */
  {
    GList *iter;
    gint i=0;
    for (iter = info->tiles; iter; iter = iter->next)
      {
        GeglTileEntry *entry = iter->data;
        guchar *data;
        GeglTile *tile;
        gint factor = 1 << entry->z;


        tile = gegl_tile_store_get_tile (GEGL_TILE_STORE (buffer),
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

        if (GEGL_IS_CACHE (buffer) && entry->z==0){
          GeglRectangle   rect;
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

