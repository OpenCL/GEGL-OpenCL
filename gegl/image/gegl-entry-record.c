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

#include "gegl-entry-record.h"
#include "gegl-cache-entry.h"
#include "gegl-cache.h"

typedef struct _GeglStoreData GeglStoreData;
struct _GeglStoreData
{
  gpointer data;
  GeglStoreDataFunc free;
  GeglStoreDataFunc dirty;
};

static GeglStoreData * gegl_store_data_new (void * data,
					    GeglStoreDataFunc free,
					    GeglStoreDataFunc dirty);
static void gegl_store_data_free (GeglStoreData * sdata, GeglEntryRecord* record, GeglCacheStore * cache_store,  gboolean free_data);
static void store_weak_notify (gpointer data, GObject * where_the_object_was);
static gboolean
free_and_remove (gpointer key, gpointer value, gpointer user_data)
{
  GeglCacheStore * cache_store = key;
  GeglStoreData * sdata = value;
  GeglEntryRecord * record = (GeglEntryRecord *) user_data;
  g_object_weak_unref (G_OBJECT(cache_store), store_weak_notify, record);
  if (sdata != NULL)
    {
      gegl_store_data_free(sdata, record, cache_store, TRUE);
    }
  return TRUE;
}

void
gegl_entry_record_free (GeglEntryRecord * record)
{
  gpointer * ref;
  if (record->entry != NULL)
    {
      /* The refrences to record->cache and record->cache_store
       are weak.*/
      g_object_unref(record->entry);
    }

  if (record->cache != NULL)
    {
      ref =  (gpointer *)&(record->cache);
      g_object_remove_weak_pointer (G_OBJECT (record->cache), ref);
    }
  if (record->store != NULL)
    {
      ref = (gpointer *)&(record->store);
      g_object_remove_weak_pointer (G_OBJECT (record->store), ref);
     }
  g_hash_table_foreach_remove (record->store_data, free_and_remove, record);
  g_hash_table_destroy(record->store_data);
  g_free (record);
}

GeglEntryRecord *
gegl_entry_record_new (GeglCache * cache,
		       GeglCacheEntry* entry)
{
  /*g_return_val_if_fail (cache != NULL, NULL);*/
  g_return_val_if_fail (entry != NULL, NULL);
  GeglEntryRecord * record;
  record = g_new (GeglEntryRecord, 1);
  record->cache = cache;
  if (cache != NULL)
    {
      g_object_add_weak_pointer (G_OBJECT (cache),(gpointer *)&(record->cache));
    }
  record->store = NULL;
  record->status = GEGL_UNDEFINED;
  g_object_ref (entry);
  record->entry = entry;
  record->store_data = g_hash_table_new (g_direct_hash, g_direct_equal);
  return record;
}

void
gegl_entry_record_set_cache (GeglEntryRecord * record,
			     GeglCache * cache)
{
  if (record->cache != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (record->cache), (gpointer *)&(record->cache));
    }
  record->cache = cache;
  g_object_add_weak_pointer (G_OBJECT (cache), (gpointer *)&(record->cache));
  
}

void
gegl_entry_record_set_cache_store (GeglEntryRecord * record,
				   GeglCacheStore * store)
{
  if (record->store != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT(record->store), (gpointer *)&(record->store));
    }
  record->store = store;
  if (record->store != NULL)
    {
      g_object_add_weak_pointer (G_OBJECT(store), (gpointer *)&(record->store));
    }
}

static GeglStoreData *
gegl_store_data_new (void * data,
		     GeglStoreDataFunc free,
		     GeglStoreDataFunc dirty)
{
  GeglStoreData * sdata = g_new (GeglStoreData, 1);
  sdata->free = free;
  sdata->dirty = dirty;
  sdata->data = data;
  return sdata;
}

static void
gegl_store_data_free (GeglStoreData * sdata, GeglEntryRecord * record, GeglCacheStore * cache_store, gboolean free_data)
{
  if (free_data && sdata->free != NULL)
    {
      sdata->free (cache_store, record, sdata->data);
    }
  g_free (sdata);
}

void
gegl_entry_record_add_store_data (GeglEntryRecord * record,
				  struct _GeglCacheStore * store,
				  gpointer data)
{
  gegl_entry_record_add_store_data_full (record, store, data, NULL, NULL);
}

void
gegl_entry_record_add_store_data_full (GeglEntryRecord * record,
				       struct _GeglCacheStore * store,
				       gpointer data,
				       GeglStoreDataFunc free_data,
				       GeglStoreDataFunc dirty)
{
  GeglStoreData * sdata = gegl_store_data_new (data, free_data, dirty); 
  g_hash_table_insert (record->store_data, store, sdata);
  g_object_weak_ref (G_OBJECT(store), store_weak_notify, record);
}

void  
gegl_entry_record_remove_store_data (GeglEntryRecord * record,
				     struct _GeglCacheStore * store,
				     gboolean free_data)
{
  GeglStoreData * data;
  data = g_hash_table_lookup (record->store_data, store);
  g_object_weak_unref (G_OBJECT(store), store_weak_notify, record);
  gegl_store_data_free (data, record, store, free_data);
  g_hash_table_steal (record->store_data, store);
}

gpointer
gegl_entry_record_get_store_data (GeglEntryRecord * record,
				  struct _GeglCacheStore * store)
{
  g_return_val_if_fail (record != NULL, NULL);
  GeglStoreData * data;
  data = g_hash_table_lookup (record->store_data, store);
  if (data == NULL)
    {
      return NULL;
    }
  return data->data;
}

void foreach_dirty (gpointer key, gpointer value, gpointer user_data)
{
  struct _GeglCacheStore * store = (struct _GeglCacheStore *) key;
  GeglStoreData * sdata = (GeglStoreData *)value;
  GeglEntryRecord * record = (GeglEntryRecord*)user_data;
  if (sdata->dirty != NULL)
    {
      sdata->dirty (store, record, sdata->data);
    }
}

void
gegl_entry_record_dirty (GeglEntryRecord * record)
{
  g_hash_table_foreach (record->store_data, foreach_dirty, record);
}

static void
store_weak_notify (gpointer data, GObject * where_the_object_was)
{
  GeglStoreData * sdata;
  GeglEntryRecord * record = (GeglEntryRecord*)data;
  GeglCacheStore * finalized_store = (GeglCacheStore*)where_the_object_was;
  sdata = g_hash_table_lookup (record->store_data, where_the_object_was);
  if (sdata != NULL)
    {
      gegl_store_data_free (sdata, record, finalized_store, TRUE);
    }
}
