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

#ifndef GEGL_CACHE_H
#define GEGL_CACHE_H

#include "gegl-buffer.h"
#include "gegl-cache-entry.h"

#define GEGL_TYPE_CACHE               (gegl_cache_get_type ())
#define GEGL_CACHE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CACHE, GeglCache))
#define GEGL_CACHE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CACHE, GeglCacheClass))
#define GEGL_IS_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CACHE))
#define GEGL_IS_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CACHE))
#define GEGL_CACHE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CACHE, GeglCacheClass))

GType gegl_cache_get_type(void);

typedef struct _GeglCache GeglCache;
struct _GeglCache
{
  GObject parent;
  gboolean persistent;
  gsize soft_limit;
  gsize hard_limit;
  struct _GeglCache* next_cache;
};

typedef struct _GeglCacheClass GeglCacheClass;
struct _GeglCacheClass
{
  GObjectClass parent_class;
};

enum GeglFetchResults {
  GEGL_FETCH_SUCCEEDED,
  GEGL_FETCH_NO_EXIST,
};

GeglCache *gegl_cache_new (gsize soft_limit,
			   gsize hard_limit,
			   gboolean persistent);
gboolean gegl_cache_try_put (GeglCache * cache,
			     GeglCacheEntry * entry,
			     gsize * entry_id);

gint gegl_cache_put (GeglCache * cache,
		     GeglCacheEntry * entry,
		     gsize * entry_id);

gint gegl_cache_fetch (GeglCache * cache,
		       gsize * entry_id,
		       GeglCacheEntry ** entry);
void gegl_cache_mark_as_dirty (GeglCache * cache, gsize * entry_id);

#endif
