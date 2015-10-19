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
 * Copyright 2006-2008 Øyvind Kolås <pippin@gimp.org>
 *           2013 Daniel Sabo
 */

#include "config.h"
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include "gegl.h"
#include "gegl-debug.h"
#include "gegl-types-internal.h"
#include "gegl-algorithms.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include "gegl-sampler.h"
#include "gegl-tile-backend.h"
#include "gegl-buffer-iterator.h"
#include "gegl-buffer-cl-cache.h"
#include "gegl-config.h"

static void gegl_buffer_iterate_read_fringed (GeglBuffer          *buffer,
                                              const GeglRectangle *roi,
                                              const GeglRectangle *abyss,
                                              guchar              *buf,
                                              gint                 buf_stride,
                                              const Babl          *format,
                                              gint                 level,
                                              GeglAbyssPolicy      repeat_mode);


static void gegl_buffer_iterate_read_dispatch (GeglBuffer          *buffer,
                                               const GeglRectangle *roi,
                                               guchar              *buf,
                                               gint                 rowstride,
                                               const Babl          *format,
                                               gint                 level,
                                               GeglAbyssPolicy      repeat_mode);

static void inline
gegl_buffer_get_pixel (GeglBuffer     *buffer,
                       gint            x,
                       gint            y,
                       const Babl     *format,
                       gpointer        data,
                       GeglAbyssPolicy repeat_mode)
{
  const GeglRectangle *abyss = &buffer->abyss;
  guchar              *buf   = data;

  

  if (y <  abyss->y ||
      x <  abyss->x ||
      y >= abyss->y + abyss->height ||
      x >= abyss->x + abyss->width)
    {
      switch (repeat_mode)
      {
        case GEGL_ABYSS_CLAMP:
          x = CLAMP (x, abyss->x, abyss->x+abyss->width-1);
          y = CLAMP (y, abyss->y, abyss->y+abyss->height-1);
          break;

        case GEGL_ABYSS_LOOP:
          x = abyss->x + GEGL_REMAINDER (x - abyss->x, abyss->width);
          y = abyss->y + GEGL_REMAINDER (y - abyss->y, abyss->height);
          break;

        case GEGL_ABYSS_BLACK:
          {
            gfloat color[4] = {0.0, 0.0, 0.0, 1.0};
            babl_process (babl_fish (babl_format ("RGBA float"), format),
                          color,
                          buf,
                          1);
            return;
          }

        case GEGL_ABYSS_WHITE:
          {
            gfloat color[4] = {1.0, 1.0, 1.0, 1.0};
            babl_process (babl_fish (babl_format ("RGBA float"),
                                     format),
                          color,
                          buf,
                          1);
            return;
          }

        default:
        case GEGL_ABYSS_NONE:
          memset (buf, 0x00, babl_format_get_bytes_per_pixel (format));
          return;
      }
    }

  if (gegl_config_threads()>1)
  g_rec_mutex_lock (&buffer->tile_storage->mutex);
  {
    gint tile_width  = buffer->tile_width;
    gint tile_height = buffer->tile_height;
    gint tiledy      = y + buffer->shift_y;
    gint tiledx      = x + buffer->shift_x;
    gint indice_x    = gegl_tile_indice (tiledx, tile_width);
    gint indice_y    = gegl_tile_indice (tiledy, tile_height);

    GeglTile *tile = buffer->tile_storage->hot_tile;
    const Babl *fish = NULL;

    if (!(tile &&
          tile->x == indice_x &&
          tile->y == indice_y))
      {
        _gegl_buffer_drop_hot_tile (buffer);
        tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                          indice_x, indice_y,
                                          0);
        buffer->tile_storage->hot_tile = tile;
      }

    if (tile)
      {
        gint tile_origin_x = indice_x * tile_width;
        gint tile_origin_y = indice_y * tile_height;
        gint       offsetx = tiledx - tile_origin_x;
        gint       offsety = tiledy - tile_origin_y;
        guchar    *tp;

        if (format != buffer->soft_format)
          {
            gint px_size = babl_format_get_bytes_per_pixel (buffer->soft_format);
            fish    = babl_fish (buffer->soft_format, format);
            tp = gegl_tile_get_data (tile) + (offsety * tile_width + offsetx) * px_size;
            babl_process (fish, tp, buf, 1);
          }
        else
          {
            gint px_size = babl_format_get_bytes_per_pixel (format);
            tp = gegl_tile_get_data (tile) + (offsety * tile_width + offsetx) * px_size;
            memcpy (buf, tp, px_size);
          }
      }
  }
  if (gegl_config_threads()>1)
  g_rec_mutex_unlock (&buffer->tile_storage->mutex);
}

static inline void
__gegl_buffer_set_pixel (GeglBuffer     *buffer,
                       gint            x,
                       gint            y,
                       const Babl     *format,
                       gconstpointer   data)
{
  const GeglRectangle *abyss = &buffer->abyss;
  const guchar        *buf   = data;

  if (y <  abyss->y ||
      x <  abyss->x ||
      y >= abyss->y + abyss->height ||
      x >= abyss->x + abyss->width)
    return;

  if (gegl_config_threads()>1)
  g_rec_mutex_lock (&buffer->tile_storage->mutex);
  {
    gint tile_width  = buffer->tile_width;
    gint tile_height = buffer->tile_height;
    gint tiledy      = y + buffer->shift_y;
    gint tiledx      = x + buffer->shift_x;
    gint indice_x    = gegl_tile_indice (tiledx, tile_width);
    gint indice_y    = gegl_tile_indice (tiledy, tile_height);

    GeglTile *tile = buffer->tile_storage->hot_tile;
    const Babl *fish = NULL;
    gint px_size;

    if (format != buffer->soft_format)
      {
        fish    = babl_fish (format, buffer->soft_format);
        px_size = babl_format_get_bytes_per_pixel (buffer->soft_format);
      }
    else
      {
        px_size = babl_format_get_bytes_per_pixel (buffer->soft_format);
      }

    if (!(tile &&
          tile->x == indice_x &&
          tile->y == indice_y))
      {
        _gegl_buffer_drop_hot_tile (buffer);
        tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                          indice_x, indice_y,
                                          0);
        buffer->tile_storage->hot_tile = tile;
      }

    if (tile)
      {
        gint tile_origin_x = indice_x * tile_width;
        gint tile_origin_y = indice_y * tile_height;
        gint       offsetx = tiledx - tile_origin_x;
        gint       offsety = tiledy - tile_origin_y;

        guchar *tp;
        gegl_tile_lock (tile);
        tp = gegl_tile_get_data (tile) + (offsety * tile_width + offsetx) * px_size;

        if (fish)
          babl_process (fish, buf, tp, 1);
        else
          memcpy (tp, buf, px_size);

        gegl_tile_unlock (tile);
      }
  }
  if (gegl_config_threads()>1)
  g_rec_mutex_unlock (&buffer->tile_storage->mutex);
}

enum _GeglBufferSetFlag {
  GEGL_BUFFER_SET_FLAG_FAST   = 0,
  GEGL_BUFFER_SET_FLAG_LOCK   = 1<<0,
  GEGL_BUFFER_SET_FLAG_NOTIFY = 1<<1,
  GEGL_BUFFER_SET_FLAG_FULL   = GEGL_BUFFER_SET_FLAG_LOCK |
                                GEGL_BUFFER_SET_FLAG_NOTIFY
};

typedef enum _GeglBufferSetFlag GeglBufferSetFlag;

void
gegl_buffer_set_with_flags (GeglBuffer          *buffer,
                            const GeglRectangle *rect,
                            gint                 level,
                            const Babl          *format,
                            const void          *src,
                            gint                 rowstride,
                            GeglBufferSetFlag    set_flags);


