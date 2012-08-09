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
 */

#include "config.h"
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include "gegl.h"
#include "gegl/gegl-debug.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include "gegl-utils.h"
#include "gegl-sampler-nearest.h"
#include "gegl-sampler-linear.h"
#include "gegl-sampler-cubic.h"
#include "gegl-sampler-nohalo.h"
#include "gegl-sampler-lohalo.h"
#include "gegl-buffer-index.h"
#include "gegl-tile-backend.h"
#include "gegl-buffer-iterator.h"
#include "gegl-buffer-cl-cache.h"

#if 0
static inline void
gegl_buffer_pixel_set (GeglBuffer *buffer,
                       gint        x,
                       gint        y,
                       const Babl *format,
                       guchar     *buf)
{
  gint  tile_width  = buffer->tile_storage->tile_width;
  gint  tile_height = buffer->tile_storage->tile_width;
  gint  px_size     = gegl_buffer_px_size (buffer);
  gint  bpx_size    = babl_format_get_bytes_per_pixel (format);
  const Babl *fish  = NULL;

  gint  abyss_x_total  = buffer->abyss.x + buffer->abyss.width;
  gint  abyss_y_total  = buffer->abyss.y + buffer->abyss.height;
  gint  buffer_x       = buffer->extent.x;
  gint  buffer_y       = buffer->extent.y;
  gint  buffer_abyss_x = buffer->abyss.x;
  gint  buffer_abyss_y = buffer->abyss.y;

  if (format != buffer->soft_format)
    {
      fish = babl_fish (buffer->soft_format, format);
    }

  if (!(buffer_y + y >= buffer_abyss_y &&
        buffer_y + y < abyss_y_total &&
        buffer_x + x >= buffer_abyss_x &&
        buffer_x + x < abyss_x_total))
    { /* in abyss */
      return;
    }
  else
    {
      gint      tiledy = buffer_y + buffer->shift_y + y;
      gint      tiledx = buffer_x + buffer->shift_x + x;

      GeglTile *tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                                  gegl_tile_indice (tiledx, tile_width),
                                                  gegl_tile_indice (tiledy, tile_height),
                                                  0);

      if (tile)
        {
          gint    offsetx = gegl_tile_offset (tiledx, tile_width);
          gint    offsety = gegl_tile_offset (tiledy, tile_height);
          guchar *tp;

          gegl_tile_lock (tile);
          tp = gegl_tile_get_data (tile) +
            (offsety * tile_width + offsetx) * px_size;
          if (fish)
            babl_process (fish, buf, tp, 1);
          else
            memcpy (tp, buf, bpx_size);

          gegl_tile_unlock (tile);
          gegl_tile_unref (tile);
        }
    }
  return;
}
#endif

static gboolean
gegl_buffer_in_abyss (GeglBuffer *buffer,
                      gint        x,
                      gint        y )
{
  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  buffer_abyss_x = buffer->abyss.x + buffer_shift_x;
  gint  buffer_abyss_y = buffer->abyss.y + buffer_shift_y;
  gint  abyss_x_total  = buffer_abyss_x + buffer->abyss.width;
  gint  abyss_y_total  = buffer_abyss_y + buffer->abyss.height;

  gint tiledy = y + buffer_shift_y;
  gint tiledx = x + buffer_shift_x;

  return !(tiledy >= buffer_abyss_y &&
           tiledy <  abyss_y_total  &&
           tiledx >= buffer_abyss_x &&
           tiledx <  abyss_x_total);
}

static inline void
gegl_buffer_set_pixel (GeglBuffer *buffer,
                       gint        x,
                       gint        y,
                       const Babl *format,
                       gpointer    data)
{
  if (gegl_buffer_in_abyss( buffer, x, y))
    /* Nothing to set here */
    return;

  {
    guchar         *buf = data;
    gint    tile_width  = buffer->tile_storage->tile_width;
    gint    tile_height = buffer->tile_storage->tile_height;
    gint buffer_shift_x = buffer->shift_x;
    gint buffer_shift_y = buffer->shift_y;
    gint         tiledy = y + buffer_shift_y;
    gint         tiledx = x + buffer_shift_x;
    gint       indice_x = gegl_tile_indice (tiledx, tile_width);
    gint       indice_y = gegl_tile_indice (tiledy, tile_height);
    GeglTile      *tile = NULL;


    if (buffer->tile_storage->hot_tile &&
        buffer->tile_storage->hot_tile->x == indice_x &&
        buffer->tile_storage->hot_tile->y == indice_y)
      {
        tile = buffer->tile_storage->hot_tile;
      }
    else
      {
        _gegl_buffer_drop_hot_tile (buffer);
        tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                         indice_x, indice_y,
                                         0);
      }

    if (tile)
      {
        const Babl *fish  = NULL;
        gint      offsetx = gegl_tile_offset (tiledx, tile_width);
        gint      offsety = gegl_tile_offset (tiledy, tile_height);
        gint     bpx_size = babl_format_get_bytes_per_pixel (format);
        gint      px_size = babl_format_get_bytes_per_pixel (buffer->soft_format);
        guchar *tp;

        if (format != buffer->soft_format)
          {
            fish = babl_fish ((gpointer) format,
                              (gpointer) buffer->soft_format);
          }

        gegl_tile_lock (tile);

        tp = gegl_tile_get_data (tile) +
             (offsety * tile_width + offsetx) * px_size;
        if (fish)
          babl_process (fish, buf, tp, 1);
        else
          memcpy (tp, buf, bpx_size);

        gegl_tile_unlock (tile);
        buffer->tile_storage->hot_tile = tile;
      }
  }
}

