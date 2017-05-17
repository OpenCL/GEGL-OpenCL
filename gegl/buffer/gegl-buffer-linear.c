#include "config.h"
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include "gegl-tile-handler-cache.h"

GeglBuffer *
gegl_buffer_linear_new (const GeglRectangle *extent,
                        const Babl          *format)
{
  GeglBuffer *buffer;

  if (extent==NULL)
    {
      g_error ("got a NULL extent");
    }

  if (format==NULL)
    format = gegl_babl_rgba_linear_float ();

  /* creating a linear buffer for GeglBuffer is a matter of
   * requesting the correct parameters when creating the
   * buffer
   */
  buffer = g_object_new (GEGL_TYPE_BUFFER,
                       "x",          extent->x,
                       "y",          extent->y,
                       "shift-x",    -extent->x,
                       "shift-y",    -extent->y,
                       "width",      extent->width,
                       "height",     extent->height,
                       "tile-width", extent->width,
                       "tile-height", extent->height,
                       "format", format,
                       NULL);

  g_object_set_data (G_OBJECT (buffer), "is-linear", (void*)0xf00);

  return buffer;
}

/* XXX:
 * this should probably be abstracted into a gegl_buffer_cache_insert_tile */
void gegl_tile_handler_cache_insert (GeglTileHandlerCache *cache,
                                     GeglTile             *tile,
                                     gint                  x,
                                     gint                  y,
                                     gint                  z);

GeglBuffer *
gegl_buffer_linear_new_from_data (const gpointer       data,
                                  const Babl          *format,
                                  const GeglRectangle *extent,
                                  gint                 rowstride,
                                  GDestroyNotify       destroy_fn,
                                  gpointer             destroy_fn_data)
{
  GeglBuffer *buffer;
  GeglTile   *tile;
  gint        bpp;

  g_return_val_if_fail (extent, NULL);
  g_return_val_if_fail (format, NULL);

  bpp = babl_format_get_bytes_per_pixel (format);

  if (rowstride == 0)
    {
      rowstride = extent->width;
    }
  else
    {
      g_return_val_if_fail (rowstride > 0, NULL);
      g_return_val_if_fail (rowstride % bpp == 0, NULL);
      rowstride = rowstride / bpp;
    }

  buffer = g_object_new (GEGL_TYPE_BUFFER,
                         "x",          extent->x,
                         "y",          extent->y,
                         "shift-x",    -extent->x,
                         "shift-y",    -extent->y,
                         "width",      extent->width,
                         "height",     extent->height,
                         "tile-width", rowstride,
                         "tile-height", extent->height,
                         "format", format,
                         "path", "RAM",
                         NULL);

  g_object_set_data (G_OBJECT (buffer), "is-linear", (void*)0xf00);

  tile = gegl_tile_new_bare ();

  tile->tile_storage = buffer->tile_storage;
  tile->x = 0;
  tile->y = 0;
  tile->z = 0;
  tile->next_shared = tile;
  tile->prev_shared = tile;
  tile->rev = tile->stored_rev + 1;
  gegl_tile_set_data_full (tile,
                           (gpointer) data,
                           bpp * rowstride * extent->height,
                           destroy_fn,
                           destroy_fn_data);

  if (buffer->tile_storage->cache)
    gegl_tile_handler_cache_insert (buffer->tile_storage->cache, tile, 0, 0, 0);
  gegl_tile_unref (tile);

  return buffer;
}

/* the information kept about a linear buffer, multiple requests can
 * be handled by the same structure, the multiple clients would have
 * an immediate shared access to the linear buffer.
 */
typedef struct {
  gpointer       buf;
  GeglRectangle  extent;
  const Babl    *format;
  gint           refs;
} BufferInfo;

/* FIXME: make this use direct data access in more cases than the
 * case of the base buffer.
 */
