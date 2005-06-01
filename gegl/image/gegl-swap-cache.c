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

static void finalize (GObject * object);
static void dispose (GObject * object);
static void insert_record (GeglCache* cache,
			   GeglEntryRecord* record);
static gboolean check_room_for (GeglCache* cache, gint64 size);
static gint64 size (GeglCache* cache);
static gint64 capacity (GeglCache* cache);
static gboolean is_persistent (GeglCache* cache);
static void flush_internal (GeglCache * cache,
			    GeglEntryRecord * record);

static gpointer parent_class;

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
  
  cache_class->insert_record = insert_record;
  cache_class->check_room_for = check_room_for;
  cache_class->size = size;
  cache_class->capacity = capacity;
  cache_class->is_persistent = is_persistent;
  cache_class->flush_internal = flush_internal;

  object_class->dispose = dispose;
  object_class->finalize=finalize;

  parent_class = g_type_class_peek_parent (g_class);
}

static void
instance_init(GTypeInstance *instance,
	      gpointer g_class)
{
  GeglSwapCache * self = GEGL_SWAP_CACHE (instance);
  
  self->heap_capacity = 0;
  self->has_disposed = FALSE;
  self->heap_stored = gegl_heap_cache_store_new ();
  self->stored = NULL;
}

static void
dispose (GObject * object)
{
  GeglSwapCache * self = GEGL_SWAP_CACHE (object);
  if (!self->has_disposed) {
    self->has_disposed = TRUE;
    g_object_unref (G_OBJECT(self->stored));
    g_object_unref (G_OBJECT(self->heap_stored));
  }
  G_OBJECT_CLASS(parent_class)->dispose(object);
}

static void
finalize (GObject * object) {
  G_OBJECT_CLASS(parent_class)->finalize(object);
  
}

GeglSwapCache*
gegl_swap_cache_new (gchar* filename_template,
		     gint64 heap_size)
{
  GeglSwapCache * self = g_object_new(GEGL_TYPE_SWAP_CACHE, NULL);
  self->stored = gegl_swap_cache_store_new (filename_template);
  self->heap_capacity = heap_size;
  return self;
}

void
insert_record (GeglCache* cache,
	       GeglEntryRecord* record)
{
  /* 
   * We are guaranteed, at this point, that either the heap cache can
   * hold the item and room in it has been made
   */

  GeglSwapCache * self = GEGL_SWAP_CACHE (cache);
  gegl_cache_store_add(GEGL_CACHE_STORE(self->heap_stored), record);
}

gboolean
check_room_for (GeglCache* cache, gint64 size)
{
  GeglSwapCache * self = GEGL_SWAP_CACHE (cache);
  /*
   * The conditions I need to check for is:
   * 1. size unable to fit in the heap_stored cache.
   * 2. heap_stored full.
   * 3. heap_stored not full.
   * 
   * I assume that the swap cache store can never be full.  This is,
   * to say the least, a simplification, though not so bad in the land
   * of huge disks.
   */
  if (size > self->heap_capacity)
    {
      /* condition 1 */
      return FALSE;
    }
  gint64 cache_size = gegl_cache_store_size (GEGL_CACHE_STORE(self->heap_stored));
  while (cache_size + size > self->heap_capacity)
    {
      GeglEntryRecord * record;
      record = gegl_cache_store_peek (GEGL_CACHE_STORE(self->heap_stored));
      gegl_cache_store_remove (GEGL_CACHE_STORE(self->heap_stored), record);
      gegl_cache_store_add(GEGL_CACHE_STORE(self->stored), record);
      cache_size = gegl_cache_store_size (GEGL_CACHE_STORE(self->heap_stored));
    }
  /* condition 2 and 3 */
  return TRUE;
}
gint64
size (GeglCache* cache)
{
  GeglSwapCache * self = GEGL_SWAP_CACHE (cache);
  return gegl_cache_store_size (GEGL_CACHE_STORE(self->stored));
  
}

gint64
capacity (GeglCache* cache)
{
  return 0;
}

gboolean
is_persistent (GeglCache* cache)
{
  return TRUE;
}
void
flush_internal (GeglCache * cache,
		GeglEntryRecord * record)
{
}
