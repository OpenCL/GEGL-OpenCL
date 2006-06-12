/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>
#include <glib/gprintf.h>
#include <glib/gstdio.h>
#include "gegl-tile-backend.h"
#include "gegl-tile-mem.h"
#include <string.h>
#include <stdio.h>

G_DEFINE_TYPE(GeglTileMem, gegl_tile_mem, GEGL_TYPE_TILE_BACKEND)
static GObjectClass *parent_class = NULL;


static gint allocs=0;
static gint mem_size=0;
static gint max_allocs=0;
static gint max_mem_size=0;

void gegl_tile_mem_stats (void)
{
  g_warning ("leaked: %i chunks (%f mb)  peak: %i (%i bytes %fmb))",
     allocs, mem_size/1024/1024.0, max_allocs, max_mem_size, max_mem_size/1024/1024.0);
}

static void dbg_alloc(int size)
{
  allocs++;
  mem_size+=size;
  if (allocs>max_allocs)
    max_allocs=allocs;
  if (mem_size>max_mem_size)
    max_mem_size=mem_size;
}
static void dbg_dealloc(int size)
{
  allocs--;
  mem_size-=size;
}



typedef struct _MemEntry MemEntry;
typedef struct _MemBank MemBank;
typedef struct _MemPriv MemPriv;

struct _MemEntry {
  gint    x;
  gint    y;
  gint    z;
  gint    size;
  guchar *offset; 
};

static MemEntry *mem_entry_new (void)
{
  MemEntry *self = g_malloc (sizeof (MemEntry));
  self->offset = NULL;
  return self;
}

static void mem_entry_destroy (gpointer self, gpointer foo)
{
  MemEntry *entry = (MemEntry*)self;
  if (entry->offset!= NULL)
    g_free (entry->offset);
  dbg_dealloc (entry->size);
  g_free (self);
}

struct _MemBank {
  guchar  *base;
  GSList  *entries;
};

static MemBank *mem_bank_new (void)
{
  MemBank *self = g_malloc (sizeof (MemBank));
  self->base = 0;
  self->entries = NULL;
  return self;
}

static void mem_bank_destroy (gpointer self, gpointer foo)
{
  MemBank *bank = (MemBank*)self;
  g_slist_foreach (bank->entries, mem_entry_destroy, NULL);
  if (bank->base)
    g_free (bank->base);
  g_free (self);
}

struct _MemPriv {
  GSList *banks;
};

static MemPriv *mem_priv_new (void)
{
  MemPriv *self = g_malloc (sizeof (MemPriv));
  self->banks = g_slist_append (NULL, mem_bank_new ());
  return self;
}

static void mem_priv_destroy (MemPriv *self)
{
  g_slist_foreach (self->banks, mem_bank_destroy, NULL);
  g_free (self);
}

static guchar * mem_priv_get_tile (MemPriv *priv,
                                   gint     x,
                                   gint     y,
                                   gint     z)
{
  GSList *banks = priv->banks;
  while (banks)
    {
      MemBank *bank    = banks->data;
      GSList  *entries = bank->entries;

      while (entries)
        {
          MemEntry *entry = entries->data;
          if (entry->x == x &&
              entry->y == y &&
              entry->z == z)
            return entry->offset;

          entries = entries->next;
        }
      banks=banks->next;
    }
  return NULL;
}

static gboolean mem_priv_set_tile (MemPriv *priv,
                                   gint     x,
                                   gint     y,
                                   gint     z,
                                   guchar  *data,
                                   gint     size)
{
  MemEntry *entry = NULL;
  GSList *banks = priv->banks;
  MemBank *bank = banks->data;

  while (banks)
    {
      MemBank *bank    = banks->data;
      GSList  *entries = bank->entries;

      while (entries)
        {
          entry = entries->data;
          if (entry->x == x &&
              entry->y == y &&
              entry->z == z)
            {
              goto search_done;
            }

          entries = entries->next;
        }
      banks=banks->next;
      entry = NULL;       /* goto jumps across this */
    }
  search_done:
  
  if (entry==NULL)
    {
      entry = mem_entry_new ();
      entry->x=x;
      entry->y=y;
      entry->z=z;
      entry->offset = g_malloc (size);
      entry->size = size;
      bank->entries = g_slist_prepend (bank->entries, entry);
      dbg_alloc (size);
    }
  memcpy (entry->offset, data, size);
  return TRUE;
}
  
