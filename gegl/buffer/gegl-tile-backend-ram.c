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
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl-tile-backend.h"
#include "gegl-tile-backend-ram.h"


static void dbg_alloc (int size);
static void dbg_dealloc (int size);

/* These entries are kept in RAM for now, they should be written as an index to the
 * swap file, at a position specified by a header block, making the header grow up
 * to a multiple of the size used in this swap file is probably a good idea
 *
 * Serializing the bablformat is probably also a good idea.
 */
typedef struct _RamEntry RamEntry;

struct _RamEntry
{
  gint    x;
  gint    y;
  gint    z;
  guchar *offset;
};

static void inline
ram_entry_read (GeglTileBackendRam *ram,
                RamEntry           *entry,
                guchar             *dest)
{
  gint tile_size = GEGL_TILE_BACKEND (ram)->tile_size;

  memcpy (dest, entry->offset, tile_size);
}

static void inline
ram_entry_write (GeglTileBackendRam *ram,
                 RamEntry           *entry,
                 guchar             *source)
{
  gint tile_size = GEGL_TILE_BACKEND (ram)->tile_size;

  memcpy (entry->offset, source, tile_size);
}

static inline RamEntry *
ram_entry_new (GeglTileBackendRam *ram)
{
  RamEntry *self = g_slice_new (RamEntry);

  self->offset = g_malloc (GEGL_TILE_BACKEND (ram)->tile_size);
  dbg_alloc (GEGL_TILE_BACKEND (ram)->tile_size);
  return self;
}

static inline void
ram_entry_destroy (RamEntry           *entry,
                   GeglTileBackendRam *ram)
{
  g_free (entry->offset);
  g_hash_table_remove (ram->entries, entry);

  dbg_dealloc (GEGL_TILE_BACKEND (ram)->tile_size);
  g_slice_free (RamEntry, entry);
}


G_DEFINE_TYPE (GeglTileBackendRam, gegl_tile_backend_ram, GEGL_TYPE_TILE_BACKEND)
static GObjectClass * parent_class = NULL;


static gint allocs        = 0;
static gint ram_size      = 0;
static gint peak_allocs   = 0;
static gint peak_ram_size = 0;

void gegl_tile_backend_ram_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
             allocs, ram_size / 1024 / 1024.0, peak_allocs, peak_ram_size, peak_ram_size / 1024 / 1024.0);
}

static void dbg_alloc (gint size)
{
  allocs++;
  ram_size += size;
  if (allocs > peak_allocs)
    peak_allocs = allocs;
  if (ram_size > peak_ram_size)
    peak_ram_size = ram_size;
}

static void dbg_dealloc (gint size)
{
  allocs--;
  ram_size -= size;
}

static inline RamEntry *
lookup_entry (GeglTileBackendRam *self,
              gint         x,
              gint         y,
              gint         z)
{
  RamEntry key;

  key.x = x;
  key.y = y;
  key.z = z;
  key.offset = 0;

  return g_hash_table_lookup (self->entries, &key);
}

/* this is the only place that actually should
 * instantiate tiles, when the cache is large enough
 * that should make sure we don't hit this function
 * too often.
 */
static GeglTile *
get_tile (GeglTileSource *tile_store,
          gint        x,
          gint        y,
          gint        z)
{
  GeglTileBackendRam     *tile_backend_ram = GEGL_TILE_BACKEND_RAM (tile_store);
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (tile_store);
  GeglTile        *tile     = NULL;

  {
    RamEntry *entry = lookup_entry (tile_backend_ram, x, y, z);

    if (!entry)
      return NULL;

    tile             = gegl_tile_new (backend->tile_size);
    tile->stored_rev = 1;
    tile->rev        = 1;

    ram_entry_read (tile_backend_ram, entry, tile->data);
  }
  return tile;
}

static
gboolean set_tile (GeglTileSource *store,
                   GeglTile   *tile,
                   gint        x,
                   gint        y,
                   gint        z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileBackendRam     *tile_backend_ram = GEGL_TILE_BACKEND_RAM (backend);

  RamEntry        *entry = lookup_entry (tile_backend_ram, x, y, z);

  if (entry == NULL)
    {
      entry    = ram_entry_new (tile_backend_ram);
      entry->x = x;
      entry->y = y;
      entry->z = z;
      g_hash_table_insert (tile_backend_ram->entries, entry, entry);
    }
  ram_entry_write (tile_backend_ram, entry, tile->data);
  tile->stored_rev = tile->rev;
  return TRUE;
}

static
gboolean void_tile (GeglTileSource *store,
                    GeglTile   *tile,
                    gint        x,
                    gint        y,
                    gint        z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileBackendRam     *tile_backend_ram = GEGL_TILE_BACKEND_RAM (backend);
  RamEntry        *entry    = lookup_entry (tile_backend_ram, x, y, z);

  if (entry != NULL)
    {
      ram_entry_destroy (entry, tile_backend_ram);
    }

  return TRUE;
}

static
gboolean exist_tile (GeglTileSource *store,
                     GeglTile   *tile,
                     gint        x,
                     gint        y,
                     gint        z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileBackendRam     *tile_backend_ram = GEGL_TILE_BACKEND_RAM (backend);
  RamEntry        *entry    = lookup_entry (tile_backend_ram, x, y, z);

  return entry != NULL;
}


enum
{
  PROP_0
};

static gpointer
gegl_tile_backend_ram_command (GeglTileSource     *tile_store,
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
        set_tile (tile_store, data, x, y, z);
        return NULL;

      case GEGL_TILE_IDLE:
        return NULL;

      case GEGL_TILE_VOID:
        void_tile (tile_store, data, x, y, z);
        return NULL;

      case GEGL_TILE_EXIST:
        return (gpointer)exist_tile (tile_store, data, x, y, z);

      default:
        g_assert (command < GEGL_TILE_LAST_COMMAND &&
                  command >= 0);
    }
  return FALSE;
}

static void set_property (GObject       *object,
                            guint          property_id,
                            const GValue *value,
                            GParamSpec    *pspec)
{
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
finalize (GObject *object)
{
  GeglTileBackendRam *self = (GeglTileBackendRam *) object;

  g_hash_table_unref (self->entries);

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static guint hashfunc (gconstpointer key)
{
  const RamEntry *e = key;
  guint           hash;
  gint            i;
  gint            srcA = e->x;
  gint            srcB = e->y;
  gint            srcC = e->z;

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

static gboolean equalfunc (gconstpointer a,
                           gconstpointer b)
{
  const RamEntry *ea = a;
  const RamEntry *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;
  return FALSE;
}

static GObject *
gegl_tile_backend_ram_constructor (GType                  type,
                                   guint                  n_params,
                                   GObjectConstructParam *params)
{
  GObject     *object;
  GeglTileBackendRam *ram;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  ram    = GEGL_TILE_BACKEND_RAM (object);

  ram->entries = g_hash_table_new (hashfunc, equalfunc);

  return object;
}

static void
gegl_tile_backend_ram_class_init (GeglTileBackendRamClass *klass)
{
  GObjectClass    *gobject_class     = G_OBJECT_CLASS (klass);
  GeglTileSourceClass *gegl_tile_source_class = GEGL_TILE_SOURCE_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->constructor  = gegl_tile_backend_ram_constructor;
  gobject_class->finalize     = finalize;

  gegl_tile_source_class->command  = gegl_tile_backend_ram_command;
}

static void
gegl_tile_backend_ram_init (GeglTileBackendRam *self)
{
  self->entries = NULL;
}
