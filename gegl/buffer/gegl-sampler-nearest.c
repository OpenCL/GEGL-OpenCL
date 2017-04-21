/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"
#include <math.h>
#include <string.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include "gegl-tile-backend.h"
#include "gegl-config.h"
#include "gegl-sampler-nearest.h"

enum
{
  PROP_0,
  PROP_LAST
};

static void
gegl_sampler_nearest_get (GeglSampler*    restrict self,
                          const gdouble            absolute_x,
                          const gdouble            absolute_y,
                          GeglMatrix2             *scale,
                          void*           restrict output,
                          GeglAbyssPolicy          repeat_mode);

static void
gegl_sampler_nearest_prepare (GeglSampler*    restrict self);

G_DEFINE_TYPE (GeglSamplerNearest, gegl_sampler_nearest, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_nearest_class_init (GeglSamplerNearestClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);

  sampler_class->get = gegl_sampler_nearest_get;
  sampler_class->prepare = gegl_sampler_nearest_prepare;
}

/*
 * It would seem that x=y=0 and width=height=1 should be enough, but
 * apparently safety w.r.t. round off or something else makes things
 * work better with width=height=3 and centering.
 */
static void
gegl_sampler_nearest_init (GeglSamplerNearest *self)
{
  GEGL_SAMPLER (self)->level[0].context_rect.x = 0;
  GEGL_SAMPLER (self)->level[0].context_rect.y = 0;
  GEGL_SAMPLER (self)->level[0].context_rect.width = 1;
  GEGL_SAMPLER (self)->level[0].context_rect.height = 1;
  GEGL_SAMPLER (self)->interpolate_format = gegl_babl_rgbA_linear_float ();
}

static void inline
gegl_sampler_get_pixel (GeglSampler    *sampler,
                        gint            x,
                        gint            y,
                        gpointer        data,
                        GeglAbyssPolicy repeat_mode)
{
  GeglSamplerNearest *nearest_sampler = (GeglSamplerNearest*)(sampler);
  GeglBuffer *buffer = sampler->buffer;
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
            babl_process (babl_fish (gegl_babl_rgba_linear_float (), sampler->format),
                          color,
                          buf,
                          1);
            return;
          }

        case GEGL_ABYSS_WHITE:
          {
            gfloat color[4] = {1.0, 1.0, 1.0, 1.0};
            babl_process (babl_fish (gegl_babl_rgba_linear_float (),
                                     sampler->format),
                          color,
                          buf,
                          1);
            return;
          }

        default:
        case GEGL_ABYSS_NONE:
          memset (buf, 0x00, babl_format_get_bytes_per_pixel (sampler->format));
          return;
      }
    }

  gegl_buffer_lock (sampler->buffer);
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

        guchar *tp         = gegl_tile_get_data (tile) + (offsety * tile_width + offsetx) * nearest_sampler->buffer_bpp;

        babl_process (sampler->fish, tp, buf, 1);
      }
  }
  if (gegl_config_threads()>1)
    g_rec_mutex_unlock (&buffer->tile_storage->mutex);
  gegl_buffer_unlock (sampler->buffer);
}

#if 0
static void
gegl_sampler_nearest_get_same_format  (      GeglSampler*    restrict  sampler,
                                       const gdouble                   absolute_x,
                                       const gdouble                   absolute_y,
                                             GeglMatrix2              *scale,
                                             void*           restrict  output,
                                             GeglAbyssPolicy           repeat_mode)
{
  GeglRectangle rectangle = {floorf(absolute_x), floorf(absolute_y), 1, 1};
  gegl_buffer_get (sampler->buffer, &rectangle, 1.0, sampler->format, output, GEGL_AUTO_ROWSTRIDE, repeat_mode);
}
#endif

static void
gegl_sampler_nearest_get_threaded (      GeglSampler*    restrict  sampler,
                          const gdouble                   absolute_x,
                          const gdouble                   absolute_y,
                                GeglMatrix2              *scale,
                                void*           restrict  output,
                                GeglAbyssPolicy           repeat_mode)
{
  GeglRectangle rect = {(gint) floorf ((double) absolute_x),
                        (gint) floorf ((double) absolute_y),1,1};
  gegl_buffer_get (sampler->buffer, &rect, 1.0, sampler->format, output, GEGL_AUTO_ROWSTRIDE, repeat_mode);
  return;
}

static void
gegl_sampler_nearest_get (      GeglSampler*    restrict  sampler,
                          const gdouble                   absolute_x,
                          const gdouble                   absolute_y,
                                GeglMatrix2              *scale,
                                void*           restrict  output,
                                GeglAbyssPolicy           repeat_mode)
{
#if 1
  gegl_sampler_get_pixel (sampler,
           floorf(absolute_x), floorf(absolute_y),
           output, repeat_mode);
#else
  const gfloat* restrict in_bptr =
    gegl_sampler_get_ptr (sampler,
                          (gint) floorf ((double) absolute_x),
                          (gint) floorf ((double) absolute_y),
                          repeat_mode);
  babl_process (sampler->fish, in_bptr, output, 1);
#endif
}


static void
gegl_sampler_nearest_prepare (GeglSampler* restrict sampler)
{
  if (!sampler->buffer) /* this happens when querying the extent of a sampler */
    return;
  GEGL_SAMPLER_NEAREST (sampler)->buffer_bpp = babl_format_get_bytes_per_pixel (sampler->buffer->format);

  if (gegl_config_threads () > 1)
    sampler->get = gegl_sampler_nearest_get_threaded;

#if 0 // maybe re-enable; when certain result is correct
  if (sampler->format == sampler->buffer->soft_format)
    {
      sampler->get = gegl_sampler_nearest_get_same_format;
    }
  else
#endif
    {
      sampler->fish = babl_fish (sampler->buffer->soft_format, sampler->format);
    }
}
