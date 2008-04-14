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
#include "gegl-tile-disk.h"

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
disk_entry_read (GeglTileDisk *disk,
                 DiskEntry    *entry,
                 guchar       *dest)
{
  gint  nleft;
  off_t offset;
  gint  tile_size = GEGL_TILE_BACKEND (disk)->tile_size;

  offset = lseek (disk->fd, (off_t) entry->offset * tile_size, SEEK_SET);
  if (offset == -1)
    {
      g_warning ("unable to seek to tile in swap: %s",
                 g_strerror (errno));
      return;
    }
  nleft = tile_size;

  while (nleft > 0)
    {
      gint err;

      do
        {
          err = read (disk->fd, dest + tile_size - nleft, nleft);
        } while ((err == -1) && ((errno == EAGAIN) || (errno == EINTR)));

      if (err <= 0)
        {
          g_message ("unable to read tile data from disk: "
                     "%s (%d/%d bytes read)",
                     g_strerror (errno), err, nleft);
          return;
        }
      nleft -= err;
    }
}

static void inline
disk_entry_write (GeglTileDisk *disk,
                  DiskEntry    *entry,
                  guchar       *source)
{
  gint   nleft;
  off_t  offset;
  gint   tile_size = GEGL_TILE_BACKEND (disk)->tile_size;

  offset = lseek (disk->fd, (off_t) entry->offset * tile_size, SEEK_SET);
  if (offset == -1)
    {
      g_warning ("unable to seek to tile in swap: %s",
                 g_strerror (errno));
      return;
    }
  nleft = tile_size;

  while (nleft > 0)
    {
      gint err = write (disk->fd, source + tile_size - nleft, nleft);

      if (err <= 0)
        {
          g_message ("unable to write tile data to disk: "
                     "%s (%d/%d bytes written)",
                     g_strerror (errno), err, nleft);
          return;
        }

      nleft -= err;
    }
}

static inline DiskEntry *
disk_entry_new (GeglTileDisk *disk)
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
          g_assert (0 == ftruncate (disk->fd,
                                    (off_t) (disk->total + grow) * (off_t) GEGL_TILE_BACKEND (disk)->tile_size));
          disk->total = self->offset;
        }
    }
  dbg_alloc (GEGL_TILE_BACKEND (disk)->tile_size);
  return self;
}

static inline void
disk_entry_destroy (DiskEntry    *entry,
                    GeglTileDisk *disk)
{
  disk->free_list = g_slist_prepend (disk->free_list,
                                     GINT_TO_POINTER (entry->offset));
  g_hash_table_remove (disk->entries, entry);

  dbg_dealloc (GEGL_TILE_BACKEND (disk)->tile_size);
  g_slice_free (DiskEntry, entry);
}


G_DEFINE_TYPE (GeglTileDisk, gegl_tile_disk, GEGL_TYPE_TILE_BACKEND)
static GObjectClass * parent_class = NULL;


static gint allocs         = 0;
static gint disk_size      = 0;
static gint peak_allocs    = 0;
static gint peak_disk_size = 0;

