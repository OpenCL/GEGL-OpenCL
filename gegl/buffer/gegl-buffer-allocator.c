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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <errno.h>

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <process.h>
#define getpid() _getpid()
#endif

#include "gegl-types.h"

#include "gegl-buffer-types.h"
#include "gegl-storage.h"
#include "gegl-buffer-allocator.h"


G_DEFINE_TYPE (GeglBufferAllocator, gegl_buffer_allocator, GEGL_TYPE_BUFFER)

static GObjectClass * parent_class = NULL;

static void
gegl_buffer_allocator_class_init (GeglBufferAllocatorClass *class)
{
  parent_class = g_type_class_peek_parent (class);
}

static void
gegl_buffer_allocator_init (GeglBufferAllocator *allocator)
{
  allocator->x_used = 0;
  allocator->y_used = 20000;
}


#include "gegl-buffer.h"

static GeglBuffer *
gegl_buffer_alloc (GeglBufferAllocator *allocator,
                   gint                 x,
                   gint                 y,
                   gint                 width,
                   gint                 height)
{
  GeglBuffer  *buffer      = GEGL_BUFFER (allocator);
  GeglStorage *storage     = buffer->storage;
  gint         tile_width  = storage->tile_width;
  gint         tile_height = storage->tile_height;

  gint needed_width  = ((width - 1) / tile_width +2) * tile_width;
  gint needed_height = ((height - 1) / tile_height +2) * tile_height;

  gint shift_x;
  gint shift_y;

  g_assert (allocator);

  if (needed_width > buffer->extent.width)
    {
      g_warning ("requested a %i wide allocation, but storage is only %i wide",
                 needed_width, buffer->extent.width);
    }

  if (allocator->y_used + needed_height > buffer->extent.height)
    {
      g_warning (
         "requested allocation (%ix%i) does not fit parent buffer (%ix%i)",
         width, height, buffer->extent.width, buffer->extent.height);
      return NULL;
    }

  if (allocator->x_used + needed_width > buffer->extent.width)
    {
      if (allocator->y_used + allocator->max_height + needed_height >
          buffer->extent.height)
        {
          g_warning (
             "requested allocation (%ix%i) does not fit parent buffer (%ix%i)",
             width, height, buffer->extent.width, buffer->extent.height);
          return NULL;
        }
      allocator->y_used    += allocator->max_height;
      allocator->x_used     = 0;
      allocator->max_height = 0;
    }

  shift_x = allocator->x_used - x;
  shift_y = allocator->y_used - y;

  allocator->x_used += needed_width;
  if (allocator->max_height < needed_height)
    allocator->max_height = needed_height;

  { GeglBuffer *tmp = g_object_new (GEGL_TYPE_BUFFER,
                                    "source", allocator,
                                    "x", x,
                                    "y", y,
                                    "width", width,
                                    "height", height,
                                    "shift-x", shift_x,
                                    "shift-y", shift_y,
                                    NULL);

    return tmp; }
}

static GHashTable *allocators = NULL;

/* if this function is made to return NULL swapping is disabled */
const gchar *
gegl_swap_dir (void)
{
  static gchar *swapdir = "";

  if (swapdir && swapdir[0] == '\0')
    {
      if (g_getenv ("GEGL_SWAP"))
        {
          if (g_str_equal (g_getenv ("GEGL_SWAP"), "RAM"))
            swapdir = NULL;
          else
            swapdir = g_strdup (g_getenv ("GEGL_SWAP"));
        }
      else
        {
          swapdir = g_build_filename (g_get_home_dir(),
                                      "." GEGL_LIBRARY,
                                      "swap",
                                      NULL);
        }

      /* Fall back to "swapping to RAM" if not able to create swap dir
       */
      if (swapdir &&
          ! g_file_test (swapdir, G_FILE_TEST_IS_DIR) &&
          g_mkdir_with_parents (swapdir, S_IRUSR | S_IWUSR | S_IXUSR) != 0)
        {
          gchar *name = g_filename_display_name (swapdir);

          g_warning ("unable to create swapdir '%s': %s",
                     name, g_strerror (errno));
          g_free (name);

          swapdir = NULL;
        }
    }

  return swapdir;
};

GeglBuffer *
gegl_buffer_new_from_format (const void *babl_format,
                             gint        x,
                             gint        y,
                             gint        width,
                             gint        height)
{
  GeglBufferAllocator *allocator = NULL;

  if (! allocators)
    allocators = g_hash_table_new (NULL, NULL);

  /* aquire a GeglBufferAllocator */
  /* iterate list of existing */
  allocator = g_hash_table_lookup (allocators, babl_format);

  /* if no match, create new */
  if (allocator == NULL)
    {
      if (gegl_swap_dir () != NULL)
        {
          GeglStorage *storage;
          gchar       *filename;
          gchar       *path;

          filename = g_strdup_printf ("GEGL-%i-%s.swap",
                                      getpid (),
                                      babl_name ((Babl *) babl_format));

          path = g_build_filename (gegl_swap_dir (), filename, NULL);
          g_free (filename);

          storage = g_object_new (GEGL_TYPE_STORAGE,
                                  "format", babl_format,
                                  "path",   path,
                                  NULL);
          allocator = g_object_new (GEGL_TYPE_BUFFER_ALLOCATOR,
                                    "source", storage,
                                    NULL);
          g_object_unref (storage);

          g_hash_table_insert (allocators, (gpointer)babl_format, allocator);

          g_free (path);
        }
      else
        {
          GeglStorage *storage = g_object_new (GEGL_TYPE_STORAGE,
                                               "format", babl_format,
                                               NULL);
          allocator = g_object_new (GEGL_TYPE_BUFFER_ALLOCATOR,
                                    "source", storage,
                                    NULL);
          g_object_unref (storage);
          g_hash_table_insert (allocators, (gpointer)babl_format, allocator);
        }
    }

  /* check if we already have a GeglBufferAllocator for the needed tile slice */
  return gegl_buffer_alloc (allocator, x, y, width, height);
}

static void free_allocator (gpointer key,
                            gpointer value,
                            gpointer user_data)
{
  GObject *allocator = value;

  g_object_unref (allocator);
}

void
gegl_buffer_allocators_free (void)
{
  if (allocators)
    {
      g_hash_table_foreach (allocators, free_allocator, NULL);
      g_hash_table_destroy (allocators);
      allocators = NULL;
    }
}
