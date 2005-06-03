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

#include "gegl-cache.h"
#include "gegl-cache-entry.h"
#include "gegl-cache-store.h"
#include "gegl-entry-record.h"
#include "gegl-null-cache-store.h"


static void                 gegl_cache_class_init (GeglCacheClass   *klass);
static void                 gegl_cache_init       (GeglCache        *self);
static void                 dispose               (GObject          *object);
static GeglPutResults       put                   (GeglCache        *cache,
                                                   GeglCacheEntry   *entry,
                                                   gsize            *entry_id);
static GeglFetchResults     fetch                 (GeglCache        *cache,
                                                   gsize             entry_id,
                                                   GeglCacheEntry  **entry);
static GeglFetchResults     unfetch               (GeglCache        *cache,
                                                   gsize             entry_id,
                                                   GeglCacheEntry   *entry);
static  void                flush                 (GeglCache        *cache,
                                                   gsize             entry_id);
static void                 discard               (GeglCache        *cache,
                                                   GeglEntryRecord  *record);
static void                 mark_as_dirty         (GeglCache        *cache,
                                                   gsize             entry_id);



G_DEFINE_ABSTRACT_TYPE (GeglCache, gegl_cache, G_TYPE_OBJECT)


static void
gegl_cache_class_init (GeglCacheClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dispose;

  klass->put            = put;
  klass->fetch          = fetch;
  klass->unfetch        = unfetch;
  klass->flush          = flush;
  klass->discard        = discard;
  klass->mark_as_dirty  = mark_as_dirty;

  klass->insert_record  = NULL;
  klass->check_room_for = NULL;
  klass->size           = NULL;
  klass->capacity       = NULL;
  klass->is_persistent  = NULL;
  klass->flush_internal = NULL;
}

static void
gegl_cache_init (GeglCache *self)
{
  self->fetched_store = GEGL_CACHE_STORE (gegl_null_cache_store_new (GEGL_FETCHED));
  self->discard_store = GEGL_CACHE_STORE (gegl_null_cache_store_new (GEGL_DISCARDED));
}

static void
dispose (GObject *object)
{
  GeglCache *self = GEGL_CACHE (object);

  if (self->fetched_store)
    {
      g_object_unref (self->fetched_store);
      self->fetched_store = NULL;
    }

  if (self->discard_store)
    {
      g_object_unref (self->discard_store);
      self->discard_store = NULL;
    }

  G_OBJECT_CLASS (gegl_cache_parent_class)->dispose (object);
}

gint
gegl_cache_put (GeglCache      *cache,
		GeglCacheEntry *entry,
		gsize          *entry_id)
{
  GeglCacheClass *class;

  g_return_val_if_fail (GEGL_IS_CACHE (cache), GEGL_PUT_INVALID);
  g_return_val_if_fail (GEGL_IS_CACHE_ENTRY (entry), GEGL_PUT_INVALID);
  g_return_val_if_fail (entry_id != NULL, GEGL_PUT_INVALID);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_val_if_fail (class->put != NULL, GEGL_PUT_INVALID);

  return class->put (cache, entry, entry_id);
}

GeglFetchResults
gegl_cache_fetch (GeglCache       *cache,
		  gsize            entry_id,
		  GeglCacheEntry **entry)
{
  GeglCacheClass *class;

  g_return_val_if_fail (GEGL_IS_CACHE (cache), GEGL_FETCH_INVALID);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_val_if_fail (class->fetch != NULL, GEGL_FETCH_INVALID);

  return class->fetch (cache, entry_id, entry);
}

GeglFetchResults
gegl_cache_unfetch (GeglCache      *cache,
                    gsize           entry_id,
                    GeglCacheEntry *entry)
{
  GeglCacheClass *class;

  g_return_val_if_fail (GEGL_IS_CACHE (cache), GEGL_FETCH_INVALID);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_val_if_fail (class->unfetch != NULL, GEGL_FETCH_INVALID);

  return class->unfetch (cache, entry_id, entry);
}

void
gegl_cache_mark_as_dirty (GeglCache *cache,
                          gsize      entry_id)
{
  GeglCacheClass *class;

  g_return_if_fail (GEGL_IS_CACHE (cache));

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_if_fail (class->mark_as_dirty != NULL);

  class->mark_as_dirty (cache, entry_id);
}

void
gegl_cache_insert_record (GeglCache       *cache,
                          GeglEntryRecord *record)
{
  GeglCacheClass *class;

  g_return_if_fail (GEGL_IS_CACHE (cache));

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_if_fail (class->insert_record != NULL);

  class->insert_record (cache, record);
}

void
gegl_cache_flush (GeglCache *cache,
                  gsize      entry_id)
{
  GeglCacheClass *class;

  g_return_if_fail (GEGL_IS_CACHE (cache));

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_if_fail (class->flush != NULL);

  class->flush (cache, entry_id);
}

