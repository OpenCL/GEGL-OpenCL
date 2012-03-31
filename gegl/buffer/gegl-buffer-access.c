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
gegl_buffer_in_abyss( GeglBuffer *buffer,
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
  guchar     *buf         = data;
  gint        tile_width  = buffer->tile_storage->tile_width;
  gint        tile_height = buffer->tile_storage->tile_height;
  gint        bpx_size    = babl_format_get_bytes_per_pixel (format);
  const Babl *fish        = NULL;

  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  px_size        = babl_format_get_bytes_per_pixel (buffer->soft_format);

  if (format != buffer->soft_format)
    {
      fish = babl_fish ((gpointer) buffer->soft_format,
                        (gpointer) format);
    }

  {
    gint tiledy   = y + buffer_shift_y;
    gint tiledx   = x + buffer_shift_x;

    if (gegl_buffer_in_abyss( buffer, x, y))
      { /* in abyss */
        return;
      }
    else
      {
        gint      indice_x = gegl_tile_indice (tiledx, tile_width);
        gint      indice_y = gegl_tile_indice (tiledy, tile_height);
        GeglTile *tile     = NULL;

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
            guchar *tp;

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
}

static inline void
gegl_buffer_get_pixel (GeglBuffer *buffer,
                       gint        x,
                       gint        y,
                       const Babl *format,
                       gpointer    data)
{
  guchar     *buf         = data;
  gint        tile_width  = buffer->tile_storage->tile_width;
  gint        tile_height = buffer->tile_storage->tile_height;
  gint        bpx_size    = babl_format_get_bytes_per_pixel (format);
  const Babl *fish        = NULL;

  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  px_size        = babl_format_get_bytes_per_pixel (buffer->soft_format);

  if (format != buffer->soft_format)
    {
      fish = babl_fish ((gpointer) buffer->soft_format,
                        (gpointer) format);
    }

  {
    gint tiledy = y + buffer_shift_y;
    gint tiledx = x + buffer_shift_x;

    if (gegl_buffer_in_abyss (buffer, x, y))
      { /* in abyss */
        memset (buf, 0x00, bpx_size);
        return;
      }
    else
      {
        gint      indice_x = gegl_tile_indice (tiledx, tile_width);
        gint      indice_y = gegl_tile_indice (tiledy, tile_height);
        GeglTile *tile     = NULL;

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
gegl_buffer_iterate (GeglBuffer          *buffer,
                     const GeglRectangle *roi, /* or NULL for extent */
                     guchar              *buf,
                     gint                 rowstride,
                     gboolean             write,
                     const Babl          *format,
                     gint                 level)
{
  gint  tile_width  = buffer->tile_storage->tile_width;
  gint  tile_height = buffer->tile_storage->tile_height;
  gint  px_size     = babl_format_get_bytes_per_pixel (buffer->soft_format);
  gint  bpx_size    = babl_format_get_bytes_per_pixel (format);
  gint  tile_stride = px_size * tile_width;
  gint  buf_stride;
  gint  bufy = 0;

  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;

  gint  width          = buffer->extent.width;
  gint  height         = buffer->extent.height;
  gint  buffer_x       = buffer->extent.x + buffer_shift_x;
  gint  buffer_y       = buffer->extent.y + buffer_shift_y;

  gint  buffer_abyss_x = buffer->abyss.x + buffer_shift_x;
  gint  buffer_abyss_y = buffer->abyss.y + buffer_shift_y;
  gint  abyss_x_total  = buffer_abyss_x + buffer->abyss.width;
  gint  abyss_y_total  = buffer_abyss_y + buffer->abyss.height;
  gint  factor         = 1<<level;
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
    {
      if (write)
        {
          fish = babl_fish ((gpointer) format,
                            (gpointer) buffer->soft_format);
        }
      else
        {
          fish = babl_fish ((gpointer) buffer->soft_format,
                            (gpointer) format);
        }
    }

  while (bufy < height)
    {
      gint tiledy  = buffer_y + bufy;
      gint offsety = gegl_tile_offset (tiledy, tile_height);

      gint bufx    = 0;

      if (!(buffer_y + bufy + (tile_height) >= buffer_abyss_y &&
            buffer_y + bufy < abyss_y_total))
        { /* entire row of tiles is in abyss */
          if (!write)
            {
              gint    row;
              gint    y  = bufy;
              guchar *bp = buf + ((bufy) * width) * bpx_size;

              for (row = offsety;
                   row < tile_height && y < height;
                   row++, y++)
                {
                  memset (bp, 0x00, buf_stride);
                  bp += buf_stride;
                }
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
              pixels = (width + offsetx - bufx) - offsetx;
            else
              pixels = tile_width - offsetx;


            if (!(buffer_x + bufx + tile_width >= buffer_abyss_x &&
                  buffer_x + bufx < abyss_x_total))
              { /* entire tile is in abyss */
                if (!write)
                  {
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
              }
            else
              {
                guchar   *tile_base, *tp;
                GeglTile *tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                                           gegl_tile_indice (tiledx, tile_width),
                                                           gegl_tile_indice (tiledy, tile_height),
                                                           level);

                gint lskip = (buffer_abyss_x) - (buffer_x + bufx);
                /* gap between left side of tile, and abyss */
                gint rskip = (buffer_x + bufx + pixels) - abyss_x_total;
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

                if (write)
                  gegl_tile_lock (tile);

                tile_base = gegl_tile_get_data (tile);
                tp        = ((guchar *) tile_base) + (offsety * tile_width + offsetx) * px_size;

                if (write)
                  {
                    gint row;
                    gint y = bufy;


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
                  }
                else /* read */
                  {
                    gint row;
                    gint y = bufy;

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
                          {
                            memset (bp, 0x00, bpx_size * lskip);
                          }

                        /* right side zeroing of abyss in tile */
                        if (rskip)
                          {
                            memset (bp + (pixels - rskip) * bpx_size, 0x00, bpx_size * rskip);
                          }
                        tp += tile_stride;
                        bp += buf_stride;
                      }
                  }
                gegl_tile_unref (tile);
              }
            bufx += (tile_width - offsetx);
          }
      bufy += (tile_height - offsety);
    }
}

void
gegl_buffer_set_unlocked (GeglBuffer          *buffer,
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
    gegl_buffer_iterate (buffer, rect, (void *) src, rowstride, TRUE, format, 0);

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


#if 0
/*
 *  slow nearest neighbour resampler that seems to be
 *  completely correct.
 */

static void
resample_nearest (void   *dest_buf,
                  void   *source_buf,
                  gint    dest_w,
                  gint    dest_h,
                  gint    source_w,
                  gint    source_h,
                  gdouble offset_x,
                  gdouble offset_y,
                  gdouble scale,
                  gint    bpp,
                  gint    rowstride)
{
  gint x, y;

  if (rowstride == GEGL_AUTO_ROWSTRIDE)
     rowstride = dest_w * bpp;

  for (y = 0; y < dest_h; y++)
    {
      gint    sy;
      guchar *dst;
      guchar *src_base;

      sy = (y + offset_y) / scale;


      if (sy >= source_h)
        sy = source_h - 1;

      dst      = ((guchar *) dest_buf) + y * rowstride;
      src_base = ((guchar *) source_buf) + sy * source_w * bpp;

      for (x = 0; x < dest_w; x++)
        {
          gint    sx;
          guchar *src;
          sx = (x + offset_x) / scale;

          if (sx >= source_w)
            sx = source_w - 1;
          src = src_base + sx * bpp;

          memcpy (dst, src, bpp);
          dst += bpp;
        }
    }
}
#endif

/* Optimized|obfuscated version of the nearest neighbour resampler
 * XXX: seems to contains some very slight inprecision in the rendering.
 */
static void
resample_nearest (void   *dest_buf,
                  void   *source_buf,
                  gint    dest_w,
                  gint    dest_h,
                  gint    source_w,
                  gint    source_h,
                  gdouble offset_x,
                  gdouble offset_y,
                  gdouble scale,
                  gint    bpp,
                  gint    rowstride)
{
  gint x, y;
  guint xdiff, ydiff, xstart, sy;

  if (rowstride == GEGL_AUTO_ROWSTRIDE)
     rowstride = dest_w * bpp;

  xdiff = 65536 / scale;
  ydiff = 65536 / scale;
  xstart = (offset_x * 65536) / scale;
  sy = (offset_y * 65536) / scale;

  for (y = 0; y < dest_h; y++)
    {
      guchar *dst;
      guchar *src_base;
      guint sx;
      guint px = 0;
      guchar *src;

      if (sy >= source_h << 16)
        sy = (source_h - 1) << 16;

      dst      = ((guchar *) dest_buf) + y * rowstride;
      src_base = ((guchar *) source_buf) + (sy >> 16) * source_w * bpp;

      sx = xstart;
      src = src_base;

      /* this is the loop that is actually properly optimized,
       * portions of the setup is done for all the rows outside the y
       * loop as well */
      for (x = 0; x < dest_w; x++)
        {
          gint diff;
          gint ssx = sx>>16;
          if ( (diff = ssx - px) > 0)
            {
              if (ssx < source_w)
                src += diff * bpp;
              px += diff;
            }
          memcpy (dst, src, bpp);
          dst += bpp;
          sx += xdiff;
        }
      sy += ydiff;
    }
}

static inline void
box_filter (guint          left_weight,
            guint          center_weight,
            guint          right_weight,
            guint          top_weight,
            guint          middle_weight,
            guint          bottom_weight,
            guint          sum,
            const guchar **src,   /* the 9 surrounding source pixels */
            guchar        *dest,
            gint           components)
{
  /* NOTE: this box filter presumes pre-multiplied alpha, if there
   * is alpha.
   */
   guint lt, lm, lb;
   guint ct, cm, cb;
   guint rt, rm, rb;

   lt = left_weight * top_weight;
   lm = left_weight * middle_weight;
   lb = left_weight * bottom_weight;
   ct = center_weight * top_weight;
   cm = center_weight * middle_weight;
   cb = center_weight * bottom_weight;
   rt = right_weight * top_weight;
   rm = right_weight * middle_weight;
   rb = right_weight * bottom_weight;

#define docomponent(i) \
      dest[i] = (src[0][i] * lt + src[3][i] * lm + src[6][i] * lb + \
                 src[1][i] * ct + src[4][i] * cm + src[7][i] * cb + \
                 src[2][i] * rt + src[5][i] * rm + src[8][i] * rb) / sum
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
resample_boxfilter_u8 (void   *dest_buf,
                       void   *source_buf,
                       gint    dest_w,
                       gint    dest_h,
                       gint    source_w,
                       gint    source_h,
                       gdouble offset_x,
                       gdouble offset_y,
                       gdouble scale,
                       gint    components,
                       gint    rowstride)
{
  gint x, y;
  gint iscale      = scale * 256;
  gint s_rowstride = source_w * components;
  gint d_rowstride = dest_w * components;

  gint          footprint_x;
  gint          footprint_y;
  guint         foosum;

  guint         left_weight;
  guint         center_weight;
  guint         right_weight;

  guint         top_weight;
  guint         middle_weight;
  guint         bottom_weight;

  footprint_y = (1.0 / scale) * 256;
  footprint_x = (1.0 / scale) * 256;
  foosum = footprint_x * footprint_y;

  if (rowstride != GEGL_AUTO_ROWSTRIDE)
    d_rowstride = rowstride;

  for (y = 0; y < dest_h; y++)
    {
      gint    sy;
      gint    dy;
      guchar *dst;
      const guchar *src_base;
      gint sx;
      gint xdelta;

      sy = ((y + offset_y) * 65536) / iscale;

      if (sy >= (source_h - 1) << 8)
        sy = (source_h - 2) << 8;/* is this the right thing to do? */

      dy = sy & 255;

      dst      = ((guchar *) dest_buf) + y * d_rowstride;
      src_base = ((guchar *) source_buf) + (sy >> 8) * s_rowstride;

      if (dy > footprint_y / 2)
        top_weight = 0;
      else
        top_weight = footprint_y / 2 - dy;

      if (0xff - dy > footprint_y / 2)
        bottom_weight = 0;
      else
        bottom_weight = footprint_y / 2 - (0xff - dy);

      middle_weight = footprint_y - top_weight - bottom_weight;

      sx = (offset_x *65536) / iscale;
      xdelta = 65536/iscale;

      /* XXX: needs quite a bit of optimization */
      for (x = 0; x < dest_w; x++)
        {
          gint          dx;
          const guchar *src[9];

          /*sx = (x << 16) / iscale;*/
          dx = sx & 255;

          if (dx > footprint_x / 2)
            left_weight = 0;
          else
            left_weight = footprint_x / 2 - dx;

          if (0xff - dx > footprint_x / 2)
            right_weight = 0;
          else
            right_weight = footprint_x / 2 - (0xff - dx);

          center_weight = footprint_x - left_weight - right_weight;

          src[4] = src_base + (sx >> 8) * components;
          src[1] = src[4] - s_rowstride;
          src[7] = src[4] + s_rowstride;

          src[2] = src[1] + components;
          src[5] = src[4] + components;
          src[8] = src[7] + components;

          src[0] = src[1] - components;
          src[3] = src[4] - components;
          src[6] = src[7] - components;

          if ((sx >>8) - 1<0)
            {
              src[0]=src[1];
              src[3]=src[4];
              src[6]=src[7];
            }
          if ((sy >> 8) - 1 < 0)
            {
              src[0]=src[3];
              src[1]=src[4];
              src[2]=src[5];
            }
          if ((sx >>8) + 1 >= source_w)
            {
              src[2]=src[1];
              src[5]=src[4];
              src[8]=src[7];
              break;
            }
          if ((sy >> 8) + 1 >= source_h)
            {
              src[6]=src[3];
              src[7]=src[4];
              src[8]=src[5];
            }

          box_filter (left_weight,
                      center_weight,
                      right_weight,
                      top_weight,
                      middle_weight,
                      bottom_weight,
                      foosum,
                      src,   /* the 9 surrounding source pixels */
                      dst,
                      components);


          dst += components;
          sx += xdelta;
        }
    }
}


void
gegl_buffer_get_unlocked (GeglBuffer          *buffer,
                          gdouble              scale,
                          const GeglRectangle *rect,
                          const Babl          *format,
                          gpointer             dest_buf,
                          gint                 rowstride)
{

  if (format == NULL)
    format = buffer->soft_format;

#if 0
  /* not thread-safe */
  if (scale == 1.0 &&
      rect &&
      rect->width == 1 &&
      rect->height == 1)  /* fast path */
    {
      gegl_buffer_get_pixel (buffer, rect->x, rect->y, format, dest_buf);
      return;
    }
#endif

  if (gegl_cl_is_accelerated ())
    {
      gegl_buffer_cl_cache_flush (buffer, rect);
    }

  if (!rect && scale == 1.0)
    {
      gegl_buffer_iterate (buffer, NULL, dest_buf, rowstride, FALSE, format, 0);
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
      gegl_buffer_iterate (buffer, rect, dest_buf, rowstride, FALSE, format, 0);
      return;
    }
  else
    {
      gint          level       = 0;
      gint          buf_width   = rect->width / scale;
      gint          buf_height  = rect->height / scale;
      gint          bpp         = babl_format_get_bytes_per_pixel (format);
      GeglRectangle sample_rect;
      void         *sample_buf;
      gint          factor = 1;
      gdouble       offset_x;
      gdouble       offset_y;

      sample_rect.x = floor(rect->x/scale);
      sample_rect.y = floor(rect->y/scale);
      sample_rect.width = buf_width;
      sample_rect.height = buf_height;

      while (scale <= 0.5)
        {
          scale  *= 2;
          factor *= 2;
          level++;
        }

      buf_width  /= factor;
      buf_height /= factor;

      /* ensure we always have some data to sample from */
      sample_rect.width  += factor * 2;
      sample_rect.height += factor * 2;
      buf_width          += 2;
      buf_height         += 2;

      offset_x = rect->x-floor(rect->x/scale) * scale;
      offset_y = rect->y-floor(rect->y/scale) * scale;

      sample_buf = g_malloc (buf_width * buf_height * bpp);
      gegl_buffer_iterate (buffer, &sample_rect, sample_buf, GEGL_AUTO_ROWSTRIDE, FALSE, format, level);
#if 1
  /* slows testing of rendering code speed too much for now and
   * no time to make a fast implementation
   */

      if (babl_format_get_type (format, 0) == babl_type ("u8")
          && !(level == 0 && scale > 1.99))
        { /* do box-filter resampling if we're 8bit (which projections are) */

          /* XXX: use box-filter also for > 1.99 when testing and probably
           * later, there are some bugs when doing so
           */
          resample_boxfilter_u8 (dest_buf,
                                 sample_buf,
                                 rect->width,
                                 rect->height,
                                 buf_width,
                                 buf_height,
                                 offset_x,
                                 offset_y,
                                 scale,
                                 bpp,
                                 rowstride);
        }
      else
#endif
        {
          resample_nearest (dest_buf,
                            sample_buf,
                            rect->width,
                            rect->height,
                            buf_width,
                            buf_height,
                            offset_x,
                            offset_y,
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
                 GeglAbyssPolicy       repeat_mode)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  gegl_buffer_get_unlocked (buffer, scale, rect, format, dest_buf, rowstride);
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
                    GeglAbyssPolicy    repeat_mode)
{
  GType desired_type;
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

/*#define USE_WORKING_SHORTCUT*/
#ifdef USE_WORKING_SHORTCUT
  gegl_buffer_get_pixel (buffer, x, y, format, dest);
  return;
#endif

  desired_type = gegl_sampler_gtype_from_enum (sampler_type);

  if (!format)
    format = buffer->soft_format;

  if (format == buffer->soft_format &&
      sampler_type == GEGL_SAMPLER_NEAREST)
    {
      /* XXX: not thread safe */
      gegl_buffer_get_pixel (buffer, x, y, format, dest);
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

  gegl_sampler_get (buffer->sampler, x, y, scale, dest, GEGL_ABYSS_NONE);
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

void
gegl_buffer_copy (GeglBuffer          *src,
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
gegl_buffer_clear (GeglBuffer          *dst,
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

void            gegl_buffer_set_pattern       (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
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

  while (x_offset > pat_width)
    x_offset -= pat_width;
  while (y_offset < pat_height)
    y_offset += pat_height;

  while (x_offset < 0)
    x_offset += pat_width;
  while (y_offset > pat_height)
    y_offset -= pat_height;

  src_rect.width = dst_rect.width = pat_width;
  src_rect.height = dst_rect.height = pat_height;

  cols = width / pat_width + 1;
  rows = height / pat_height + 1;

  for (row = 0; row <= rows + 1; row++)
    for (col = 0; col <= cols + 1; col++)
      {
        dst_rect.x = x_offset + (col-1) * pat_width;
        dst_rect.y = y_offset + (row-1) * pat_height;
        gegl_buffer_copy (pattern, &src_rect, buffer, &dst_rect);
      }
}

void            gegl_buffer_set_color         (GeglBuffer          *dst,
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