static inline void
gegl_buffer_get_pixel (GeglBuffer     *buffer,
                       gint            x,
                       gint            y,
                       const Babl     *format,
                       gpointer        data,
                       GeglAbyssPolicy repeat_mode)
{
  guchar     *buf         = data;
  gint        bpx_size    = babl_format_get_bytes_per_pixel (format);

  if (gegl_buffer_in_abyss (buffer, x, y))
    { /* in abyss */
      const GeglRectangle *abyss;
      switch (repeat_mode)
      {
        case GEGL_ABYSS_CLAMP:
          abyss = gegl_buffer_get_abyss (buffer);
          x = CLAMP (x, abyss->x, abyss->x+abyss->width-1);
          y = CLAMP (y, abyss->y, abyss->y+abyss->height-1);
          break;

        case GEGL_ABYSS_LOOP:
          abyss = gegl_buffer_get_abyss (buffer);
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
          memset (buf, 0x00, bpx_size);
          return;
      }
    }

  {
    gint  tile_width     = buffer->tile_storage->tile_width;
    gint  tile_height    = buffer->tile_storage->tile_height;

    gint  buffer_shift_x = buffer->shift_x;
    gint  buffer_shift_y = buffer->shift_y;
    gint  px_size        = babl_format_get_bytes_per_pixel (buffer->soft_format);

    gint tiledy = y + buffer_shift_y;
    gint tiledx = x + buffer_shift_x;

    gint      indice_x = gegl_tile_indice (tiledx, tile_width);
    gint      indice_y = gegl_tile_indice (tiledy, tile_height);
    GeglTile *tile     = NULL;

    const Babl *fish   = NULL;

    if (format != buffer->soft_format)
      {
        fish = babl_fish ((gpointer) buffer->soft_format,
                          (gpointer) format);
      }

    if (buffer->tile_storage->hot_tile &&
        buffer->tile_storage->hot_tile->x == indice_x &&
        buffer->tile_storage->hot_tile->y == indice_y)
      {
        tile = buffer->tile_storage->hot_tile;
      }
    else
      {
        _gegl_buffer_drop_hot_tile (buffer);
        tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                          indice_x, indice_y,
                                          0);
      }

    if (tile)
      {
        gint    offsetx = gegl_tile_offset (tiledx, tile_width);
        gint    offsety = gegl_tile_offset (tiledy, tile_height);
        guchar *tp      = gegl_tile_get_data (tile) +
                          (offsety * tile_width + offsetx) * px_size;
        if (fish)
          babl_process (fish, tp, buf, 1);
        else
          memcpy (buf, tp, px_size);

        /*gegl_tile_unref (tile);*/
        buffer->tile_storage->hot_tile = tile;
      }
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

  if ((GeglBufferHeader*)(backend->priv->header))
    {
      GeglBufferHeader* header = backend->priv->header;
      header->x = buffer->extent.x;
      header->y = buffer->extent.y;
      header->width =buffer->extent.width;
      header->height =buffer->extent.height;
    }

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
          gint      lskip, rskip, pixels, row;
          guchar   *bp, *tile_base, *tp;
          GeglTile *tile;

          bp = buf + bufy * buf_stride + bufx * bpx_size;

          if (width + offsetx - bufx < tile_width)
            pixels = (width + offsetx - bufx) - offsetx;
          else
            pixels = tile_width - offsetx;

          tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                            gegl_tile_indice (tiledx, tile_width),
                                            gegl_tile_indice (tiledy, tile_height),
                                            level);

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

