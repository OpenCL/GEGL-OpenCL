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

#include "gegl-cache.h"
#include "gegl-cache-store.h"
#include "gegl-null-cache-store.h"

static void class_init(gpointer g_class,
                       gpointer class_data);
static void instance_init(GTypeInstance *instance,
                          gpointer g_class);
static void dispose (GObject * object);
static void finalize (GObject * object);
static GeglPutResults put (GeglCache * cache,
			   GeglCacheEntry * entry,
			   gsize * entry_id);
static GeglFetchResults fetch (GeglCache * cache,
			       gsize entry_id,
			       GeglCacheEntry ** entry);
static GeglFetchResults unfetch (GeglCache* cache,
				 gsize entry_id,
				 GeglCacheEntry * entry);
static  void flush (GeglCache * cache,
		    gsize entry_id);
static void discard (GeglCache * cache,
		     GeglEntryRecord * record);
static void mark_as_dirty (GeglCache * cache,
			   gsize entry_id);

static gpointer parent_class;

GType
gegl_cache_get_type (void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof (GeglCacheClass),
	  NULL, /*base_init*/
	  NULL, /* base_finalize */
	
	  /* classed types, instantiated types */
	  class_init, /* class_init */
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	
	  /* instantiated types */
	  sizeof(GeglCache),
	  0, /* n_preallocs */
	  instance_init, /* instance_init */
	
	  /* value handling */
	  NULL /* value_table */
	};

      type = g_type_register_static (G_TYPE_OBJECT ,
				     "GeglCache",
				     &typeInfo,
				     G_TYPE_FLAG_ABSTRACT);
    }
  return type;
}

static void
class_init(gpointer g_class,
	   gpointer class_data)
{
  GeglCacheClass * class = GEGL_CACHE_CLASS(g_class);
  GObjectClass * object_class = G_OBJECT_CLASS (g_class);

  class->put = put;
  class->fetch = fetch;
  class->unfetch = unfetch;
  class->flush = flush;
  class->discard = discard;
  class->mark_as_dirty = mark_as_dirty;

  class->insert_record=NULL;
  class->check_room_for = NULL;
  class->size = NULL;
  class->capacity = NULL;
  class->is_persistent = NULL;
  class->flush_internal = NULL;

  object_class->finalize = finalize;
  object_class->dispose = dispose;

  parent_class = g_type_class_peek_parent (g_class);
}

static void
instance_init(GTypeInstance *instance,
	      gpointer g_class)
{
  GeglCache * self = GEGL_CACHE(instance);
  self->fetched_store = GEGL_CACHE_STORE(gegl_null_cache_store_new (GEGL_FETCHED));
  self->discard_store = GEGL_CACHE_STORE(gegl_null_cache_store_new (GEGL_DISCARDED));
  self->has_disposed = FALSE;
}

static void
dispose (GObject * object)
{
  GeglCache * self = GEGL_CACHE(object);
  if (!self->has_disposed)
    {
      self->has_disposed = TRUE;
      g_object_unref (self->fetched_store);
      g_object_unref (self->discard_store);
    }
}
static void
finalize (GObject * object)
{
  G_OBJECT_CLASS(parent_class)->finalize(object);
}

gint
gegl_cache_put (GeglCache * cache,
		GeglCacheEntry * entry,
		gsize * entry_id)
{
  g_return_val_if_fail (GEGL_IS_CACHE(cache), GEGL_PUT_INVALID);
  g_return_val_if_fail (GEGL_IS_CACHE_ENTRY(entry), GEGL_PUT_INVALID);
  g_return_val_if_fail (entry_id != NULL, GEGL_PUT_INVALID);

  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->put != NULL, GEGL_PUT_INVALID);
  return class->put (cache, entry, entry_id);
}
GeglFetchResults
gegl_cache_fetch (GeglCache * cache,
		  gsize entry_id,
		  GeglCacheEntry ** entry)
{
  g_return_val_if_fail (GEGL_IS_CACHE(cache), GEGL_FETCH_INVALID);

  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->fetch != NULL, GEGL_FETCH_INVALID);
  return class->fetch (cache, entry_id, entry);
}
GeglFetchResults
gegl_cache_unfetch (GeglCache * cache,
		  gsize entry_id,
		  GeglCacheEntry * entry)
{
  g_return_val_if_fail (GEGL_IS_CACHE(cache), GEGL_FETCH_INVALID);

  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->unfetch != NULL, GEGL_FETCH_INVALID);
  return class->unfetch (cache, entry_id, entry);
}
void gegl_cache_mark_as_dirty (GeglCache * cache,
			       gsize entry_id)
{
  g_return_if_fail (GEGL_IS_CACHE(cache));
  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_if_fail (class->mark_as_dirty != NULL);
  class->mark_as_dirty(cache, entry_id);
}

void gegl_cache_insert_record (GeglCache* cache,
			       GeglEntryRecord* record)
{
  g_return_if_fail (GEGL_IS_CACHE(cache));
  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_if_fail (class->insert_record != NULL);
  class->insert_record(cache, record);
}

