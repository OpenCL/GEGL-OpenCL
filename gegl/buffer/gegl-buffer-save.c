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

typedef struct {
  GeglBufferFileHeader  header;
  GList      *tiles;
  gchar      *path;
  gint        fd;
  gint        tile_size;
  gint        x_tile_shift;
  gint        y_tile_shift;
  gint        offset;
} SaveInfo;


GeglTileEntry *tile_entry_new (gint x,
                           gint y,
                           gint z)
{
  GeglTileEntry *entry = g_malloc0 (sizeof (GeglTileEntry));
  entry->x=x;
  entry->y=y;
  entry->z=z;
  return entry;
}

void tile_entry_destroy (GeglTileEntry *entry)
{
  g_free (entry);
}

void save_info_destroy (SaveInfo *info)
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
          tile_entry_destroy (entry);
        }
      g_list_free (info->tiles);
      info->tiles = NULL;
    }
  g_free (info);
}



static glong z_order (const GeglTileEntry *entry)
{
  glong value;

  gint  i;
  gint  srcA=entry->x;
  gint  srcB=entry->y;
  gint  srcC=entry->z;
  /* interleave the 10 least significant bits of all coordinates,
   * this gives us Z-order / morton order of the space and should
   * work well as a hash
  */
  value = 0;
  for (i=20;i>=0;i--)
    {
#define ADD_BIT(bit)  do{value|=(((bit)!=0)?1:0);value <<= 1;}while(0)
      ADD_BIT (srcA & (1<<i));
      ADD_BIT (srcB & (1<<i));
      ADD_BIT (srcC & (1<<i));
#undef ADD_BIT
    }
  return value;
}

static gint z_order_compare (gconstpointer a,
                             gconstpointer b)
{
  const GeglTileEntry *entryA = a;
  const GeglTileEntry *entryB = b;

  glong zA = z_order (entryA);
  glong zB = z_order (entryB);
  return zB-zA;
}

void
gegl_buffer_save (GeglBuffer    *buffer,
                  const gchar   *path,
                  GeglRectangle *roi)
{
  SaveInfo *info = g_malloc0 (sizeof (SaveInfo));

  if (sizeof (GeglBufferFileHeader)!=256)
    {
      g_warning ("GeglBufferFileHeader is %i bytes, should be 256 padding is off by: %i bytes %i ints", (int)sizeof (GeglBufferFileHeader), (int)sizeof(GeglBufferFileHeader)-256, (int)(sizeof(GeglBufferFileHeader)-256)/4);
      return;
    }

  strcpy (info->header.magic, "_G_E_G_L");
  info->path = g_strdup (path);
  info->fd = g_open (info->path, O_WRONLY| O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (info->fd == -1)
    {
      g_message ("Unable to open '%s' when saving a buffer", info->path);
      save_info_destroy (info);
      return;
    }

  info->header.width       = buffer->width;
  info->header.height      = buffer->height;
  info->header.x           = buffer->x;
  info->header.y           = buffer->y;
  info->header.tile_width  = gegl_buffer_storage (buffer)->tile_width;
  info->header.tile_height = gegl_buffer_storage (buffer)->tile_height;
  info->header.bpp = gegl_buffer_px_size (buffer);

  info->tile_size = info->header.tile_width * info->header.tile_height * info->header.bpp;
  strcpy (info->header.format, ((Babl*)(gegl_buffer_storage(buffer)->format))->instance.name);

  /* collect list of tiles to be written */
    {
      gint width   = buffer->width;
      gint height  = buffer->height;
      gint tile_width = info->header.tile_width;
      gint tile_height = info->header.tile_height;
      gint x       = 0;
      gint y       = 0;

      int bufy;

      if (roi)
        {
          x = roi->x;
          y = roi->y;
          width = roi->width ;
          height = roi->height;
        }

      info->x_tile_shift = -buffer->total_shift_x/tile_width;
      info->y_tile_shift = -buffer->total_shift_y/tile_height;


      {
        gint z;
        gint factor=1;
        for (z=0;z<10;z++)
          {
            bufy=y;
            while (bufy < buffer->y + height)
              {
                gint tiledy  = buffer->y + buffer->total_shift_y + bufy;
                gint offsety = gegl_tile_offset (tiledy, tile_height);
                gint bufx = x;

                while (bufx < buffer->x + width)
                  {
                    gint tiledx = buffer->x + bufx + buffer->total_shift_x;
                    gint offsetx = gegl_tile_offset (tiledx, tile_width);

                    gint tx=gegl_tile_indice(tiledx/factor,tile_width);
                    gint ty=gegl_tile_indice(tiledy/factor,tile_height);

                    if (1)
                      {
                    if (gegl_tile_store_message (GEGL_TILE_STORE (buffer),
                        GEGL_TILE_EXIST, tx, ty, z, NULL))
                          {
                            tx += info->x_tile_shift/factor;
                            ty += info->y_tile_shift/factor;

                            info->tiles = g_list_append (info->tiles, tile_entry_new (tx,ty,z));
                          }
                      }
                    bufx += (tile_width - offsetx)*factor;
                  }
                bufy += (tile_height - offsety)*factor;
              }
            factor*=2;
          }
      }
    }

  info->header.tile_count = g_list_length (info->tiles);
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
        offset += info->tile_size;
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
        info->offset+=write (info->fd, entry, sizeof (GeglTileEntry));
      }
  }

  /* save each tile */
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
                                         entry->x - info->x_tile_shift/factor,
                                         entry->y - info->y_tile_shift/factor,
                                         entry->z);
        g_assert (tile);
        data = gegl_tile_get_data (tile);
        g_assert (data);

        info->offset+=write (info->fd, data, info->tile_size);
        g_object_unref (G_OBJECT (tile));
        /*fprintf (stderr, "\r%f%% (%ikb)", (1.0*i)/info->header.tile_count, info->offset /1024);*/
        i++;
      }
    /*fprintf (stderr, "done      \n");*/
  }


  save_info_destroy (info);
}

