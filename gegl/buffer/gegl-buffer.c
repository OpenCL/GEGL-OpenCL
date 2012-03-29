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

#include <math.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <process.h>
#define getpid() _getpid()
#endif

#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>
#include <gio/gio.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-handler.h"
#include "gegl-tile-storage.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-file.h"
#include "gegl-tile-backend-tiledir.h"
#include "gegl-tile-backend-ram.h"
#include "gegl-tile.h"
#include "gegl-tile-handler-cache.h"
#include "gegl-tile-handler-log.h"
#include "gegl-tile-handler-empty.h"
#include "gegl-sampler-nearest.h"
#include "gegl-sampler-linear.h"
#include "gegl-sampler-cubic.h"
#include "gegl-sampler-lohalo.h"
#include "gegl-types-internal.h"
#include "gegl-utils.h"
#include "gegl-id-pool.h"
#include "gegl-buffer-index.h"
#include "gegl-config.h"
#include "gegl-buffer-cl-cache.h"

/* #define GEGL_BUFFER_DEBUG_ALLOCATIONS */

/* #define GEGL_BUFFER_DEBUG_ALLOCATIONS to print allocation stack
 * traces for leaked GeglBuffers using GNU C libs backtrace_symbols()
 */
#ifdef HAVE_EXECINFO_H
#include <execinfo.h>
#endif


G_DEFINE_TYPE (GeglBuffer, gegl_buffer, GEGL_TYPE_TILE_HANDLER)

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
  PROP_TILE_WIDTH,
  PROP_TILE_HEIGHT,
  PROP_FORMAT,
  PROP_PX_SIZE,
  PROP_PIXELS,
  PROP_PATH,
  PROP_BACKEND
};

enum {
  CHANGED,
  LAST_SIGNAL
};

static GeglBuffer *gegl_buffer_new_from_format     (const void *babl_fmt,
                                                    gint        x,
                                                    gint        y,
                                                    gint        width,
                                                    gint        height,
                                                    gint        tile_width,
                                                    gint        tile_height,
                                                    gboolean    use_ram);
static const void *gegl_buffer_internal_get_format (GeglBuffer *buffer);


guint gegl_buffer_signals[LAST_SIGNAL] = { 0 };


static inline gint gegl_buffer_needed_tiles (gint w,
                                             gint stride)
{
  return ((w - 1) / stride) + 1;
}

static inline gint gegl_buffer_needed_width (gint w,
                                             gint stride)
{
  return gegl_buffer_needed_tiles (w, stride) * stride;
}

