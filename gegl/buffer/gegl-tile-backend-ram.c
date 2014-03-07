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
 * Copyright 2013 Daniel Sabo <DanielSabo@gmail.com>
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-buffer-backend.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-backend-ram.h"

/* We need the private header so we can check the ref_count of tiles */
#include "gegl-buffer-private.h"

typedef struct _RamEntry RamEntry;

struct _RamEntry
{
  gint    x;
  gint    y;
  GeglTile *tile;
};

G_DEFINE_TYPE (GeglTileBackendRam, gegl_tile_backend_ram, GEGL_TYPE_TILE_BACKEND)
#define parent_class gegl_tile_backend_ram_parent_class

static GeglTile *get_tile   (GeglTileSource *tile_store,
                             gint             x,
                             gint             y,
                             gint             z);
static gboolean  set_tile   (GeglTileSource *store,
                             GeglTile       *tile,
                             gint            x,
                             gint            y,
                             gint            z);
static gboolean  exist_tile (GeglTileSource *store,
                             GeglTile       *tile,
                             gint            x,
                             gint            y,
                             gint            z);
static gboolean  void_tile  (GeglTileSource *store,
                             GeglTile       *tile,
                             gint            x,
                             gint            y,
                             gint            z);

void gegl_tile_backend_ram_stats (void)
{
  /* FIXME: Stats disabled for now as there's nothing meaningful to report */
}

static inline RamEntry *
lookup_entry (GeglTileBackendRam *self,
              gint                x,
              gint                y)
{
  RamEntry key;

  key.x = x;
  key.y = y;
  key.tile = NULL;

  return g_hash_table_lookup (self->entries, &key);
}

static GeglTile *
get_tile (GeglTileSource *tile_store,
          gint        x,
          gint        y,
          gint        z)
{
  GeglTileBackendRam *tile_backend_ram = GEGL_TILE_BACKEND_RAM (tile_store);

  if (z == 0)
    {
      RamEntry *entry = lookup_entry (tile_backend_ram, x, y);
      if (entry)
        return gegl_tile_ref (entry->tile);
    }

  return NULL;
}

static
gboolean set_tile (GeglTileSource *store,
                   GeglTile       *tile,
                   gint            x,
                   gint            y,
                   gint            z)
{
  GeglTileBackendRam *tile_backend_ram = GEGL_TILE_BACKEND_RAM (store);
  RamEntry           *entry  = NULL;
  gboolean            is_dup = FALSE;

  if (z != 0)
    return FALSE;

  entry = lookup_entry (tile_backend_ram, x, y);

  if (tile->ref_count == 0)
    {
      /* We've been handed a dead tile to store, this happens
       * when tile_unref is called on a tile that had never
       * been stored.
       * At this stage in tile_unref it's still safe to duplicate
       * the tile, which will prevent it's data from actually
       * being free'd.
       */
       tile = gegl_tile_dup (tile);

       /* The x,y,z values aren't copied by gegl_tile_dup */
       tile->x = x;
       tile->y = y;
       tile->z = z;

       is_dup = TRUE;
    }

  if (!entry)
    {
      entry = g_slice_new (RamEntry);
      entry->x = x;
      entry->y = y;
      entry->tile = NULL;
      g_hash_table_insert (tile_backend_ram->entries, entry, entry);
    }
  else if (entry->tile == tile)
    {
      gegl_tile_mark_as_stored (tile);
      return TRUE;
    }
  else
    {
      /* Mark as stored to prevent a recursive attempt to store by tile_unref */
      gegl_tile_mark_as_stored (entry->tile);
      gegl_tile_unref (entry->tile);
    }

  entry->tile = tile;

  if (!is_dup)
    gegl_tile_ref (entry->tile);

  gegl_tile_mark_as_stored (entry->tile);

  return TRUE;
}

