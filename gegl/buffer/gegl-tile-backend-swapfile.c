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
 * Copyright 2006, 2007, 2008 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <gio/gio.h>
#include <string.h>
#include <errno.h>

#include <glib-object.h>

#include "gegl-tile-backend.h"
#include "gegl-tile-backend-swapfile.h"

struct _GeglTileBackendSwapfile
{
  GeglTileBackend  parent_instance;

  gchar         *path;
  GFile         *file;
  GOutputStream *o;
  GInputStream  *i;

  /*gint             fd;*/
  GHashTable      *entries;
  GSList          *free_list;
  guint            next_unused;
  guint            total;
};

static void dbg_alloc (int size);
static void dbg_dealloc (int size);

/* These entries are kept in RAM for now, they should be written as an
 * index to the swap file, at a position specified by a header block,
 * making the header grow up to a multiple of the size used in this
 * swap file is probably a good idea
 *
 * Serializing the bablformat is probably also a good idea.
 */
typedef struct _DiskEntry DiskEntry;

struct _DiskEntry
{
  gint x;
  gint y;
  gint z;
  gint offset;
};

static void inline
disk_entry_read (GeglTileBackendSwapfile *disk,
                 DiskEntry               *entry,
                 guchar                  *dest)
{
  gint  nleft;
  gboolean success;
  gint  tile_size = GEGL_TILE_BACKEND (disk)->tile_size;

  success = g_seekable_seek (G_SEEKABLE (disk->i), 
                             (goffset) entry->offset * tile_size, G_SEEK_SET,
                             NULL, NULL);
  if (success == FALSE)
    {
      g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
      return;
    }
  nleft = tile_size;

  while (nleft > 0)
    {
      gint read;

      read = g_input_stream_read (G_INPUT_STREAM (disk->i),
                                 dest + tile_size - nleft, nleft,
                                 NULL, NULL);
      if (read <= 0)
        {
          g_message ("unable to read tile data from disk: "
                     "%s (%d/%d bytes read)",
                     g_strerror (errno), read, nleft);
          return;
        }
      nleft -= read;
    }
}

static void inline
disk_entry_write (GeglTileBackendSwapfile *disk,
                  DiskEntry               *entry,
                  guchar                  *source)
{
  gint     nleft;
  gboolean success;
  gint     tile_size = GEGL_TILE_BACKEND (disk)->tile_size;

  success = g_seekable_seek (G_SEEKABLE (disk->o), 
                             (goffset) entry->offset * tile_size, G_SEEK_SET,
                             NULL, NULL);
  if (success == FALSE)
    {
      g_warning ("unable to seek to tile in buffer: %s", g_strerror (errno));
      return;
    }
  nleft = tile_size;

  while (nleft > 0)
    {
      gint wrote;
          wrote = g_output_stream_write (disk->o, source + tile_size - nleft,
                                         nleft, NULL, NULL);

      if (wrote <= 0)
        {
          g_message ("unable to write tile data to disk: "
                     "%s (%d/%d bytes written)",
                     g_strerror (errno), wrote, nleft);
          return;
        }
      nleft -= wrote;
    }
}

static inline DiskEntry *
disk_entry_new (GeglTileBackendSwapfile *disk)
{
  DiskEntry *self = g_slice_new (DiskEntry);

  if (disk->free_list)
    {
      self->offset    = GPOINTER_TO_INT (disk->free_list->data);
      disk->free_list = g_slist_remove (disk->free_list, disk->free_list->data);
    }
  else
    {
      self->offset = disk->next_unused++;

      if (self->offset >= disk->total)
        {
          gint grow = 32; /* grow 32 tiles of swap space at a time */
          gint tile_size = GEGL_TILE_BACKEND (disk)->tile_size;
          g_assert (g_seekable_truncate (G_SEEKABLE (disk->o),
                    (goffset) (disk->total + grow) * (goffset) tile_size,
                    NULL,NULL));
          disk->total = self->offset;
        }
    }
  dbg_alloc (GEGL_TILE_BACKEND (disk)->tile_size);
  return self;
}

static inline void
disk_entry_destroy (DiskEntry               *entry,
                    GeglTileBackendSwapfile *disk)
{
  disk->free_list = g_slist_prepend (disk->free_list,
                                     GINT_TO_POINTER (entry->offset));
  g_hash_table_remove (disk->entries, entry);

  dbg_dealloc (GEGL_TILE_BACKEND (disk)->tile_size);
  g_slice_free (DiskEntry, entry);
}