static inline void
gegl_buffer_iterate_read_abyss_none (GeglBuffer          *buffer,
                                     const GeglRectangle *roi,
                                     const GeglRectangle *abyss,
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

  gint buffer_abyss_x = abyss->x;
  gint buffer_abyss_y = abyss->y;
  gint abyss_x_total  = buffer_abyss_x + abyss->width;
  gint abyss_y_total  = buffer_abyss_y + abyss->height;

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

      if (!(buffer_y + bufy + (tile_height) >= buffer_abyss_y &&
            buffer_y + bufy < abyss_y_total))
        { /* entire row of tiles is in abyss */
          gint    row;
          gint    y  = bufy;
          guchar *bp = buf + bufy * buf_stride;

          for (row = offsety;
               row < tile_height && y < height;
               row++, y++)
            {
              memset (bp, 0x00, width * bpx_size);
              bp += buf_stride;
            }
        }
      else

        while (bufx < width)
          {
            gint    tiledx  = buffer_x + bufx;
            gint    offsetx = gegl_tile_offset (tiledx, tile_width);
            gint    pixels;
            guchar *bp;

            bp = buf + bufy * buf_stride + bufx * bpx_size;

            if (width + offsetx - bufx < tile_width)
              pixels = width - bufx;
            else
              pixels = tile_width - offsetx;

            if (!(buffer_x + bufx + tile_width >= buffer_abyss_x &&
                  buffer_x + bufx < abyss_x_total))
              { /* entire tile is in abyss */
                gint row;
                gint y = bufy;

                for (row = offsety;
                     row < tile_height && y < height;
                     row++, y++)
                  {
                    memset (bp, 0x00, pixels * bpx_size);
                    bp += buf_stride;
                  }
              }
            else
              {
                guchar   *tile_base, *tp;
                gint      row, y, lskip, rskip;
                GeglTile *tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                                            gegl_tile_indice (tiledx, tile_width),
                                                            gegl_tile_indice (tiledy, tile_height),
                                                            level);

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

                tile_base = gegl_tile_get_data (tile);
                tp        = ((guchar *) tile_base) + (offsety * tile_width + offsetx) * px_size;

                y = bufy;
                for (row = offsety;
                     row < tile_height && y < height;
                     row++, y++)
                  {
                    if (buffer_y + y >= buffer_abyss_y &&
                        buffer_y + y < abyss_y_total)
                      {
                        if (fish)
                          babl_process (fish, tp, bp, pixels);
                        else
                          memcpy (bp, tp, pixels * px_size);
                      }
                    else
                      {
                        /* entire row in abyss */
                        memset (bp, 0x00, pixels * bpx_size);
                      }

                    /* left hand zeroing of abyss in tile */
                    if (lskip)
                      memset (bp, 0x00, bpx_size * lskip);

                    /* right side zeroing of abyss in tile */
                    if (rskip)
                      memset (bp + (pixels - rskip) * bpx_size, 0x00, bpx_size * rskip);

                    tp += tile_stride;
                    bp += buf_stride;
                  }
                gegl_tile_unref (tile);
              }
            bufx += (tile_width - offsetx);
          }
      bufy += (tile_height - offsety);
    }
}

static inline void
gegl_buffer_iterate_read_abyss_color (GeglBuffer          *buffer,
                                      const GeglRectangle *roi,
                                      const GeglRectangle *abyss,
                                      guchar              *buf,
                                      gint                 buf_stride,
                                      const Babl          *format,
                                      gint                 level,
                                      guchar              *color)
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

  gint buffer_abyss_x = abyss->x;
  gint buffer_abyss_y = abyss->y;
  gint abyss_x_total  = buffer_abyss_x + abyss->width;
  gint abyss_y_total  = buffer_abyss_y + abyss->height;

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
      gint i;

      if (!(buffer_y + bufy + (tile_height) >= buffer_abyss_y &&
            buffer_y + bufy < abyss_y_total))
        { /* entire row of tiles is in abyss */
          gint    row;
          gint    y  = bufy;
          guchar *bp = buf + bufy * buf_stride;

          for (row = offsety;
               row < tile_height && y < height;
               row++, y++)
            {
              for (i = 0; i < width * bpx_size; i += bpx_size)
                memcpy (bp + i, color, bpx_size);
              bp += buf_stride;
            }
        }
      else
        while (bufx < width)
          {
            gint    tiledx  = buffer_x + bufx;
            gint    offsetx = gegl_tile_offset (tiledx, tile_width);
            gint    pixels;
            guchar *bp;

            bp = buf + bufy * buf_stride + bufx * bpx_size;

            if (width + offsetx - bufx < tile_width)
              pixels = width - bufx;
            else
              pixels = tile_width - offsetx;

            if (!(buffer_x + bufx + tile_width >= buffer_abyss_x &&
                  buffer_x + bufx < abyss_x_total))
              { /* entire tile is in abyss */
                gint row;
                gint y = bufy;

                for (row = offsety;
                     row < tile_height && y < height;
                     row++, y++)
                  {
                    for (i = 0; i < pixels * bpx_size; i += bpx_size)
                      memcpy (bp + i, color, bpx_size);
                    bp += buf_stride;
                  }
              }
            else
              {
                guchar   *tile_base, *tp;
                gint      row, y, lskip, rskip;
                GeglTile *tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                                            gegl_tile_indice (tiledx, tile_width),
                                                            gegl_tile_indice (tiledy, tile_height),
                                                            level);

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

                tile_base = gegl_tile_get_data (tile);
                tp        = ((guchar *) tile_base) + (offsety * tile_width + offsetx) * px_size;

                y = bufy;
                for (row = offsety;
                     row < tile_height && y < height;
                     row++, y++)
                  {
                    if (buffer_y + y >= buffer_abyss_y &&
                        buffer_y + y < abyss_y_total)
                      {
                        if (fish)
                          babl_process (fish, tp, bp, pixels);
                        else
                          memcpy (bp, tp, pixels * px_size);
                      }
                    else
                      {
                        /* entire row in abyss */
                        for (i = 0; i < pixels * bpx_size; i += bpx_size)
                          memcpy (bp + i, color, bpx_size);
                      }

                    /* left hand zeroing of abyss in tile */
                    if (lskip)
                      {
                        for (i = 0; i < lskip * bpx_size; i += bpx_size)
                          memcpy (bp + i, color, bpx_size);
                      }

                    /* right side zeroing of abyss in tile */
                    if (rskip)
                      {
                        guchar *bp_ = bp + (pixels - rskip) * bpx_size;
                        for (i = 0; i < rskip * bpx_size; i += bpx_size)
                          memcpy (bp_ + i, color, bpx_size);
                      }

                    tp += tile_stride;
                    bp += buf_stride;
                  }
                gegl_tile_unref (tile);
              }
            bufx += (tile_width - offsetx);
          }
      bufy += (tile_height - offsety);
    }
}