void
gegl_cache_flush_internal (GeglCache       *cache,
                           GeglEntryRecord *record)
{
  GeglCacheClass *class;

  g_return_if_fail (cache != NULL);
  g_return_if_fail (record != NULL);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_if_fail (class->flush_internal != NULL);

  class->flush_internal (cache, record);
}

gboolean
gegl_cache_check_room_for (GeglCache *cache,
                           gint64     size)
{
  GeglCacheClass *class;

  g_return_val_if_fail (cache != NULL, FALSE);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_val_if_fail (class->check_room_for != NULL, FALSE);

  return class->check_room_for (cache, size);
}

gint64
gegl_cache_size (GeglCache* cache)
{
  GeglCacheClass *class;

  g_return_val_if_fail (cache !=NULL, 0);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_val_if_fail (class->size != NULL, 0);

  return class->size (cache);
}

gint64
gegl_cache_capacity (GeglCache *cache)
{
  GeglCacheClass *class;

  g_return_val_if_fail (cache != NULL, 0);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_val_if_fail (class->capacity != NULL, 0);

  return class->capacity (cache);
}

gboolean
gegl_cache_is_persistent (GeglCache *cache)
{
  GeglCacheClass *class;

  g_return_val_if_fail (cache != NULL, FALSE);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_val_if_fail (class->is_persistent != NULL, FALSE);

  return class->is_persistent(cache);
}

void
gegl_cache_discard (GeglCache       *cache,
                    GeglEntryRecord *record)
{
  GeglCacheClass *class;

  g_return_if_fail (cache != NULL);
  g_return_if_fail (record != NULL);
  g_return_if_fail (record->cache == cache);

  class = GEGL_CACHE_GET_CLASS (cache);

  g_return_if_fail (class->discard != NULL);

  class->discard (cache, record);
}

static GeglPutResults
put (GeglCache      *cache,
     GeglCacheEntry *entry,
     gsize          *entry_id)
{

  GeglEntryRecord *record;
  gsize            new_size = gegl_cache_entry_flattened_size (entry);

  if (! gegl_cache_check_room_for (cache, new_size))
    return GEGL_PUT_CACHE_FULL;

  if (*entry_id != 0)
    {
      g_error ("put called with non-zero entry_id.");
      return GEGL_PUT_INVALID;
    }

  record = gegl_entry_record_new (cache, entry);

  *entry_id = GPOINTER_TO_SIZE (record);

  gegl_cache_insert_record (cache, record);

  return GEGL_PUT_SUCCEEDED;
}

static GeglFetchResults
fetch (GeglCache       *cache,
       gsize            entry_id,
       GeglCacheEntry **entry_handle)
{

  GeglEntryRecord *record = GSIZE_TO_POINTER (entry_id);
  GeglCacheEntry  *entry  = record->entry;

  g_return_val_if_fail (record->cache == cache,GEGL_FETCH_INVALID);

  *entry_handle = NULL;

  if (record->status == GEGL_DISCARDED)
    return GEGL_FETCH_EXPIRED;

  g_object_ref (entry);

  g_return_val_if_fail (entry != NULL, GEGL_FETCH_INVALID);

  gegl_cache_store_remove (record->store, record);

  gegl_entry_record_set_cache_store (record, cache->fetched_store);
  gegl_cache_store_add (cache->fetched_store, record);

  *entry_handle=entry;

  return GEGL_FETCH_SUCCEEDED;
}

static GeglFetchResults
unfetch (GeglCache      *cache,
	 gsize           entry_id,
	 GeglCacheEntry *entry)
{
  GeglEntryRecord *record = GSIZE_TO_POINTER (entry_id);

  g_return_val_if_fail (record->cache == cache, GEGL_FETCH_INVALID);
  g_return_val_if_fail (record->status == GEGL_FETCHED, GEGL_FETCH_INVALID);

  g_object_ref (entry);

  record->entry = entry;

  gegl_cache_store_remove (record->store, record);
  gegl_entry_record_set_cache_store (record, NULL);
  gegl_cache_insert_record (cache, record);

  return GEGL_FETCH_SUCCEEDED;
}

static void
flush (GeglCache *cache,
       gsize      entry_id)
{
  GeglEntryRecord *record;

  g_return_if_fail (entry_id != 0);

  record = GSIZE_TO_POINTER (entry_id);

  gegl_cache_flush_internal (cache, record);
  gegl_cache_store_remove (record->store, record);
  gegl_entry_record_set_cache_store (record, NULL);
  gegl_entry_record_free (record);
}

static void
discard (GeglCache       *cache,
	 GeglEntryRecord *record)
{

  gegl_cache_store_remove (record->store, record);
  gegl_entry_record_set_cache_store (record, cache->discard_store);
  gegl_cache_store_add (cache->discard_store, record);
}

static void
mark_as_dirty (GeglCache *cache,
               gsize      entry_id)
{
  GeglEntryRecord *record;

  g_return_if_fail (entry_id != 0);

  record = GSIZE_TO_POINTER (entry_id);

  gegl_entry_record_dirty (record);
}
