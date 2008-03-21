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
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-allocator.h"
#include "gegl-storage.h"
#include "gegl-tile-backend.h"
#include "gegl-handler.h"
#include "gegl-tile.h"
#include "gegl-handler-cache.h"
#include "gegl-handler-log.h"
#include "gegl-handler-empty.h"
#include "gegl-buffer-allocator.h"
#include "gegl-sampler-nearest.h"
#include "gegl-sampler-linear.h"
#include "gegl-sampler-cubic.h"
#include "gegl-types.h"
#include "gegl-utils.h"


G_DEFINE_TYPE (GeglBuffer, gegl_buffer, GEGL_TYPE_TILE_TRAITS)

#if ENABLE_MP
GStaticRecMutex mutex = G_STATIC_REC_MUTEX_INIT;
#endif

static GObjectClass * parent_class = NULL;

enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_SHIFT_X,
  PROP_SHIFT_Y,
  PROP_ABYSS_X,
  PROP_ABYSS_Y,
  PROP_ABYSS_WIDTH,
  PROP_ABYSS_HEIGHT,
  PROP_FORMAT,
  PROP_PX_SIZE,
  PROP_PIXELS
};


static inline gint needed_tiles (gint w,
                                 gint stride)
{
  return ((w - 1) / stride) + 1;
}

static inline gint needed_width (gint w,
                                 gint stride)
{
  return needed_tiles (w, stride) * stride;
}

