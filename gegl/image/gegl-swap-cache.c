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

#include "gegl-swap-cache.h"

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

GType
gegl_swap_cache_get_type (void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof(GeglSwapCacheClass),
	  NULL, /*base_init*/
	  NULL, /* base_finalize */
	  
	  /* classed types, instantiated types */
	  class_init, /* class_init */
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	  
	  /* instantiated types */
	  sizeof(GeglSwapCache),
	  0, /* n_preallocs */
	  instance_init, /* instance_init */
	  
	  /* value handling */
	  NULL /* value_table */
	};
      
      type = g_type_register_static (GEGL_TYPE_CACHE ,
				     "GeglSwapCache",
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

  object_class->constructor = constructor;
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
  /* g_message ("EntryRecord %d freed.", GPOINTER_TO_SIZE(record)); */
}


static void
instance_init (GTypeInstance *instance,
	       gpointer g_class)
{
  GeglSwapCache* swap_cache=GEGL_SWAP_CACHE(instance);

  swap_cache->stored_entries=NULL;
  swap_cache->discarded_entries = NULL;
  swap_cache->fetched_entries = NULL;

  swap_cache->filename = NULL;
  swap_cache->gaps = NULL;
  swap_cache->swap_file = NULL;
  swap_cache->swap_file_length = 0;

  swap_cache->current_size=0;
  swap_cache->has_disposed = FALSE;
}

static void
finalize(GObject *object)
{
  G_OBJECT_CLASS (g_type_class_peek_parent (GEGL_MEMORY_CACHE_GET_CLASS (object)))->finalize(object);
}

static void dispose (GObject *object)
{
  GeglMemoryCache* self=GEGL_MEMORY_CACHE(object);
  if (!self->has_disposed)
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
      self->has_disposed = TRUE;
      
      if (self->filename != NULL)
	{
	  g_free (filename);
	  filename = NULL;
	}
      

    }
  G_OBJECT_CLASS (g_type_class_peek_parent (GEGL_MEMORY_CACHE_GET_CLASS (object)))->dispose(object);
}

static gint
try_put (GeglCache * cache,
	 GeglCacheEntry * entry,
	 gsize * entry_id)
{
  GeglSwapCache* swap_cache=GEGL_SWAP_CACHE(cache);
  gsize new_size=gegl_cache_entry_flattened_size (entry);
  if ((cache->persistent == TRUE) &&
      (swap_cache->hard_limit != 0) && 
      (swap_cache->current_size + new_size > cache->hard_limit))
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
  GeglSwapCache* swap_cache=GEGL_SWAP_CACHE(cache);
  gsize new_size=gegl_cache_entry_flattened_size (entry);
  GList *link,*old_link;

  if ((cache->persistent == TRUE) &&
      (swap_cache->hard_limit != 0) &&
      (swap_cache->current_size + new_size > cache->hard_limit))
    {
#ifdef GEGL_THREADS
#else
      /* This is broken.  Something else, better, should be done here.
       * Basically, In a single threaded application we can't make this
       * block forever. That would be bad.  So if threads are not enabled
       * then we cannot block here like I would like to.
       * 
       * So, some other reasonable action should be taken.  This may
       * be as simple as providing a more reasonable named return
       * value, but I have not thought about it deeply.
       */
      return GEGL_PUT_WOULD_BLOCK;
#endif
    }
  if ((cache->hard_limit != 0) &&
      (swap_cache->current_size + new_size > cache->hard_limit))
    {
      link = g_list_last (mem_cache->stored_entries);
      while ((swap_cache->current_size != 0) &&
	     
	     ((swap_cache->current_size > cache->soft_limit) ||
	      (swap_cache->current_size + new_size > cache->hard_limit)) &&
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
  EntryRecord * record;
  if (*entry_id == 0)
    {
      record = g_new (EntryRecord, 1);
      /* g_message ("EntryRecord %d allocated.", GPOINTER_TO_SIZE(record)); */
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
      record = (EntryRecord *) (link->data);
      g_return_val_if_fail (record->magic_number == cache, GEGL_PUT_INVALID);
      if (record->status == DISCARDED)
	{
	  record->data = entry;
	  mem_cache->current_size += gegl_cache_entry_flattened_size (entry);
	  mem_cache->discarded_entries = g_list_remove_link (mem_cache->discarded_entries, link);
	  mem_cache->stored_entries = g_list_concat (link, mem_cache->stored_entries);
	  record->status = STORED;
	  return GEGL_PUT_SUCCEEDED;
	}
      else
	{
	  g_return_val_if_fail (record->status == FETCHED, GEGL_PUT_INVALID);
	  record->data = entry;
	  mem_cache->current_size += gegl_cache_entry_flattened_size (entry);
	  mem_cache->fetched_entries = g_list_remove_link (mem_cache->fetched_entries, link);
	  mem_cache->stored_entries = g_list_concat (link, mem_cache->stored_entries);
	  record->status = STORED;
	}
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
  Status status = record->status;
  gsize entry_size;

  switch (status)
    {
    case STORED:
      entry_size=gegl_cache_entry_flattened_size (record->data);
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
  free_entry_data(record,NULL);
}
