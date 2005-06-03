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

#include "config.h"

#include <glib-object.h>

#include "gegl-image-types.h"

#include "gegl-cache-entry.h"


static void gegl_cache_entry_class_init (GeglCacheEntryClass *klass);
static void gegl_cache_entry_init       (GeglCacheEntry      *self);


G_DEFINE_ABSTRACT_TYPE (GeglCacheEntry, gegl_cache_entry, G_TYPE_OBJECT)


static void
gegl_cache_entry_class_init (GeglCacheEntryClass *klass)
{
  klass->flattened_size = NULL;
  klass->flatten        = NULL;
  klass->unflatten      = NULL;
}

static void
gegl_cache_entry_init (GeglCacheEntry *self)
{
  self->cache_hints = NULL;
}

gsize
gegl_cache_entry_flattened_size (const GeglCacheEntry* self)
{
  GeglCacheEntryClass *class = GEGL_CACHE_ENTRY_GET_CLASS (self);

  g_return_val_if_fail (GEGL_IS_CACHE_ENTRY (self),0);
  g_return_val_if_fail (class->flattened_size != NULL, 0);

  return class->flattened_size (self);
}

void
gegl_cache_entry_flatten (GeglCacheEntry *self,
                          gpointer        buffer,
                          gsize           length)
{
  GeglCacheEntryClass *class;

  g_return_if_fail (GEGL_IS_CACHE_ENTRY (self));

  class = GEGL_CACHE_ENTRY_GET_CLASS (self);

  g_return_if_fail (class->flatten != NULL);
  g_return_if_fail (gegl_cache_entry_flattened_size (self) <= length);

  class->flatten (self, buffer, length);
}

void
gegl_cache_entry_unflatten (GeglCacheEntry *self,
                            gpointer        buffer,
                            gsize           length)
{
  GeglCacheEntryClass *class;

  g_return_if_fail (GEGL_IS_CACHE_ENTRY (self));

  class = GEGL_CACHE_ENTRY_GET_CLASS (self);

  g_return_if_fail (class->unflatten != NULL);
  g_return_if_fail (gegl_cache_entry_flattened_size (self) <= length);

  class->unflatten (self, buffer, length);
}

void
gegl_cache_entry_discard (GeglCacheEntry *self)
{
  GeglCacheEntryClass *class;

  g_return_if_fail (GEGL_IS_CACHE_ENTRY (self));

  class = GEGL_CACHE_ENTRY_GET_CLASS (self);

  g_return_if_fail (class->discard != NULL);

  class->discard (self);
}