gpointer
gegl_buffer_linear_open (GeglBuffer          *buffer,
                         const GeglRectangle *extent,   /* if NULL, use buf  */
                         gint                *rowstride,/* returns rowstride */
                         const Babl          *format)   /* if NULL, from buf */
{
  if (!format)
    format = buffer->soft_format;

  if (extent == NULL)
    extent=&buffer->extent;

  /*gegl_buffer_lock (buffer);*/
  g_rec_mutex_lock (&buffer->tile_storage->mutex);
  if (extent->x     == buffer->extent.x &&
      extent->y     == buffer->extent.y &&
      extent->width == buffer->tile_width &&
      extent->height <= buffer->tile_height &&
      buffer->soft_format == format)
    {
      GeglTile *tile;

      g_assert (buffer->tile_width <= buffer->tile_storage->tile_width);
      g_assert (buffer->tile_height == buffer->tile_storage->tile_height);

      tile = g_object_get_data (G_OBJECT (buffer), "linear-tile");
      g_assert (tile == NULL); /* We need to reference count returned direct
                                * linear buffers to allow multiple open like
                                * the copying case.
                                */
      tile = gegl_tile_source_get_tile ((GeglTileSource*) (buffer),
                                        0,0,0);
      g_assert (tile);
      gegl_tile_lock (tile);

      g_object_set_data (G_OBJECT (buffer), "linear-tile", tile);

      if(rowstride)*rowstride = buffer->tile_storage->tile_width * babl_format_get_bytes_per_pixel (format);
      return (gpointer)gegl_tile_get_data (tile);
    }
  /* first check if there is a linear buffer, share the existing buffer if one
   * exists.
   */
    {
      GList *linear_buffers;
      GList *iter;
      BufferInfo *info = NULL;
      linear_buffers = g_object_get_data (G_OBJECT (buffer), "linear-buffers");

      for (iter = linear_buffers; iter; iter=iter->next)
        {
          info = iter->data;
          if (info->format        == format           &&
              info->extent.x      == buffer->extent.x &&
              info->extent.y      == buffer->extent.y &&
              info->extent.width  == buffer->extent.width &&
              info->extent.height == buffer->extent.height
              )
            {
              info->refs++;
              g_print ("!!!!!! sharing a linear buffer!!!!!\n");
              return info->buf;
            }
        }
    }

  {
    BufferInfo *info = g_new0 (BufferInfo, 1);
    GList *linear_buffers;
    gint rs;
    linear_buffers = g_object_get_data (G_OBJECT (buffer), "linear-buffers");
    linear_buffers = g_list_append (linear_buffers, info);
    g_object_set_data (G_OBJECT (buffer), "linear-buffers", linear_buffers);

    info->extent = *extent;
    info->format = format;

    rs = info->extent.width * babl_format_get_bytes_per_pixel (format);
    if(rowstride)*rowstride = rs;

    info->buf = gegl_malloc (rs * info->extent.height);
    gegl_buffer_get_unlocked (buffer, 1.0, &info->extent, format, info->buf, rs, GEGL_ABYSS_NONE);
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
      gegl_tile_unref (tile);
      g_object_set_data (G_OBJECT (buffer), "linear-tile", NULL);
    }
  else
    {
      GList *linear_buffers;
      GList *iter;
      linear_buffers = g_object_get_data (G_OBJECT (buffer), "linear-buffers");

      for (iter = linear_buffers; iter; iter=iter->next)
        {
          BufferInfo *info = iter->data;

          if (info->buf == linear)
            {
              info->refs--;

              if (info->refs>0)
                {
                  g_print ("EEeeek! %s\n", G_STRLOC);
                return; /* there are still others holding a reference to
                         * this linear buffer
                         */
                }

              linear_buffers = g_list_remove (linear_buffers, info);
              g_object_set_data (G_OBJECT (buffer), "linear-buffers", linear_buffers);

              g_rec_mutex_unlock (&buffer->tile_storage->mutex);
              /* XXX: potential race */
              gegl_buffer_set (buffer, &info->extent, 0, info->format, info->buf, 0);

              gegl_free (info->buf);
              g_free (info);

              g_rec_mutex_lock (&buffer->tile_storage->mutex);
              break;
            }
        }
    }
  /*gegl_buffer_unlock (buffer);*/
  g_rec_mutex_unlock (&buffer->tile_storage->mutex);
  return;
}
