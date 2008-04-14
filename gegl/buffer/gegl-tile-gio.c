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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <gio/gio.h>

#include <string.h>
#include <errno.h>

#define _GNU_SOURCE    /* for O_DIRECT */

#include <fcntl.h>

#ifndef O_DIRECT
#define O_DIRECT    0
#endif

/* Microsoft Windows does distinguish between binary and text files.
 * We deal with binary files here and have to tell it to open them
 * as binary files. Unortunately the O_BINARY flag used for this is
 * specific to this platform, so we define it for others.
 */
#ifndef O_BINARY
#define O_BINARY    0
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <io.h>
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif
#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif
#define ftruncate(f,d) g_win32_ftruncate(f,d)
#endif

#include "gegl-tile-backend.h"
#include "gegl-tile-gio.h"

/* These entries are kept in RAM for now, they should be written as an
 * index to the swap file, at a position specified by a header block,
 * making the header grow up to a multiple of the size used in this
 * swap file is probably a good idea
 *
 * Serializing the bablformat is probably also a good idea.
 */
typedef struct _GioEntry GioEntry;

struct _GioEntry
{
  gint x;
  gint y;
  gint z;
};

static gboolean
exist_tile (GeglProvider *store,
            GeglTile     *tile,
            gint          x,
            gint          y,
            gint          z);

static GFile *make_tile_file (GeglTileGio *gio,
                              gint         x,
                              gint         y,
                              gint         z)
{
  gchar      buf[64];
  g_sprintf (buf, "%i-%i-%i", x, y, z);
  return g_file_get_child (gio->buffer_dir, buf);
}

static void inline
gio_entry_read (GeglTileGio *gio,
                GioEntry    *entry,
                guchar      *dest)
{
  GFile            *file;
  gint              tile_size = GEGL_TILE_BACKEND (gio)->tile_size;
  GFileInputStream *i;

  file = make_tile_file (gio, entry->x, entry->y, entry->z);
  i = g_file_read (file, NULL, NULL);

  g_input_stream_read (G_INPUT_STREAM (i), dest, tile_size, NULL, NULL);
  g_input_stream_close (G_INPUT_STREAM (i), NULL, NULL);

  g_object_unref (G_OBJECT (i));
  g_object_unref (G_OBJECT (file));
}

static void inline
gio_entry_write (GeglTileGio *gio,
                 GioEntry    *entry,
                 guchar       *source)
{
  gint               tile_size = GEGL_TILE_BACKEND (gio)->tile_size;
  GFile             *file;
  GFileOutputStream *o;

  file = make_tile_file (gio, entry->x, entry->y, entry->z);
  o = g_file_replace (file, NULL, FALSE, 
                      G_FILE_CREATE_NONE, NULL, NULL);

  g_output_stream_write (G_OUTPUT_STREAM (o), source, tile_size, NULL, NULL);
  g_output_stream_close (G_OUTPUT_STREAM (o), NULL, NULL);

  g_object_unref (G_OBJECT (o));
  g_object_unref (G_OBJECT (file));
}


G_DEFINE_TYPE (GeglTileGio, gegl_tile_gio, GEGL_TYPE_TILE_BACKEND)
static GObjectClass * parent_class = NULL;


static gint allocs         = 0;
static gint gio_size      = 0;
static gint peak_allocs    = 0;
static gint peak_gio_size = 0;

void
gegl_tile_gio_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
             allocs, gio_size / 1024 / 1024.0,
             peak_allocs, peak_gio_size, peak_gio_size / 1024 / 1024.0);
}

/* this is the only place that actually should
 * instantiate tiles, when the cache is large enough
 * that should make sure we don't hit this function
 * too often.
 */
static GeglTile *
get_tile (GeglProvider *tile_store,
          gint          x,
          gint          y,
          gint          z)
{
  GeglTileGio    *tile_gio = GEGL_TILE_GIO (tile_store);
  GeglTileBackend *backend   = GEGL_TILE_BACKEND (tile_store);
  GeglTile        *tile      = NULL;

 if (exist_tile (tile_store, NULL, x, y, z))
  {
    GioEntry entry = {x,y,z};

    tile             = gegl_tile_new (backend->tile_size);
    tile->stored_rev = 1;
    tile->rev        = 1;

    gio_entry_read (tile_gio, &entry, tile->data);
    return tile;
  }
 return NULL;
}

