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

#ifndef __GEGL_NULL_CACHE_STORE_H__
#define __GEGL_NULL_CACHE_STORE_H__

#include "gegl-entry-record.h"
#include "gegl-cache-store.h"

#define GEGL_TYPE_NULL_CACHE_STORE               (gegl_null_cache_store_get_type ())
#define GEGL_NULL_CACHE_STORE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NULL_CACHE_STORE, GeglNullCacheStore))
#define GEGL_NULL_CACHE_STORE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NULL_CACHE_STORE, GeglNullCacheStoreClass))
#define GEGL_IS_NULL_CACHE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NULL_CACHE_STORE))
#define GEGL_IS_NULL_CACHE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NULL_CACHE_STORE))
#define GEGL_NULL_CACHE_STORE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NULL_CACHE_STORE, GeglNullCacheStoreClass))

GType gegl_null_cache_store_get_type(void) G_GNUC_CONST;

/*
 * GeglCache
 *
 */

typedef struct _GeglNullCacheStore GeglNullCacheStore;
struct _GeglNullCacheStore
{
  GeglCacheStore parent;
  GeglCacheStatus status;
  GList * record_head;
};

typedef struct _GeglNullCacheStoreClass GeglNullCacheStoreClass;
struct _GeglNullCacheStoreClass
{
  GeglCacheStoreClass parent_class;
};

GeglNullCacheStore * gegl_null_cache_store_new (GeglCacheStatus status);

#endif