static inline void
_gegl_buffer_set_pixel (GeglBuffer       *buffer,
                        gint              x,
                        gint              y,
                        const Babl       *format,
                        gconstpointer     data,
                        GeglBufferSetFlag flags)
{
  GeglRectangle rect = {x,y,1,1};
  switch (flags)
  {
    case GEGL_BUFFER_SET_FLAG_FAST:
      __gegl_buffer_set_pixel (buffer, x, y, format, data);
    break;
    case GEGL_BUFFER_SET_FLAG_LOCK:
      gegl_buffer_lock (buffer);
      __gegl_buffer_set_pixel (buffer, x, y, format, data);
      gegl_buffer_unlock (buffer);
      break;
    case GEGL_BUFFER_SET_FLAG_NOTIFY:
      __gegl_buffer_set_pixel (buffer, x, y, format, data);
      gegl_buffer_emit_changed_signal (buffer, &rect);
      break;
    case GEGL_BUFFER_SET_FLAG_LOCK|GEGL_BUFFER_SET_FLAG_NOTIFY:
    default:
      gegl_buffer_lock (buffer);
      __gegl_buffer_set_pixel (buffer, x, y, format, data);
      gegl_buffer_unlock (buffer);
      gegl_buffer_emit_changed_signal (buffer, &rect);
      break;
  }
}

/* flush any unwritten data (flushes the hot-cache of a single
 * tile used by gegl_buffer_set for 1x1 pixel sized rectangles
 */
void
gegl_buffer_flush (GeglBuffer *buffer)
{
  GeglTileBackend *backend;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  backend = gegl_buffer_backend (buffer);

  _gegl_buffer_drop_hot_tile (buffer);

  if (backend)
    gegl_tile_backend_set_extent (backend, &buffer->extent);

  gegl_tile_source_command (GEGL_TILE_SOURCE (buffer),
                            GEGL_TILE_FLUSH, 0,0,0,NULL);
}

static inline void
gegl_buffer_iterate_write (GeglBuffer          *buffer,
                           const GeglRectangle *roi,
                           guchar              *buf,
                           gint                 rowstride,
                           const Babl          *format,
                           gint                 level)
{
  gint tile_width  = buffer->tile_storage->tile_width;
  gint tile_height = buffer->tile_storage->tile_height;
  gint px_size     = babl_format_get_bytes_per_pixel (buffer->soft_format);
  gint bpx_size    = babl_format_get_bytes_per_pixel (format);
  gint tile_stride = px_size * tile_width;
  gint buf_stride;
  gint bufy        = 0;

  gint buffer_shift_x = buffer->shift_x;
  gint buffer_shift_y = buffer->shift_y;

  gint width    = buffer->extent.width;
  gint height   = buffer->extent.height;
  gint buffer_x = buffer->extent.x + buffer_shift_x;
  gint buffer_y = buffer->extent.y + buffer_shift_y;

  gint buffer_abyss_x = buffer->abyss.x + buffer_shift_x;
  gint buffer_abyss_y = buffer->abyss.y + buffer_shift_y;
  gint abyss_x_total  = buffer_abyss_x + buffer->abyss.width;
  gint abyss_y_total  = buffer_abyss_y + buffer->abyss.height;
  gint factor         = 1<<level;
  const Babl *fish;

  /* roi specified, override buffers extent */
  if (roi)
    {
      width  = roi->width;
      height = roi->height;
      buffer_x = roi->x + buffer_shift_x;
      buffer_y = roi->y + buffer_shift_y;
    }

  buffer_abyss_x /= factor;
  buffer_abyss_y /= factor;
  abyss_x_total  /= factor;
  abyss_y_total  /= factor;
  buffer_x       /= factor;
  buffer_y       /= factor;
  width          /= factor;
  height         /= factor;

  buf_stride = width * bpx_size;
  if (rowstride != GEGL_AUTO_ROWSTRIDE)
    buf_stride = rowstride;

  if (format == buffer->soft_format)
    {
      fish = NULL;
    }
  else
      fish = babl_fish ((gpointer) format,
                        (gpointer) buffer->soft_format);

  while (bufy < height)
    {
      gint tiledy  = buffer_y + bufy;
      gint offsety = gegl_tile_offset (tiledy, tile_height);
      gint bufx    = 0;

      while (bufx < width)
        {
          gint      tiledx  = buffer_x + bufx;
          gint      offsetx = gegl_tile_offset (tiledx, tile_width);
          gint      y       = bufy;
          gint index_x;
          gint index_y;
          gint      lskip, rskip, pixels, row;
          guchar   *bp, *tile_base, *tp;
          GeglTile *tile;

          bp = buf + bufy * buf_stride + bufx * bpx_size;

          if (width + offsetx - bufx < tile_width)
            pixels = (width + offsetx - bufx) - offsetx;
          else
            pixels = tile_width - offsetx;

          index_x = gegl_tile_indice (tiledx, tile_width);
          index_y = gegl_tile_indice (tiledy, tile_height);

          tile = gegl_buffer_get_tile (buffer, index_x, index_y, level);

          lskip = (buffer_abyss_x) - (buffer_x + bufx);
          /* gap between left side of tile, and abyss */
          rskip = (buffer_x + bufx + pixels) - abyss_x_total;
          /* gap between right side of tile, and abyss */

          if (lskip < 0)
            lskip = 0;
          if (lskip > pixels)
            lskip = pixels;
          if (rskip < 0)
            rskip = 0;
          if (rskip > pixels)
            rskip = pixels;

          if (!tile)
            {
              g_warning ("didn't get tile, trying to continue");
              bufx += (tile_width - offsetx);
              continue;
            }

          gegl_tile_lock (tile);

          tile_base = gegl_tile_get_data (tile);
          tp        = ((guchar *) tile_base) + (offsety * tile_width + offsetx) * px_size;

          if (fish)
            {
              for (row = offsety;
                   row < tile_height &&
                     y < height &&
                     buffer_y + y < abyss_y_total;
                   row++, y++)
                {

                  if (buffer_y + y >= buffer_abyss_y &&
                      buffer_y + y < abyss_y_total)
                    {
                      babl_process (fish, bp + lskip * bpx_size, tp + lskip * px_size,
                                    pixels - lskip - rskip);
                    }

                  tp += tile_stride;
                  bp += buf_stride;
                }
            }
          else
            {
              for (row = offsety;
                   row < tile_height && y < height;
                   row++, y++)
                {

                  if (buffer_y + y >= buffer_abyss_y &&
                      buffer_y + y < abyss_y_total)
                    {

                      memcpy (tp + lskip * px_size, bp + lskip * px_size,
                              (pixels - lskip - rskip) * px_size);
                    }

                  tp += tile_stride;
                  bp += buf_stride;
                }
            }

          gegl_tile_unlock (tile);
          gegl_tile_unref (tile);
          bufx += (tile_width - offsetx);
        }
      bufy += (tile_height - offsety);
    }
}

static inline void
gegl_buffer_set_internal (GeglBuffer          *buffer,
                          const GeglRectangle *rect,
                          gint                 level,
                          const Babl          *format,
                          const void          *src,
                          gint                 rowstride)
{
  if (gegl_cl_is_accelerated ())
    {
      gegl_buffer_cl_cache_flush (buffer, rect);
    }

  gegl_buffer_iterate_write (buffer, rect, (void *) src, rowstride, format, level);

  if (gegl_buffer_is_shared (buffer))
    {
      gegl_buffer_flush (buffer);
    }
}

static void inline
_gegl_buffer_set_with_flags (GeglBuffer       *buffer,
                            const GeglRectangle *rect,
                            gint                 level,
                            const Babl          *format,
                            const void          *src,
                            gint                 rowstride,
                            GeglBufferSetFlag    flags)
{
  switch (flags)
  {
    case GEGL_BUFFER_SET_FLAG_FAST:
      gegl_buffer_set_internal (buffer, rect, level, format, src, rowstride);
    break;
    case GEGL_BUFFER_SET_FLAG_LOCK:
      gegl_buffer_lock (buffer);
      gegl_buffer_set_internal (buffer, rect, level, format, src, rowstride);
      gegl_buffer_unlock (buffer);
      break;
    case GEGL_BUFFER_SET_FLAG_NOTIFY:
      gegl_buffer_set_internal (buffer, rect, level, format, src, rowstride);
      if (flags & GEGL_BUFFER_SET_FLAG_NOTIFY)
        gegl_buffer_emit_changed_signal(buffer, rect);
      break;
    case GEGL_BUFFER_SET_FLAG_LOCK|GEGL_BUFFER_SET_FLAG_NOTIFY:
    default:
      gegl_buffer_lock (buffer);
      gegl_buffer_set_internal (buffer, rect, level, format, src, rowstride);
      gegl_buffer_unlock (buffer);
      gegl_buffer_emit_changed_signal(buffer, rect);
      break;
  }
}

