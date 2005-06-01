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

#ifndef __GEGL_SWAP_CACHE_STORE_H__
#define __GEGL_SWAP_CACHE_STORE_H__

#include "gegl-entry-record.h"
#include "gegl-cache-store.h"

#define GEGL_TYPE_SWAP_CACHE_STORE               (gegl_swap_cache_store_get_type ())
#define GEGL_SWAP_CACHE_STORE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_SWAP_CACHE_STORE, GeglSwapCacheStore))
#define GEGL_SWAP_CACHE_STORE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_SWAP_CACHE_STORE, GeglSwapCacheStoreClass))
#define GEGL_IS_SWAP_CACHE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_SWAP_CACHE_STORE))
#define GEGL_IS_SWAP_CACHE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_SWAP_CACHE_STORE))
#define GEGL_SWAP_CACHE_STORE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_SWAP_CACHE_STORE, GeglSwapCacheStoreClass))

GType gegl_swap_cache_store_get_type(void) G_GNUC_CONST;

/*
 * GeglCache
 *
 */

typedef struct _GeglSwapCacheStore GeglSwapCacheStore;
struct _GeglSwapCacheStore
{
  GeglCacheStore parent;
  GList * record_head;
  gint64 size;
  gchar * filename;
  GIOChannel * channel;
  gint fd;
  GList * gaps;
  gint64 length;
  gboolean has_disposed;
};

typedef struct _GeglSwapCacheStoreClass GeglSwapCacheStoreClass;
struct _GeglSwapCacheStoreClass
{
  GeglCacheStoreClass parent_class;
};

GeglSwapCacheStore * gegl_swap_cache_store_new (const gchar * template);
gint64 gegl_swap_cache_length (GeglSwapCacheStore * self);

#endif
