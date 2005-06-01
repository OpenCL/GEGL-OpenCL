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

#ifndef __GEGL_CACHE_STORE_H__
#define __GEGL_CACHE_STORE_H__

#include <glib.h>
#include "gegl-entry-record.h"

#define GEGL_TYPE_CACHE_STORE               (gegl_cache_store_get_type ())
#define GEGL_CACHE_STORE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CACHE_STORE, GeglCacheStore))
#define GEGL_CACHE_STORE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CACHE_STORE, GeglCacheStoreClass))
#define GEGL_IS_CACHE_STORE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CACHE_STORE))
#define GEGL_IS_CACHE_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CACHE_STORE))
#define GEGL_CACHE_STORE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CACHE_STORE, GeglCacheStoreClass))

GType gegl_cache_store_get_type(void) G_GNUC_CONST;

typedef struct _GeglCacheStore GeglCacheStore;
struct _GeglCacheStore
{
  GObject parent;
};

typedef struct _GeglCacheStoreClass GeglCacheStoreClass;
struct _GeglCacheStoreClass
{
  GObjectClass parent_class;

  void (*add) (GeglCacheStore * self, GeglEntryRecord * record);
  void (*remove) (GeglCacheStore* self, GeglEntryRecord* record);
  void (*zap) (GeglCacheStore * self, GeglEntryRecord * record);
  gint64 (*size) (GeglCacheStore* self);
  GeglEntryRecord * (*pop) (GeglCacheStore * self);
  GeglEntryRecord * (*peek) (GeglCacheStore * self);
};

void gegl_cache_store_add (GeglCacheStore * self, GeglEntryRecord * record);
void gegl_cache_store_remove (GeglCacheStore * self, GeglEntryRecord * record);
void gegl_cache_store_zap (GeglCacheStore * self, GeglEntryRecord * record);
gint64 gegl_cache_store_size (GeglCacheStore * self);
GeglEntryRecord * gegl_cache_store_pop (GeglCacheStore * self);
GeglEntryRecord * gegl_cache_store_peek (GeglCacheStore * self);

#endif
