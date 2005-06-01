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

#include "gegl-image-mock-cache-entry.h"
#include <string.h>

static void class_init(gpointer g_class,
                       gpointer class_data);
static void instance_init(GTypeInstance *instance,
                          gpointer g_class);
static void finalize(GObject *object);
static gsize flattened_size (const GeglCacheEntry* self);
static void flatten (GeglCacheEntry* self, gpointer buffer, gsize length);
static void unflatten (GeglCacheEntry* self, const gpointer buffer, gsize length);
static void discard (GeglCacheEntry * entry);

static gpointer parent_class;

static gpointer parent_class;

GType
gegl_mock_cache_entry_get_type (void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof(GeglMockCacheEntryClass),
	  NULL, /*base_init*/
	  NULL, /* base_finalize */
	
	  /* classed types, instantiated types */
	  class_init, /* class_init */
	  NULL, /* class_finalize */
	  NULL, /* class_data */
	
	  /* instantiated types */
	  sizeof(GeglMockCacheEntry),
	  0, /* n_preallocs */
	  instance_init, /* instance_init */
	
	  /* value handling */
	  NULL /* value_table */
	};

      type = g_type_register_static (GEGL_TYPE_CACHE_ENTRY ,
				     "GeglMockCacheEntry",
				     &typeInfo,
				     0);
    }
  return type;
}

static void
class_init(gpointer g_class,
	   gpointer class_data)
{
  GeglCacheEntryClass * cache_entry_class = GEGL_CACHE_ENTRY_CLASS(g_class);
  GObjectClass * object_class = G_OBJECT_CLASS(g_class);

  cache_entry_class->flattened_size = flattened_size;
  cache_entry_class->flatten = flatten;
  cache_entry_class->unflatten = unflatten;
  cache_entry_class->discard = discard;

  object_class->finalize = finalize;
  parent_class = g_type_class_peek_parent (g_class);
}

static void
instance_init(GTypeInstance *instance,
                          gpointer g_class)
{
  GeglMockCacheEntry * mock_entry = GEGL_MOCK_CACHE_ENTRY (instance);
  mock_entry->data = NULL;
  mock_entry->data_length = 0;
}

static void
finalize(GObject *object)
{
  GeglMockCacheEntry * mock_entry = GEGL_MOCK_CACHE_ENTRY (object);
  if (mock_entry->data != NULL)
    {
      g_free (mock_entry->data);
      mock_entry->data = NULL;
      mock_entry->data_length = 0;
    }
  G_OBJECT_CLASS (parent_class)->finalize(object);
}

static gsize
flattened_size (const GeglCacheEntry* self)
{
  GeglMockCacheEntry * mock_entry = GEGL_MOCK_CACHE_ENTRY (self);
  return mock_entry->data_length * sizeof (gint);
}

static void
flatten (GeglCacheEntry* self, gpointer buffer, gsize length)
{
  GeglMockCacheEntry * mock_entry = GEGL_MOCK_CACHE_ENTRY (self);
  memcpy(buffer, mock_entry->data, mock_entry->data_length * sizeof (gint));
  if (mock_entry->data != NULL)
    {
      g_free (mock_entry->data);
      mock_entry->data = NULL;
    }
}

static void
unflatten (GeglCacheEntry* self, const gpointer buffer, gsize length)
{
  GeglMockCacheEntry * mock_entry = GEGL_MOCK_CACHE_ENTRY (self);
  g_return_if_fail (length >= mock_entry->data_length * sizeof (gint));
  if (mock_entry->data != NULL)
    {
      g_free (mock_entry->data);
    }
  mock_entry->data = g_new (gint, mock_entry->data_length);
  memcpy(mock_entry->data, buffer, mock_entry->data_length * sizeof (gint));
}

static void
discard (GeglCacheEntry * self)
{
  GeglMockCacheEntry * mock_entry = GEGL_MOCK_CACHE_ENTRY (self);
  if (mock_entry->data != NULL)
    {
      g_free (mock_entry->data);
      mock_entry->data = NULL;
    }
}

GeglMockCacheEntry *
gegl_mock_cache_entry_new (gsize length)
{
  GeglMockCacheEntry * self = g_object_new (GEGL_TYPE_MOCK_CACHE_ENTRY, NULL);
  self->data = g_new (gint, length);
  self->data_length = length;
  return self;
}