static inline void
gegl_buffer_iterate_read_abyss_clamp (GeglBuffer          *buffer,
                                      const GeglRectangle *roi,
                                      const GeglRectangle *abyss,
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

  gint buffer_abyss_x = abyss->x;
  gint buffer_abyss_y = abyss->y;
  gint abyss_x_total  = buffer_abyss_x + abyss->width;
  gint abyss_y_total  = buffer_abyss_y + abyss->height;

  const Babl *fish;

  if (format == buffer->soft_format)
    fish = NULL;
  else
    fish = babl_fish ((gpointer) buffer->soft_format,
                      (gpointer) format);

  while (bufy < height)
    {
      gint     tiledy       = CLAMP (buffer_y + bufy, buffer_abyss_y, abyss_y_total - 1);
      gint     offsety      = gegl_tile_offset (tiledy, tile_height);
      gint     bufx         = 0;
      gboolean row_in_abyss = !(buffer_y + bufy + (tile_height) >= buffer_abyss_y &&
                                buffer_y + bufy < abyss_y_total);

      while (bufx < width)
        {
          gint      tiledx  = CLAMP (buffer_x + bufx, buffer_abyss_x, abyss_x_total - 1);
          gint      offsetx = gegl_tile_offset (tiledx, tile_width);
          gint      row, y, pixels, lskip, rskip;
          guchar   *bp, *tile_base, *tp;
          GeglTile *tile;

          bp = buf + bufy * buf_stride + bufx * bpx_size;

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
          if (tiledx != buffer_x + bufx)
            { /* x was clamped. Copy a single color since x remains clamped in
               this iteration. */
              guchar color[128];
              gint   i;

              /* gap between current column and left side of abyss rect */
              lskip = (buffer_abyss_x) - (buffer_x + bufx);
              /* gap between current column and end of roi */
              rskip = width - bufx;
              pixels = (lskip > 0 && lskip < width) ? lskip : rskip;

              if (row_in_abyss)
                { /* y remains clamped in this iteration so don't change the color */
                  if (fish)
                    babl_process (fish, tp, color, 1);
                  else
                    memcpy (color, tp, px_size);

                  for (row = 0;
                       row < tile_height && y < height;
                       row++, y++)
                    {
                      for (i = 0; i < pixels * bpx_size; i += bpx_size)
                        memcpy (bp + i, color, bpx_size);
                      bp += buf_stride;
                    }
                }
              else
                {
                  for (row = offsety;
                       row < tile_height && y < height;
                       row++, y++)
                    {
                      if (fish)
                        babl_process (fish, tp, color, 1);
                      else
                        memcpy (color, tp, px_size);

                      for (i = 0; i < pixels * bpx_size; i += bpx_size)
                        memcpy (bp + i, color, bpx_size);

                      if (buffer_y + y >= buffer_abyss_y &&
                          buffer_y + y < abyss_y_total - 1)
                        tp += tile_stride;
                      bp += buf_stride;
                    }
                }
            }
          else
            {
              if (width + offsetx - bufx < tile_width)
                pixels = width - bufx;
              else
                pixels = tile_width - offsetx;

              /* gap between current column and right side of abyss rect */
              rskip = abyss_x_total - (buffer_x + bufx);
              if (rskip > 0 && rskip < pixels)
                pixels = rskip;

              for (row = (row_in_abyss) ? 0 : offsety;
                   row < tile_height && y < height;
                   row++, y++)
                {
                  if (fish)
                    babl_process (fish, tp, bp, pixels);
                  else
                    memcpy (bp, tp, pixels * px_size);

                  if (buffer_y + y >= buffer_abyss_y &&
                      buffer_y + y < abyss_y_total - 1)
                    tp += tile_stride;
                  bp += buf_stride;
                }
            }
          gegl_tile_unref (tile);
          bufx += pixels;
        }
      if (row_in_abyss)
        bufy += tile_height;
      else
        bufy += (tile_height - offsety);
    }
}

