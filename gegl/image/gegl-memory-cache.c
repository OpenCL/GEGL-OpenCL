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

#include "gegl-memory-cache.h"

static void class_init(gpointer g_class,
		       gpointer class_data);
static void instance_init(GTypeInstance *instance,
                          gpointer g_class);
static void finalize(GObject *object);
static void dispose (GObject *object);
static gint try_put (GeglCache * cache,
		     GeglCacheEntry * entry,
		     gsize * entry_id);
static gint put (GeglCache * cache,
		 GeglCacheEntry * entry,
		 gsize * entry_id);
static gint fetch (GeglCache * cache,
		   gsize entry_id,
		   GeglCacheEntry ** entry);
static void mark_as_dirty (GeglCache * cache,
			   gsize entry_id);
static void flush (GeglCache * cache,
		   gsize entry_id);

typedef enum Status_ Status;
enum Status_
  {
    STORED,
    FETCHED,
    DISCARDED
  };

typedef struct EntryRecord_ EntryRecord;
struct EntryRecord_
{
  /*
   * magic_number is used to help ensure that any random entry_id is
   * contained in this cache.
   */
  GeglCache * magic_number;
  Status status;
  GeglCacheEntry * data;
};

GType
gegl_memory_cache_get_type (void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof(GeglMemoryCacheClass),
	  NULL, /*base_init*/
	  NULL, /* base_finalize */
	  
	  /* classed types, instantiated types */
	  class_init, /* class_init */
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  
	  /* instantiated types */
	  sizeof(GeglMemoryCache),
	  0, /* n_preallocs */
	  instance_init, /* instance_init */
	  
	  /* value handling */
	  NULL /* value_table */
	};
      
      type = g_type_register_static (GEGL_TYPE_CACHE ,
				     "GeglMemoryCache",
				     &typeInfo,
				     0);
    }
  return type;
}

static void
class_init(gpointer g_class,
	   gpointer class_data)
{
  GeglCacheClass* cache_class = GEGL_CACHE_CLASS(g_class);
  GObjectClass* object_class = G_OBJECT_CLASS(g_class);
  cache_class->try_put=try_put;
  cache_class->put=put;
  cache_class->fetch=fetch;
  cache_class->mark_as_dirty=mark_as_dirty;
  cache_class->flush=flush;
  object_class->dispose = dispose;
  object_class->finalize=finalize;
}

static void
free_entry_data (gpointer data, gpointer user_data)
{
  if (data == NULL)
    {
      return;
    }
  EntryRecord * record = (EntryRecord *) data;
  if (record->data != NULL)
    {
      g_object_unref(record->data);
    }
  g_free (record);
}


static void
instance_init (GTypeInstance *instance,
	       gpointer g_class)
{
  GeglMemoryCache* mem_cache=GEGL_MEMORY_CACHE(instance);
  mem_cache->current_size=0;
  mem_cache->stored_entries=NULL;
  mem_cache->discarded_entries = NULL;
  mem_cache->fetched_entries = NULL;
}

static void
finalize(GObject *object)
{
}

static void dispose (GObject *object)
{
  GeglMemoryCache* self=GEGL_MEMORY_CACHE(object);
  static gboolean has_disposed=FALSE;
  if (!has_disposed)
    {
      g_list_foreach(self->discarded_entries,free_entry_data,NULL);
      g_list_free(self->discarded_entries);
      g_list_foreach(self->fetched_entries,free_entry_data,NULL);
      g_list_free(self->fetched_entries);
      g_list_foreach(self->stored_entries,free_entry_data,NULL);
      g_list_free(self->stored_entries);
      self->discarded_entries = NULL;
      self->fetched_entries = NULL;
      self->stored_entries = NULL;
      has_disposed = TRUE;
    }
}

static gint
try_put (GeglCache * cache,
	 GeglCacheEntry * entry,
	 gsize * entry_id)
{
  GeglMemoryCache* mem_cache=GEGL_MEMORY_CACHE(cache);
  gsize new_size=gegl_cache_entry_flattened_size (entry);
  if ((cache->persistent == TRUE) &&
      (mem_cache->current_size + new_size > cache->hard_limit))
    {
      return GEGL_PUT_WOULD_BLOCK;
    }
  return put(cache,entry,entry_id);
}