void
gegl_buffer_set_with_flags (GeglBuffer       *buffer,
                            const GeglRectangle *rect,
                            gint                 level,
                            const Babl          *format,
                            const void          *src,
                            gint                 rowstride,
                            GeglBufferSetFlag    flags)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  if (format == NULL)
    format = buffer->soft_format;
  _gegl_buffer_set_with_flags (buffer, rect, level, format, src, rowstride, flags);
}

static void
gegl_buffer_iterate_read_simple (GeglBuffer          *buffer,
                                 const GeglRectangle *roi,
                                 guchar              *buf,
                                 gint                 buf_stride,
                                 const Babl          *format,
                                 gint                 level)
{
  gint tile_width  = buffer->tile_storage->tile_width;
  gint tile_height = buffer->tile_storage->tile_height;
  gint px_size     = babl_format_get_bytes_per_pixel (buffer->soft_format);
  gint bpx_size    = babl_format_get_bytes_per_pixel (format);
  gint tile_stride = px_size * tile_width;
  gint bufy        = 0;

  gint width    = roi->width;
  gint height   = roi->height;
  gint buffer_x = roi->x;
  gint buffer_y = roi->y;

  const Babl *fish;

  if (format == buffer->soft_format)
    fish = NULL;
  else
    fish = babl_fish ((gpointer) buffer->soft_format,
                      (gpointer) format);

  while (bufy < height)
    {
      gint tiledy  = buffer_y + bufy;
      gint offsety = gegl_tile_offset (tiledy, tile_height);
      gint bufx    = 0;

      while (bufx < width)
        {
          gint      tiledx  = buffer_x + bufx;
          gint      offsetx = gegl_tile_offset (tiledx, tile_width);
          guchar   *bp, *tile_base, *tp;
          gint      pixels, row, y;
          GeglTile *tile;

          bp = buf + bufy * buf_stride + bufx * bpx_size;

          if (width + offsetx - bufx < tile_width)
            pixels = width - bufx;
          else
            pixels = tile_width - offsetx;

          tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                            gegl_tile_indice (tiledx, tile_width),
                                            gegl_tile_indice (tiledy, tile_height),
                                            level);

          if (!tile)
            {
              g_warning ("didn't get tile, trying to continue");
              bufx += (tile_width - offsetx);
              continue;
            }

          tile_base = gegl_tile_get_data (tile);
          tp        = ((guchar *) tile_base) + (offsety * tile_width + offsetx) * px_size;

          y = bufy;
          for (row = offsety;
               row < tile_height && y < height;
               row++, y++)
            {
              if (fish)
                babl_process (fish, tp, bp, pixels);
              else
                memcpy (bp, tp, pixels * px_size);

              tp += tile_stride;
              bp += buf_stride;
            }

          gegl_tile_unref (tile);
          bufx += (tile_width - offsetx);
        }
      bufy += (tile_height - offsety);
    }
}

static void
fill_abyss_none (guchar *buf, gint width, gint height, gint buf_stride, gint pixel_size)
{
  const int byte_width = width * pixel_size;

  if (buf_stride == byte_width)
    {
      memset (buf, 0, byte_width * height);
    }
  else
    {
      while (height--)
        {
          memset (buf, 0, byte_width);
          buf += buf_stride;
        }
    }
}

static void
fill_abyss_color (guchar *buf, gint width, gint height, gint buf_stride, guchar *pixel, gint pixel_size)
{
  if (buf_stride == width * pixel_size)
    {
      gegl_memset_pattern (buf, pixel, pixel_size, width * height);
    }
  else
    {
      while (height--)
        {
          gegl_memset_pattern (buf, pixel, pixel_size, width);
          buf += buf_stride;
        }
    }
}

static void
gegl_buffer_iterate_read_abyss_color (GeglBuffer          *buffer,
                                      const GeglRectangle *roi,
                                      const GeglRectangle *abyss,
                                      guchar              *buf,
                                      gint                 buf_stride,
                                      const Babl          *format,
                                      gint                 level,
                                      guchar              *color,
                                      GeglAbyssPolicy      repeat_mode)
{
  GeglRectangle current_roi = *roi;
  gint bpp = babl_format_get_bytes_per_pixel (format);

  if (current_roi.y < abyss->y)
    {
      /* Abyss above image */
      gint height = abyss->y - current_roi.y;
      if (current_roi.height < height)
        height = current_roi.height;
      if (color)
        fill_abyss_color (buf, current_roi.width, height, buf_stride, color, bpp);
      else
        fill_abyss_none (buf, current_roi.width, height, buf_stride, bpp);
      buf += buf_stride * height;
      current_roi.y += height;
      current_roi.height -= height;
    }

  if (current_roi.height && (current_roi.y < abyss->y + abyss->height))
    {
      GeglRectangle inner_roi = current_roi;
      guchar *inner_buf       = buf;

      if (inner_roi.height + inner_roi.y > abyss->height + abyss->y)
        {
          /* Clamp inner_roi to the in abyss height */
          inner_roi.height -= (inner_roi.height + inner_roi.y) - (abyss->height + abyss->y);
        }

      if (inner_roi.x < abyss->x)
        {
          /* Abyss left of image */
          gint width = abyss->x - inner_roi.x;
          if (width > inner_roi.width)
            width = inner_roi.width;

          if (color)
            fill_abyss_color (inner_buf, width, inner_roi.height, buf_stride, color, bpp);
          else
            fill_abyss_none (inner_buf, width, inner_roi.height, buf_stride, bpp);
          inner_buf += width * bpp;
          inner_roi.x += width;
          inner_roi.width -= width;
        }

      if (inner_roi.width && (inner_roi.x < abyss->x + abyss->width))
        {
          gint full_width = inner_roi.width;

          if (inner_roi.width + inner_roi.x > abyss->width + abyss->x)
            {
              /* Clamp inner_roi to the in abyss width */
              inner_roi.width -= (inner_roi.width + inner_roi.x) - (abyss->width + abyss->x);
            }

          if (level)
            gegl_buffer_iterate_read_fringed (buffer,
                                              &inner_roi,
                                              abyss,
                                              inner_buf,
                                              buf_stride,
                                              format,
                                              level,
                                              repeat_mode);
          else
            gegl_buffer_iterate_read_simple (buffer,
                                             &inner_roi,
                                             inner_buf,
                                             buf_stride,
                                             format,
                                             level);

          inner_buf += inner_roi.width * bpp;
          inner_roi.width = full_width - inner_roi.width;
        }

      if (inner_roi.width)
        {
          /* Abyss right of image */
          if (color)
            fill_abyss_color (inner_buf, inner_roi.width, inner_roi.height, buf_stride, color, bpp);
          else
            fill_abyss_none (inner_buf, inner_roi.width, inner_roi.height, buf_stride, bpp);
        }

      buf += inner_roi.height * buf_stride;
      /* current_roi.y += inner_roi.height; */
      current_roi.height -= inner_roi.height;
    }

  if (current_roi.height)
    {
      /* Abyss below image */
      if (color)
        fill_abyss_color (buf, current_roi.width, current_roi.height, buf_stride, color, bpp);
      else
        fill_abyss_none (buf, current_roi.width, current_roi.height, buf_stride, bpp);
    }
}

