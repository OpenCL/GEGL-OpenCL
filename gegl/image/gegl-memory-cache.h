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

#ifndef GEGL_MEMORY_CACHE_H
#define GEGL_MEMORY_CACHE_H

#include "gegl-cache.h"

#define GEGL_TYPE_MEMORY_CACHE               (gegl_memory_cache_get_type ())
#define GEGL_MEMORY_CACHE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MEMORY_CACHE, GeglMemoryCache))
#define GEGL_MEMORY_CACHE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MEMORY_CACHE, GeglMemoryCacheClass))
#define GEGL_IS_MEMORY_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MEMORY_CACHE))
#define GEGL_IS_MEMORY_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MEMORY_CACHE))
#define GEGL_MEMORY_CACHE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MEMORY_CACHE, GeglMemoryCacheClassClass))

GType gegl_memory_cache_get_type(void);

typedef struct _GeglMemoryCache GeglMemoryCache;
struct _GeglMemoryCache
{
  GeglCache parent;
  GList* entries;
  guint64 current_size;
};

typedef struct _GeglMemoryCacheClass GeglMemoryCacheClass;
struct _GeglMemoryCacheClass
{
  GeglCacheClass parent_class;
};

#endif
