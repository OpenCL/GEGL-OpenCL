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

#include "gegl-heap-cache.h"
#include "gegl-heap-cache-store.h"


static void     gegl_heap_cache_class_init (GeglHeapCacheClass *klass);
static void     gegl_heap_cache_init       (GeglHeapCache      *self);

static void     finalize       (GObject         *object);
static void     insert_record  (GeglCache       *cache,
                                GeglEntryRecord *record);
static gboolean check_room_for (GeglCache       *cache,
                                gint64           size);
static gint64   size           (GeglCache       *cache);
static gint64   capacity       (GeglCache       *cache);
static gboolean is_persistent  (GeglCache       *cache);
static void     flush_internal (GeglCache       *cache,
                                GeglEntryRecord *record);


G_DEFINE_TYPE (GeglHeapCache, gegl_heap_cache, GEGL_TYPE_CACHE)


GeglHeapCache *
gegl_heap_cache_new (gsize capacity, gboolean is_persistent)
{
  GeglHeapCache * self = g_object_new (GEGL_TYPE_HEAP_CACHE, NULL);
  self->is_persistent = is_persistent;
  self->capacity = capacity;
  return self;
}

static void
gegl_heap_cache_class_init (GeglHeapCacheClass *klass)
{
  GeglCacheClass *cache_class  = GEGL_CACHE_CLASS (klass);
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize      = finalize;

  cache_class->insert_record  = insert_record;
  cache_class->check_room_for = check_room_for;
  cache_class->size           = size;
  cache_class->capacity       = capacity;
  cache_class->is_persistent  = is_persistent;
  cache_class->flush_internal = flush_internal;
}

static void
gegl_heap_cache_init (GeglHeapCache *self)
{
  self->capacity      = 0;
  self->is_persistent = FALSE;
  self->stored        = gegl_heap_cache_store_new ();
}

static void
finalize (GObject *object)
{
  GeglHeapCache *self = GEGL_HEAP_CACHE (object);

  g_object_unref (self->stored);

  G_OBJECT_CLASS (gegl_heap_cache_parent_class)->finalize (object);
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