static void
gegl_buffer_iterate_read_abyss_clamp (GeglBuffer          *buffer,
                                      const GeglRectangle *roi,
                                      const GeglRectangle *abyss,
                                      guchar              *buf,
                                      gint                 buf_stride,
                                      const Babl          *format,
                                      gint                 level)
{
  GeglRectangle read_output_rect;
  GeglRectangle read_input_rect;

  gint    bpp           = babl_format_get_bytes_per_pixel (format);
  gint    x_read_offset = 0;
  gint    y_read_offset = 0;
  gint    buf_offset_cols;
  gint    buf_offset_rows;
  gint    top_rows, left_cols, right_cols, bottom_rows;
  guchar *read_buf;

  if (roi->x >= abyss->x + abyss->width) /* Right of */
    x_read_offset = roi->x - (abyss->x + abyss->width) + 1;
  else if (roi->x + roi->width <= abyss->x) /* Left of */
    x_read_offset = (roi->x + roi->width) - abyss->x - 1;

  if (roi->y >= abyss->y + abyss->height) /* Above */
    y_read_offset = roi->y - (abyss->y + abyss->height) + 1;
  else if (roi->y + roi->height <= abyss->y) /* Below of */
    y_read_offset = (roi->y + roi->height) - abyss->y - 1;

  /* Intersect our shifted abyss with the roi */
  gegl_rectangle_intersect (&read_output_rect,
                            roi,
                            GEGL_RECTANGLE (abyss->x + x_read_offset,
                                            abyss->y + y_read_offset,
                                            abyss->width,
                                            abyss->height));

  /* Offset into *buf based on the intersected rect's x & y */
  buf_offset_cols = read_output_rect.x - roi->x;
  buf_offset_rows = read_output_rect.y - roi->y;
  read_buf = buf + (buf_offset_cols * bpp + buf_offset_rows * buf_stride);

  /* Convert the read output to a coresponding input */
  read_input_rect.x = read_output_rect.x - x_read_offset;
  read_input_rect.y = read_output_rect.y - y_read_offset;
  read_input_rect.width = read_output_rect.width;
  read_input_rect.height = read_output_rect.height;

  if (level)
    gegl_buffer_iterate_read_fringed (buffer,
                                      &read_input_rect,
                                      abyss,
                                      read_buf,
                                      buf_stride,
                                      format,
                                      level,
                                      GEGL_ABYSS_CLAMP);
  else
    gegl_buffer_iterate_read_simple (buffer,
                                     &read_input_rect,
                                     read_buf,
                                     buf_stride,
                                     format,
                                     level);

  /* All calculations are done relative to read_output_rect because it is guranteed
   * to be inside of the roi rect and none of these calculations can return a value
   * less than 0.
   */
  top_rows = read_output_rect.y - roi->y;
  left_cols = read_output_rect.x - roi->x;
  right_cols = (roi->x + roi->width) - (read_output_rect.x + read_output_rect.width);
  bottom_rows = (roi->y + roi->height) - (read_output_rect.y + read_output_rect.height);

  if (top_rows)
    {
      guchar *fill_buf = buf;
      /* Top left pixel */
      if (left_cols)
        {
          guchar *src_pixel = read_buf;
          fill_abyss_color (fill_buf, left_cols, top_rows, buf_stride, src_pixel, bpp);
          fill_buf += left_cols * bpp;
        }

      /* Top rows */
      {
        guchar *src_pixel = read_buf;
        guchar *row_fill_buf = fill_buf;
        gint byte_width = read_output_rect.width * bpp;
        gint i;
        for (i = 0; i < top_rows; ++i)
          {
            memcpy (row_fill_buf, src_pixel, byte_width);
            row_fill_buf += buf_stride;
          }
      }

      fill_buf += (read_input_rect.width) * bpp;
      /* Top right pixel */
      if (right_cols)
        {
          guchar *src_pixel = read_buf + (read_input_rect.width - 1) * bpp;
          fill_abyss_color (fill_buf, right_cols, top_rows, buf_stride, src_pixel, bpp);
        }
    }

  /* Left */
  if (left_cols)
    {
      guchar *row_fill_buf = buf + (top_rows * buf_stride);
      guchar *src_pixel = read_buf;
      gint i;

      for (i = 0; i < read_output_rect.height; ++i)
        {
          gegl_memset_pattern (row_fill_buf, src_pixel, bpp, left_cols);
          row_fill_buf += buf_stride;
          src_pixel += buf_stride;
        }
    }

  /* Right */
  if (right_cols)
    {
      guchar *row_fill_buf = buf + (read_input_rect.width + left_cols) * bpp
                                 + top_rows * buf_stride;
      guchar *src_pixel = read_buf + (read_input_rect.width - 1) * bpp;
      gint i;

      for (i = 0; i < read_output_rect.height; ++i)
        {
          gegl_memset_pattern (row_fill_buf, src_pixel, bpp, right_cols);
          row_fill_buf += buf_stride;
          src_pixel += buf_stride;
        }
    }

  if (bottom_rows)
    {
      guchar *fill_buf = buf + (read_input_rect.height + top_rows) * buf_stride;
      /* Bottom left */
      if (left_cols)
        {
          guchar *src_pixel = read_buf + (read_input_rect.height - 1) * buf_stride;
          fill_abyss_color (fill_buf, left_cols, bottom_rows, buf_stride, src_pixel, bpp);
          fill_buf += left_cols * bpp;
        }

      /* Bottom rows */
      {
        guchar *src_pixel = read_buf + (read_input_rect.height - 1) * buf_stride;
        guchar *row_fill_buf = fill_buf;
        gint byte_width = read_output_rect.width * bpp;
        gint i;
        for (i = 0; i < bottom_rows; ++i)
          {
            memcpy (row_fill_buf, src_pixel, byte_width);
            row_fill_buf += buf_stride;
          }
      }

      fill_buf += read_input_rect.width * bpp;
      /* Bottom right */
      if (right_cols)
        {
          guchar *src_pixel = read_buf + (read_input_rect.width - 1) * bpp + (read_input_rect.height - 1) * buf_stride;
          fill_abyss_color (fill_buf, right_cols, bottom_rows, buf_stride, src_pixel, bpp);
        }
    }
}

static void
gegl_buffer_iterate_read_abyss_loop (GeglBuffer          *buffer,
                                     const GeglRectangle *roi,
                                     const GeglRectangle *abyss,
                                     guchar              *buf,
                                     gint                 buf_stride,
                                     const Babl          *format,
                                     gint                 level)
{
  GeglRectangle current_roi;
  gint          bpp = babl_format_get_bytes_per_pixel (format);
  gint          origin_x;

  /* Loop abyss works like iterating over a grid of tiles the size of the abyss */
  gint loop_chunk_ix = gegl_tile_indice (roi->x - abyss->x, abyss->width);
  gint loop_chunk_iy = gegl_tile_indice (roi->y - abyss->y, abyss->height);

  current_roi.x = loop_chunk_ix * abyss->width + abyss->x;
  current_roi.y = loop_chunk_iy * abyss->height + abyss->y;

  current_roi.width  = abyss->width;
  current_roi.height = abyss->height;

  origin_x = current_roi.x;

  while (current_roi.y < roi->y + roi->height)
    {
      guchar *inner_buf  = buf;
      gint    row_height = 0;

      while (current_roi.x < roi->x + roi->width)
        {
          GeglRectangle simple_roi;
          gegl_rectangle_intersect (&simple_roi, &current_roi, roi);

          gegl_buffer_iterate_read_simple (buffer,
                                           GEGL_RECTANGLE (abyss->x + (simple_roi.x - current_roi.x),
                                                           abyss->y + (simple_roi.y - current_roi.y),
                                                           simple_roi.width,
                                                           simple_roi.height),
                                           inner_buf,
                                           buf_stride,
                                           format,
                                           level);

          row_height  = simple_roi.height;
          inner_buf  += simple_roi.width * bpp;

          current_roi.x += abyss->width;
        }

      buf += buf_stride * row_height;

      current_roi.x  = origin_x;
      current_roi.y += abyss->height;
    }
}