static
gboolean void_tile (GeglTileSource *store,
                    GeglTile       *tile,
                    gint            x,
                    gint            y,
                    gint            z)
{
  GeglTileBackendRam *tile_backend_ram = GEGL_TILE_BACKEND_RAM (store);

  if (z == 0)
    {
      RamEntry *entry = lookup_entry (tile_backend_ram, x, y);

      if (entry != NULL)
        g_hash_table_remove (tile_backend_ram->entries, entry);
    }

  return TRUE;
}

static
gboolean exist_tile (GeglTileSource *store,
                     GeglTile       *tile,
                     gint            x,
                     gint            y,
                     gint            z)
{
  GeglTileBackendRam *tile_backend_ram = GEGL_TILE_BACKEND_RAM (store);

  if (z != 0)
    return FALSE;

  return lookup_entry (tile_backend_ram, x, y) != NULL;
}


enum
{
  PROP_0
};

static gpointer
gegl_tile_backend_ram_command (GeglTileSource  *tile_store,
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
        set_tile (tile_store, data, x, y, z);
        return NULL;

      case GEGL_TILE_IDLE:
        return NULL;

      case GEGL_TILE_VOID:
        void_tile (tile_store, data, x, y, z);
        return NULL;

      case GEGL_TILE_EXIST:
        return GINT_TO_POINTER (exist_tile (tile_store, data, x, y, z));

      default:
        g_assert (command < GEGL_TILE_LAST_COMMAND &&
                  command >= 0);
    }
  return NULL;
}

static void
set_property (GObject       *object,
              guint          property_id,
              const GValue  *value,
              GParamSpec    *pspec)
{
  switch (property_id)
    {
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
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
gegl_tile_backend_ram_finalize (GObject *object)
{
  GeglTileBackendRam *self = GEGL_TILE_BACKEND_RAM (object);

  g_hash_table_unref (self->entries);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static guint
ram_entry_hash_func (gconstpointer key)
{
  const RamEntry *e = key;
  guint           hash;
  gint            i;
  gint            srcA = e->x;
  gint            srcB = e->y;

  /* interleave the 16 least significant bits of the coordinates,
   * this gives us Z-order / morton order of the space and should
   * work well as a hash
   */
  hash = 0;
  for (i = 16; i >= 0; i--)
    {
#define ADD_BIT(bit)    do { hash |= (((bit) != 0) ? 1 : 0); hash <<= 1; \
    } \
  while (0)
      ADD_BIT (srcA & (1 << i));
      ADD_BIT (srcB & (1 << i));
#undef ADD_BIT
    }
  return hash;
}

static gboolean
ram_entry_equal_func (gconstpointer a,
                      gconstpointer b)
{
  const RamEntry *ea = a;
  const RamEntry *eb = b;

  if (ea->x == eb->x &&
      ea->y == eb->y)
    return TRUE;
  return FALSE;
}

static void
ram_entry_free_func (gpointer data)
{
  RamEntry *entry = (RamEntry *)data;

  if (entry->tile)
    {
      /* Mark as stored to prevent an attempt to store by tile_unref */
      gegl_tile_mark_as_stored (entry->tile);
      gegl_tile_unref (entry->tile);
      entry->tile = NULL;
    }
  g_slice_free (RamEntry, entry);
}

static void
gegl_tile_backend_ram_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  gegl_tile_backend_set_flush_on_destroy (GEGL_TILE_BACKEND (object), FALSE);
}

static void
gegl_tile_backend_ram_class_init (GeglTileBackendRamClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->constructed  = gegl_tile_backend_ram_constructed;
  gobject_class->finalize     = gegl_tile_backend_ram_finalize;
}

static void
gegl_tile_backend_ram_init (GeglTileBackendRam *self)
{
  GEGL_TILE_SOURCE (self)->command = gegl_tile_backend_ram_command;

  self->entries = g_hash_table_new_full (ram_entry_hash_func,
                                         ram_entry_equal_func,
                                         NULL,
                                         ram_entry_free_func);
}