static gboolean mem_priv_void_tile (MemPriv *priv,
                                    gint     x,
                                    gint     y,
                                    gint     z)
{
  MemEntry *entry = NULL;
  GSList *banks = priv->banks;
  MemBank *bank = banks->data;

  while (banks)
    {
      MemBank *bank    = banks->data;
      GSList  *entries = bank->entries;

      while (entries)
        {
          entry = entries->data;
          if (entry->x == x &&
              entry->y == y &&
              entry->z == z)
            {
              goto search_done;
            }

          entries = entries->next;
        }
      banks=banks->next;
      entry = NULL;       /* goto jumps across this */
    }
  search_done:
  
  if (entry!=NULL)
    {
      bank->entries = g_slist_remove (bank->entries, entry);
      mem_entry_destroy (entry, NULL);
    }
  return TRUE;
}
  
/* this is the only place that actually should
 * instantiate tiles, when the cache is large enough
 * that should make sure we don't hit this function
 * too often.
 */
static GeglTile *
get_tile (GeglTileStore *tile_store,
          gint          x,
          gint          y,
          gint          z)
{
  GeglTileMem *tile_mem = GEGL_TILE_MEM (tile_store);
  GeglTileBackend *backend = GEGL_TILE_BACKEND (tile_store);
  GeglTile *tile = NULL;
  
    {
      guchar *data = mem_priv_get_tile ((MemPriv*)tile_mem->priv, x, y, z);
      if (!data)
        return NULL;

      tile = gegl_tile_new (backend->tile_size);
      tile->stored_rev = 1;
      tile->rev = 1;
      memcpy (tile->data, data, backend->tile_size);
    }
  return tile;
}

static
gboolean set_tile (GeglTileStore *store,
                   GeglTile      *tile,
                   gint           x,
                   gint           y,
                   gint           z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileMem     *tile_mem = GEGL_TILE_MEM (backend);

  mem_priv_set_tile ((MemPriv*)tile_mem->priv, x, y, z,
                     tile->data, backend->tile_size);
  return TRUE;
}

static
gboolean void_tile (GeglTileStore *store,
                   GeglTile      *tile,
                   gint           x,
                   gint           y,
                   gint           z)
{
  GeglTileBackend *backend  = GEGL_TILE_BACKEND (store);
  GeglTileMem     *tile_mem = GEGL_TILE_MEM (backend);

  mem_priv_void_tile ((MemPriv*)tile_mem->priv, x, y, z);
  return TRUE;
}

enum {
  PROP_0,
  PROP_PATH
};

static gboolean
message (GeglTileStore   *tile_store,
         GeglTileMessage  message,
         gint             x,
         gint             y,
         gint             z,
         gpointer         data)
{
  switch (message)
    {
      case GEGL_TILE_SET:
        return set_tile (tile_store, data, x, y, z);
      case GEGL_TILE_IDLE:
        return FALSE;
      case GEGL_TILE_IS_DIRTY:
        return FALSE;
      case GEGL_TILE_VOID:
        return void_tile (tile_store, data, x, y, z);
        break;
      default:
        g_assert (message <  GEGL_TILE_LAST_MESSAGE &&
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

static void get_property (GObject      *object,
                          guint         property_id,
                          GValue       *value,
                          GParamSpec   *pspec)
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
  GeglTileMem *tile_mem = (GeglTileMem *) object;
  if (tile_mem->priv)
    {
      mem_priv_destroy ((MemPriv*)tile_mem->priv);
      tile_mem->priv = NULL;
    }
  (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


static void
gegl_tile_mem_class_init (GeglTileMemClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglTileStoreClass *gegl_tile_store_class = GEGL_TILE_STORE_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  gobject_class->get_property = get_property;
  gobject_class->set_property = set_property;
  gobject_class->finalize = finalize;
  
  gegl_tile_store_class->get_tile = get_tile;
  gegl_tile_store_class->message  = message;
}

static void
gegl_tile_mem_init (GeglTileMem *self)
{
  self->priv = (void*)mem_priv_new ();
}
