/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003-2004 Daniel S. Rogers
 *
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-image-types.h"

#include "gegl-cache-entry.h"
#include "gegl-heap-cache-store.h"
#include "gegl-entry-record.h"


static void gegl_heap_cache_store_class_init (GeglHeapCacheStoreClass *klass);
static void gegl_heap_cache_store_init       (GeglHeapCacheStore      *self);

static void             dispose       (GObject         *object);
static void             finalize      (GObject         *object);

static void             add           (GeglCacheStore  *self,
                                       GeglEntryRecord *record);
static void             remove        (GeglCacheStore  *self,
                                       GeglEntryRecord *record);
static void             zap           (GeglCacheStore  *self,
                                       GeglEntryRecord *record);
static gint64           size          (GeglCacheStore  *self);
static GeglEntryRecord *pop           (GeglCacheStore  *self);
static GeglEntryRecord *peek          (GeglCacheStore  *self);


G_DEFINE_TYPE (GeglHeapCacheStore, gegl_heap_cache_store, GEGL_TYPE_CACHE_STORE)


static void
gegl_heap_cache_store_class_init (GeglHeapCacheStoreClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GeglCacheStoreClass *store_class  = GEGL_CACHE_STORE_CLASS (klass);

  object_class->dispose  = dispose;
  object_class->finalize = finalize;

  store_class->add       = add;
  store_class->remove    = remove;
  store_class->zap       = zap;
  store_class->size      = size;
  store_class->pop       = pop;
  store_class->peek      = peek;
}

static void
gegl_heap_cache_store_init (GeglHeapCacheStore *self)
{
  self->size        = 0;
  self->record_head = NULL;
}

static void
dispose (GObject * object)
{
  GeglHeapCacheStore * self = GEGL_HEAP_CACHE_STORE (object);

  if (self->record_head)
    {
      g_list_foreach (self->record_head, (GFunc) gegl_entry_record_free, NULL);
      g_list_free (self->record_head);
      self->record_head = NULL;
    }

  G_OBJECT_CLASS (gegl_heap_cache_store_parent_class)->dispose (object);
}

static void
finalize (GObject * object)
{
  G_OBJECT_CLASS (gegl_heap_cache_store_parent_class)->finalize (object);
}

GeglHeapCacheStore *
gegl_heap_cache_store_new (void)
{
  return g_object_new (GEGL_TYPE_HEAP_CACHE_STORE, NULL);
}

static void
add (GeglCacheStore  *store,
     GeglEntryRecord *record)
{
  GeglHeapCacheStore *self = GEGL_HEAP_CACHE_STORE (store);
  GList              *record_list;

  gegl_entry_record_set_cache_store (record, store);

  record_list = g_list_append (NULL, record);

  self->record_head = g_list_concat (self->record_head, record_list);

  gegl_entry_record_add_store_data (record, store, record_list);

  self->size += gegl_cache_entry_flattened_size (record->entry);

  record->status = GEGL_STORED;
}

static void
remove (GeglCacheStore  *store,
        GeglEntryRecord *record)
{
  GeglHeapCacheStore *self = GEGL_HEAP_CACHE_STORE (store);
  GList              *record_list;

  record_list = gegl_entry_record_get_store_data (record, store);

  self->record_head = g_list_delete_link (self->record_head, record_list);

  gegl_entry_record_remove_store_data (record, store, FALSE);
  gegl_entry_record_set_cache_store (record, NULL);
  self->size -= gegl_cache_entry_flattened_size (record->entry);

  record->status = GEGL_UNDEFINED;
}

static void
zap (GeglCacheStore  *store,
     GeglEntryRecord *record)
{
  remove (store, record);

  gegl_entry_record_free (record);
}

static gint64
size (GeglCacheStore *store)
{
  GeglHeapCacheStore *self = GEGL_HEAP_CACHE_STORE (store);

  return self->size;
}

static GeglEntryRecord *
pop (GeglCacheStore *store)
{
  GeglEntryRecord *record = peek (store);

  if (record)
    remove (store, record);

  return record;
}

static GeglEntryRecord *
peek (GeglCacheStore *store)
{
  GeglHeapCacheStore *self = GEGL_HEAP_CACHE_STORE (store);
  GeglEntryRecord    *record;

  if (! self->record_head)
    return NULL;

  record = self->record_head->data;

  g_return_val_if_fail (record != NULL, NULL);

  return record;
}
