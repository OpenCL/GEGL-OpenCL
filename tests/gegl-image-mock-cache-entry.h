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

#ifndef __GEGL_MOCK_CACHE_ENTRY_H__
#define __GEGL_MOCK_CACHE_ENTRY_H__

#include "image/gegl-cache-entry.h"

#define GEGL_TYPE_MOCK_CACHE_ENTRY               (gegl_mock_cache_entry_get_type ())
#define GEGL_MOCK_CACHE_ENTRY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MOCK_CACHE_ENTRY, GeglMockCacheEntry))
#define GEGL_MOCK_CACHE_ENTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_MOCK_CACHE_ENTRY, GeglMockCacheEntryClass))
#define GEGL_IS_MOCK_CACHE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MOCK_CACHE_ENTRY))
#define GEGL_IS_MOCK_CACHE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_MOCK_CACHE_ENTRY))
#define GEGL_MOCK_CACHE_ENTRY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_MOCK_CACHE_ENTRY, GeglMockCacheEntryClass))

GType gegl_mock_cache_entry_get_type (void) G_GNUC_CONST;

typedef struct _GeglMockCacheEntry GeglMockCacheEntry;
struct _GeglMockCacheEntry
{
  GeglCacheEntry parent;
  gsize data_length;
  gint* data;
};

typedef struct _GeglMockCacheEntryClass GeglMockCacheEntryClass;
struct _GeglMockCacheEntryClass
{
  GeglCacheEntryClass parent_class;
};

GeglMockCacheEntry * gegl_mock_cache_entry_new (gsize length);

#endif