static inline void
gegl_buffer_iterate_read_abyss_loop (GeglBuffer          *buffer,
                                     const GeglRectangle *roi,
                                     const GeglRectangle *abyss,
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

  gint buffer_abyss_x = abyss->x;
  gint buffer_abyss_y = abyss->y;
  gint abyss_x_total  = buffer_abyss_x + abyss->width;
  gint abyss_y_total  = buffer_abyss_y + abyss->height;

  const Babl *fish;

  if (format == buffer->soft_format)
    fish = NULL;
  else
    fish = babl_fish ((gpointer) buffer->soft_format,
                      (gpointer) format);

  while (bufy < height)
    {
      gint tiledy  = buffer_abyss_y +
        GEGL_REMAINDER (buffer_y + bufy - buffer_abyss_y, abyss->height);
      gint offsety = gegl_tile_offset (tiledy, tile_height);
      gint bufx    = 0;
      gint rows, topskip, bottomskip;

      if (height + offsety - bufy < tile_height)
        rows = height - bufy;
      else
        rows = tile_height - offsety;

      /* gap between current row and top of abyss rect */
      topskip = buffer_abyss_y - tiledy;
      /* gap between current row and bottom of abyss rect */
      bottomskip = abyss_y_total - tiledy;

      if (topskip > 0 && topskip < rows)
        rows = topskip;
      else if (bottomskip > 0 && bottomskip < rows)
        rows = bottomskip;

      while (bufx < width)
        {
          gint      tiledx  = buffer_abyss_x +
            GEGL_REMAINDER (buffer_x + bufx - buffer_abyss_x, abyss->width);
          gint      offsetx = gegl_tile_offset (tiledx, tile_width);
          guchar   *bp, *tile_base, *tp;
          gint      pixels, row, y, lskip, rskip;
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

          /* gap between current column and left side of abyss rect */
          lskip = buffer_abyss_x - tiledx;
          /* gap between current column and right side of abyss rect */
          rskip = abyss_x_total - tiledx;

          if (lskip > 0 && lskip < pixels)
            pixels = lskip;
          else if (rskip > 0 && rskip < pixels)
            pixels = rskip;

          if (!tile)
            {
              g_warning ("didn't get tile, trying to continue");
              bufx += pixels;
              continue;
            }

          tile_base = gegl_tile_get_data (tile);
          tp        = ((guchar *) tile_base) + (offsety * tile_width + offsetx) * px_size;

          y = bufy;
          for (row = 0;
               row < rows && y < height;
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
          bufx += pixels;
        }
      bufy += rows;
    }
}

static inline void
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

  if (rowstride == GEGL_AUTO_ROWSTRIDE)
    rowstride = roi_factored.width * babl_format_get_bytes_per_pixel (format);

  if (gegl_rectangle_contains (&abyss, roi))
    {
      gegl_buffer_iterate_read_simple (buffer, &roi_factored, buf, rowstride, format, level);
    }
  else if (repeat_mode == GEGL_ABYSS_NONE)
    {
      gegl_buffer_iterate_read_abyss_none (buffer, &roi_factored, &abyss_factored,
                                           buf, rowstride, format, level);
    }
  else if (repeat_mode == GEGL_ABYSS_WHITE ||
           repeat_mode == GEGL_ABYSS_BLACK)
    {
      gfloat color_a[4] = {0.0, 0.0, 0.0, 1.0};
      guchar color[128];
      gint   i;

      if (repeat_mode == GEGL_ABYSS_WHITE)
        for (i = 0; i < 3; i++)
          color_a[i] = 1.0;
      babl_process (babl_fish (babl_format ("RGBA float"), format),
                    color_a, color, 1);

      gegl_buffer_iterate_read_abyss_color (buffer, &roi_factored, &abyss_factored,
                                            buf, rowstride, format, level, color);
    }
  else if (repeat_mode == GEGL_ABYSS_CLAMP)
    {
      gegl_buffer_iterate_read_abyss_clamp (buffer, &roi_factored, &abyss_factored,
                                            buf, rowstride, format, level);
    }
  else
    {
      gegl_buffer_iterate_read_abyss_loop (buffer, &roi_factored, &abyss_factored,
                                           buf, rowstride, format, level);
    }
}

void
gegl_buffer_set_unlocked (GeglBuffer          *buffer,
                          const GeglRectangle *rect,
                          const Babl          *format,
                          const void          *src,
                          gint                 rowstride)
{
    gegl_buffer_set_unlocked_no_notify(buffer, rect, format, src, rowstride);
    gegl_buffer_emit_changed_signal(buffer, rect);
}


void
gegl_buffer_set_unlocked_no_notify (GeglBuffer          *buffer,
                          const GeglRectangle *rect,
                          const Babl          *format,
                          const void          *src,
                          gint                 rowstride)
{
  if (format == NULL)
    format = buffer->soft_format;

  if (gegl_cl_is_accelerated ())
    {
      gegl_buffer_cl_cache_flush (buffer, rect);
    }

#if 0 /* XXX: not thread safe */
  if (rect && rect->width == 1 && rect->height == 1) /* fast path */
    {
      gegl_buffer_set_pixel (buffer, rect->x, rect->y, format, src);
    }
  else
#endif
    gegl_buffer_iterate_write (buffer, rect, (void *) src, rowstride, format, 0);

  if (gegl_buffer_is_shared(buffer))
    {
      gegl_buffer_flush (buffer);
    }
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

  gegl_buffer_lock (buffer);
  gegl_buffer_set_unlocked (buffer, rect, format, src, rowstride);
  gegl_buffer_unlock (buffer);
}

#ifdef BOXFILTER_FLOAT
typedef float            T;
#define double2T(a)      (a)
#define projectionT      babl_type ("float")
#define T_components     babl_format_get_n_components (format)
#else
typedef guchar           T;
#define double2T(a)      (lrint (a))
#define projectionT      babl_type ("u8")
#define T_components     bpp
#endif
#define EPSILON 1.e-6

