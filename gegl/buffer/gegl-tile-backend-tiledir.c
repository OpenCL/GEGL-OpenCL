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
#include <glib/gprintf.h>

#include <string.h>
#include <errno.h>

#include "gegl-types.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-tiledir.h"

struct _GeglTileBackendTileDir
{
  GeglTileBackend  parent_instance;

  gchar           *path;         /* the base path of the buffer */
  GFile           *buffer_dir;
};

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
exist_tile (GeglTileSource *store,
            GeglTile       *tile,
            gint            x,
            gint            y,
            gint            z);

static GFile *make_tile_file (GeglTileBackendTileDir *gio,
                              gint                     x,
                              gint                     y,
                              gint                     z)
{
  gchar      buf[64];
  g_sprintf (buf, "%i-%i-%i", x, y, z);
  return g_file_get_child (gio->buffer_dir, buf);
}

static inline void
gio_entry_read (GeglTileBackendTileDir *gio,
                GioEntry                *entry,
                guchar                  *dest)
{
  GFile            *file;
  gint   tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (gio));
  GFileInputStream *i;
  gsize             bytes_read;

  file = make_tile_file (gio, entry->x, entry->y, entry->z);
  i = g_file_read (file, NULL, NULL);

  g_input_stream_read_all (G_INPUT_STREAM (i), dest, tile_size, &bytes_read,
                           NULL, NULL);
  g_assert (bytes_read == tile_size);
  g_input_stream_close (G_INPUT_STREAM (i), NULL, NULL);

  g_object_unref (G_OBJECT (i));
  g_object_unref (G_OBJECT (file));
}

static inline void
gio_entry_write (GeglTileBackendTileDir *gio,
                 GioEntry                *entry,
                 guchar                  *source)
{
  gint   tile_size = gegl_tile_backend_get_tile_size (GEGL_TILE_BACKEND (gio));
  GFile             *file;
  GFileOutputStream *o;
  gsize              bytes_written;

  file = make_tile_file (gio, entry->x, entry->y, entry->z);
  o = g_file_replace (file, NULL, FALSE,
                      G_FILE_CREATE_NONE, NULL, NULL);

  g_output_stream_write_all (G_OUTPUT_STREAM (o), source, tile_size,
                             &bytes_written, NULL, NULL);
  g_assert (bytes_written == tile_size);
  g_output_stream_close (G_OUTPUT_STREAM (o), NULL, NULL);

  g_object_unref (G_OBJECT (o));
  g_object_unref (G_OBJECT (file));
}


G_DEFINE_TYPE (GeglTileBackendTileDir, gegl_tile_backend_tiledir, GEGL_TYPE_TILE_BACKEND)
static GObjectClass * parent_class = NULL;

static gint allocs         = 0;
static gint gio_size      = 0;
static gint peak_allocs    = 0;
static gint peak_gio_size = 0;

void
gegl_tile_backend_tiledir_stats (void)
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
get_tile (GeglTileSource *tile_store,
          gint            x,
          gint            y,
          gint            z)
{
  GeglTileBackendTileDir     *tile_backend_tiledir = GEGL_TILE_BACKEND_TILE_DIR (tile_store);
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (tile_store);
  GeglTile        *tile     = NULL;

 if (exist_tile (tile_store, NULL, x, y, z))
  {
    GioEntry       entry;
    gint   tile_size = gegl_tile_backend_get_tile_size (backend);

    entry.x = x;
    entry.y = y;
    entry.z = z;

    tile = gegl_tile_new (tile_size);

    gio_entry_read (tile_backend_tiledir, &entry, gegl_tile_get_data (tile));
    return tile;
  }
 return NULL;
}

static gpointer
set_tile (GeglTileSource *store,
          GeglTile       *tile,
          gint            x,
          gint            y,
          gint            z)
{
  GeglTileBackend         *backend   = GEGL_TILE_BACKEND (store);
  GeglTileBackendTileDir *tile_backend_tiledir = GEGL_TILE_BACKEND_TILE_DIR (backend);
  GioEntry       entry;

  entry.x = x;
  entry.y = y;
  entry.z = z;

  gio_entry_write (tile_backend_tiledir, &entry, gegl_tile_get_data (tile));
  gegl_tile_mark_as_stored (tile);
  return NULL;
}

static gpointer
void_tile (GeglTileSource *store,
           GeglTile       *tile,
           gint            x,
           gint            y,
           gint            z)
{
  GeglTileBackend         *backend  = GEGL_TILE_BACKEND (store);
  GeglTileBackendTileDir *gio = GEGL_TILE_BACKEND_TILE_DIR (backend);
  GFile                   *file;

  file = make_tile_file (gio, x, y, z);
  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
  return NULL;
}

static gboolean
exist_tile (GeglTileSource *store,
            GeglTile       *tile,
            gint            x,
            gint            y,
            gint            z)
{
  GeglTileBackend         *backend  = GEGL_TILE_BACKEND (store);
  GeglTileBackendTileDir *gio = GEGL_TILE_BACKEND_TILE_DIR (backend);
  GFileInfo               *file_info;
  GFile                   *file;
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
gegl_tile_backend_tiledir_command (GeglTileSource  *tile_store,
                                   GeglTileCommand  command,
                                   gint             x,
                                   gint             y,
                                   gint             z,
                                   gpointer         data)
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
        return GINT_TO_POINTER (exist_tile (tile_store, data, x, y, z));

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
  GeglTileBackendTileDir *self = GEGL_TILE_BACKEND_TILE_DIR (object);

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
  GeglTileBackendTileDir *self = GEGL_TILE_BACKEND_TILE_DIR (object);

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
  GeglTileBackendTileDir *self = (GeglTileBackendTileDir *) object;
  GFileEnumerator *enumerator;
  GFileInfo       *info;

  enumerator = g_file_enumerate_children (self->buffer_dir, "standard::*",
                          G_FILE_QUERY_INFO_NONE, NULL, NULL);

  for (info = g_file_enumerator_next_file (enumerator, NULL, NULL);
       info;
       info = g_file_enumerator_next_file (enumerator, NULL, NULL))
    {
      const gchar *name = g_file_info_get_name (info);
      if (name)
        {
          GFile *file = g_file_get_child (self->buffer_dir, name);
          if (file)
            {
              g_file_delete (file, NULL, NULL);
              g_object_unref (file);
            }
        }
      g_object_unref (info);
    }

  g_object_unref (enumerator);

  g_file_delete (self->buffer_dir, NULL, NULL);
  g_object_unref (self->buffer_dir);

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static GObject *
gegl_tile_backend_tiledir_constructor (GType                  type,
                                         guint                  n_params,
                                         GObjectConstructParam *params)
{
  GObject      *object;
  GeglTileBackendTileDir *gio;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  gio    = GEGL_TILE_BACKEND_TILE_DIR (object);

  gio->buffer_dir = g_file_new_for_commandline_arg (gio->path);
  g_file_make_directory (gio->buffer_dir, NULL, NULL);
  ((GeglTileSource*)(object))->command = gegl_tile_backend_tiledir_command;
  return object;
}

static void
gegl_tile_backend_tiledir_class_init (GeglTileBackendTileDirClass *klass)
{
  GObjectClass    *gobject_class     = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->constructor  = gegl_tile_backend_tiledir_constructor;
  gobject_class->finalize     = finalize;

  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "The base path for this backing file for a buffer",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void
gegl_tile_backend_tiledir_init (GeglTileBackendTileDir *self)
{
  self->path        = NULL;
}
