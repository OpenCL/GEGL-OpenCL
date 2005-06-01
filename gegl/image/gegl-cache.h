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

#ifndef __GEGL_CACHE_H__
#define __GEGL_CACHE_H__

#include "gegl-cache-entry.h"
#include "gegl-cache-store.h"
#include "gegl-entry-record.h"

#define GEGL_TYPE_CACHE               (gegl_cache_get_type ())
#define GEGL_CACHE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CACHE, GeglCache))
#define GEGL_CACHE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CACHE, GeglCacheClass))
#define GEGL_IS_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CACHE))
#define GEGL_IS_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CACHE))
#define GEGL_CACHE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CACHE, GeglCacheClass))

GType gegl_cache_get_type(void) G_GNUC_CONST;


typedef enum
{
  GEGL_FETCH_SUCCEEDED,
  GEGL_FETCH_INVALID,
  GEGL_FETCH_EXPIRED,
} GeglFetchResults;

typedef enum
{
  GEGL_PUT_SUCCEEDED,
  GEGL_PUT_INVALID,
  GEGL_PUT_CACHE_FULL,
  GEGL_PUT_WOULD_BLOCK,
} GeglPutResults;


/*
 * GeglCache
 *
 */

typedef struct _GeglCache GeglCache;
struct _GeglCache
{
  GObject parent;
  GeglCacheStore* fetched_store;
  GeglCacheStore* discard_store;
  gboolean has_disposed;
};

typedef struct _GeglCacheClass GeglCacheClass;
struct _GeglCacheClass
{
  GObjectClass parent_class;
  GeglPutResults (*put) (GeglCache * cache,
			 GeglCacheEntry * entry,
			 gsize * entry_id);
  GeglFetchResults (*fetch) (GeglCache * cache,
			     gsize entry_id,
			     GeglCacheEntry ** entry);
  GeglFetchResults (*unfetch) (GeglCache * cache,
			       gsize entry_id,
			       GeglCacheEntry * entry);
  void (*flush) (GeglCache * cache,
		 gsize entry_id);
  void (*mark_as_dirty) (GeglCache * cache,
			 gsize entry_id);
  /* protected */
  void (*discard) (GeglCache* cache,
		   GeglEntryRecord * record);
  /* virtual */
  gint64 (*size) (GeglCache* cache);
  gint64 (*capacity) (GeglCache* cache);
  gboolean (*is_persistent) (GeglCache* cache);

  /* protected virtual */
  void (*insert_record) (GeglCache* cache,
			 GeglEntryRecord* record);
  gboolean (*check_room_for) (GeglCache* cache, gint64 size);
  void (*flush_internal) (GeglCache * cache,
			  GeglEntryRecord * record);
};


gint gegl_cache_put (GeglCache * cache,
		     GeglCacheEntry * entry,
		     gsize * entry_id);
GeglFetchResults gegl_cache_fetch (GeglCache * cache,
		       gsize entry_id,
		       GeglCacheEntry ** entry);
GeglFetchResults gegl_cache_unfetch (GeglCache * cache,
				     gsize entry_id,
				     GeglCacheEntry * entry);
void gegl_cache_mark_as_dirty (GeglCache * cache, gsize entry_id);
void gegl_cache_flush (GeglCache * cache,
		       gsize entry_id);
void gegl_cache_flush_internal (GeglCache * cache, GeglEntryRecord * record);
void gegl_cache_discard (GeglCache * cache, GeglEntryRecord * record);
gint64 gegl_cache_size (GeglCache* cache);
gint64 gegl_cache_capacity (GeglCache* cache);
gboolean gegl_cache_is_persistent (GeglCache* cache);
void gegl_cache_insert_record (GeglCache* cache,
			       GeglEntryRecord* record);
gboolean gegl_cache_check_room_for (GeglCache* cache, gint64 size);

#endif