static void
resample_nearest (guchar              *dst,
                  const guchar        *src,
                  const GeglRectangle *dst_rect,
                  const GeglRectangle *src_rect,
                  const gint           src_stride,
                  const gdouble        scale,
                  const gint           bpp,
                  const gint           dst_stride)
{
  int i, j;

  for (i = 0; i < dst_rect->height; i++)
    {
      const gdouble sy = (dst_rect->y + .5 + i) / scale - src_rect->y;
      const gint    ii = floor (sy + EPSILON);

      for (j = 0; j < dst_rect->width; j++)
        {
          const gdouble sx = (dst_rect->x + .5 + j) / scale - src_rect->x;
          const gint    jj = floor (sx + EPSILON);

          memcpy (&dst[i * dst_stride + j * bpp],
                  &src[ii * src_stride + jj * bpp],
                  bpp);
        }
    }
}

static inline void
box_filter (gdouble   left_weight,
            gdouble   center_weight,
            gdouble   right_weight,
            gdouble   top_weight,
            gdouble   middle_weight,
            gdouble   bottom_weight,
            const T **src,   /* the 9 surrounding source pixels */
            T        *dest,
            gint      components)
{
   const gdouble lt = left_weight * top_weight;
   const gdouble lm = left_weight * middle_weight;
   const gdouble lb = left_weight * bottom_weight;
   const gdouble ct = center_weight * top_weight;
   const gdouble cm = center_weight * middle_weight;
   const gdouble cb = center_weight * bottom_weight;
   const gdouble rt = right_weight * top_weight;
   const gdouble rm = right_weight * middle_weight;
   const gdouble rb = right_weight * bottom_weight;

#define docomponent(i) \
      dest[i] = double2T (src[0][i] * lt + src[3][i] * lm + src[6][i] * lb + \
                          src[1][i] * ct + src[4][i] * cm + src[7][i] * cb + \
                          src[2][i] * rt + src[5][i] * rm + src[8][i] * rb)
  switch (components)
    {
      case 5: docomponent(4);
      case 4: docomponent(3);
      case 3: docomponent(2);
      case 2: docomponent(1);
      case 1: docomponent(0);
    }
#undef docomponent
}