void
gegl_tile_disk_stats (void)
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
lookup_entry (GeglTileDisk *self,
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
get_tile (GeglProvider *tile_store,
          gint          x,
          gint          y,
          gint          z)
{
  GeglTileDisk    *tile_disk = GEGL_TILE_DISK (tile_store);
  GeglTileBackend *backend   = GEGL_TILE_BACKEND (tile_store);
  GeglTile        *tile      = NULL;

  {
    DiskEntry *entry = lookup_entry (tile_disk, x, y, z);

    if (!entry)
      return NULL;

    tile             = gegl_tile_new (backend->tile_size);
    tile->stored_rev = 1;
    tile->rev        = 1;

    disk_entry_read (tile_disk, entry, tile->data);
  }
  return tile;
}

static gpointer
set_tile (GeglProvider *store,
          GeglTile     *tile,
          gint          x,
          gint          y,
          gint          z)
{
  GeglTileBackend *backend   = GEGL_TILE_BACKEND (store);
  GeglTileDisk    *tile_disk = GEGL_TILE_DISK (backend);

  DiskEntry       *entry = lookup_entry (tile_disk, x, y, z);

  if (entry == NULL)
    {
      entry    = disk_entry_new (tile_disk);
      entry->x = x;
      entry->y = y;
      entry->z = z;
      g_hash_table_insert (tile_disk->entries, entry, entry);
    }

  g_assert (tile->flags == 0); /* when this one is triggered, dirty pyramid data
                                  has been tried written to persistent storage.
                                */
  disk_entry_write (tile_disk, entry, tile->data);
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
  GeglTileBackend *backend   = GEGL_TILE_BACKEND (store);
  GeglTileDisk    *tile_disk = GEGL_TILE_DISK (backend);
  DiskEntry       *entry     = lookup_entry (tile_disk, x, y, z);

  if (entry != NULL)
    {
      disk_entry_destroy (entry, tile_disk);
    }

  return NULL;
}

static gpointer
exist_tile (GeglProvider *store,
            GeglTile     *tile,
            gint          x,
            gint          y,
            gint          z)
{
  GeglTileBackend *backend   = GEGL_TILE_BACKEND (store);
  GeglTileDisk    *tile_disk = GEGL_TILE_DISK (backend);
  DiskEntry       *entry     = lookup_entry (tile_disk, x, y, z);

  return entry!=NULL?((gpointer)0x1):NULL;
}

enum
{
  PROP_0,
  PROP_PATH
};

static gpointer
message (GeglProvider  *tile_store,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  switch (message)
    {
      case GEGL_TILE_SET:
        return set_tile (tile_store, data, x, y, z);

      case GEGL_TILE_IDLE:
        return NULL;

      case GEGL_TILE_VOID:
        return void_tile (tile_store, data, x, y, z);

      case GEGL_TILE_EXIST:
        return exist_tile (tile_store, data, x, y, z);

      default:
        g_assert (message < GEGL_TILE_LAST_MESSAGE &&
                  message >= 0);
    }
  return FALSE;
}

static void
set_property (GObject      *object,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglTileDisk *self = GEGL_TILE_DISK (object);

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
  GeglTileDisk *self = GEGL_TILE_DISK (object);

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
  GeglTileDisk *self = (GeglTileDisk *) object;

  g_hash_table_unref (self->entries);

  close (self->fd);
  g_unlink (self->path);

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
gegl_tile_disk_constructor (GType                  type,
                            guint                  n_params,
                            GObjectConstructParam *params)
{
  GObject      *object;
  GeglTileDisk *disk;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  disk   = GEGL_TILE_DISK (object);

  disk->fd = g_open (disk->path,
                     O_CREAT | O_RDWR | O_BINARY, S_IRUSR | S_IWUSR | O_DIRECT);

  if (disk->fd == -1)
    {
      gchar *name = g_filename_display_name (disk->path);

      g_message ("Unable to open swap file '%s': %s\n"
                 "GEGL is unable to initialize virtual memory",
                 name, g_strerror (errno));

      g_free (name);
    }

  disk->entries = g_hash_table_new (hashfunc, equalfunc);

  return object;
}

static void
gegl_tile_disk_class_init (GeglTileDiskClass *klass)
{
  GObjectClass      *gobject_class       = G_OBJECT_CLASS (klass);
  GeglProviderClass *gegl_provider_class = GEGL_PROVIDER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->constructor  = gegl_tile_disk_constructor;
  gobject_class->finalize     = finalize;

  gegl_provider_class->get_tile = get_tile;
  gegl_provider_class->message  = message;


  g_object_class_install_property (gobject_class, PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "path",
                                                        "The base path for this backing file for a buffer",
                                                        NULL,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void
gegl_tile_disk_init (GeglTileDisk *self)
{
  self->path        = NULL;
  self->fd          = 0;
  self->entries     = NULL;
  self->free_list   = NULL;
  self->next_unused = 0;
  self->total       = 0;
}