static gpointer
gegl_buffer_read_at_level (GeglBuffer          *buffer,
                           const GeglRectangle *roi,
                           guchar              *buf,
                           gint                 rowstride,
                           const Babl          *format,
                           gint                 level,
                           GeglAbyssPolicy      repeat_mode)
{
  gint bpp = babl_format_get_bytes_per_pixel (format);

  if (level == 0)
    {
      if (!buf)
        {
          gpointer scratch = gegl_malloc (bpp * roi->width * roi->height);

          gegl_buffer_iterate_read_dispatch (buffer, roi, scratch, roi->width * bpp, format, 0, repeat_mode);

          return scratch;
        }
      else
        {
          gegl_buffer_iterate_read_dispatch (buffer, roi, buf, rowstride, format, 0, repeat_mode);

          return NULL;
        }
    }
  else
    {
      gpointer scratch;
      GeglRectangle next_roi;
      next_roi.x = roi->x * 2;
      next_roi.y = roi->y * 2;
      next_roi.width = roi->width * 2;
      next_roi.height = roi->height * 2;

      /* If the next block is too big split it in half */
      if (next_roi.width * next_roi.height > 256 * 256)
        {
          GeglRectangle next_roi_a = next_roi;
          GeglRectangle next_roi_b = next_roi;
          gint scratch_stride = next_roi.width * bpp;
          gpointer scratch_a;
          gpointer scratch_b;
          scratch = gegl_malloc (bpp * next_roi.width * next_roi.height);

          if (next_roi.width > next_roi.height)
            {
              next_roi_a.width = roi->width;
              next_roi_b.width = roi->width;
              next_roi_b.x += next_roi_a.width;

              scratch_a = scratch;
              scratch_b = (guchar *)scratch + next_roi_a.width * bpp;
            }
          else
            {
              next_roi_a.height = roi->height;
              next_roi_b.height = roi->height;
              next_roi_b.y += next_roi_a.height;

              scratch_a = scratch;
              scratch_b = (guchar *)scratch + next_roi_a.height * scratch_stride;
            }

          gegl_buffer_read_at_level (buffer, &next_roi_a, scratch_a, scratch_stride, format, level - 1, repeat_mode);
          gegl_buffer_read_at_level (buffer, &next_roi_b, scratch_b, scratch_stride, format, level - 1, repeat_mode);

        }
      else
        {
          scratch = gegl_buffer_read_at_level (buffer, &next_roi, NULL, 0, format, level - 1, repeat_mode);
        }

      if (buf)
        {
          gegl_downscale_2x2 (format,
                              next_roi.width,
                              next_roi.height,
                              scratch,
                              next_roi.width * bpp,
                              buf,
                              rowstride);
          gegl_free (scratch);
          return NULL;
        }
      else
        {
          gegl_downscale_2x2 (format,
                              next_roi.width,
                              next_roi.height,
                              scratch,
                              next_roi.width * bpp,
                              scratch,
                              roi->width * bpp);
          return scratch;
        }
    }
}

static void
gegl_buffer_iterate_read_fringed (GeglBuffer          *buffer,
                                  const GeglRectangle *roi,
                                  const GeglRectangle *abyss,
                                  guchar              *buf,
                                  gint                 buf_stride,
                                  const Babl          *format,
                                  gint                 level,
                                  GeglAbyssPolicy      repeat_mode)
{
  gint x = roi->x;
  gint y = roi->y;
  gint width  = roi->width;
  gint height = roi->height;
  guchar        *inner_buf = buf;

  gint bpp = babl_format_get_bytes_per_pixel (format);

  if (x <= abyss->x)
    {
      GeglRectangle fringe_roi = {x, y, 1, height};
      guchar *fringe_buf = inner_buf;

      gegl_buffer_read_at_level (buffer, &fringe_roi, fringe_buf, buf_stride, format, level, repeat_mode);
      inner_buf += bpp;
      x     += 1;
      width -= 1;

      if (!width)
        return;
    }

  if (y <= abyss->y)
    {
      GeglRectangle fringe_roi = {x, y, width, 1};
      guchar *fringe_buf = inner_buf;

      gegl_buffer_read_at_level (buffer, &fringe_roi, fringe_buf, buf_stride, format, level, repeat_mode);
      inner_buf += buf_stride;
      y      += 1;
      height -= 1;

      if (!height)
        return;
    }

  if (y + height >= abyss->y + abyss->height)
    {
      GeglRectangle fringe_roi = {x, y + height - 1, width, 1};
      guchar *fringe_buf = inner_buf + (height - 1) * buf_stride;

      gegl_buffer_read_at_level (buffer, &fringe_roi, fringe_buf, buf_stride, format, level, repeat_mode);
      height -= 1;

      if (!height)
        return;
    }

  if (x + width >= abyss->x + abyss->width)
    {
      GeglRectangle fringe_roi = {x + width - 1, y, 1, height};
      guchar *fringe_buf = inner_buf + (width - 1) * bpp;

      gegl_buffer_read_at_level (buffer, &fringe_roi, fringe_buf, buf_stride, format, level, repeat_mode);
      width -= 1;

      if (!width)
        return;
    }

  gegl_buffer_iterate_read_simple (buffer,
                                   GEGL_RECTANGLE (x, y, width, height),
                                   inner_buf,
                                   buf_stride,
                                   format,
                                   level);
}

static void
gegl_buffer_iterate_read_dispatch (GeglBuffer          *buffer,
                                   const GeglRectangle *roi,
                                   guchar              *buf,
                                   gint                 rowstride,
                                   const Babl          *format,
                                   gint                 level,
                                   GeglAbyssPolicy      repeat_mode)
{
  GeglRectangle abyss          = buffer->abyss;
  GeglRectangle abyss_factored = abyss;
  GeglRectangle roi_factored   = *roi;

  if (level)
    {
      const gint    factor         = 1 << level;
      const gint    x1 = buffer->shift_x + abyss.x;
      const gint    y1 = buffer->shift_y + abyss.y;
      const gint    x2 = buffer->shift_x + abyss.x + abyss.width;
      const gint    y2 = buffer->shift_y + abyss.y + abyss.height;

      abyss_factored.x      = (x1 + (x1 < 0 ? 1 - factor : 0)) / factor;
      abyss_factored.y      = (y1 + (y1 < 0 ? 1 - factor : 0)) / factor;
      abyss_factored.width  = (x2 + (x2 < 0 ? 0 : factor - 1)) / factor - abyss_factored.x;
      abyss_factored.height = (y2 + (y2 < 0 ? 0 : factor - 1)) / factor - abyss_factored.y;

      roi_factored.x       = (buffer->shift_x + roi_factored.x) / factor;
      roi_factored.y       = (buffer->shift_y + roi_factored.y) / factor;
      roi_factored.width  /= factor;
      roi_factored.height /= factor;
    }
  else
    {
      roi_factored.x += buffer->shift_x;
      roi_factored.y += buffer->shift_y;
      abyss_factored.x += buffer->shift_x;
      abyss_factored.y += buffer->shift_y;
    }

  if (rowstride == GEGL_AUTO_ROWSTRIDE)
    rowstride = roi_factored.width * babl_format_get_bytes_per_pixel (format);

  if (gegl_rectangle_contains (&abyss, roi))
    {
      gegl_buffer_iterate_read_simple (buffer, &roi_factored, buf, rowstride, format, level);
    }
  else if (repeat_mode == GEGL_ABYSS_NONE)
    {
      gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                            buf, rowstride, format, level, NULL,
                                            GEGL_ABYSS_NONE);
    }
  else if (repeat_mode == GEGL_ABYSS_WHITE)
    {
      guchar color[128];
      gfloat in_color[] = {1.0f, 1.0f, 1.0f, 1.0f};

      babl_process (babl_fish (babl_format ("RGBA float"), format),
                    in_color, color, 1);

      gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                            buf, rowstride, format, level, color,
                                            GEGL_ABYSS_WHITE);
    }
  else if (repeat_mode == GEGL_ABYSS_BLACK)
    {
      guchar color[128];
      gfloat  in_color[] = {0.0f, 0.0f, 0.0f, 1.0f};

      babl_process (babl_fish (babl_format ("RGBA float"), format),
                    in_color, color, 1);

      gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                            buf, rowstride, format, level, color,
                                            GEGL_ABYSS_BLACK);
    }
  else if (repeat_mode == GEGL_ABYSS_CLAMP)
    {
      if (abyss_factored.width == 0 || abyss_factored.height == 0)
        gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                              buf, rowstride, format, level, NULL,
                                              GEGL_ABYSS_NONE);
      else
        gegl_buffer_iterate_read_abyss_clamp (buffer, &roi_factored, &abyss_factored,
                                              buf, rowstride, format, level);
    }
  else
    {
      if (abyss_factored.width == 0 || abyss_factored.height == 0)
        gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                              buf, rowstride, format, level, NULL,
                                              GEGL_ABYSS_NONE);
      else
        gegl_buffer_iterate_read_abyss_loop (buffer, &roi_factored, &abyss_factored,
                                             buf, rowstride, format, level);
    }
}

void
gegl_buffer_set_unlocked (GeglBuffer          *buffer,
                          const GeglRectangle *rect,
                          gint                 level,
                          const Babl          *format,
                          const void          *src,
                          gint                 rowstride)
{
  _gegl_buffer_set_with_flags (buffer, rect, level, format, src, rowstride,
                               GEGL_BUFFER_SET_FLAG_NOTIFY);
}

void
gegl_buffer_set_unlocked_no_notify (GeglBuffer          *buffer,
                                    const GeglRectangle *rect,
                                    gint                 level,
                                    const Babl          *format,
                                    const void          *src,
                                    gint                 rowstride)
{
  _gegl_buffer_set_with_flags (buffer, rect, level, format, src, rowstride,
                               GEGL_BUFFER_SET_FLAG_FAST);
}


