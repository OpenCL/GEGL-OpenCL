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

#include "gegl-cache-store.h"


static void gegl_cache_store_class_init (GeglCacheStoreClass *klass);
static void gegl_cache_store_init       (GeglCacheStore      *self);


G_DEFINE_ABSTRACT_TYPE (GeglCacheStore, gegl_cache_store, G_TYPE_OBJECT)


static void
gegl_cache_store_class_init (GeglCacheStoreClass *klass)
{
  klass->add    = NULL;
  klass->remove = NULL;
  klass->size   = NULL;
}

static void
gegl_cache_store_init (GeglCacheStore *self)
{
}

void
gegl_cache_store_add (GeglCacheStore * self, GeglEntryRecord * record)
{
  GeglCacheStoreClass *class;

  g_return_if_fail (GEGL_IS_CACHE_STORE (self));

  class = GEGL_CACHE_STORE_GET_CLASS (self);

  g_return_if_fail (class->add != NULL);

  class->add(self, record);
}

void gegl_cache_store_remove (GeglCacheStore * self, GeglEntryRecord * record)
{
  GeglCacheStoreClass * class;
  g_return_if_fail (self != NULL);
  class = GEGL_CACHE_STORE_GET_CLASS (self);
  g_return_if_fail (class != NULL);
  g_return_if_fail (class->remove != NULL);
  class->remove (self, record);
}

void gegl_cache_store_zap (GeglCacheStore * self, GeglEntryRecord * record)
{
  GeglCacheStoreClass * class;
  g_return_if_fail (self != NULL);
  class = GEGL_CACHE_STORE_GET_CLASS (self);
  g_return_if_fail (class != NULL);
  g_return_if_fail (class->zap != NULL);
  class->zap (self, record);
}

gint64 gegl_cache_store_size (GeglCacheStore * self)
{
  GeglCacheStoreClass * class;
  g_return_val_if_fail (self != NULL, 0);
  class = GEGL_CACHE_STORE_GET_CLASS (self);
  g_return_val_if_fail (class != NULL, 0);
  g_return_val_if_fail (class->size != NULL, 0);
  return class->size (self);
}

GeglEntryRecord *
gegl_cache_store_pop (GeglCacheStore * self)
{
  GeglCacheStoreClass * class;
  g_return_val_if_fail (self != NULL, NULL);
  class = GEGL_CACHE_STORE_GET_CLASS (self);
  g_return_val_if_fail (class != NULL, NULL);
  g_return_val_if_fail (class->pop != NULL, NULL);
  return class->pop (self);
}

GeglEntryRecord *
gegl_cache_store_peek (GeglCacheStore * self)
{
  GeglCacheStoreClass * class;
  g_return_val_if_fail (self != NULL, NULL);
  class = GEGL_CACHE_STORE_GET_CLASS (self);
  g_return_val_if_fail (class != NULL, NULL);
  g_return_val_if_fail (class->pop != NULL, NULL);
  return class->peek (self);
}