G_DEFINE_TYPE (GeglTileBackendSwapfile, gegl_tile_backend_swapfile, GEGL_TYPE_TILE_BACKEND)
static GObjectClass * parent_class = NULL;


static gint allocs         = 0;
static gint disk_size      = 0;
static gint peak_allocs    = 0;
static gint peak_disk_size = 0;

void
gegl_tile_backend_swapfile_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
             allocs, disk_size / 1024 / 1024.0,
             peak_allocs, peak_disk_size, peak_disk_size / 1024 / 1024.0);
}

static void
dbg_alloc (gint size)
{
  allocs++;
  disk_size += size;
  if (allocs > peak_allocs)
    peak_allocs = allocs;
  if (disk_size > peak_disk_size)
    peak_disk_size = disk_size;
}

static void
dbg_dealloc (gint size)
{
  allocs--;
  disk_size -= size;
}

static inline DiskEntry *
lookup_entry (GeglTileBackendSwapfile *self,
              gint          x,
              gint          y,
              gint          z)
{
  DiskEntry key = { x, y, z, 0 };

  return g_hash_table_lookup (self->entries, &key);
}

/* this is the only place that actually should
 * instantiate tiles, when the cache is large enough
 * that should make sure we don't hit this function
 * too often.
 */
static GeglTile *
get_tile (GeglTileSource *self,
          gint            x,
          gint            y,
          gint            z)
{

  GeglTileBackend         *backend;
  GeglTileBackendSwapfile *tile_backend_swapfile;
  DiskEntry               *entry;
  GeglTile                *tile = NULL;

  backend               = GEGL_TILE_BACKEND (self);
  tile_backend_swapfile = GEGL_TILE_BACKEND_SWAPFILE (backend);
  entry                 = lookup_entry (tile_backend_swapfile, x, y, z);

  if (!entry)
    return NULL;

  tile             = gegl_tile_new (backend->tile_size);
  tile->stored_rev = 1;
  tile->rev        = 1;

  disk_entry_read (tile_backend_swapfile, entry, tile->data);
  return tile;
}

static gpointer
set_tile (GeglTileSource *self,
          GeglTile       *tile,
          gint            x,
          gint            y,
          gint            z)
{
  GeglTileBackend         *backend;
  GeglTileBackendSwapfile *tile_backend_swapfile;
  DiskEntry               *entry;

  backend               = GEGL_TILE_BACKEND (self);
  tile_backend_swapfile = GEGL_TILE_BACKEND_SWAPFILE (backend);
  entry                 = lookup_entry (tile_backend_swapfile, x, y, z);


  if (entry == NULL)
    {
      entry    = disk_entry_new (tile_backend_swapfile);
      entry->x = x;
      entry->y = y;
      entry->z = z;
      g_hash_table_insert (tile_backend_swapfile->entries, entry, entry);
    }

  g_assert (tile->flags == 0); /* when this one is triggered, dirty pyramid data
                                  has been tried written to persistent tile_storage.
                                */
  disk_entry_write (tile_backend_swapfile, entry, tile->data);
  tile->stored_rev = tile->rev;
  return NULL;
}

static gpointer
void_tile (GeglTileSource *self,
           GeglTile       *tile,
           gint            x,
           gint            y,
           gint            z)
{
  GeglTileBackend         *backend;
  GeglTileBackendSwapfile *tile_backend_swapfile;
  DiskEntry               *entry;

  backend               = GEGL_TILE_BACKEND (self);
  tile_backend_swapfile = GEGL_TILE_BACKEND_SWAPFILE (backend);
  entry                 = lookup_entry (tile_backend_swapfile, x, y, z);

  if (entry != NULL)
    {
      disk_entry_destroy (entry, tile_backend_swapfile);
    }

  return NULL;
}

static gpointer
exist_tile (GeglTileSource *self,
            GeglTile       *tile,
            gint            x,
            gint            y,
            gint            z)
{
  GeglTileBackend         *backend;
  GeglTileBackendSwapfile *tile_backend_swapfile;
  DiskEntry               *entry;

  backend               = GEGL_TILE_BACKEND (self);
  tile_backend_swapfile = GEGL_TILE_BACKEND_SWAPFILE (backend);
  entry                 = lookup_entry (tile_backend_swapfile, x, y, z);

  return entry!=NULL?((gpointer)0x1):NULL;
}