static gpointer
set_tile (GeglProvider *store,
          GeglTile     *tile,
          gint          x,
          gint          y,
          gint          z)
{
  GeglTileBackend *backend   = GEGL_TILE_BACKEND (store);
  GeglTileGio    *tile_gio = GEGL_TILE_GIO (backend);

  GioEntry       entry = {x,y,z};

  g_assert (tile->flags == 0); /* when this one is triggered, dirty pyramid data
                                  has been tried written to persistent storage.
                                */
  gio_entry_write (tile_gio, &entry, tile->data);
  tile->stored_rev = tile->rev;
  return NULL;
}

static gpointer
void_tile (GeglProvider *store,
           GeglTile     *tile,
           gint          x,
           gint          y,
           gint          z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileGio     *gio = GEGL_TILE_GIO (backend);
  GFile           *file;

  file = make_tile_file (gio, x, y, z);
  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
  return NULL;
}

static gboolean
exist_tile (GeglProvider *store,
            GeglTile     *tile,
            gint          x,
            gint          y,
            gint          z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileGio     *gio = GEGL_TILE_GIO (backend);
  GFileInfo       *file_info;
  GFile           *file;
  gboolean found = FALSE;

  file = make_tile_file (gio, x, y, z);
  file_info = g_file_query_info (file, "standard::*", G_FILE_QUERY_INFO_NONE, 
   NULL, NULL);

  if (file_info)
    {
      if (g_file_info_get_file_type (file_info) == G_FILE_TYPE_REGULAR)
        found = TRUE;
      g_object_unref (file_info);
    }
  g_object_unref (file);
  return found;
}

enum
{
  PROP_0,
  PROP_PATH
};

static gpointer
command (GeglProvider  *tile_store,
         GeglTileCommand command,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  switch (command)
    {
      case GEGL_TILE_GET:
        return get_tile (tile_store, x, y, z);

      case GEGL_TILE_SET:
        return set_tile (tile_store, data, x, y, z);

      case GEGL_TILE_IDLE:
        /* this backend has nothing to do on idle calls */
        return NULL;

      case GEGL_TILE_VOID:
        return void_tile (tile_store, data, x, y, z);

      case GEGL_TILE_EXIST:
        return (gpointer)exist_tile (tile_store, data, x, y, z);

      default:
        g_assert (command < GEGL_TILE_LAST_COMMAND &&
                  command >= 0);
    }
  return NULL;
}

static void
set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglTileGio *self = GEGL_TILE_GIO (object);

  switch (property_id)
    {
      case PROP_PATH:
        if (self->path)
          g_free (self->path);
        self->path = g_value_dup_string (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
get_property (GObject    *object,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileGio *self = GEGL_TILE_GIO (object);

  switch (property_id)
    {
      case PROP_PATH:
        g_value_set_string (value, self->path);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
finalize (GObject *object)
{
  GeglTileGio *self = (GeglTileGio *) object;

  g_file_delete (self->buffer_dir, NULL, NULL);
  g_object_unref (self->buffer_dir);

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static GObject *
gegl_tile_gio_constructor (GType                   type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject      *object;
  GeglTileGio *gio;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  gio    = GEGL_TILE_GIO (object);

  gio->buffer_dir = g_file_new_for_commandline_arg (gio->path);
  g_file_make_directory (gio->buffer_dir, NULL, NULL);
  return object;
}

static void
gegl_tile_gio_class_init (GeglTileGioClass *klass)
{
  GObjectClass      *gobject_class       = G_OBJECT_CLASS (klass);
  GeglProviderClass *gegl_provider_class = GEGL_PROVIDER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->constructor  = gegl_tile_gio_constructor;
  gobject_class->finalize     = finalize;

  gegl_provider_class->command  = command;


  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "The base path for this backing file for a buffer",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void
gegl_tile_gio_init (GeglTileGio *self)
{
  self->path        = NULL;
}