void gegl_cache_flush (GeglCache * cache,
		       gsize entry_id)
{
  g_return_if_fail (GEGL_IS_CACHE(cache));
  GeglCacheClass * class=GEGL_CACHE_GET_CLASS (cache);
  g_return_if_fail (class->flush != NULL);
  class->flush (cache, entry_id);
}

void gegl_cache_flush_internal (GeglCache * cache,
				GeglEntryRecord * record)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (record != NULL);
  GeglCacheClass * class = GEGL_CACHE_GET_CLASS (cache);
  g_return_if_fail (class->flush_internal != NULL);
  class->flush_internal (cache, record);
}

gboolean
gegl_cache_check_room_for (GeglCache* cache, gint64 size)
{
  g_return_val_if_fail (cache != NULL, FALSE);
  GeglCacheClass * class = GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->check_room_for != NULL, FALSE);
  return class->check_room_for (cache, size);
}

gint64
gegl_cache_size (GeglCache* cache)
{
  g_return_val_if_fail (cache !=NULL, 0);
  GeglCacheClass * class = GEGL_CACHE_GET_CLASS (cache);

  g_return_val_if_fail (class->size != NULL, 0);
  return class->size (cache);
}

gint64
gegl_cache_capacity (GeglCache* cache)
{
  GeglCacheClass * class;
  g_return_val_if_fail (cache != NULL, 0);
  class = GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->capacity != NULL, 0);
  return class->capacity (cache);
}

gboolean
gegl_cache_is_persistent (GeglCache* cache)
{
  GeglCacheClass * class;
  g_return_val_if_fail (cache != NULL, FALSE);
  class = GEGL_CACHE_GET_CLASS (cache);
  g_return_val_if_fail (class->is_persistent != NULL, FALSE);
  return class->is_persistent(cache);
}

void
gegl_cache_discard (GeglCache * cache, GeglEntryRecord * record)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (record != NULL);
  g_return_if_fail (record->cache == cache);

  GeglCacheClass * class = GEGL_CACHE_GET_CLASS (cache);
  g_return_if_fail (class->discard != NULL);
  class->discard (cache, record);
}

static GeglPutResults
put (GeglCache * cache,
     GeglCacheEntry * entry,
     gsize * entry_id)
{

  GeglEntryRecord* record;
  gsize new_size=gegl_cache_entry_flattened_size (entry);
  gboolean has_room_for;

  has_room_for = gegl_cache_check_room_for (cache, new_size);

  if (!has_room_for)
    {
      return GEGL_PUT_CACHE_FULL;
    }

  if (*entry_id != 0)
    {
      g_error("put called with non-zero entry_id.");
      return GEGL_PUT_INVALID;
    }
  record = gegl_entry_record_new (cache, entry);
  *entry_id = GPOINTER_TO_SIZE(record);
  gegl_cache_insert_record (cache, record);

  return GEGL_PUT_SUCCEEDED;

}

static GeglFetchResults
fetch (GeglCache * cache,
       gsize entry_id,
       GeglCacheEntry ** entry_handle)
{

  GeglEntryRecord * record = GSIZE_TO_POINTER (entry_id);
  GeglCacheEntry* entry = record->entry;
  g_return_val_if_fail (record->cache == cache,GEGL_FETCH_INVALID);
  *entry_handle = NULL;
  if (record->status == GEGL_DISCARDED)
    {
      return GEGL_FETCH_EXPIRED;
    }
  g_object_ref (entry);

  g_return_val_if_fail (entry != NULL, GEGL_FETCH_INVALID);
  gegl_cache_store_remove (record->store, record);

  gegl_entry_record_set_cache_store(record, cache->fetched_store);
  gegl_cache_store_add (cache->fetched_store, record);

  *entry_handle=entry;
  return GEGL_FETCH_SUCCEEDED;
}

static GeglFetchResults
unfetch (GeglCache* cache,
	 gsize entry_id,
	 GeglCacheEntry * entry)
{
  GeglEntryRecord * record = GSIZE_TO_POINTER (entry_id);

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
flush (GeglCache * cache,
       gsize entry_id)
{
  GeglEntryRecord * record;
  g_return_if_fail (entry_id != 0);

  record = GSIZE_TO_POINTER (entry_id);
  gegl_cache_flush_internal (cache, record);
  gegl_cache_store_remove (record->store, record);
  gegl_entry_record_set_cache_store (record, NULL);
  gegl_entry_record_free (record);
}


static void
discard (GeglCache * cache,
	 GeglEntryRecord * record)
{

  gegl_cache_store_remove (record->store, record);
  gegl_entry_record_set_cache_store(record, cache->discard_store);
  gegl_cache_store_add(cache->discard_store, record);
}

static void
mark_as_dirty (GeglCache * cache, gsize entry_id)
{
  GeglEntryRecord * record;
  g_return_if_fail (entry_id != 0);
  record = GSIZE_TO_POINTER (entry_id);
  gegl_entry_record_dirty (record);
}
