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

#ifndef GEGL_HEAP_CACHE_H
#define GEGL_HEAP_CACHE_H

#include "gegl-cache.h"
#include "gegl-heap-cache-store.h"

#define GEGL_TYPE_HEAP_CACHE          (gegl_heap_cache_get_type ())
#define GEGL_HEAP_CACHE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CACHE, GeglHeapCache))
#define GEGL_HEAP_CACHE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CACHE, GeglHeapCacheClass))
#define GEGL_IS_HEAP_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CACHE))
#define GEGL_IS_HEAP_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CACHE))
#define GEGL_HEAP_CACHE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CACHE, GeglHeapCacheClass))

GType gegl_heap_cache_get_type(void);

/*
 * GeglHeapCache
 *
 */

typedef struct _GeglHeapCache GeglHeapCache;
struct _GeglHeapCache
{
  GeglCache parent;
  GeglHeapCacheStore * stored;
  gsize capacity;
  gboolean is_persistent;
};

typedef struct _GeglHeapCacheClass GeglHeapCacheClass;
struct _GeglHeapCacheClass
{
  GeglCacheClass parent_class;

};

GeglHeapCache * gegl_heap_cache_new (gsize capacity, gboolean is_persistent);

#endif