static const void *int_gegl_buffer_get_format (GeglBuffer *buffer);
static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglBuffer *buffer = GEGL_BUFFER (gobject);

  switch (property_id)
    {
      case PROP_WIDTH:
        g_value_set_int (value, buffer->extent.width);
        break;

      case PROP_HEIGHT:
        g_value_set_int (value, buffer->extent.height);
        break;

      case PROP_PIXELS:
        g_value_set_int (value, buffer->extent.width * buffer->extent.height);
        break;

      case PROP_PX_SIZE:
        g_value_set_int (value, buffer->storage->px_size);
        break;

      case PROP_FORMAT:
        /* might already be set the first time, if it was set during
         * construction, we're caching the value in the buffer itself,
         * since it will never change.
         */

        if (buffer->format == NULL)
          buffer->format = int_gegl_buffer_get_format (buffer);

        g_value_set_pointer (value, (void*)buffer->format); /* Eeeek? */
        break;

      case PROP_X:
        g_value_set_int (value, buffer->extent.x);
        break;

      case PROP_Y:
        g_value_set_int (value, buffer->extent.y);
        break;

      case PROP_SHIFT_X:
        g_value_set_int (value, buffer->shift_x);
        break;

      case PROP_SHIFT_Y:
        g_value_set_int (value, buffer->shift_y);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglBuffer *buffer = GEGL_BUFFER (gobject);

  switch (property_id)
    {
      case PROP_X:
        buffer->extent.x = g_value_get_int (value);
        break;

      case PROP_Y:
        buffer->extent.y = g_value_get_int (value);
        break;

      case PROP_WIDTH:
        buffer->extent.width = g_value_get_int (value);
        break;

      case PROP_HEIGHT:
        buffer->extent.height = g_value_get_int (value);
        break;

      case PROP_SHIFT_X:
        buffer->shift_x = g_value_get_int (value);
        break;

      case PROP_SHIFT_Y:
        buffer->shift_y = g_value_get_int (value);
        break;

      case PROP_ABYSS_X:
        buffer->abyss.x = g_value_get_int (value);
        break;

      case PROP_ABYSS_Y:
        buffer->abyss.y = g_value_get_int (value);
        break;

      case PROP_ABYSS_WIDTH:
        buffer->abyss.width = g_value_get_int (value);
        break;

      case PROP_ABYSS_HEIGHT:
        buffer->abyss.height = g_value_get_int (value);
        break;

      case PROP_FORMAT:
        /* Do not set to NULL even if asked to do so by a non-overriden
         * value, this is needed since a default value can not be specified
         * for a gpointer paramspec
         */
        if (g_value_get_pointer (value))
          buffer->format = g_value_get_pointer (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static gint allocated_buffers    = 0;
static gint de_allocated_buffers = 0;

void gegl_buffer_stats (void)
{
  g_warning ("Buffer statistics: allocated:%i deallocated:%i balance:%i",
             allocated_buffers, de_allocated_buffers, allocated_buffers - de_allocated_buffers);
}

gint gegl_buffer_leaks (void)
{
  return allocated_buffers - de_allocated_buffers;
}

static void gegl_buffer_void (GeglBuffer *buffer);

static void
gegl_buffer_dispose (GObject *object)
{
  GeglBuffer  *buffer  = GEGL_BUFFER (object);
  GeglHandler *handler = GEGL_HANDLER (object);

  gegl_buffer_sample_cleanup (buffer);

  if (handler->provider &&
      GEGL_IS_BUFFER_ALLOCATOR (handler->provider))
    {
      gegl_buffer_void (buffer);
#if 0
      handler->provider = NULL; /* this might be a dangerous way of marking that we have already voided */
#endif
    }

  if (buffer->hot_tile)
    {
      g_object_unref (buffer->hot_tile);
      buffer->hot_tile = NULL;
    }

  de_allocated_buffers++; /* XXX: is it correct to count that, shouldn't that
                             only be counted in finalize? */

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static GeglTileBackend *
gegl_buffer_backend (GeglBuffer *buffer)
{
  GeglProvider *tmp = GEGL_PROVIDER (buffer);

  if (!tmp)
    return NULL;

  do
    {
      tmp = GEGL_HANDLER (tmp)->provider;
    } while (tmp &&
             /*GEGL_IS_TILE_TRAIT (tmp) &&*/
             !GEGL_IS_TILE_BACKEND (tmp));
  if (!tmp &&
      !GEGL_IS_TILE_BACKEND (tmp))
    return NULL;

  return (GeglTileBackend *) tmp;
}

static GeglStorage *
gegl_buffer_storage (GeglBuffer *buffer)
{
  GeglProvider *tmp = GEGL_PROVIDER (buffer);

  do
    {
      tmp = ((GeglHandler *) (tmp))->provider;
    } while (!GEGL_IS_STORAGE (tmp));

  return (GeglStorage *) tmp;
}

void babl_backtrack (void);

static GObject *
gegl_buffer_constructor (GType                  type,
                         guint                  n_params,
                         GObjectConstructParam *params)
{
  GObject         *object;
  GeglBuffer      *buffer;
  GeglHandlers    *handlers;
  GeglTileBackend *backend;
  GeglHandler     *handler;
  GeglProvider    *provider;
  gint             tile_width;
  gint             tile_height;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  buffer    = GEGL_BUFFER (object);
  handlers  = GEGL_HANDLERS (object);
  handler   = GEGL_HANDLER (object);
  provider  = handler->provider;
  backend   = gegl_buffer_backend (buffer);

  if (provider)
    {
      if (GEGL_IS_STORAGE (provider))
        buffer->format = GEGL_STORAGE (provider)->format;
      else if (GEGL_IS_BUFFER (provider))
        buffer->format = GEGL_BUFFER (provider)->format;
    }

  if (!provider)
    {
      /* if no provider is specified if a format is specified, we
       * we need to create our own
       * provider (this adds a redirectin buffer in between for
       * all "allocated from format", type buffers.
       */
      g_assert (buffer->format);

      provider = GEGL_PROVIDER (gegl_buffer_new_from_format (buffer->format,
                                                             buffer->extent.x,
                                                             buffer->extent.y,
                                                             buffer->extent.width,
                                                             buffer->extent.height));
      /* after construction,. x and y should be set to reflect
       * the top level behavior exhibited by this buffer object.
       */
      g_object_set (buffer,
                    "provider", provider,
                    NULL);
      g_object_unref (provider);

      g_assert (provider);
      backend = gegl_buffer_backend (GEGL_BUFFER (provider));
      g_assert (backend);
    }

  g_assert (backend);

  tile_width  = backend->tile_width;
  tile_height = backend->tile_height;

  if (buffer->extent.width == -1 &&
      buffer->extent.height == -1) /* no specified extents, inheriting from provider */
    {
      if (GEGL_IS_BUFFER (provider))
        {
          buffer->extent.x = GEGL_BUFFER (provider)->extent.x;
          buffer->extent.y = GEGL_BUFFER (provider)->extent.y;
          buffer->extent.width  = GEGL_BUFFER (provider)->extent.width;
          buffer->extent.height = GEGL_BUFFER (provider)->extent.height;
        }
      else if (GEGL_IS_STORAGE (provider))
        {
          buffer->extent.x = 0;
          buffer->extent.y = 0;
          buffer->extent.width  = GEGL_STORAGE (provider)->width;
          buffer->extent.height = GEGL_STORAGE (provider)->height;
        }
    }

  if (buffer->abyss.width == 0 &&
      buffer->abyss.height == 0 &&
      buffer->abyss.x == 0 &&
      buffer->abyss.y == 0)      /* 0 sized extent == inherit buffer extent
                                  */
    {
      buffer->abyss.x      = buffer->extent.x;
      buffer->abyss.y      = buffer->extent.y;
      buffer->abyss.width  = buffer->extent.width;
      buffer->abyss.height = buffer->extent.height;
    }
  else if (buffer->abyss.width == 0 &&
           buffer->abyss.height == 0)
    {
      g_warning ("peculiar abyss dimensions: %i,%i %ix%i",
                 buffer->abyss.x,
                 buffer->abyss.y,
                 buffer->abyss.width,
                 buffer->abyss.height);
    }
  else if (buffer->abyss.width == -1 ||
           buffer->abyss.height == -1)
    {
      buffer->abyss.x      = GEGL_BUFFER (provider)->abyss.x - buffer->shift_x;
      buffer->abyss.y      = GEGL_BUFFER (provider)->abyss.y - buffer->shift_y;
      buffer->abyss.width  = GEGL_BUFFER (provider)->abyss.width;
      buffer->abyss.height = GEGL_BUFFER (provider)->abyss.height;
    }

  /* intersect our own abyss with parent's abyss if it exists
   */
  if (GEGL_IS_BUFFER (provider))
    {
      GeglRectangle parent = {
        GEGL_BUFFER (provider)->abyss.x - buffer->shift_x,
        GEGL_BUFFER (provider)->abyss.y - buffer->shift_y,
        GEGL_BUFFER (provider)->abyss.width,
        GEGL_BUFFER (provider)->abyss.height
      };
      GeglRectangle request = {
        buffer->abyss.x,
        buffer->abyss.y,
        buffer->abyss.width,
        buffer->abyss.height
      };
      GeglRectangle self;
      gegl_rectangle_intersect (&self, &parent, &request);

      buffer->abyss.x      = self.x;
      buffer->abyss.y      = self.y;
      buffer->abyss.width  = self.width;
      buffer->abyss.height = self.height;
    }

  /* compute our own total shift <- this should probably happen approximatly first */
  if (GEGL_IS_BUFFER (provider))
    {
      GeglBuffer *provider_buf;

      provider_buf = GEGL_BUFFER (provider);

      buffer->shift_x += provider_buf->shift_x;
      buffer->shift_y += provider_buf->shift_y;
    }
  else
    {
    }

  buffer->storage = gegl_buffer_storage (buffer);

  if (0) gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_EMPTY,
                                                     "backend", backend,
                                                     NULL));

  /* add a full width (should probably add height as well) sized cache for
   * this gegl_buffer only if the width is < 16384. The huge buffers used
   * by the allocator are thus not affected, but all other buffers should
   * have a enough tiles to be scanline iteratable.
   */

  if (0 && buffer->extent.width < 1 << 14)
    gegl_handlers_add (handlers, g_object_new (GEGL_TYPE_HANDLER_CACHE,
                                                "size",
                                                needed_tiles (buffer->extent.width, tile_width) + 1,
                                                NULL));

  return object;
}

static GeglTile *
get_tile (GeglProvider *tile_store,
          gint          x,
          gint          y,
          gint          z)
{
  GeglHandlers *handlers = (GeglHandlers*)(tile_store);
  GeglProvider *provider = ((GeglHandler*)tile_store)->provider;
  GeglTile     *tile   = NULL;

  if (handlers->chain != NULL)
    tile = gegl_provider_get_tile ((GeglProvider*)(handlers->chain->data),
                                     x, y, z);
  else if (provider)
    tile = gegl_provider_get_tile (provider, x, y, z);
  else
    g_assert (0);

  if (tile)
    {
      GeglBuffer *buffer = (GeglBuffer*)(tile_store);
      tile->x = x;
      tile->y = y;
      tile->z = z;

      if (x < buffer->min_x)
        buffer->min_x = x;
      if (y < buffer->min_y)
        buffer->min_y = y;
      if (x > buffer->max_x)
        buffer->max_x = x;
      if (y > buffer->max_y)
        buffer->max_y = y;
      if (z > buffer->max_z)
        buffer->max_z = z;

      /* storing information in tile, to enable the dispose function of the
       * tile instance to "hook" back to the storage with correct coordinates.
       */
      {
        tile->storage   = buffer->storage;
        tile->storage_x = x;
        tile->storage_y = y;
        tile->storage_z = z;
      }
    }

  return tile;
}

static void
gegl_buffer_class_init (GeglBufferClass *class)
{
  GObjectClass      *gobject_class       = G_OBJECT_CLASS (class);
  GeglProviderClass *tile_provider_class = GEGL_PROVIDER_CLASS (class);

  parent_class                = g_type_class_peek_parent (class);
  gobject_class->dispose      = gegl_buffer_dispose;
  gobject_class->constructor  = gegl_buffer_constructor;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  tile_provider_class->get_tile  = get_tile;

  g_object_class_install_property (gobject_class, PROP_PX_SIZE,
                                   g_param_spec_int ("px-size", "pixel-size", "size of a single pixel in bytes.",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class, PROP_PIXELS,
                                   g_param_spec_int ("pixels", "pixels", "total amount of pixels in image (width×height)",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width", "width", "pixel width of buffer",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", "height", "pixel height of buffer",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x", "x", "local origin's offset relative to provider origin",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y", "y", "local origin's offset relative to provider origin",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_ABYSS_WIDTH,
                                   g_param_spec_int ("abyss-width", "abyss-width", "pixel width of abyss",
                                                     -1, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_ABYSS_HEIGHT,
                                   g_param_spec_int ("abyss-height", "abyss-height", "pixel height of abyss",
                                                     -1, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_ABYSS_X,
                                   g_param_spec_int ("abyss-x", "abyss-x", "",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_ABYSS_Y,
                                   g_param_spec_int ("abyss-y", "abyss-y", "",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_SHIFT_X,
                                   g_param_spec_int ("shift-x", "shift-x", "",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_SHIFT_Y,
                                   g_param_spec_int ("shift-y", "shift-y", "",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format", "format", "babl format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gegl_buffer_init (GeglBuffer *buffer)
{
  buffer->extent.x      = 0;
  buffer->extent.y      = 0;
  buffer->extent.width  = 0;
  buffer->extent.height = 0;

  buffer->shift_x      = 0;
  buffer->shift_y      = 0;
  buffer->abyss.x      = 0;
  buffer->abyss.y      = 0;
  buffer->abyss.width  = 0;
  buffer->abyss.height = 0;
  buffer->format       = NULL;
  buffer->hot_tile     = NULL;

  buffer->min_x = 0;
  buffer->min_y = 0;
  buffer->max_x = 0;
  buffer->max_y = 0;
  buffer->max_z = 0;

  allocated_buffers++;
}

#if 0
gboolean
gegl_buffer_void_tile (GeglBuffer *buffer,
                       gint        x,
                       gint        y)
{
  gint z = 0;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return gegl_provider_message (GEGL_PROVIDER (buffer),
                                  GEGL_TILE_VOID, x, y, z, NULL);
}

/* returns TRUE if something was done */
gboolean
gegl_buffer_idle (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), FALSE);

  return gegl_provider_message (GEGL_PROVIDER (buffer),
                                GEGL_TILE_IDLE, 0, 0, 0, NULL);
}
#endif

/***************************************************************************/

static const void *int_gegl_buffer_get_format (GeglBuffer *buffer)
{
  g_assert (buffer);
  if (buffer->format != NULL)
    return buffer->format;
  return gegl_buffer_backend (buffer)->format;
}

static void
gegl_buffer_void (GeglBuffer *buffer)
{
  gint width       = buffer->extent.width;
  gint height      = buffer->extent.height;
  gint tile_width  = buffer->storage->tile_width;
  gint tile_height = buffer->storage->tile_height;
  gint bufy        = 0;

  {
    gint z;
    gint factor = 1;
    for (z = 0; z <= buffer->max_z; z++)
      {
        bufy = 0;
        while (bufy < height)
          {
            gint tiledy  = buffer->extent.y + buffer->shift_y + bufy;
            gint offsety = gegl_tile_offset (tiledy, tile_height);
            gint bufx    = 0;
            gint ty = gegl_tile_indice (tiledy / factor, tile_height);

            if (z != 0 ||  /* FIXME: handle z==0 correctly */
                ty >= buffer->min_y)
              while (bufx < width)
                {
                  gint tiledx  = buffer->extent.x + buffer->shift_x + bufx;
                  gint offsetx = gegl_tile_offset (tiledx, tile_width);

                  gint tx = gegl_tile_indice (tiledx / factor, tile_width);

                  if (z != 0 ||
                      tx >= buffer->min_x)
                  gegl_provider_message (GEGL_PROVIDER (buffer),
                                         GEGL_TILE_VOID, tx, ty, z, NULL);

                  if (z != 0 ||
                      tx > buffer->max_x)
                    goto done_with_row;

                  bufx += (tile_width - offsetx) * factor;
                }
done_with_row:
            bufy += (tile_height - offsety) * factor;

                  if (z != 0 ||
                      ty > buffer->max_y)
                    break;
          }
        factor *= 2;
      }
  }
}


/*
 * babl conversion should probably be done on a tile by tile, or even scanline by
 * scanline basis instead of allocating large temporary buffers. (using babl for "memcpy")
 */

#ifdef BABL
#undef BABL
#endif

#define BABL(o)     ((Babl *) (o))

#ifdef FMTPXS
#undef FMTPXS
#endif
#define FMTPXS(fmt)    (BABL (fmt)->format.bytes_per_pixel)

#if 0
static inline void
pset (GeglBuffer *buffer,
      gint        x,
      gint        y,
      const Babl *format,
      guchar     *buf)
{
  gint  tile_width  = buffer->storage->tile_width;
  gint  tile_height  = buffer->storage->tile_width;
  gint  px_size     = gegl_buffer_px_size (buffer);
  gint  bpx_size    = FMTPXS (format);
  Babl *fish        = NULL;

  gint  abyss_x_total  = buffer->abyss.x + buffer->abyss.width;
  gint  abyss_y_total  = buffer->abyss.y + buffer->abyss.height;
  gint  buffer_x       = buffer->extent.x;
  gint  buffer_y       = buffer->extent.y;
  gint  buffer_abyss_x = buffer->abyss.x;
  gint  buffer_abyss_y = buffer->abyss.y;

  if (format != buffer->format)
    {
      fish = babl_fish (buffer->format, format);
    }

  {
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

        GeglTile *tile = gegl_provider_get_tile ((GeglProvider *) (buffer),
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
            g_object_unref (tile);
          }
      }
  }
  return;
}
#endif

static inline void
pset (GeglBuffer *buffer,
      gint        x,
      gint        y,
      const Babl *format,
      gpointer    data)
{
  guchar *buf         = data;
  gint    tile_width  = buffer->storage->tile_width;
  gint    tile_height = buffer->storage->tile_height;
  gint    bpx_size    = FMTPXS (format);
  Babl   *fish        = NULL;

  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  buffer_abyss_x = buffer->abyss.x + buffer_shift_x;
  gint  buffer_abyss_y = buffer->abyss.y + buffer_shift_y;
  gint  abyss_x_total  = buffer_abyss_x + buffer->abyss.width;
  gint  abyss_y_total  = buffer_abyss_y + buffer->abyss.height;
  gint  px_size        = FMTPXS (buffer->format);

  if (format != buffer->format)
    {
      fish = babl_fish ((gpointer) buffer->format,
                        (gpointer) format);
    }

  {
    gint tiledy   = y + buffer_shift_y;
    gint tiledx   = x + buffer_shift_x;

    if (!(tiledy >= buffer_abyss_y &&
          tiledy  < abyss_y_total &&
          tiledx >= buffer_abyss_x &&
          tiledx  < abyss_x_total))
      { /* in abyss */
        return;
      }
    else
      {
        gint      indice_x = gegl_tile_indice (tiledx, tile_width);
        gint      indice_y = gegl_tile_indice (tiledy, tile_height);
        GeglTile *tile     = NULL;

        if (buffer->hot_tile &&
            buffer->hot_tile->storage_x == indice_x &&
            buffer->hot_tile->storage_y == indice_y)
          {
            tile = buffer->hot_tile;
          }
        else
          {
            if (buffer->hot_tile)
              {
                g_object_unref (buffer->hot_tile);
                buffer->hot_tile = NULL;
              }
            tile = gegl_provider_get_tile ((GeglProvider *) (buffer),
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
            buffer->hot_tile = tile;
          }
      }
  }
}

static inline void
pget (GeglBuffer *buffer,
      gint        x,
      gint        y,
      const Babl *format,
      gpointer    data)
{
  guchar *buf         = data;
  gint    tile_width  = buffer->storage->tile_width;
  gint    tile_height = buffer->storage->tile_height;
  gint    bpx_size    = FMTPXS (format);
  Babl   *fish        = NULL;

  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  buffer_abyss_x = buffer->abyss.x + buffer_shift_x;
  gint  buffer_abyss_y = buffer->abyss.y + buffer_shift_y;
  gint  abyss_x_total  = buffer_abyss_x + buffer->abyss.width;
  gint  abyss_y_total  = buffer_abyss_y + buffer->abyss.height;
  gint  px_size        = FMTPXS (buffer->format);

  if (format != buffer->format)
    {
      fish = babl_fish ((gpointer) buffer->format,
                        (gpointer) format);
    }

  {
    gint tiledy = y + buffer_shift_y;
    gint tiledx = x + buffer_shift_x;

    if (!(tiledy >= buffer_abyss_y &&
          tiledy <  abyss_y_total  &&
          tiledx >= buffer_abyss_x &&
          tiledx <  abyss_x_total))
      { /* in abyss */
        memset (buf, 0x00, bpx_size);
        return;
      }
    else
      {
        gint      indice_x = gegl_tile_indice (tiledx, tile_width);
        gint      indice_y = gegl_tile_indice (tiledy, tile_height);
        GeglTile *tile     = NULL;

        if (buffer->hot_tile &&
            buffer->hot_tile->storage_x == indice_x &&
            buffer->hot_tile->storage_y == indice_y)
          {
            tile = buffer->hot_tile;
          }
        else
          {
            if (buffer->hot_tile)
              {
                g_object_unref (buffer->hot_tile);
                buffer->hot_tile = NULL;
              }
            tile = gegl_provider_get_tile ((GeglProvider *) (buffer),
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

            /*g_object_unref (tile);*/
            buffer->hot_tile = tile;
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
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  if (buffer->hot_tile)
    {
      g_object_unref (buffer->hot_tile);
      buffer->hot_tile = NULL;
    }
}


static void inline
gegl_buffer_iterate (GeglBuffer *buffer,
                     guchar     *buf,
                     gint        rowstride,
                     gboolean    write,
                     const Babl *format,
                     gint        level)
{
  gint  width       = buffer->extent.width;
  gint  height      = buffer->extent.height;
  gint  tile_width  = buffer->storage->tile_width;
  gint  tile_height = buffer->storage->tile_height;
  gint  px_size     = FMTPXS (buffer->format);
  gint  bpx_size    = FMTPXS (format);
  gint  tile_stride = px_size * tile_width;
  gint  buf_stride;
  gint  bufy = 0;
  Babl *fish;

  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  buffer_x       = buffer->extent.x + buffer_shift_x;
  gint  buffer_y       = buffer->extent.y + buffer_shift_y;
  gint  buffer_abyss_x = buffer->abyss.x + buffer_shift_x;
  gint  buffer_abyss_y = buffer->abyss.y + buffer_shift_y;
  gint  abyss_x_total  = buffer_abyss_x + buffer->abyss.width;
  gint  abyss_y_total  = buffer_abyss_y + buffer->abyss.height;
  gint  i;
  gint  factor = 1;

  for (i = 0; i < level; i++)
    {
      factor *= 2;
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

  if (format == buffer->format)
    {
      fish = NULL;
    }
  else
    {
      if (write)
        {
          fish = babl_fish ((gpointer) format,
                            (gpointer) buffer->format);
        }
      else
        {
          fish = babl_fish ((gpointer) buffer->format,
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
                GeglTile *tile = gegl_provider_get_tile ((GeglProvider *) (buffer),
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
                g_object_unref (tile);
              }
            bufx += (tile_width - offsetx);
          }
      bufy += (tile_height - offsety);
    }
}

void
gegl_buffer_set (GeglBuffer          *buffer,
                 const GeglRectangle *rect,
                 const Babl          *format,
                 void                *src,
                 gint                 rowstride)
{
  GeglBuffer *sub_buf;

  g_return_if_fail (GEGL_IS_BUFFER (buffer));

#if ENABLE_MP
  g_static_rec_mutex_lock (&mutex);
#endif

  if (format == NULL)
    format = buffer->format;

  /* FIXME: go through chain of providers up to but not including
   * storage and disassociated Sampler */

  if (rect && rect->width == 1 && rect->height == 1) /* fast path */
    {
      pset (buffer, rect->x, rect->y, format, src);
    }
  /* FIXME: if rect->width == TILE_WIDTH and rect->height == TILE_HEIGHT and
   * aligned with tile grid, do a fast path, also provide helper functions
   * for getting the upper left coords of tiles.
   */
  else if (rect == NULL)
    {
      gegl_buffer_iterate (buffer, src, rowstride, TRUE, format, 0);
    }
  else
    {
      sub_buf = gegl_buffer_create_sub_buffer (buffer, rect);
      gegl_buffer_iterate (sub_buf, src, rowstride, TRUE, format, 0);
      g_object_unref (sub_buf);
    }

#if ENABLE_MP
  g_static_rec_mutex_unlock (&mutex);
#endif
}

/*
 * buffer: the buffer to get data from
 * rect:   the (full size rectangle to sample)
 * dst:    the destination buffer to write to
 * format: the format to write in
 * level:  halving levels 0 = 1:1 1=1:2 2=1:4 3=1:8 ..
 *
 */
static void
gegl_buffer_get_scaled (GeglBuffer          *buffer,
                        const GeglRectangle *rect,
                        void                *dst,
                        gint                 rowstride,
                        const void          *format,
                        gint                 level)
{
  GeglBuffer *sub_buf = gegl_buffer_create_sub_buffer (buffer, rect);
  gegl_buffer_iterate (sub_buf, dst, rowstride, FALSE, format, level);
  g_object_unref (sub_buf);
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
  gint i;
  for (i = 0; i < components; i++)
    {
      dest[i] = ( left_weight   * ((src[0][i] * top_weight) +
                                   (src[3][i] * middle_weight) +
                                   (src[6][i] * bottom_weight))
                + center_weight * ((src[1][i] * top_weight) +
                                   (src[4][i] * middle_weight) +
                                   (src[7][i] * bottom_weight))
                + right_weight  * ((src[2][i] * top_weight) +
                                   (src[5][i] * middle_weight) +
                                   (src[8][i] * bottom_weight))) / sum;
    }
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
gegl_buffer_get (GeglBuffer          *buffer,
                 gdouble              scale,
                 const GeglRectangle *rect,
                 const Babl          *format,
                 gpointer             dest_buf,
                 gint                 rowstride)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
#if ENABLE_MP
  g_static_rec_mutex_lock (&mutex);
#endif

  if (format == NULL)
    format = buffer->format;

  if (scale == 1.0 &&
      rect &&
      rect->width == 1 &&
      rect->height == 1)  /* fast path */
    {
      pget (buffer, rect->x, rect->y, format, dest_buf);
#if ENABLE_MP
      g_static_rec_mutex_unlock (&mutex);
#endif
      return;
    }

  if (!rect && scale == 1.0)
    {
      gegl_buffer_iterate (buffer, dest_buf, rowstride, FALSE, format, 0);
#if ENABLE_MP
      g_static_rec_mutex_unlock (&mutex);
#endif
      return;
    }
  if (rect->width == 0 ||
      rect->height == 0)
    {
#if ENABLE_MP
      g_static_rec_mutex_unlock (&mutex);
#endif
      return;
    }
  if (GEGL_FLOAT_EQUAL (scale, 1.0))
    {
      gegl_buffer_get_scaled (buffer, rect, dest_buf, rowstride, format, 0);
#if ENABLE_MP
      g_static_rec_mutex_unlock (&mutex);
#endif
      return;
    }
  else
    {
      gint          level       = 0;
      gint          buf_width   = rect->width / scale;
      gint          buf_height  = rect->height / scale;
      gint          bpp         = BABL (format)->format.bytes_per_pixel;
      GeglRectangle sample_rect = { floor(rect->x/scale),
                                    floor(rect->y/scale),
                                    buf_width,
                                    buf_height };
      void         *sample_buf;
      gint          factor = 1;
      gdouble       offset_x;
      gdouble       offset_y;

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
      gegl_buffer_get_scaled (buffer, &sample_rect, sample_buf, GEGL_AUTO_ROWSTRIDE, format, level);

      if (BABL (format)->format.type[0] == (BablType *) babl_type ("u8")
          && !(level == 0 && scale > 1.99))
        { /* do box-filter resampling if we're 8bit (which projections are) */

          /* XXX: use box-filter also for > 1.99 when testing and probably later,
           * there are some bugs when doing so
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
#if ENABLE_MP
  g_static_rec_mutex_unlock (&mutex);
#endif
}

const GeglRectangle *
gegl_buffer_get_abyss (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return &buffer->abyss;
}

void
gegl_buffer_sample (GeglBuffer       *buffer,
                    gdouble           x,
                    gdouble           y,
                    gdouble           scale,
                    gpointer          dest,
                    const Babl       *format,
                    GeglInterpolation interpolation)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

/*#define USE_WORKING_SHORTCUT*/
#ifdef USE_WORKING_SHORTCUT
  pget (buffer, x, y, format, dest);
  return;
#endif

#if ENABLE_MP
  g_static_rec_mutex_lock (&mutex);
#endif

  /* look up appropriate sampler,. */
  if (buffer->sampler == NULL)
    {
      /* FIXME: should probably check if the desired form of interpolation
       * changes from the currently cached sampler.
       */
      GType interpolation_type = 0;

      switch (interpolation)
        {
          case GEGL_INTERPOLATION_NEAREST:
            interpolation_type=GEGL_TYPE_SAMPLER_NEAREST;
            break;
          case GEGL_INTERPOLATION_LINEAR:
            interpolation_type=GEGL_TYPE_SAMPLER_LINEAR;
            break;
          default:
            g_warning ("unimplemented interpolation type %i", interpolation);
        }
      buffer->sampler = g_object_new (interpolation_type,
                                           "buffer", buffer,
                                           "format", format,
                                           NULL);
      gegl_sampler_prepare (buffer->sampler);
    }
  gegl_sampler_get (buffer->sampler, x, y, dest);

#if ENABLE_MP
  g_static_rec_mutex_unlock (&mutex);
#endif

  /* if none found, create a singleton sampler for this buffer,
   * a function to clean up the samplers set for a buffer should
   * also be provided */

  /* if (scale < 1.0) do decimation, possibly using pyramid instead */

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

const GeglRectangle *
gegl_buffer_get_extent (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return &(buffer->extent);
}

GeglBuffer *
gegl_buffer_new (const GeglRectangle *extent,
                 const Babl          *format)
{
  GeglRectangle empty={0,0,0,0};

  if (extent==NULL)
    extent = &empty;

  if (format==NULL)
    format = babl_format ("RGBA float");

  return g_object_new (GEGL_TYPE_BUFFER,
                       "x", extent->x,
                       "y", extent->y,
                       "width", extent->width,
                       "height", extent->height,
                       "format", format,
                       NULL);
}

GeglBuffer*
gegl_buffer_create_sub_buffer (GeglBuffer          *buffer,
                               const GeglRectangle *extent)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);
  g_return_val_if_fail (extent != NULL, NULL);

  return g_object_new (GEGL_TYPE_BUFFER,
                       "provider", buffer,
                       "x", extent->x,
                       "y", extent->y,
                       "width", extent->width,
                       "height", extent->height,
       /*              "abyss-width", 0,
                       "abyss-height", 0,  */
                       NULL);
}

void
gegl_buffer_copy (GeglBuffer          *src,
                  const GeglRectangle *src_rect,
                  GeglBuffer          *dst,
                  const GeglRectangle *dst_rect)
{
  /* FIXME: make gegl_buffer_copy work with COW shared tiles when possible */

  GeglRectangle src_line;
  GeglRectangle dst_line;
  const Babl   *format;
  guchar       *temp;
  guint         i;
  gint          pxsize;

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

  pxsize = src->storage->px_size;
  format = src->format;

  src_line = *src_rect;
  src_line.height = 1;

  dst_line = *dst_rect;
  dst_line.width = src_line.width;
  dst_line.height = src_line.height;

  temp = g_malloc (src_line.width * pxsize);

  for (i=0; i<src_rect->height; i++)
    {
      gegl_buffer_get (src, 1.0, &src_line, format, temp, GEGL_AUTO_ROWSTRIDE);
      gegl_buffer_set (dst, &dst_line, format, temp, GEGL_AUTO_ROWSTRIDE);
      src_line.y++;
      dst_line.y++;
    }
  g_free (temp);
}

GeglBuffer *
gegl_buffer_dup (GeglBuffer *buffer)
{
  GeglBuffer *new;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  new = gegl_buffer_new (gegl_buffer_get_extent (buffer), buffer->format);
  gegl_buffer_copy (buffer, gegl_buffer_get_extent (buffer),
                    new, gegl_buffer_get_extent (buffer));
  return new;
}


void
gegl_buffer_destroy (GeglBuffer *buffer)
{
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  g_object_unref (buffer);
}

GeglInterpolation
gegl_buffer_interpolation_from_string (const gchar *string)
{
  if (g_str_equal (string, "nearest") ||
      g_str_equal (string, "none"))
    return GEGL_INTERPOLATION_NEAREST;

  if (g_str_equal (string, "linear") ||
      g_str_equal (string, "bilinear"))
    return GEGL_INTERPOLATION_LINEAR;

 return GEGL_INTERPOLATION_NEAREST;
}
