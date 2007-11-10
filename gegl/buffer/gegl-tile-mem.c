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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "gegl-tile-backend.h"
#include "gegl-tile-mem.h"
#include <string.h>
#include <stdio.h>

static void dbg_alloc (int size);
static void dbg_dealloc (int size);

/* These entries are kept in RAM for now, they should be written as an index to the
 * swap file, at a position specified by a header block, making the header grow up
 * to a multiple of the size used in this swap file is probably a good idea
 *
 * Serializing the bablformat is probably also a good idea.
 */
typedef struct _MemEntry MemEntry;

struct _MemEntry
{
  gint    x;
  gint    y;
  gint    z;
  guchar *offset;
};

static void inline
mem_entry_read (GeglTileMem *mem,
                MemEntry    *entry,
                guchar      *dest)
{
  gint tile_size = GEGL_TILE_BACKEND (mem)->tile_size;

  memcpy (dest, entry->offset, tile_size);
}

static void inline
mem_entry_write (GeglTileMem *mem,
                 MemEntry    *entry,
                 guchar      *source)
{
  gint tile_size = GEGL_TILE_BACKEND (mem)->tile_size;

  memcpy (entry->offset, source, tile_size);
}

static inline MemEntry *
mem_entry_new (GeglTileMem *mem)
{
  MemEntry *self = g_malloc (sizeof (MemEntry));

  self->offset = g_malloc (GEGL_TILE_BACKEND (mem)->tile_size);
  dbg_alloc (GEGL_TILE_BACKEND (mem)->tile_size);
  return self;
}

static inline void
mem_entry_destroy (MemEntry    *entry,
                   GeglTileMem *mem)
{
  g_free (entry->offset);
  g_hash_table_remove (mem->entries, entry);

  dbg_dealloc (GEGL_TILE_BACKEND (mem)->tile_size);
  g_free (entry);
}


G_DEFINE_TYPE (GeglTileMem, gegl_tile_mem, GEGL_TYPE_TILE_BACKEND)
static GObjectClass * parent_class = NULL;


static gint allocs        = 0;
static gint mem_size      = 0;
static gint peak_allocs   = 0;
static gint peak_mem_size = 0;

void gegl_tile_mem_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
             allocs, mem_size / 1024 / 1024.0, peak_allocs, peak_mem_size, peak_mem_size / 1024 / 1024.0);
}

static void dbg_alloc (gint size)
{
  allocs++;
  mem_size += size;
  if (allocs > peak_allocs)
    peak_allocs = allocs;
  if (mem_size > peak_mem_size)
    peak_mem_size = mem_size;
}

static void dbg_dealloc (gint size)
{
  allocs--;
  mem_size -= size;
}

static inline MemEntry *
lookup_entry (GeglTileMem *self,
              gint         x,
              gint         y,
              gint         z)
{
  MemEntry key = { x, y, z, 0 };

  return g_hash_table_lookup (self->entries, &key);
}

/* this is the only place that actually should
 * instantiate tiles, when the cache is large enough
 * that should make sure we don't hit this function
 * too often.
 */
static GeglTile *
get_tile (GeglProvider *tile_store,
          gint           x,
          gint           y,
          gint           z)
{
  GeglTileMem     *tile_mem = GEGL_TILE_MEM (tile_store);
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (tile_store);
  GeglTile        *tile     = NULL;

  {
    MemEntry *entry = lookup_entry (tile_mem, x, y, z);

    if (!entry)
      return NULL;

    tile             = gegl_tile_new (backend->tile_size);
    tile->stored_rev = 1;
    tile->rev        = 1;

    mem_entry_read (tile_mem, entry, tile->data);
  }
  return tile;
}

static
gboolean set_tile (GeglProvider *store,
                   GeglTile      *tile,
                   gint           x,
                   gint           y,
                   gint           z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileMem     *tile_mem = GEGL_TILE_MEM (backend);

  MemEntry        *entry = lookup_entry (tile_mem, x, y, z);

  if (entry == NULL)
    {
      entry    = mem_entry_new (tile_mem);
      entry->x = x;
      entry->y = y;
      entry->z = z;
      g_hash_table_insert (tile_mem->entries, entry, entry);
    }
  g_assert (tile->flags == 0); /* when this one is triggered, dirty pyramid data
                                  has been tried written to persistent storage.
                                */
  mem_entry_write (tile_mem, entry, tile->data);
  tile->stored_rev = tile->rev;
  return TRUE;
}

static
gboolean void_tile (GeglProvider *store,
                    GeglTile      *tile,
                    gint           x,
                    gint           y,
                    gint           z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileMem     *tile_mem = GEGL_TILE_MEM (backend);
  MemEntry        *entry    = lookup_entry (tile_mem, x, y, z);

  if (entry != NULL)
    {
      mem_entry_destroy (entry, tile_mem);
    }

  return TRUE;
}

static
gboolean exist_tile (GeglProvider *store,
                     GeglTile      *tile,
                     gint           x,
                     gint           y,
                     gint           z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileMem     *tile_mem = GEGL_TILE_MEM (backend);
  MemEntry        *entry    = lookup_entry (tile_mem, x, y, z);

  return entry != NULL;
}


enum
{
  PROP_0,
};

static gboolean
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
        return FALSE;

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

static void set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
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
  GeglTileMem *self = (GeglTileMem *) object;

  g_hash_table_unref (self->entries);

  (*G_OBJECT_CLASS (parent_class)->finalize)(object);
}

static guint hashfunc (gconstpointer key)
{
  const MemEntry *e = key;
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
  const MemEntry *ea = a;
  const MemEntry *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y &&
      ea->z == eb->z)
    return TRUE;
  return FALSE;
}

static GObject *
gegl_tile_mem_constructor (GType                  type,
                           guint                  n_params,
                           GObjectConstructParam *params)
{
  GObject     *object;
  GeglTileMem *mem;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  mem    = GEGL_TILE_MEM (object);

  mem->entries = g_hash_table_new (hashfunc, equalfunc);

  return object;
}

static void
gegl_tile_mem_class_init (GeglTileMemClass *klass)
{
  GObjectClass       *gobject_class         = G_OBJECT_CLASS (klass);
  GeglProviderClass *gegl_provider_class = GEGL_PROVIDER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->constructor  = gegl_tile_mem_constructor;
  gobject_class->finalize     = finalize;

  gegl_provider_class->get_tile = get_tile;
  gegl_provider_class->message  = message;
}

static void
gegl_tile_mem_init (GeglTileMem *self)
{
  self->entries = NULL;
}