void
gegl_buffer_set (GeglBuffer          *buffer,
                 const GeglRectangle *rect,
                 gint                 level,
                 const Babl          *format,
                 const void          *src,
                 gint                 rowstride)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  if (format == NULL)
    format = buffer->soft_format;

  if (rect && (rect->width == 1 && rect->height == 1))
      _gegl_buffer_set_pixel (buffer, rect->x, rect->y, format, src,
                              GEGL_BUFFER_SET_FLAG_LOCK|GEGL_BUFFER_SET_FLAG_NOTIFY);
  else
    _gegl_buffer_set_with_flags (buffer, rect, level, format, src, rowstride,
                                 GEGL_BUFFER_SET_FLAG_LOCK|
                                 GEGL_BUFFER_SET_FLAG_NOTIFY);
}

/* Expand roi by scale so it uncludes all pixels needed
 * to satisfy a gegl_buffer_get() call at level 0.
 */
GeglRectangle
_gegl_get_required_for_scale (const Babl          *format,
                              const GeglRectangle *roi,
                              gdouble              scale)
{
  if (GEGL_FLOAT_EQUAL (scale, 1.0))
    return *roi;
  else
    {
      gint x1 = floorf (roi->x / scale + GEGL_SCALE_EPSILON);
      gint x2 = ceil ((roi->x + roi->width) / scale - GEGL_SCALE_EPSILON);
      gint y1 = floorf (roi->y / scale + GEGL_SCALE_EPSILON);
      gint y2 = ceil ((roi->y + roi->height) / scale - GEGL_SCALE_EPSILON);

      gint pad = (1.0 / scale > 1.0) ? ceil (1.0 / scale) : 1;

      if (scale < 1.0)
        {
          return *GEGL_RECTANGLE (x1 - pad,
                                  y1 - pad,
                                  x2 - x1 + 2 * pad,
                                  y2 - y1 + 2 * pad);
        }
      else
        {
          return *GEGL_RECTANGLE (x1,
                                  y1,
                                  x2 - x1,
                                  y2 - y1);
        }
      }
}

static inline void
_gegl_buffer_get_unlocked (GeglBuffer          *buffer,
                           gdouble              scale,
                           const GeglRectangle *rect,
                           const Babl          *format,
                           gpointer             dest_buf,
                           gint                 rowstride,
                           GeglAbyssPolicy      repeat_mode)
{
  g_return_if_fail (scale > 0.0);

  if (format == NULL)
    format = buffer->soft_format;

  if (scale == 1.0 &&
      rect &&
      rect->width == 1 &&
      rect->height == 1)
    {
      gegl_buffer_get_pixel (buffer, rect->x, rect->y, format, dest_buf,
                             repeat_mode);
      return;
    }

  if (gegl_cl_is_accelerated ())
    {
      gegl_buffer_cl_cache_flush (buffer, rect);
    }

  if (!rect && GEGL_FLOAT_EQUAL (scale, 1.0))
    {
      gegl_buffer_iterate_read_dispatch (buffer, &buffer->extent, dest_buf,
                                         rowstride, format, 0, repeat_mode);
      return;
    }

  g_return_if_fail (rect);
  if (rect->width == 0 ||
      rect->height == 0)
    {
      return;
    }
  if (GEGL_FLOAT_EQUAL (scale, 1.0))
    {
      gegl_buffer_iterate_read_dispatch (buffer, rect, dest_buf, rowstride,
                                         format, 0, repeat_mode);
      return;
    }
  else
    {
      GeglRectangle sample_rect;
      gint    level       = 0;
      gint    buf_width;
      gint    buf_height;
      gint    bpp         = babl_format_get_bytes_per_pixel (format);
      void   *sample_buf;
      gint    x1 = floorf (rect->x / scale + GEGL_SCALE_EPSILON);
      gint    x2 = ceil ((rect->x + rect->width) / scale - GEGL_SCALE_EPSILON);
      gint    y1 = floorf (rect->y / scale + GEGL_SCALE_EPSILON);
      gint    y2 = ceil ((rect->y + rect->height) / scale - GEGL_SCALE_EPSILON);
      gint    factor = 1;
      gint    offset = 0;

      while (scale <= 0.5)
        {
          x1 = 0 < x1 ? x1 / 2 : (x1 - 1) / 2;
          y1 = 0 < y1 ? y1 / 2 : (y1 - 1) / 2;
          x2 = 0 < x2 ? (x2 + 1) / 2 : x2 / 2;
          y2 = 0 < y2 ? (y2 + 1) / 2 : y2 / 2;
          scale  *= 2;
          factor *= 2;
          level++;
        }


      sample_rect.x      = factor * x1;
      sample_rect.y      = factor * y1;
      sample_rect.width  = factor * (x2 - x1);
      sample_rect.height = factor * (y2 - y1);

      if (scale == 1.0)
        {
          gegl_buffer_iterate_read_dispatch (buffer, &sample_rect,
                                             (guchar*)dest_buf, rowstride,
                                             format, level, repeat_mode);
          return;
        }

      if (rowstride == GEGL_AUTO_ROWSTRIDE)
        rowstride = rect->width * bpp;

      buf_width  = x2 - x1;
      buf_height = y2 - y1;

      if (scale <= 1.99)
        {
          buf_width  += 2;
          buf_height += 2;
          offset = (buf_width + 1) * bpp;
          sample_buf = g_malloc0 (buf_height * buf_width * bpp);
          
          gegl_buffer_iterate_read_dispatch (buffer, &sample_rect,
                                         (guchar*)sample_buf + offset,
                                         buf_width * bpp,
                                         format, level, repeat_mode);

          sample_rect.x      = x1 - 1;
          sample_rect.y      = y1 - 1;
          sample_rect.width  = x2 - x1 + 2;
          sample_rect.height = y2 - y1 + 2;

          gegl_resample_boxfilter (dest_buf,
                                   sample_buf,
                                   rect,
                                   &sample_rect,
                                   buf_width * bpp,
                                   scale,
                                   format,
                                   rowstride);
          g_free (sample_buf);
        }
      else
        {
          sample_buf = g_malloc (buf_height * buf_width * bpp);
          
          gegl_buffer_iterate_read_dispatch (buffer, &sample_rect,
                                         (guchar*)sample_buf,
                                         buf_width * bpp,
                                         format, level, repeat_mode);

          sample_rect.x      = x1;
          sample_rect.y      = y1;
          sample_rect.width  = x2 - x1;
          sample_rect.height = y2 - y1;

          gegl_resample_nearest (dest_buf,
                                 sample_buf,
                                 rect,
                                 &sample_rect,
                                 buf_width * bpp,
                                 scale,
                                 bpp,
                                 rowstride);
          g_free (sample_buf);
        }
    }
}

void
gegl_buffer_get_unlocked (GeglBuffer          *buffer,
                          gdouble              scale,
                          const GeglRectangle *rect,
                          const Babl          *format,
                          gpointer             dest_buf,
                          gint                 rowstride,
                          GeglAbyssPolicy      repeat_mode)
{
  return _gegl_buffer_get_unlocked (buffer, scale, rect, format, dest_buf, rowstride, repeat_mode);
}

void
gegl_buffer_get (GeglBuffer          *buffer,
                 const GeglRectangle *rect,
                 gdouble              scale,
                 const Babl          *format,
                 gpointer             dest_buf,
                 gint                 rowstride,
                 GeglAbyssPolicy      repeat_mode)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  gegl_buffer_lock (buffer);
  _gegl_buffer_get_unlocked (buffer, scale, rect, format, dest_buf, rowstride, repeat_mode);
  gegl_buffer_unlock (buffer);
}

void
gegl_buffer_sample_cleanup (GeglBuffer *buffer)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  if (buffer->sampler)
    {
      g_object_unref (buffer->sampler);
      buffer->sampler = NULL;
    }
}