static void
resample_boxfilter_T  (guchar              *dest_buf,
                       const guchar        *source_buf,
                       const GeglRectangle *dst_rect,
                       const GeglRectangle *src_rect,
                       const gint           s_rowstride,
                       const gdouble        scale,
                       const gint           components,
                       const gint           d_rowstride)
{
  gdouble  left_weight, center_weight, right_weight;
  gdouble  top_weight, middle_weight, bottom_weight;
  const T *src[9];
  gint     x, y;

  for (y = 0; y < dst_rect->height; y++)
    {
      const gdouble  sy = (dst_rect->y + y + .5) / scale - src_rect->y;
      const gint     ii = floor (sy);
      T             *dst = (T*)(dest_buf + y * d_rowstride);
      const guchar  *src_base = source_buf + ii * s_rowstride;

      top_weight    = MAX (0., .5 - scale * (sy - ii));
      bottom_weight = MAX (0., .5 - scale * ((ii + 1 ) - sy));
      middle_weight = 1. - top_weight - bottom_weight;

      for (x = 0; x < dst_rect->width; x++)
        {
          const gdouble  sx = (dst_rect->x + x + .5) / scale - src_rect->x;
          const gint     jj = floor (sx);

          left_weight   = MAX (0., .5 - scale * (sx - jj));
          right_weight  = MAX (0., .5 - scale * ((jj + 1) - sx));
          center_weight = 1. - left_weight - right_weight;

          src[4] = (const T*)src_base + jj * components;
          src[1] = (const T*)(src_base - s_rowstride) + jj * components;
          src[7] = (const T*)(src_base + s_rowstride) + jj * components;

          src[2] = src[1] + components;
          src[5] = src[4] + components;
          src[8] = src[7] + components;

          src[0] = src[1] - components;
          src[3] = src[4] - components;
          src[6] = src[7] - components;

          box_filter (left_weight,
                      center_weight,
                      right_weight,
                      top_weight,
                      middle_weight,
                      bottom_weight,
                      src,   /* the 9 surrounding source pixels */
                      dst,
                      components);

          dst += components;
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
  g_return_if_fail (scale > 0.0);

  if (format == NULL)
    format = buffer->soft_format;

#if 0
  /* not thread-safe */
  if (scale == 1.0 &&
      rect &&
      rect->width == 1 &&
      rect->height == 1)  /* fast path */
    {
      gegl_buffer_get_pixel (buffer, rect->x, rect->y, format, dest_buf, repeat_mode);
      return;
    }
#endif

  if (gegl_cl_is_accelerated ())
    {
      gegl_buffer_cl_cache_flush (buffer, rect);
    }

  if (!rect && scale == 1.0)
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
      gint          level       = 0;
      gint          buf_width;
      gint          buf_height;
      gint          bpp         = babl_format_get_bytes_per_pixel (format);
      GeglRectangle sample_rect;
      void         *sample_buf;
      gint          x1 = floor (rect->x / scale + EPSILON);
      gint          x2 = ceil ((rect->x + rect->width) / scale - EPSILON);
      gint          y1 = floor (rect->y / scale + EPSILON);
      gint          y2 = ceil ((rect->y + rect->height) / scale - EPSILON);
      gint          factor = 1;
      gint          stride;
      gint          offset = 0;

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

      buf_width  = x2 - x1;
      buf_height = y2 - y1;

      /* ensure we always have some data to sample from */
      if (scale != 1.0 && scale < 1.99 &&
          babl_format_get_type (format, 0) == projectionT)
        {
          buf_width  += 2;
          buf_height += 2;
          offset = (buf_width + 1) * bpp;
        }

      sample_rect.x      = factor * x1;
      sample_rect.y      = factor * y1;
      sample_rect.width  = factor * (x2 - x1);
      sample_rect.height = factor * (y2 - y1);

      sample_buf = scale == 1.0 ? dest_buf : g_malloc0 (buf_height * buf_width * bpp);
      stride     = scale == 1.0 ? rowstride : buf_width * bpp;

      gegl_buffer_iterate_read_dispatch (buffer, &sample_rect, (guchar*)sample_buf + offset, stride,
                                         format, level, repeat_mode);

      if (scale == 1.0)
        return;

      if (rowstride == GEGL_AUTO_ROWSTRIDE)
        rowstride = rect->width * bpp;

      if (scale <= 1.99 && babl_format_get_type (format, 0) == projectionT)
        { /* do box-filter resampling if we're 8bit (which projections are) */
          sample_rect.x      = x1 - 1;
          sample_rect.y      = y1 - 1;
          sample_rect.width  = x2 - x1 + 2;
          sample_rect.height = y2 - y1 + 2;

          resample_boxfilter_T  (dest_buf,
                                 sample_buf,
                                 rect,
                                 &sample_rect,
                                 buf_width * bpp,
                                 scale,
                                 T_components,
                                 rowstride);
        }
      else
        {
          sample_rect.x      = x1;
          sample_rect.y      = y1;
          sample_rect.width  = x2 - x1;
          sample_rect.height = y2 - y1;

          resample_nearest (dest_buf,
                            sample_buf,
                            rect,
                            &sample_rect,
                            buf_width * bpp,
                            scale,
                            bpp,
                            rowstride);
        }
      g_free (sample_buf);
    }
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
  gegl_buffer_get_unlocked (buffer, scale, rect, format, dest_buf, rowstride, repeat_mode);
}

const GeglRectangle *
gegl_buffer_get_abyss (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return &buffer->abyss;
}

GType
gegl_sampler_gtype_from_enum (GeglSamplerType sampler_type);

void
gegl_buffer_sample (GeglBuffer       *buffer,
                    gdouble           x,
                    gdouble           y,
                    GeglMatrix2      *scale,
                    gpointer          dest,
                    const Babl       *format,
                    GeglSamplerType   sampler_type,
                    GeglAbyssPolicy   repeat_mode)
{
  GType desired_type;
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

/*#define USE_WORKING_SHORTCUT*/
#ifdef USE_WORKING_SHORTCUT
  gegl_buffer_get_pixel (buffer, x, y, format, dest, repeat_mode);
  return;
#endif

  desired_type = gegl_sampler_gtype_from_enum (sampler_type);

  if (!format)
    format = buffer->soft_format;

  if (format == buffer->soft_format &&
      sampler_type == GEGL_SAMPLER_NEAREST)
    {
      /* XXX: not thread safe */
      gegl_buffer_get_pixel (buffer, x, y, format, dest, repeat_mode);
      return;
    }
  /* unset the cached sampler if it dosn't match the needs */
  if (buffer->sampler != NULL &&
     (!G_TYPE_CHECK_INSTANCE_TYPE (buffer->sampler, desired_type) ||
       buffer->sampler_format != format
      ))
    {
      g_object_unref (buffer->sampler);
      buffer->sampler = NULL;
    }

  /* look up appropriate sampler,. */
  if (buffer->sampler == NULL)
    {
      buffer->sampler = g_object_new (desired_type,
                                      "buffer", buffer,
                                      "format", format,
                                      NULL);
      buffer->sampler_format = format;
      gegl_sampler_prepare (buffer->sampler);
    }

  gegl_sampler_get (buffer->sampler, x, y, scale, dest, repeat_mode);
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
                   GeglBuffer          *dst,
                   const GeglRectangle *dst_rect)
{
  const Babl *fish;
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

  fish = babl_fish (src->soft_format, dst->soft_format);

    {
      GeglRectangle dest_rect_r = *dst_rect;
      GeglBufferIterator *i;
      gint read;

      dest_rect_r.width = src_rect->width;
      dest_rect_r.height = src_rect->height;

      i = gegl_buffer_iterator_new (dst, &dest_rect_r, 0, dst->soft_format,
                                    GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
      read = gegl_buffer_iterator_add (i, src, src_rect, 0, src->soft_format,
                                       GEGL_BUFFER_READ, GEGL_ABYSS_NONE);
      while (gegl_buffer_iterator_next (i))
        babl_process (fish, i->data[read], i->data[0], i->length);
    }
}

void
gegl_buffer_copy (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
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

            gint dst_x, dst_y;
            GeglTileHandlerChain   *storage;
            GeglTileHandlerCache   *cache;

            storage = GEGL_TILE_HANDLER_CHAIN (dst->tile_storage);
            cache = GEGL_TILE_HANDLER_CACHE (gegl_tile_handler_chain_get_first (storage, GEGL_TYPE_TILE_HANDLER_CACHE));


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

#if 1
                src_tile = gegl_tile_source_get_tile ((GeglTileSource*)(src),
                                                      stx, sty, 0);

                dst_tile = gegl_tile_dup (src_tile);
                dst_tile->tile_storage = (void*)storage;

                /* XXX: this call should only be neccesary as long as GIMP
                 *      is dropping tile caches behind our back
                 */
                if(gegl_tile_source_set_tile ((GeglTileSource*)dst, dtx, dty, 0,
                                           dst_tile));
                gegl_tile_handler_cache_insert (cache, dst_tile, dtx, dty, 0);

                gegl_tile_unref (src_tile);
                gegl_tile_unref (dst_tile);
#else
                src_tile = gegl_tile_source_get_tile (
                  (GeglTileSource*)(src), stx, sty, 0);
                dst_tile = gegl_tile_source_get_tile (
                  (GeglTileSource*)(dst), dtx, dty, 0);
                gegl_tile_lock (dst_tile);
                g_assert (src_tile->size == dst_tile->size);

                memcpy (dst_tile->data, src_tile->data, src_tile->size);

                gegl_tile_unlock (dst_tile);
                gegl_tile_unref (dst_tile);
                gegl_tile_unref (src_tile);
#endif
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
                             dst, &top);
          if (bottom.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (bottom.x-dst_rect->x),
                                             src_rect->y + (bottom.y-dst_rect->y),
                                 bottom.width, bottom.height),
                             dst, &bottom);
          if (left.width)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (left.x-dst_rect->x),
                                             src_rect->y + (left.y-dst_rect->y),
                                 left.width, left.height),
                             dst, &left);
          if (right.width && right.height)
          gegl_buffer_copy2 (src,
                             GEGL_RECTANGLE (src_rect->x + (right.x-dst_rect->x),
                                             src_rect->y + (right.y-dst_rect->y),
                                 right.width, right.height),
                             dst, &right);
        }
      }
    }
  else
    {
      gegl_buffer_copy2 (src, src_rect, dst, dst_rect);
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
                                GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
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

  goto nocow; // cow for clearing is currently broken, go to nocow case
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
    {
nocow:
      gegl_buffer_clear2 (dst, dst_rect);
    }
}

