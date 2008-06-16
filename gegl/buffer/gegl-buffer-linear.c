#include "config.h"
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl-types.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-utils.h"

static GeglBuffer *
gegl_buffer_linear_new2 (const GeglRectangle *extent,
                         const Babl          *format,
                         gint                 rowstride)
{
  GeglRectangle empty={0,0,0,0};

  if (extent==NULL)
    extent = &empty;

  if (format==NULL)
    format = babl_format ("RGBA float");

  if (rowstride <= 0)
    rowstride = extent->width;

  return g_object_new (GEGL_TYPE_BUFFER,
                       "x", extent->x,
                       "y", extent->y,
                       "width", extent->width,
                       "height", extent->height,
                       "tile-width", rowstride,
                       "tile-height", extent->height,
                       "format", format,
                       NULL);
}

GeglBuffer *
gegl_buffer_linear_new (const GeglRectangle *extent,
                        const Babl          *format)
{
  return gegl_buffer_linear_new2 (extent, format, 0);
}


void gegl_tile_handler_cache_insert (GeglTileHandlerCache *cache,
                                     GeglTile             *tile,
                                     gint                  x,
                                     gint                  y,
                                     gint                  z);

GeglBuffer *
gegl_buffer_linear_new_from_data (const gpointer data,
                                  const Babl    *format,
                                  gint           width,
                                  gint           height,
                                  gint           rowstride,
                                  GCallback      destroy_fn,
                                  gpointer       destroy_fn_data)
{
  GeglBuffer *buffer;
  GeglRectangle extent={0,0,width, height};

  g_assert (format);

  if (rowstride <= 0)
    rowstride = width;
  else
    rowstride = rowstride / format->format.bytes_per_pixel;
  buffer = gegl_buffer_linear_new2 (&extent, format, rowstride);

  {
    GeglTile *tile = g_object_new (GEGL_TYPE_TILE, NULL);

    tile->rev        = 1;
    tile->stored_rev = 1;
    tile->tile_storage = buffer->tile_storage;
    tile->x = 0;
    tile->y = 0;
    tile->z = 0;
    tile->data       = (gpointer)data;
    tile->size       = format->format.bytes_per_pixel * rowstride * height;
    tile->next_shared = tile;
    tile->prev_shared = tile;

    {
      GeglTileHandlerCache *cache = g_object_get_data (G_OBJECT (buffer->tile_storage), "cache");
      if (cache)
        gegl_tile_handler_cache_insert (cache, tile, 0, 0, 0);
    }
    g_object_unref (tile);
  }

  return buffer;
}

typedef struct {
  gpointer       buf;
  GeglRectangle  extent;
  const Babl    *format;
} BufferInfo;

gpointer *
gegl_buffer_linear_open (GeglBuffer *buffer,
                         gint       *width,
                         gint       *height,
                         gint       *rowstride,
                         const Babl *format)
{
  if (!format)
    format = buffer->format;

  if (buffer->extent.width == buffer->tile_width &&
      buffer->extent.height == buffer->tile_height &&
      buffer->format == format)
    {
      GeglTile *tile;

      g_assert (buffer->tile_width <= buffer->tile_storage->tile_width);
      g_assert (buffer->tile_height == buffer->tile_storage->tile_height);

      tile = g_object_get_data (G_OBJECT (buffer), "linear-tile");
      g_assert (tile == NULL);
      tile = gegl_tile_source_get_tile ((GeglTileSource*) (buffer),
                                        0,0,0);
      g_assert (tile);
      gegl_buffer_lock (buffer);
      gegl_tile_lock (tile);

      g_object_set_data (G_OBJECT (buffer), "linear-tile", tile);

      if(width)*width = buffer->extent.width;
      if(height)*height = buffer->extent.height;
      if(rowstride)*rowstride = buffer->tile_storage->tile_width * format->format.bytes_per_pixel;
      return (gpointer)gegl_tile_get_data (tile);
    }
  /* FIXME: first check if there is a linear buffer, we should share that one to
   * avoid conflicts.
   */
  {
    BufferInfo *info = g_new0 (BufferInfo, 1);
    GList *linear_buffers;
    gint rs;
    linear_buffers = g_object_get_data (G_OBJECT (buffer), "linear-buffers");
    linear_buffers = g_list_append (linear_buffers, info);
    g_object_set_data (G_OBJECT (buffer), "linear-buffers", linear_buffers);

    info->extent = buffer->extent;
    info->format = format;

    rs = info->extent.width * format->format.bytes_per_pixel;
    if(width)*width = info->extent.width;
    if(height)*height = info->extent.height;
    if(rowstride)*rowstride = rs;

    info->buf = gegl_malloc (rs * info->extent.height);
    gegl_buffer_get (buffer, 1.0, &info->extent, format, info->buf, rs);
    return info->buf;
  }
  return NULL;
}

void
gegl_buffer_linear_close (GeglBuffer *buffer,
                          gpointer    linear)
{
  GeglTile *tile;
  tile = g_object_get_data (G_OBJECT (buffer), "linear-tile");
  if (tile)
    {
      gegl_tile_unlock (tile);
      gegl_buffer_unlock (buffer);
      g_object_set_data (G_OBJECT (buffer), "linear-tile", NULL);
      tile = NULL;
    }
  else
    {
      GList *linear_buffers;
      GList *iter;
      BufferInfo *info = NULL;
      linear_buffers = g_object_get_data (G_OBJECT (buffer), "linear-buffers");

      for (iter = linear_buffers; iter; iter=iter->next)
        {
          info = iter->data;
          if (info->buf == linear)
            {
              gegl_buffer_set (buffer, &info->extent, info->format, info->buf, 0);
              break;
            }
          else
            {
              info = NULL;
            }
        }

      if (info)
        {
          linear_buffers = g_list_remove (linear_buffers, linear);
          gegl_free (info->buf);
          g_free (info);
        }

      g_object_set_data (G_OBJECT (buffer), "linear-buffers", linear_buffers);
    }
  return;
}