enum
{
  PROP_0,
  PROP_PATH
};

static gpointer
command (GeglTileSource  *self,
         GeglTileCommand  command,
         gint             x,
         gint             y,
         gint             z,
         gpointer         data)
{
  switch (command)
    {
      case GEGL_TILE_GET:
        return get_tile (self, x, y, z);
      case GEGL_TILE_SET:
        return set_tile (self, data, x, y, z);

      case GEGL_TILE_IDLE:
        return NULL;

      case GEGL_TILE_VOID:
        return void_tile (self, data, x, y, z);

      case GEGL_TILE_EXIST:
        return exist_tile (self, data, x, y, z);

      default:
        g_assert (command < GEGL_TILE_LAST_COMMAND &&
                  command >= 0);
    }
  return FALSE;
}

static void
set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglTileBackendSwapfile *self = GEGL_TILE_BACKEND_SWAPFILE (object);

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
  GeglTileBackendSwapfile *self = GEGL_TILE_BACKEND_SWAPFILE (object);

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
  GeglTileBackendSwapfile *self = (GeglTileBackendSwapfile *) object;

  g_hash_table_unref (self->entries);

  g_object_unref (self->i);
  g_object_unref (self->o);
  /* check if we should nuke the buffer or not */ 
  g_file_delete  (self->file, NULL, NULL);
  g_object_unref (self->file);

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static guint
hashfunc (gconstpointer key)
{
  const DiskEntry *e = key;
  guint            hash;
  gint             i;
  gint             srcA = e->x;
  gint             srcB = e->y;
  gint             srcC = e->z;

  /* interleave the 10 least significant bits of all coordinates,
   * this gives us Z-order / morton order of the space and should
   * work well as a hash
   */
  hash = 0;
  for (i = 9; i >= 0; i--)
    {
#define ADD_BIT(bit)    do { hash |= (((bit) != 0) ? 1 : 0); hash <<= 1; \
    } \
  while (0)
      ADD_BIT (srcA & (1 << i));
      ADD_BIT (srcB & (1 << i));
      ADD_BIT (srcC & (1 << i));
#undef ADD_BIT
    }
  return hash;
}

static gboolean
equalfunc (gconstpointer a,
           gconstpointer b)
{
  const DiskEntry *ea = a;
  const DiskEntry *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;

  return FALSE;
}

static GObject *
gegl_tile_backend_swapfile_constructor (GType                  type,
                                        guint                  n_params,
                                        GObjectConstructParam *params)
{
  GObject      *object;
  GeglTileBackendSwapfile *disk;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  disk   = GEGL_TILE_BACKEND_SWAPFILE (object);

  disk->file = g_file_new_for_commandline_arg (disk->path);
  disk->o = G_OUTPUT_STREAM (g_file_replace (disk->file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL));
  g_output_stream_flush (disk->o, NULL, NULL);
  
  disk->i = G_INPUT_STREAM (g_file_read (disk->file, NULL, NULL));
  /*disk->fd = g_open (disk->path,
                     O_CREAT | O_RDWR | O_BINARY, S_IRUSR | S_IWUSR | O_DIRECT);*/


  if (!disk->file)
    {
      /*gchar *name = g_filename_display_name (disk->path);

      g_message ("Unable to open swap file '%s': %s\n"
                 "GEGL is unable to initialize virtual memory",
                 name, g_strerror (errno));

      g_free (name);*/
    }
  g_assert (disk->file);
  g_assert (disk->i);
  g_assert (disk->o);

  disk->entries = g_hash_table_new (hashfunc, equalfunc);

  return object;
}

static void
gegl_tile_backend_swapfile_class_init (GeglTileBackendSwapfileClass *klass)
{
  GObjectClass    *gobject_class     = G_OBJECT_CLASS (klass);
  GeglTileSourceClass *gegl_tile_source_class = GEGL_TILE_SOURCE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->constructor  = gegl_tile_backend_swapfile_constructor;
  gobject_class->finalize     = finalize;

  gegl_tile_source_class->command  = command;


  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "The base path for this backing file for a buffer",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void
gegl_tile_backend_swapfile_init (GeglTileBackendSwapfile *self)
{
  self->path        = NULL;
  self->file        = NULL;
  self->i           = NULL;
  self->o           = NULL;
  self->entries     = NULL;
  self->free_list   = NULL;
  self->next_unused = 0;
  self->total       = 0;
}
