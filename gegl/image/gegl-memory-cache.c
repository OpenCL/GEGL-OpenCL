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
 *  Copyright 2003 Daniel S. Rogers
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
  GObject* object=G_OBJECT(data);
  g_object_unref(object);
}


static void
instance_init (GTypeInstance *instance,
	       gpointer g_class)
{
  GeglMemoryCache* mem_cache=GEGL_MEMORY_CACHE(instance);
  mem_cache->current_size=0;
  mem_cache->entries=NULL;
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
      g_list_foreach(self->entries,free_entry_data,NULL);
      g_list_free(self->entries);
      self->entries=NULL;
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
  GList* new_element;
  gsize new_size=gegl_cache_entry_flattened_size (entry);
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
      GList* link;
      link = g_list_first(mem_cache->entries);
      while ((mem_cache->current_size != 0) &&
	     (mem_cache->current_size > cache->soft_limit) &&
	     (link != NULL))
	{
	  gsize entry_size;
	  entry_size=gegl_cache_entry_flattened_size (GEGL_CACHE_ENTRY(link->data));
	  if (link->data != NULL)
	    {
	      free_entry_data(link->data,NULL);
	      link->data = NULL;
	      mem_cache->current_size -= entry_size;
	    }
	  link=g_list_next(link);
	}
    }

  g_object_ref(entry);
  if (*entry_id == 0)
    {
      mem_cache->entries=g_list_prepend(mem_cache->entries,entry);
      new_element=g_list_first(mem_cache->entries);
      *entry_id = GPOINTER_TO_SIZE(new_element);
    }
  else
    {
      new_element = GSIZE_TO_POINTER(*entry_id);
      new_element->data = entry;
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
  GeglCacheEntry* entry = GEGL_CACHE_ENTRY(link->data);
  if (link->data == NULL)
    {
      return GEGL_FETCH_EXPIRED;
    }
  mem_cache->entries=g_list_remove_link (mem_cache->entries,link);
  if (mem_cache->entries == NULL)
    {
      mem_cache->entries = link;
    }
  else
    {
      mem_cache->entries->prev=link;
      link->next=mem_cache->entries;
      mem_cache->entries = link;
    }
  link->data = NULL;
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
  gsize entry_size=gegl_cache_entry_flattened_size (GEGL_CACHE_ENTRY(link->data));
  free_entry_data(link->data,NULL);
  mem_cache->current_size -= entry_size;
  mem_cache->entries=g_list_delete_link(mem_cache->entries, link);
}