static void
gegl_buffer_copy2 (GeglBuffer          *src,
                   const GeglRectangle *src_rect,
                   GeglAbyssPolicy      repeat_mode,
                   GeglBuffer          *dst,
                   const GeglRectangle *dst_rect)
{
  g_return_if_fail (GEGL_IS_BUFFER (src));
  g_return_if_fail (GEGL_IS_BUFFER (dst));

  if (!src_rect)
    {
      src_rect = gegl_buffer_get_extent (src);
    }

  if (!dst_rect)
    {
      dst_rect = src_rect;
    }

  if (src_rect->width == 0 || src_rect->height == 0)
    return;

    {
      GeglRectangle dest_rect_r = *dst_rect;
      GeglBufferIterator *i;
      gint offset_x = src_rect->x - dst_rect->x;
      gint offset_y = src_rect->y - dst_rect->y;

      dest_rect_r.width = src_rect->width;
      dest_rect_r.height = src_rect->height;

      i = gegl_buffer_iterator_new (dst, &dest_rect_r, 0, dst->soft_format,
                                    GEGL_ACCESS_WRITE, repeat_mode);
      while (gegl_buffer_iterator_next (i))
        {
          GeglRectangle src_rect = i->roi[0];
          src_rect.x += offset_x;
          src_rect.y += offset_y;
          gegl_buffer_iterate_read_dispatch (src, &src_rect, i->data[0], 0,
                                             dst->soft_format, 0, repeat_mode);
        }
    }
}

void
gegl_buffer_copy (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglAbyssPolicy      repeat_mode,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect)
{
  g_return_if_fail (GEGL_IS_BUFFER (src));
  g_return_if_fail (GEGL_IS_BUFFER (dst));

  if (!src_rect)
    {
      src_rect = gegl_buffer_get_extent (src);
    }

  if (!dst_rect)
    {
      dst_rect = src_rect;
    }


  if (src->soft_format == dst->soft_format &&
      src_rect->width >= src->tile_width &&
      src_rect->height >= src->tile_height &&
      src->tile_width == dst->tile_width  &&
      src->tile_height == dst->tile_height &&
      !g_object_get_data (G_OBJECT (dst), "is-linear") &&
      gegl_buffer_scan_compatible (src, src_rect->x, src_rect->y,
                                   dst, dst_rect->x, dst_rect->y))
    {
      GeglRectangle dest_rect_r = *dst_rect;

      gint tile_width = dst->tile_width;
      gint tile_height = dst->tile_height;

      dest_rect_r.width = src_rect->width;
      dest_rect_r.height = src_rect->height;
      dst_rect = &dest_rect_r;

      {
        GeglRectangle cow_rect = *dst_rect;

        /* adjust origin until we match the start of tile alignment */
        while ( (cow_rect.x + dst->shift_x) % tile_width)
          {
            cow_rect.x ++;
            cow_rect.width --;
          }
        while ( (cow_rect.y + dst->shift_y) % tile_height)
          {
            cow_rect.y ++;
            cow_rect.height --;
          }
        /* adjust size of rect to match multiple of tiles */

        cow_rect.width  = cow_rect.width  - (cow_rect.width  % tile_width);
        cow_rect.height = cow_rect.height - (cow_rect.height % tile_height);

        g_assert (cow_rect.width >= 0);
        g_assert (cow_rect.height >= 0);

        {
          GeglRectangle top, bottom, left, right;

          /* iterate over rectangle that can be cow copied, duplicating
           * one and one tile
           */
          {
            /* first we do a dumb copy,. but with fetched tiles */
            GeglTileHandlerCache *cache = dst->tile_storage->cache;
            gint dst_x, dst_y;

            for (dst_y = cow_rect.y + dst->shift_y; dst_y < cow_rect.y + dst->shift_y + cow_rect.height; dst_y += tile_height)
            for (dst_x = cow_rect.x + dst->shift_x; dst_x < cow_rect.x + dst->shift_x + cow_rect.width; dst_x += tile_width)
              {
                GeglTile *src_tile;
                GeglTile *dst_tile;
                gint src_x, src_y;
                gint stx, sty, dtx, dty;

                src_x = dst_x - (dst_rect->x - src_rect->x) + src->shift_x;
                src_y = dst_y - (dst_rect->y - src_rect->y) + src->shift_y;

                stx = gegl_tile_indice (src_x, tile_width);
                sty = gegl_tile_indice (src_y, tile_height);
                dtx = gegl_tile_indice (dst_x, tile_width);
                dty = gegl_tile_indice (dst_y, tile_height);

                src_tile = gegl_buffer_get_tile (src, stx, sty, 0);

                dst_tile = gegl_tile_dup (src_tile);
                dst_tile->tile_storage = dst->tile_storage;
                dst_tile->x = dtx;
                dst_tile->y = dty;
                dst_tile->z = 0;
                dst_tile->rev++;

                gegl_tile_handler_cache_insert (cache, dst_tile, dtx, dty, 0);

                gegl_tile_unref (src_tile);
                gegl_tile_unref (dst_tile);
              }
          }

          top = *dst_rect;
          top.height = (cow_rect.y - dst_rect->y);


          left = *dst_rect;
          left.y = cow_rect.y;
          left.height = cow_rect.height;
          left.width = (cow_rect.x - dst_rect->x);

          bottom = *dst_rect;
          bottom.y = (cow_rect.y + cow_rect.height);
          bottom.height = (dst_rect->y + dst_rect->height) -
                          (cow_rect.y  + cow_rect.height);

          if (bottom.height < 0)
            bottom.height = 0;

          right  =  *dst_rect;
          right.x = (cow_rect.x + cow_rect.width);
          right.width = (dst_rect->x + dst_rect->width) -
                          (cow_rect.x  + cow_rect.width);
          right.y = cow_rect.y;
          right.height = cow_rect.height;

          if (right.width < 0)
            right.width = 0;

          if (top.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (top.x-dst_rect->x),
                                             src_rect->y + (top.y-dst_rect->y),
                                 top.width, top.height),
                             repeat_mode, dst, &top);
          if (bottom.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (bottom.x-dst_rect->x),
                                             src_rect->y + (bottom.y-dst_rect->y),
                                 bottom.width, bottom.height),
                             repeat_mode, dst, &bottom);
          if (left.width)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (left.x-dst_rect->x),
                                             src_rect->y + (left.y-dst_rect->y),
                                 left.width, left.height),
                             repeat_mode, dst, &left);
          if (right.width && right.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (right.x-dst_rect->x),
                                             src_rect->y + (right.y-dst_rect->y),
                                 right.width, right.height),
                             repeat_mode, dst, &right);
        }
      }
    }
  else
    {
      gegl_buffer_copy2 (src, src_rect, repeat_mode, dst, dst_rect);
    }
}

static void
gegl_buffer_clear2 (GeglBuffer          *dst,
                    const GeglRectangle *dst_rect)
{
  GeglBufferIterator *i;
  gint                pxsize;

  g_return_if_fail (GEGL_IS_BUFFER (dst));

  if (!dst_rect)
    {
      dst_rect = gegl_buffer_get_extent (dst);
    }
  if (dst_rect->width == 0 ||
      dst_rect->height == 0)
    return;

  pxsize = babl_format_get_bytes_per_pixel (dst->soft_format);

  if (gegl_cl_is_accelerated ())
    gegl_buffer_cl_cache_invalidate (dst, dst_rect);

  /* FIXME: this can be even further optimized by special casing it so
   * that fully voided tiles are dropped.
   */
  i = gegl_buffer_iterator_new (dst, dst_rect, 0, dst->soft_format,
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (i))
    {
      memset (((guchar*)(i->data[0])), 0, i->length * pxsize);
    }
}

