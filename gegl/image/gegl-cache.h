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

#include "gegl-cache-entry.h"

#define GEGL_TYPE_CACHE               (gegl_cache_get_type ())
#define GEGL_CACHE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CACHE, GeglCache))
#define GEGL_CACHE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CACHE, GeglCacheClass))
#define GEGL_IS_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CACHE))
#define GEGL_IS_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CACHE))
#define GEGL_CACHE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CACHE, GeglCacheClass))

GType gegl_cache_get_type(void);

/*
 * GeglCache
 *
 * If a cache is persistent then you are always gaurenteed to be able
 * to retrieve a tile you put in.
 * 
 * soft_limit is the limit which, when exceeded, may cause the cache
 * to begin expiring tiles without blocking.  This may do other things
 * as well, like compressing tiles, expiring old, non-dirty, still in
 * use tiles or generally any operation appropiate for an almost full
 * cache.
 *
 * hard_limit is the limit which the cache may not exceed.  Any
 * attempt to exceed this limit will cause put() to block until
 * put()ing the new tile will not exceed hard_limit.  If the cache is
 * persistent, then this may block forever.  If the cache is not
 * persistent then this should force an immediate (and possibly
 * time-expensive) expiring of tiles from the cache.
 *
 * Both hard_limit or soft_limit may be set to zero, in which case one
 * or the other is ignored.  If hard_limit and soft_limit are both
 * zero the cache should be effectively unlimited.  Whether or not it
 * it actually is unlimited depends on the details of the cache and
 * the physical limits of the hardware invovled.  (this means that
 * unlimited caches can potentially recover from out of memory errors
 * by gracefully expiring entries, or blocking on put).
 */

typedef struct _GeglCache GeglCache;
struct _GeglCache
{
  GObject parent;
  gboolean persistent;
  /* These need to be really big even on 32 bit platforms 
    to allow for the large limits on the bigfile interface. */
  guint64 soft_limit;
  guint64 hard_limit;

};

typedef struct _GeglCacheClass GeglCacheClass;
struct _GeglCacheClass
{
  GObjectClass parent_class;
  gint (*try_put) (GeglCache * cache,
		    GeglCacheEntry * entry,
		    gsize * entry_id);
  gint (*put) (GeglCache * cache,
		GeglCacheEntry * entry,
		gsize * entry_id);
  gint (*fetch) (GeglCache * cache,
		 gsize entry_id,
		 GeglCacheEntry ** entry);
  void (*mark_as_dirty) (GeglCache * cache,
			 gsize entry_id);
  void (*flush) (GeglCache * cache,
		 gsize entry_id);
};

enum GeglFetchResults {
  GEGL_FETCH_SUCCEEDED,
  GEGL_FETCH_INVALID,
  GEGL_FETCH_EXPIRED,
};

enum GeglPutResults {
  GEGL_PUT_SUCCEEDED,
  GEGL_PUT_INVALID,
  GEGL_PUT_WOULD_BLOCK,
};

/*
 * This is the same as put, except will return GEGL_PUT_WOULD_BLOCK
 * if the put operation would have blocked
 */
gint gegl_cache_try_put (GeglCache * cache,
			 GeglCacheEntry * entry,
			 gsize * entry_id);
/*
 * This tries puts an entry into the cache.  The first time an entry
 * is placed into a cache, *entry_id should be 0.  If put succeeds,
 * then entry_id will point to the entry_id for this entry.  The
 * entry_id is used to refer to all future operations regarding this
 * entry. gegl_cache_put returns an enum GeglPutResults that describes
 * the result of this operation.
 *
 * Once you put an entry into the cache, you should call
 * g_object_unref().  You must not keep a refrence for yourself to an
 * entry after it is put into the cache.
 *
 * After an entry is fetch'ed, the cache may keep around stale copies
 * of this entry if it is reasonable for the cache to do so.  As such,
 * all subsequent put()s of the same entry should reuse the same
 * entry_id.
 *
 * Don't try to put() an entry_id that is already in the
 * cache.
 *
 * Also, don't put the same entry into more than one cache.  That
 * would be silly.
 *
 * On falure, entry_id will be untouched.  Entry_id is also globally
 * unique to a single process space (i.e. it's a pointer).
 */
gint gegl_cache_put (GeglCache * cache,
		     GeglCacheEntry * entry,
		     gsize * entry_id);
/*
 * This attempts to fetch an entry from the cache.  entry_id is the
 * entry_id that was assigned by put().  If the entry has been expired
 * from the cache and no longer exists in any useable form, then this
 * will return GEGL_FETCH_EXPIRED.  Don't fetch() an entry that you
 * didn't put() into the cache.  That would be bad.
 *
 * There is no need to call g_object_ref() after calling fetch(),
 * fetch() will do this for you.
 *
 */
gint gegl_cache_fetch (GeglCache * cache,
		       gsize entry_id,
		       GeglCacheEntry ** entry);
/*
 * Use mark_as_dirty to indicate that the cache should discard any
 * saved state (like on-disk representations) assocated with entry_id.
 * Always call mark_as_dirty() if you modify and entry _before_ you
 * put it back into a cache.
 *
 * You should only mark something as dirty if it has been put() into
 * the cache previously (thus entry_id is a valid id), has been pulled
 * from the cache with fetch(), and the id has not been flush()ed.
 */
void gegl_cache_mark_as_dirty (GeglCache * cache, gsize entry_id);
/*
 * flush() will cause entry_id to be invalidated, allowing it to be
 * potentially reassigned to another Entry as well as causing any
 * stored state associated with that entry_id to be freed.  Always
 * call this when you are done entirely with an entry (even if that
 * entry was EXPIED).  If you do try and put an entry back into this
 * cache, you must let put() reassign a new entry_id by initalizing
 * the entry_id passed to put() to zero.
 *
 * Once flush() is called, don't fetch() the entry again.
 */
void gegl_cache_flush (GeglCache * cache,
		       gsize entry_id);
#endif