static gint
put (GeglCache * cache,
     GeglCacheEntry * entry,
     gsize * entry_id)
{
  GeglMemoryCache* mem_cache=GEGL_MEMORY_CACHE(cache);
  gsize new_size=gegl_cache_entry_flattened_size (entry);
  GList *link,*old_link;
  if ((cache->persistent == TRUE) &&
      (mem_cache->current_size + new_size > cache->hard_limit))
    {
#ifdef GEGL_THREADS
#else
      /* This is broken.  Something else, better, should be done here.*/
      return GEGL_PUT_WOULD_BLOCK;
#endif
    }
  if (mem_cache->current_size + new_size > cache->hard_limit)
    {
      link = g_list_last (mem_cache->stored_entries);
      while ((mem_cache->current_size != 0) &&
	     (mem_cache->current_size > cache->soft_limit) &&
	     (link != NULL))
	{
	  gsize entry_size;
	  EntryRecord * record = (EntryRecord *) (link->data);
	  entry_size=gegl_cache_entry_flattened_size (record->data);
	  g_return_val_if_fail (record->data != NULL, GEGL_PUT_INVALID);
	  g_return_val_if_fail (record->status == STORED, GEGL_PUT_INVALID);
	  g_return_val_if_fail (record->magic_number == cache, GEGL_PUT_INVALID);
	  g_object_unref(record->data);
	  /*
	   * Might want to keep weak references to the old entries
	   * here.
	   */
	  record->data = NULL;
	  mem_cache->current_size -= entry_size;
	  old_link = link;
	  link=g_list_previous(link);
	  mem_cache->stored_entries = g_list_remove_link (mem_cache->stored_entries, old_link);
	  mem_cache->discarded_entries = g_list_concat (old_link, mem_cache->discarded_entries);
	  record->status = DISCARDED;
	}
    }

  g_object_ref(entry);
  if (*entry_id == 0)
    {
      EntryRecord * record = g_new (EntryRecord, 1);
      record->data = entry;
      record->status = STORED;
      record->magic_number = cache;
      mem_cache->stored_entries=g_list_append(mem_cache->stored_entries, record);
      mem_cache->current_size += gegl_cache_entry_flattened_size (entry);
      link=g_list_last(mem_cache->stored_entries);
      *entry_id = GPOINTER_TO_SIZE(link);
    }
  else
    {
      link = GSIZE_TO_POINTER(*entry_id);
      EntryRecord * record = (EntryRecord *) (link->data);
      g_return_val_if_fail (record->status == FETCHED, GEGL_PUT_INVALID);
      g_return_val_if_fail (record->magic_number == cache, GEGL_PUT_INVALID);
      record->data = entry;
      mem_cache->current_size += gegl_cache_entry_flattened_size (entry);
      mem_cache->fetched_entries = g_list_remove_link (mem_cache->fetched_entries, link);
      mem_cache->stored_entries = g_list_concat (link, mem_cache->stored_entries);
      record->status = STORED;
    }
  return GEGL_PUT_SUCCEEDED;
}

static gint
fetch (GeglCache * cache,
       gsize entry_id,
       GeglCacheEntry ** entry_handle)
{
  GeglMemoryCache* mem_cache=GEGL_MEMORY_CACHE(cache);
  GList* link = GSIZE_TO_POINTER(entry_id);
  EntryRecord * record = (EntryRecord *) (link->data);
  GeglCacheEntry* entry = GEGL_CACHE_ENTRY(record->data);
  g_return_val_if_fail (record->magic_number == cache,GEGL_FETCH_INVALID);
  if (record->status == DISCARDED)
    {
      return GEGL_FETCH_EXPIRED;
    }
  g_return_val_if_fail (entry != NULL, GEGL_FETCH_INVALID);
  record->data = NULL;
  mem_cache->stored_entries = g_list_remove_link (mem_cache->stored_entries, link);
  mem_cache->fetched_entries = g_list_concat (link, mem_cache->fetched_entries);
  record->status = FETCHED;
  mem_cache->current_size -= gegl_cache_entry_flattened_size (entry);
  *entry_handle=entry;
  return GEGL_FETCH_SUCCEEDED;
}

static void
mark_as_dirty (GeglCache * cache,
	       gsize entry_id)
{
}

static void
flush (GeglCache * cache,
       gsize entry_id)
{
  GList* link = GSIZE_TO_POINTER(entry_id);
  GeglMemoryCache* mem_cache = GEGL_MEMORY_CACHE(cache);
  EntryRecord * record = (EntryRecord *) link->data;
  gsize entry_size=gegl_cache_entry_flattened_size (record->data);
  Status status = record->status;
  free_entry_data(link->data,NULL);
  switch (status)
    {
    case STORED:
      mem_cache->current_size -= entry_size;
      mem_cache->stored_entries = g_list_delete_link(mem_cache->stored_entries, link);
      break;
    case FETCHED:
      mem_cache->fetched_entries = g_list_delete_link(mem_cache->fetched_entries, link);
      break;
    case DISCARDED:
      mem_cache->discarded_entries = g_list_delete_link(mem_cache->discarded_entries, link);
      break;
    default:
      g_warning ("GeglMemoryCache: unhandled status type in flush()");
      break;
    }
}