void
gegl_buffer_set_pattern (GeglBuffer          *buffer,
                         const GeglRectangle *rect, /* XXX:should be respected*/
                         GeglBuffer          *pattern,
                         gdouble              x_offset,
                         gdouble              y_offset)
{
  GeglRectangle src_rect = {0,}, dst_rect;
  int pat_width, pat_height;
  int cols, rows;
  int col, row;
  int width, height;

  pat_width  = gegl_buffer_get_width (pattern);
  pat_height = gegl_buffer_get_height (pattern);
  width      = gegl_buffer_get_width (buffer);
  height     = gegl_buffer_get_height (buffer);

  while (y_offset < 0) y_offset += pat_height;
  while (x_offset < 0) x_offset += pat_width;

  x_offset = fmod (x_offset, pat_width);
  y_offset = fmod (y_offset, pat_height);

  src_rect.width  = dst_rect.width  = pat_width;
  src_rect.height = dst_rect.height = pat_height;

  cols = width  / pat_width  + 1;
  rows = height / pat_height + 1;

  for (row = 0; row <= rows + 1; row++)
    for (col = 0; col <= cols + 1; col++)
      {
        dst_rect.x = x_offset + (col-1) * pat_width;
        dst_rect.y = y_offset + (row-1) * pat_height;
        gegl_buffer_copy (pattern, &src_rect, buffer, &dst_rect);
      }
}

void
gegl_buffer_set_color (GeglBuffer          *dst,
                       const GeglRectangle *dst_rect,
                       GeglColor           *color)
{
  GeglBufferIterator *i;
  gchar               buf[128];
  gint                pxsize;

  g_return_if_fail (GEGL_IS_BUFFER (dst));
  g_return_if_fail (color);

  gegl_color_get_pixel (color, dst->soft_format, buf);

  if (!dst_rect)
    {
      dst_rect = gegl_buffer_get_extent (dst);
    }
  if (dst_rect->width == 0 ||
      dst_rect->height == 0)
    return;

  pxsize = babl_format_get_bytes_per_pixel (dst->soft_format);

  /* FIXME: this can be even further optimized by special casing it so
   * that fully filled tiles are shared.
   */
  i = gegl_buffer_iterator_new (dst, dst_rect, 0, dst->soft_format,
                                GEGL_BUFFER_WRITE, GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (i))
    {
      int j;
      for (j = 0; j < i->length; j++)
        memcpy (((guchar*)(i->data[0])) + pxsize * j, buf, pxsize);
    }
}

GeglBuffer *
gegl_buffer_dup (GeglBuffer *buffer)
{
  GeglBuffer *new_buffer;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  new_buffer = gegl_buffer_new (gegl_buffer_get_extent (buffer), buffer->soft_format);
  gegl_buffer_copy (buffer, gegl_buffer_get_extent (buffer),
                    new_buffer, gegl_buffer_get_extent (buffer));
  return new_buffer;
}