void
gegl_buffer_clear (GeglBuffer          *dst,
                   const GeglRectangle *dst_rect)
{
  g_return_if_fail (GEGL_IS_BUFFER (dst));

  if (!dst_rect)
    {
      dst_rect = gegl_buffer_get_extent (dst);
    }

#if 0
  /* cow for clearing is currently broken */
  if (!g_object_get_data (G_OBJECT (dst), "is-linear"))
    {
      gint tile_width = dst->tile_width;
      gint tile_height = dst->tile_height;

      {
        GeglRectangle cow_rect = *dst_rect;

        /* adjust origin until we match the start of tile alignment */
        while ( (cow_rect.x + dst->shift_x) % tile_width)
          {
            cow_rect.x ++;
            cow_rect.width --;
          }
        while ( (cow_rect.y + dst->shift_y) % tile_height)
          {
            cow_rect.y ++;
            cow_rect.height --;
          }
        /* adjust size of rect to match multiple of tiles */

        cow_rect.width  = cow_rect.width  - (cow_rect.width  % tile_width);
        cow_rect.height = cow_rect.height - (cow_rect.height % tile_height);


        g_assert (cow_rect.width >= 0);
        g_assert (cow_rect.height >= 0);

        {
          GeglRectangle top, bottom, left, right;

          /* iterate over rectangle that can be cow copied, duplicating
           * one and one tile
           */
          {
            /* first we do a dumb copy,. but with fetched tiles */

            gint dst_x, dst_y;

            for (dst_y = cow_rect.y + dst->shift_y; dst_y < cow_rect.y + dst->shift_y + cow_rect.height; dst_y += tile_height)
            for (dst_x = cow_rect.x + dst->shift_x; dst_x < cow_rect.x + dst->shift_x + cow_rect.width; dst_x += tile_width)
              {
                gint dtx, dty;

                dtx = gegl_tile_indice (dst_x, tile_width);
                dty = gegl_tile_indice (dst_y, tile_height);

                if(gegl_tile_source_void ((GeglTileSource*)dst, dtx, dty, 0));
              }
          }

          top = *dst_rect;
          top.height = (cow_rect.y - dst_rect->y);


          left = *dst_rect;
          left.y = cow_rect.y;
          left.height = cow_rect.height;
          left.width = (cow_rect.x - dst_rect->x);

          bottom = *dst_rect;
          bottom.y = (cow_rect.y + cow_rect.height);
          bottom.height = (dst_rect->y + dst_rect->height) -
                          (cow_rect.y  + cow_rect.height);

          if (bottom.height < 0)
            bottom.height = 0;

          right  =  *dst_rect;
          right.x = (cow_rect.x + cow_rect.width);
          right.width = (dst_rect->x + dst_rect->width) -
                          (cow_rect.x  + cow_rect.width);
          right.y = cow_rect.y;
          right.height = cow_rect.height;

          if (right.width < 0)
            right.width = 0;

          if (top.height)     gegl_buffer_clear2 (dst, &top);
          if (bottom.height)  gegl_buffer_clear2 (dst, &bottom);
          if (left.width)     gegl_buffer_clear2 (dst, &left);
          if (right.width)    gegl_buffer_clear2 (dst, &right);
        }
      }
    }
  else
#endif
    {
      gegl_buffer_clear2 (dst, dst_rect);
    }
}

void
gegl_buffer_set_pattern (GeglBuffer          *buffer,
                         const GeglRectangle *rect,
                         GeglBuffer          *pattern,
                         gint                 x_offset,
                         gint                 y_offset)
{
  const GeglRectangle *pattern_extent;
  const Babl          *buffer_format;
  GeglRectangle        roi;                  /* the rect if not NULL, else the whole buffer */
  GeglRectangle        pattern_data_extent;  /* pattern_extent clamped to rect */
  GeglRectangle        extended_data_extent; /* many patterns to avoid copying too small chunks of data */
  gint                 bpp;
  gint                 x, y;
  gint                 rowstride;
  gpointer             pattern_data;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (GEGL_IS_BUFFER (pattern));

  if (rect != NULL)
    roi = *rect;
  else
    roi = *gegl_buffer_get_extent (buffer);

  pattern_extent = gegl_buffer_get_extent (pattern);
  buffer_format  = gegl_buffer_get_format (buffer);

  pattern_data_extent.x      = - x_offset + roi.x;
  pattern_data_extent.y      = - y_offset + roi.y;
  pattern_data_extent.width  = MIN (pattern_extent->width,  roi.width);
  pattern_data_extent.height = MIN (pattern_extent->height, roi.height);

  /* Sanity */
  if (pattern_data_extent.width < 1 || pattern_data_extent.height < 1)
    return;

  bpp = babl_format_get_bytes_per_pixel (buffer_format);

  extended_data_extent = pattern_data_extent;

  /* Avoid gegl_buffer_set on too small chunks */
  while (extended_data_extent.width < buffer->tile_width * 2 &&
         extended_data_extent.width < roi.width)
    extended_data_extent.width += pattern_extent->width;

  while (extended_data_extent.height < buffer->tile_height * 2 &&
         extended_data_extent.height < roi.height)
    extended_data_extent.height += pattern_extent->height;

  /* XXX: Bad taste, the pattern needs to be small enough.
   * See Bug 712814 for an alternative malloc-free implementation */
  pattern_data = gegl_malloc (extended_data_extent.width *
                              extended_data_extent.height *
                              bpp);

  rowstride = extended_data_extent.width * bpp;

  /* only do babl conversions once on the whole pattern */
  gegl_buffer_get (pattern, &pattern_data_extent, 1.0,
                   buffer_format, pattern_data,
                   rowstride, GEGL_ABYSS_LOOP);

  /* fill the remaining space by duplicating the small pattern */
  for (y = 0; y < pattern_data_extent.height; y++)
    for (x = pattern_extent->width;
         x < extended_data_extent.width;
         x *= 2)
      {
        guchar *src  = ((guchar*) pattern_data) + y * rowstride;
        guchar *dst  = src + x * bpp;
        gint    size = bpp * MIN (extended_data_extent.width - x, x);
        memcpy (dst, src, size);
      }

  for (y = pattern_extent->height;
       y < extended_data_extent.height;
       y *= 2)
    {
      guchar *src  = ((guchar*) pattern_data);
      guchar *dst  = src + y * rowstride;
      gint    size = rowstride * MIN (extended_data_extent.height - y, y);
      memcpy (dst, src, size);
    }

  /* Now fill the acutal buffer */
  for (y = roi.y; y < roi.y + roi.height; y += extended_data_extent.height)
    for (x = roi.x; x < roi.x + roi.width; x += extended_data_extent.width)
      {
        GeglRectangle dest_rect = {x, y,
                                   extended_data_extent.width,
                                   extended_data_extent.height};

        gegl_rectangle_intersect (&dest_rect, &dest_rect, &roi);

        gegl_buffer_set (buffer, &dest_rect, 0, buffer_format,
                         pattern_data, rowstride);
      }

  gegl_free (pattern_data);
}

void
gegl_buffer_set_color (GeglBuffer          *dst,
                       const GeglRectangle *dst_rect,
                       GeglColor           *color)
{
  GeglBufferIterator *i;
  gchar               pixel[128];
  gint                bpp;

  g_return_if_fail (GEGL_IS_BUFFER (dst));
  g_return_if_fail (color);

  gegl_color_get_pixel (color, dst->soft_format, pixel);

  if (!dst_rect)
    {
      dst_rect = gegl_buffer_get_extent (dst);
    }
  if (dst_rect->width == 0 ||
      dst_rect->height == 0)
    return;

  bpp = babl_format_get_bytes_per_pixel (dst->soft_format);

  /* FIXME: this can be even further optimized by special casing it so
   * that fully filled tiles are shared.
   */
  i = gegl_buffer_iterator_new (dst, dst_rect, 0, dst->soft_format,
                                GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (i))
    {
      gegl_memset_pattern (i->data[0], pixel, bpp, i->length);
    }
}

GeglBuffer *
gegl_buffer_dup (GeglBuffer *buffer)
{
  GeglBuffer *new_buffer;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  new_buffer = gegl_buffer_new (gegl_buffer_get_extent (buffer), buffer->soft_format);
  gegl_buffer_copy (buffer, gegl_buffer_get_extent (buffer), GEGL_ABYSS_NONE,
                    new_buffer, gegl_buffer_get_extent (buffer));
  return new_buffer;
}

/*
 *  check whether iterations on two buffers starting from the given coordinates with
 *  the same width and height would be able to run parallell.
 */
gboolean gegl_buffer_scan_compatible (GeglBuffer *bufferA,
                                      gint        xA,
                                      gint        yA,
                                      GeglBuffer *bufferB,
                                      gint        xB,
                                      gint        yB)
{
  if (bufferA->tile_storage->tile_width !=
      bufferB->tile_storage->tile_width)
    return FALSE;
  if (bufferA->tile_storage->tile_height !=
      bufferB->tile_storage->tile_height)
    return FALSE;
  if ( (abs((bufferA->shift_x+xA) - (bufferB->shift_x+xB))
        % bufferA->tile_storage->tile_width) != 0)
    return FALSE;
  if ( (abs((bufferA->shift_y+yA) - (bufferB->shift_y+yB))
        % bufferA->tile_storage->tile_height) != 0)
    return FALSE;
  return TRUE;
}