static void
gegl_buffer_get_property (GObject    *gobject,
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
      case PROP_TILE_WIDTH:
        g_value_set_int (value, buffer->tile_width);
        break;

      case PROP_TILE_HEIGHT:
        g_value_set_int (value, buffer->tile_height);
        break;

      case PROP_PATH:
          {
            GeglTileBackend *backend = gegl_buffer_backend (buffer);
            if (GEGL_IS_TILE_BACKEND_FILE(backend))
              {
                if (buffer->path)
                  g_free (buffer->path);
                buffer->path = NULL;
                g_object_get (backend, "path", &buffer->path, NULL);
              }
          }
          g_value_set_string (value, buffer->path);
        break;
      case PROP_PIXELS:
        g_value_set_int (value, buffer->extent.width * buffer->extent.height);
        break;

      case PROP_PX_SIZE:
        g_value_set_int (value, buffer->tile_storage->px_size);
        break;

      case PROP_FORMAT:
        /* might already be set the first time, if it was set during
         * construction, we're caching the value in the buffer itself,
         * since it will never change.
         */

        {
          const Babl *format;

            format = buffer->soft_format;
          if (format == NULL)
            format = buffer->format;
          if (format == NULL)
            format = gegl_buffer_internal_get_format (buffer);

          g_value_set_pointer (value, (void*)format);
        }
        break;

      case PROP_BACKEND:
        g_value_set_pointer (value, buffer->backend);
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
gegl_buffer_set_property (GObject      *gobject,
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

      case PROP_TILE_HEIGHT:
        buffer->tile_height = g_value_get_int (value);
        break;

      case PROP_TILE_WIDTH:
        buffer->tile_width = g_value_get_int (value);
        break;

      case PROP_PATH:
        if (buffer->path)
          g_free (buffer->path);
        buffer->path = g_value_dup_string (value);
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
          {
            const Babl *format = g_value_get_pointer (value);
            /* XXX: need to check if the internal format matches, should
             * perhaps do different things here depending on whether
             * we are during construction or not
             */
            if (buffer->soft_format)
              {
                gegl_buffer_set_format (buffer, format);
              }
            else
              {
                buffer->format = format;
              }
          }
        break;
      case PROP_BACKEND:
        if (g_value_get_pointer (value))
          buffer->backend = g_value_get_pointer (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

#ifdef GEGL_BUFFER_DEBUG_ALLOCATIONS
static GList *allocated_buffers_list = NULL;
#endif
static gint   allocated_buffers      = 0;
static gint   de_allocated_buffers   = 0;

/* this should only be possible if this buffer matches all the buffers down to
 * storage, all of those parent buffers would change size as well, no tiles
 * would be voided as a result of changing the extent.
 */
gboolean
gegl_buffer_set_extent (GeglBuffer          *buffer,
                        const GeglRectangle *extent)
{
  g_return_val_if_fail(GEGL_IS_BUFFER(buffer), FALSE);
   (*(GeglRectangle*)gegl_buffer_get_extent (buffer))=*extent;

  if ((GeglBufferHeader*)(gegl_buffer_backend (buffer)->priv->header))
    {
      GeglBufferHeader *header =
        ((GeglBufferHeader*)(gegl_buffer_backend (buffer)->priv->header));
      header->x = buffer->extent.x;
      header->y = buffer->extent.y;
      header->width = buffer->extent.width;
      header->height = buffer->extent.height;
    }

  return TRUE;
}

void gegl_buffer_stats (void)
{
  g_warning ("Buffer statistics: allocated:%i deallocated:%i balance:%i",
             allocated_buffers, de_allocated_buffers, allocated_buffers - de_allocated_buffers);
}

gint gegl_buffer_leaks (void)
{
#ifdef GEGL_BUFFER_DEBUG_ALLOCATIONS
  {
    GList *leaked_buffer = NULL;

    for (leaked_buffer = allocated_buffers_list;
         leaked_buffer != NULL;
         leaked_buffer = leaked_buffer->next)
      {
        GeglBuffer *buffer = GEGL_BUFFER (leaked_buffer->data);

        g_printerr ("\n"
                   "Leaked buffer allocation stack trace:\n");
        g_printerr ("%s\n", buffer->alloc_stack_trace);
      }
  }
  g_list_free (allocated_buffers_list);
  allocated_buffers_list = NULL;
#endif

  return allocated_buffers - de_allocated_buffers;
}

void
_gegl_buffer_drop_hot_tile (GeglBuffer *buffer)
{
  GeglTileStorage *storage = buffer->tile_storage;
  if (storage->hot_tile)
    {
      gegl_tile_unref (storage->hot_tile);
      storage->hot_tile = NULL;
    }
}

static void
gegl_buffer_dispose (GObject *object)
{
  GeglBuffer  *buffer  = GEGL_BUFFER (object);
  GeglTileHandler *handler = GEGL_TILE_HANDLER (object);

  gegl_buffer_sample_cleanup (buffer);

  if (gegl_cl_is_accelerated ())
    gegl_buffer_cl_cache_invalidate (GEGL_BUFFER (object), NULL);

  if (handler->source &&
      GEGL_IS_TILE_STORAGE (handler->source))
    {
      GeglTileBackend *backend = gegl_buffer_backend (buffer);

      /* only flush non-internal backends,. */
      if (!(GEGL_IS_TILE_BACKEND_FILE (backend) ||
            GEGL_IS_TILE_BACKEND_RAM (backend) ||
            GEGL_IS_TILE_BACKEND_TILE_DIR (backend)))
        gegl_buffer_flush (buffer);

      gegl_tile_source_reinit (GEGL_TILE_SOURCE (handler->source));

#if 0
      g_object_unref (handler->source);
      handler->source = NULL; /* this might be a dangerous way of marking that we have already voided */
#endif
    }

  _gegl_buffer_drop_hot_tile (buffer);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gegl_buffer_finalize (GObject *object)
{
#ifdef GEGL_BUFFER_DEBUG_ALLOCATIONS
  g_free (GEGL_BUFFER (object)->alloc_stack_trace);
  allocated_buffers_list = g_list_remove (allocated_buffers_list, object);
#endif

  g_free (GEGL_BUFFER (object)->path);
  de_allocated_buffers++;
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GeglTileBackend *
gegl_buffer_backend (GeglBuffer *buffer)
{
  GeglTileSource *tmp = GEGL_TILE_SOURCE (buffer);

  if (!tmp)
    return NULL;

  do
    {
      tmp = GEGL_TILE_HANDLER (tmp)->source;
    } while (tmp &&
             /*GEGL_IS_TILE_TRAIT (tmp) &&*/
             !GEGL_IS_TILE_BACKEND (tmp));
  if (!tmp &&
      !GEGL_IS_TILE_BACKEND (tmp))
    return NULL;

  return (GeglTileBackend *) tmp;
}

static GeglTileStorage *
gegl_buffer_tile_storage (GeglBuffer *buffer)
{
  GeglTileSource *tmp = GEGL_TILE_SOURCE (buffer);

  do
    {
      tmp = ((GeglTileHandler *) (tmp))->source;
    } while (!GEGL_IS_TILE_STORAGE (tmp));

  g_assert (tmp);

  return (GeglTileStorage *) tmp;
}

void babl_backtrack (void);

static void gegl_buffer_storage_changed (GeglTileStorage     *storage,
                                         const GeglRectangle *rect,
                                         gpointer             userdata)
{
  g_signal_emit_by_name (GEGL_BUFFER (userdata), "changed", rect, NULL);
}

static GObject *
gegl_buffer_constructor (GType                  type,
                         guint                  n_params,
                         GObjectConstructParam *params)
{
  GObject         *object;
  GeglBuffer      *buffer;
  GeglTileBackend *backend;
  GeglTileHandler *handler;
  GeglTileSource  *source;

  gint width;
  gint height;
  gint x;
  gint y;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  buffer    = GEGL_BUFFER (object);
  handler   = GEGL_TILE_HANDLER (object);
  source  = handler->source;
  backend   = gegl_buffer_backend (buffer);

  x=buffer->extent.x;
  y=buffer->extent.y;
  width=buffer->extent.width;
  height=buffer->extent.height;

  if (source)
    {
      if (GEGL_IS_TILE_STORAGE (source))
        buffer->format = GEGL_TILE_STORAGE (source)->format;
      else if (GEGL_IS_BUFFER (source))
        buffer->format = GEGL_BUFFER (source)->format;
    }
  else
    {
      /* if no source is specified if a format is specified, we
       * we need to create our own
       * source (this adds a redirection buffer in between for
       * all "allocated from format", type buffers.
       */
      if (buffer->backend)
        {
          void             *storage;

          storage = gegl_tile_storage_new (buffer->backend);

          source = g_object_new (GEGL_TYPE_BUFFER, "source", storage, NULL);

          g_object_unref (storage);

          gegl_tile_handler_set_source ((GeglTileHandler*)(buffer), source);
          g_object_unref (source);

          g_signal_connect (storage, "changed",
                            G_CALLBACK(gegl_buffer_storage_changed), buffer);

          g_assert (source);
          backend = gegl_buffer_backend (GEGL_BUFFER (source));
          g_assert (backend);
	        g_assert (backend == buffer->backend);
	      }
      else if (buffer->path && g_str_equal (buffer->path, "RAM"))
        {
          source = GEGL_TILE_SOURCE (gegl_buffer_new_from_format (buffer->format,
                                                             buffer->extent.x,
                                                             buffer->extent.y,
                                                             buffer->extent.width,
                                                             buffer->extent.height,
                                                             buffer->tile_width,
                                                             buffer->tile_height, TRUE));
          /* after construction,. x and y should be set to reflect
           * the top level behavior exhibited by this buffer object.
           */
          gegl_tile_handler_set_source ((GeglTileHandler*)(buffer), source);
          g_object_unref (source);

          g_assert (source);
          backend = gegl_buffer_backend (GEGL_BUFFER (source));
          g_assert (backend);
        }
      else if (buffer->path)
        {
          GeglBufferHeader *header;
          void             *storage;

          backend = g_object_new (GEGL_TYPE_TILE_BACKEND_FILE,
                                  "tile-width", 128,
                                  "tile-height", 64,
                                  "format", buffer->format?buffer->format:babl_format ("RGBA float"),
                                  "path", buffer->path,
                                  NULL);
          storage = gegl_tile_storage_new (backend);

          source = g_object_new (GEGL_TYPE_BUFFER, "source", storage, NULL);

          /* after construction,. x and y should be set to reflect
           * the top level behavior exhibited by this buffer object.
           */
          gegl_tile_handler_set_source ((GeglTileHandler*)(buffer), source);
          g_object_unref (source);

          g_signal_connect (storage, "changed",
                            G_CALLBACK(gegl_buffer_storage_changed), buffer);

          g_assert (source);
          backend = gegl_buffer_backend (GEGL_BUFFER (source));
          g_assert (backend);
          header = backend->priv->header;
          buffer->extent.x = header->x;
          buffer->extent.y = header->y;
          buffer->extent.width = header->width;
          buffer->extent.height = header->height;
          buffer->format = gegl_tile_backend_get_format (backend);
        }
      else if (buffer->format)
        {
          source = GEGL_TILE_SOURCE (gegl_buffer_new_from_format (buffer->format,
                                                             buffer->extent.x,
                                                             buffer->extent.y,
                                                             buffer->extent.width,
                                                             buffer->extent.height,
                                                             buffer->tile_width,
                                                             buffer->tile_height, FALSE));
          /* after construction,. x and y should be set to reflect
           * the top level behavior exhibited by this buffer object.
           */
          gegl_tile_handler_set_source ((GeglTileHandler*)(buffer), source);
          g_object_unref (source);

          g_assert (source);
          backend = gegl_buffer_backend (GEGL_BUFFER (source));
          g_assert (backend);
        }
      else
        {
          g_warning ("not enough data to have a tile source for our buffer");
        }
        /* we reset the size if it seems to have been set to 0 during a on
         * disk buffer creation, nasty but it does the job.
         */

      if (buffer->extent.width == 0)
        {
          buffer->extent.width = width;
          buffer->extent.height = height;
          buffer->extent.x = x;
          buffer->extent.y = y;
        }
    }

  g_assert (backend);

  if (buffer->extent.width == -1 &&
      buffer->extent.height == -1) /* no specified extents,
                                      inheriting from source */
    {
      if (GEGL_IS_BUFFER (source))
        {
          buffer->extent.x = GEGL_BUFFER (source)->extent.x;
          buffer->extent.y = GEGL_BUFFER (source)->extent.y;
          buffer->extent.width  = GEGL_BUFFER (source)->extent.width;
          buffer->extent.height = GEGL_BUFFER (source)->extent.height;
        }
      else if (GEGL_IS_TILE_STORAGE (source))
        {
          buffer->extent.x = 0;
          buffer->extent.y = 0;
          buffer->extent.width  = GEGL_TILE_STORAGE (source)->width;
          buffer->extent.height = GEGL_TILE_STORAGE (source)->height;
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
      buffer->abyss.x      = GEGL_BUFFER (source)->abyss.x - buffer->shift_x;
      buffer->abyss.y      = GEGL_BUFFER (source)->abyss.y - buffer->shift_y;
      buffer->abyss.width  = GEGL_BUFFER (source)->abyss.width;
      buffer->abyss.height = GEGL_BUFFER (source)->abyss.height;
    }

  /* intersect our own abyss with parent's abyss if it exists
   */
  if (GEGL_IS_BUFFER (source))
    {
      GeglRectangle parent;
      GeglRectangle request;
      GeglRectangle self;

      parent.x = GEGL_BUFFER (source)->abyss.x - buffer->shift_x;
      parent.y = GEGL_BUFFER (source)->abyss.y - buffer->shift_y;
      parent.width = GEGL_BUFFER (source)->abyss.width;
      parent.height = GEGL_BUFFER (source)->abyss.height;

      request.x = buffer->abyss.x;
      request.y = buffer->abyss.y;
      request.width = buffer->abyss.width;
      request.height = buffer->abyss.height;

      gegl_rectangle_intersect (&self, &parent, &request);

      buffer->abyss.x      = self.x;
      buffer->abyss.y      = self.y;
      buffer->abyss.width  = self.width;
      buffer->abyss.height = self.height;
    }

  /* compute our own total shift <- this should probably happen
   * approximatly first */
  if (GEGL_IS_BUFFER (source))
    {
      GeglBuffer *source_buf;

      source_buf = GEGL_BUFFER (source);

      buffer->shift_x += source_buf->shift_x;
      buffer->shift_y += source_buf->shift_y;
    }
  else
    {
    }

  buffer->tile_storage = gegl_buffer_tile_storage (buffer);

  /* intialize the soft format to be equivalent to the actual
   * format
   */
  buffer->soft_format = buffer->format;

  return object;
}

static GeglTile *
gegl_buffer_get_tile (GeglTileSource *source,
                      gint        x,
                      gint        y,
                      gint        z)
{
  GeglTileHandler *handler = GEGL_TILE_HANDLER (source);
  GeglTile    *tile   = NULL;
  source = handler->source;

  if (source)
    tile = gegl_tile_source_get_tile (source, x, y, z);
  else
    g_assert (0);

  if (tile)
    {
      GeglBuffer *buffer = GEGL_BUFFER (handler);

      /* storing information in tile, to enable the dispose function of the
       * tile instance to "hook" back to the storage with correct
       * coordinates.
       */
      {
        if (!tile->tile_storage)
          {
            gegl_tile_lock (tile);
            tile->tile_storage = buffer->tile_storage;
            gegl_tile_unlock (tile);
            tile->rev--;
          }
        tile->x = x;
        tile->y = y;
        tile->z = z;
      }
    }

  return tile;
}


static gpointer
gegl_buffer_command (GeglTileSource *source,
                     GeglTileCommand command,
                     gint            x,
                     gint            y,
                     gint            z,
                     gpointer        data)
{
  GeglTileHandler *handler = GEGL_TILE_HANDLER (source);
  switch (command)
    {
      case GEGL_TILE_GET:
        return gegl_buffer_get_tile (source, x, y, z);
      default:
        return gegl_tile_handler_source_command (handler, command, x, y, z, data);
    }
}

static void
gegl_buffer_class_init (GeglBufferClass *class)
{
  GObjectClass      *gobject_class       = G_OBJECT_CLASS (class);
  parent_class                = g_type_class_peek_parent (class);
  gobject_class->dispose      = gegl_buffer_dispose;
  gobject_class->finalize     = gegl_buffer_finalize;
  gobject_class->constructor  = gegl_buffer_constructor;
  gobject_class->set_property = gegl_buffer_set_property;
  gobject_class->get_property = gegl_buffer_get_property;

  g_object_class_install_property (gobject_class, PROP_PX_SIZE,
                                   g_param_spec_int ("px-size", "pixel-size", "size of a single pixel in bytes.",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class, PROP_PIXELS,
                                   g_param_spec_int ("pixels", "pixels", "total amount of pixels in image (width x height)",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READABLE));
  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width", "width", "pixel width of buffer",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", "height", "pixel height of buffer",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x", "x", "local origin's offset relative to source origin",
                                                     G_MININT / 2, G_MAXINT / 2, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y", "y", "local origin's offset relative to source origin",
                                                     G_MININT / 2, G_MAXINT / 2, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
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
                                                     G_MININT / 2, G_MAXINT / 2, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_ABYSS_Y,
                                   g_param_spec_int ("abyss-y", "abyss-y", "",
                                                     G_MININT / 2, G_MAXINT / 2, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_SHIFT_X,
                                   g_param_spec_int ("shift-x", "shift-x", "",
                                                     G_MININT / 2, G_MAXINT / 2, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_SHIFT_Y,
                                   g_param_spec_int ("shift-y", "shift-y", "",
                                                     G_MININT / 2, G_MAXINT / 2, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format", "format", "babl format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_BACKEND,
                                   g_param_spec_pointer ("backend", "backend", "A custom tile-backend instance to use",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (gobject_class, PROP_TILE_HEIGHT,
                                   g_param_spec_int ("tile-height", "tile-height", "height of a tile",
                                                     -1, G_MAXINT, gegl_config()->tile_height,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_TILE_WIDTH,
                                   g_param_spec_int ("tile-width", "tile-width", "width of a tile",
                                                     -1, G_MAXINT, gegl_config()->tile_width,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path", "Path",
                                                        "URI to where the buffer is stored",
                                     NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  gegl_buffer_signals[CHANGED] =
        g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOXED,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_RECTANGLE);

}

#ifdef GEGL_BUFFER_DEBUG_ALLOCATIONS
#define MAX_N_FUNCTIONS 100
static gchar *
gegl_buffer_get_alloc_stack (void)
{
  char  *result         = NULL;
#ifndef HAVE_EXECINFO_H
  result = g_strdup ("backtrace() not available for this platform\n");
#else
  void  *functions[MAX_N_FUNCTIONS];
  int    n_functions    = 0;
  char **function_names = NULL;
  int    i              = 0;
  int    result_size    = 0;

  n_functions = backtrace (functions, MAX_N_FUNCTIONS);
  function_names = backtrace_symbols (functions, n_functions);

  for (i = 0; i < n_functions; i++)
    {
      result_size += strlen (function_names[i]);
      result_size += 1; /* for '\n' */
    }

  result = g_new (gchar, result_size + 1);
  result[0] = '\0';

  for (i = 0; i < n_functions; i++)
    {
      strcat (result, function_names[i]);
      strcat (result, "\n");
    }

  free (function_names);
#endif

  return result;
}
#endif

void gegl_bt (void);
void gegl_bt (void)
{
#ifdef GEGL_BUFFER_DEBUG_ALLOCATIONS
  g_print ("%s\n", gegl_buffer_get_alloc_stack ());
#endif
}

static void
gegl_buffer_init (GeglBuffer *buffer)
{
  buffer->tile_width = 128;
  buffer->tile_height = 64;
  ((GeglTileSource*)buffer)->command = gegl_buffer_command;

  allocated_buffers++;

#ifdef GEGL_BUFFER_DEBUG_ALLOCATIONS
  buffer->alloc_stack_trace = gegl_buffer_get_alloc_stack ();
  allocated_buffers_list = g_list_prepend (allocated_buffers_list, buffer);
#endif
}



const GeglRectangle *
gegl_buffer_get_extent (GeglBuffer *buffer)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  return &(buffer->extent);
}


GeglBuffer *
gegl_buffer_new_ram (const GeglRectangle *extent,
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
                       "path", "RAM",
                       NULL);
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

GeglBuffer *
gegl_buffer_new_for_backend (const GeglRectangle *extent,
                             void                *backend)
{
  GeglRectangle rect={0,0,0,0};
  const Babl *format;

  /* if no extent is passed in inherit from backend */
  if (extent==NULL)
    {
      extent = &rect;
      rect = gegl_tile_backend_get_extent (backend);
      /* if backend didnt have a rect, make it an infinite plane  */
      if (gegl_rectangle_is_empty (extent))
        rect = gegl_rectangle_infinite_plane ();
    }

  /* use the format of the backend */
  format = gegl_tile_backend_get_format (backend);

  return g_object_new (GEGL_TYPE_BUFFER,
                       "x", extent->x,
                       "y", extent->y,
                       "width", extent->width,
                       "height", extent->height,
                       "format", format,
                       "backend", backend,
                       NULL);
}


/* FIXME: this function needs optimizing, perhaps keep a pool
 * of GeglBuffer shells that can be adapted to the needs
 * on runtime, and recycling them through a hashtable?
 */
GeglBuffer*
gegl_buffer_create_sub_buffer (GeglBuffer          *buffer,
                               const GeglRectangle *extent)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

#if 1
  if (extent == NULL || gegl_rectangle_equal (extent, &buffer->extent))
    {
      g_object_ref (buffer);
      return buffer;
    }
#else
   if (extent == NULL)
      extent = gegl_buffer_get_extent (buffer);
#endif

  if (extent->width < 0 || extent->height < 0)
    {
      g_warning ("avoiding creating buffer of size: %ix%i returning an empty buffer instead.\n", extent->width, extent->height);
      return g_object_new (GEGL_TYPE_BUFFER,
                           "source", buffer,
                           "x", extent->x,
                           "y", extent->y,
                           "width", 0,
                           "height", 0,
                           NULL);
    }
  return g_object_new (GEGL_TYPE_BUFFER,
                       "source", buffer,
                       "x", extent->x,
                       "y", extent->y,
                       "width", extent->width,
                       "height", extent->height,
                       NULL);
}


typedef struct TileStorageCacheItem {
  GeglTileStorage *storage;
  gboolean         ram;
  gint             tile_width;
  gint             tile_height;
  const void      *babl_fmt;
} TileStorageCacheItem;

static GStaticMutex storage_cache_mutex = G_STATIC_MUTEX_INIT;
static GSList *storage_cache = NULL;

/* returns TRUE if it could be done */
gboolean gegl_tile_storage_cached_release (GeglTileStorage *storage);
gboolean gegl_tile_storage_cached_release (GeglTileStorage *storage)
{
  TileStorageCacheItem *item = g_object_get_data (G_OBJECT (storage), "storage-cache-item");

  if (!item)
    return FALSE;
  g_static_mutex_lock (&storage_cache_mutex);
  storage_cache = g_slist_prepend (storage_cache, item);
  g_static_mutex_unlock (&storage_cache_mutex);
  return TRUE;
}

void gegl_tile_storage_cache_cleanup (void);
void gegl_tile_storage_cache_cleanup (void)
{
  g_static_mutex_lock (&storage_cache_mutex);
  for (;storage_cache; storage_cache = g_slist_remove (storage_cache, storage_cache->data))
    {
      TileStorageCacheItem *item = storage_cache->data;
      g_object_unref (item->storage);
    }
  g_static_mutex_unlock (&storage_cache_mutex);
}

static GeglTileStorage *
gegl_tile_storage_new_cached (gint tile_width, gint tile_height,
                              const void *babl_fmt, gboolean use_ram)
{
  GeglTileStorage *storage = NULL;
  GSList *iter;
  g_static_mutex_lock (&storage_cache_mutex);
  for (iter = storage_cache; iter; iter = iter->next)
    {
      TileStorageCacheItem *item = iter->data;
      if (item->babl_fmt == babl_fmt &&
          item->tile_width == tile_width &&
          item->tile_height == tile_height &&
          item->ram == item->ram)
       {
         storage = item->storage;
         storage_cache = g_slist_remove (storage_cache, item);
         break;
       }
    }

  if (!storage)
    {
      TileStorageCacheItem *item = g_new0 (TileStorageCacheItem, 1);

      item->tile_width = tile_width;
      item->tile_height = tile_height;
      item->babl_fmt = babl_fmt;

      if (use_ram ||
          !gegl_config()->swap ||
          g_str_equal (gegl_config()->swap, "RAM") ||
          g_str_equal (gegl_config()->swap, "ram"))
        {
          GeglTileBackend *backend;
          item->ram = TRUE;
          backend = g_object_new (GEGL_TYPE_TILE_BACKEND_RAM,
                                  "tile-width", tile_width,
                                  "tile-height", tile_height,
                                  "format", babl_fmt,
                                  NULL);
          storage = gegl_tile_storage_new (backend);
        }
      else
        {
          static gint no = 1;
          GeglTileBackend *backend;

          gchar *filename;
          gchar *path;
          item->ram = FALSE;

#if 0
          filename = g_strdup_printf ("GEGL-%i-%s-%i.swap",
                                      getpid (),
                                      babl_name ((Babl *) babl_fmt),
                                      no++);
#endif

          filename = g_strdup_printf ("%i-%i", getpid(), no);
          g_atomic_int_inc (&no);
          path = g_build_filename (gegl_config()->swap, filename, NULL);
          g_free (filename);

          backend = g_object_new (GEGL_TYPE_TILE_BACKEND_FILE,
                                  "tile-width", tile_width,
                                  "tile-height", tile_height,
                                  "format", babl_fmt,
                                  "path", path,
                                  NULL);
          storage = gegl_tile_storage_new (backend);
          g_free (path);
        }
      item->storage = storage;
      g_object_set_data_full (G_OBJECT (storage), "storage-cache-item", item, g_free);
    }

  g_static_mutex_unlock (&storage_cache_mutex);
  return storage;
}

static GeglBuffer *
gegl_buffer_new_from_format (const void *babl_fmt,
                             gint        x,
                             gint        y,
                             gint        width,
                             gint        height,
                             gint        tile_width,
                             gint        tile_height,
                             gboolean    use_ram)
{
  GeglTileStorage *tile_storage;
  GeglBuffer  *buffer;

  tile_storage = gegl_tile_storage_new_cached (tile_width, tile_height, babl_fmt, use_ram);

  buffer = g_object_new (GEGL_TYPE_BUFFER,
                         "source",      tile_storage,
                         "x",           x,
                         "y",           y,
                         "width",       width,
                         "height",      height,
                         "tile-width",  tile_width,
                         "tile-height", tile_height,
                         NULL);

  g_object_unref (tile_storage);

  return buffer;
}

static const void *gegl_buffer_internal_get_format (GeglBuffer *buffer)
{
  g_assert (buffer);
  if (buffer->format != NULL)
    return buffer->format;
  return gegl_tile_backend_get_format (gegl_buffer_backend (buffer));
}

const Babl *
gegl_buffer_get_format (GeglBuffer *buffer)
{
  return buffer?buffer->format:NULL;
}

const Babl *
gegl_buffer_set_format (GeglBuffer *buffer,
                        const Babl *format)
{
  if (format == NULL)
    {
      buffer->soft_format = buffer->format;
      return buffer->soft_format;
    }
  if (babl_format_get_bytes_per_pixel (format) ==
      babl_format_get_bytes_per_pixel (buffer->format))
    {
      buffer->soft_format = format;
      return buffer->soft_format;
    }
  g_warning ("tried to set format of different bpp on buffer\n");
  return NULL;
}

gboolean gegl_buffer_is_shared (GeglBuffer *buffer)
{
  GeglTileBackend *backend = gegl_buffer_backend (buffer);
  return backend->priv->shared;
}

gboolean gegl_buffer_try_lock (GeglBuffer *buffer)
{
  gboolean ret;
  GeglTileBackend *backend = gegl_buffer_backend (buffer);
  g_mutex_lock (buffer->tile_storage->mutex);
  if (buffer->lock_count>0)
    {
      buffer->lock_count++;
      g_mutex_unlock (buffer->tile_storage->mutex);
      return TRUE;
    }
  if (gegl_buffer_is_shared(buffer))
    ret =gegl_tile_backend_file_try_lock (GEGL_TILE_BACKEND_FILE (backend));
  else
    ret = TRUE;
  if (ret)
    buffer->lock_count++;
  g_mutex_unlock (buffer->tile_storage->mutex);
  return TRUE;
}

/* this locking is for synchronising shared buffers */
gboolean gegl_buffer_lock (GeglBuffer *buffer)
{
  while (gegl_buffer_try_lock (buffer)==FALSE)
    {
      g_print ("waiting to aquire buffer..");
        g_usleep (100000);
    }
  return TRUE;
}

gboolean gegl_buffer_unlock (GeglBuffer *buffer)
{
  gboolean ret = TRUE;
  GeglTileBackend *backend = gegl_buffer_backend (buffer);
  g_mutex_lock (buffer->tile_storage->mutex);
  g_assert (buffer->lock_count >=0);
  buffer->lock_count--;
  g_assert (buffer->lock_count >=0);
  if (buffer->lock_count == 0 && gegl_buffer_is_shared (buffer))
    {
      ret = gegl_tile_backend_file_unlock (GEGL_TILE_BACKEND_FILE (backend));
    }
  g_mutex_unlock (buffer->tile_storage->mutex);
  return ret;
}
