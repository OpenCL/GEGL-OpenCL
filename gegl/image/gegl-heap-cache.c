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

#include "gegl-heap-cache.h"

static gpointer parent_class;

static void class_init(gpointer g_class,
                       gpointer class_data);
static void instance_init(GTypeInstance *instance,
                          gpointer g_class);
static void finalize (GObject * object);
static void insert_record (GeglCache* cache,
		       GeglEntryRecord* record);
static gboolean check_room_for (GeglCache* cache, gint64 size);
static gint64 size (GeglCache* cache);
static gint64 capacity (GeglCache* cache);
static gboolean is_persistent (GeglCache* cache);
static void flush_internal (GeglCache * cache,
		     GeglEntryRecord * record);
GType
gegl_heap_cache_get_type (void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof (GeglHeapCacheClass),
	  NULL, /*base_init*/
	  NULL, /* base_finalize */
	
	  /* classed types, instantiated types */
	  class_init, /* class_init */
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	
	  /* instantiated types */
	  sizeof(GeglHeapCache),
	  0, /* n_preallocs */
	  instance_init, /* instance_init */
	
	  /* value handling */
	  NULL /* value_table */
	};

      type = g_type_register_static (GEGL_TYPE_CACHE ,
				     "GeglHeapCache",
				     &typeInfo,
				     0);
    }
  return type;
}

GeglHeapCache *
gegl_heap_cache_new (gsize capacity, gboolean is_persistent)
{
  GeglHeapCache * self = g_object_new (GEGL_TYPE_HEAP_CACHE, NULL);
  self->is_persistent = is_persistent;
  self->capacity = capacity;
  return self;
}

static void
class_init(gpointer g_class,
	   gpointer class_data)
{
  GeglCacheClass * cache_class = GEGL_CACHE_CLASS (g_class);
  GObjectClass * object_class = G_OBJECT_CLASS (g_class);

  cache_class->insert_record = insert_record;
  cache_class->check_room_for = check_room_for;
  cache_class->size = size;
  cache_class->capacity = capacity;
  cache_class->is_persistent = is_persistent;
  cache_class->flush_internal = flush_internal;

  object_class->finalize = finalize;

  parent_class = g_type_class_peek_parent (g_class);
}
static void
instance_init(GTypeInstance *instance,
	      gpointer g_class)
{
  GeglHeapCache * self = GEGL_HEAP_CACHE (instance);

  self->capacity = 0;
  self->is_persistent = FALSE;
  self->stored = gegl_heap_cache_store_new ();
}

static void
finalize (GObject * object)
{
  GeglHeapCache * self = GEGL_HEAP_CACHE (object);
  g_object_unref (G_OBJECT(self->stored));

  G_OBJECT_CLASS(parent_class)->finalize(object);
}

void
insert_record (GeglCache* cache,
	       GeglEntryRecord* record)
{
  GeglHeapCache * self = GEGL_HEAP_CACHE (cache);
  gegl_cache_store_add(GEGL_CACHE_STORE(self->stored), record);
}

gboolean
check_room_for (GeglCache* cache, gint64 size)
{
  GeglHeapCache * self = GEGL_HEAP_CACHE (cache);
  /*
   * The conditions I need to check for is:
   * 1. size unable to fit in the cache.
   * 2. is_persistent = TRUE, cache full.
   * 3. is_persistent = TRUE, cache not full.
   * 4. is_persistent = FALSE, cache full.
   * 5. is_persistent = FALSE, cache not full.
   */
  if (size > self->capacity)
    {
      /* condition 1 */
      return FALSE;
    }
  gint64 cache_size = gegl_cache_size (cache);
  if ((cache_size + size) > self->capacity)
    {
      if (self->is_persistent)
	{
	  /* condition 2 */
	  return FALSE;
	}
      while (cache_size + size > self->capacity)
	{
	  GeglEntryRecord * record;
	  record = gegl_cache_store_peek (GEGL_CACHE_STORE(self->stored));
	  gegl_cache_discard (cache, record);
	  cache_size = gegl_cache_size (cache);
	}
    }
  /* conditions 3, 4 and 5 */
  return TRUE;
}
gint64
size (GeglCache* cache)
{
  GeglHeapCache * self = GEGL_HEAP_CACHE (cache);
  return gegl_cache_store_size (GEGL_CACHE_STORE(self->stored));

}

gint64
capacity (GeglCache* cache)
{
  GeglHeapCache * self = GEGL_HEAP_CACHE (cache);
  return self->capacity;
}

gboolean
is_persistent (GeglCache* cache)
{
  GeglHeapCache * self = GEGL_HEAP_CACHE (cache);
  return self->is_persistent;
}
void
flush_internal (GeglCache * cache,
		GeglEntryRecord * record)
{
}
