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

#ifndef GEGL_CACHE_ENTRY_H
#define GEGL_CACHE_ENTRY_H

#include "gegl-object.h"

/* GeglCacheEntry
 *
 * This object ecapsultes all the operations a GeglCache needs to know
 * about in order to serialize and keep track of an object being
 * stored in the Cache.  Data structures that wish to insert
 * themselves into a GeglCache should subclass and override the
 * virtual methods and pass a GeglCacheEntry into the GeglCache
 */

#define GEGL_TYPE_CACHE_ENTRY               (gegl_cache_entry_get_type ())
#define GEGL_CACHE_ENTRY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_CACHE_ENTRY, GeglCacheEntry))
#define GEGL_CACHE_ENTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_CACHE_ENTRY, GeglCacheEntryClass))
#define GEGL_IS_CACHE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_CACHE_ENTRY))
#define GEGL_IS_CACHE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_CACHE_ENTRY))
#define GEGL_CACHE_ENTRY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_CACHE_ENTRY, GeglCacheEntryClass))

GType gegl_cache_entry_get_type (void);


typedef struct _GeglCacheEntry GeglCacheEntry;
struct _GeglCacheEntry
{
  GeglObject object;
  /* a hint for the future */
  GHashTable* cache_hints;
};

typedef struct _GeglCacheEntryClass GeglCacheEntryClass;
struct _GeglCacheEntryClass
{
  GeglObjectClass object_class;
  gsize (*flattened_size) (const GeglCacheEntry* self);
  void (*flatten) (GeglCacheEntry* self, gpointer buffer, gsize length);
  void (*unflatten) (GeglCacheEntry* self, const gpointer buffer, gsize length);
};
/*
 * The serialized length of the buffer needed to serialize this entry
 */
gsize gegl_cache_entry_flattened_size (const GeglCacheEntry* self);
/*
 * The function to serialize this data
 */
void gegl_cache_entry_flatten (GeglCacheEntry* self, gpointer buffer, gsize length);
/*
 * The function un unserialize this data
 */
void gegl_cache_entry_unflatten (GeglCacheEntry* self, gpointer buffer, gsize length);

#endif
