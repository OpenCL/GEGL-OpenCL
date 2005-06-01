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

#include "gegl-null-cache-store.h"

static void class_init(gpointer g_class,
                       gpointer class_data);
static void instance_init(GTypeInstance *instance,
                          gpointer g_class);
static void add (GeglCacheStore * self, GeglEntryRecord * record);
static void remove (GeglCacheStore* self, GeglEntryRecord* record);
static void zap (GeglCacheStore* self, GeglEntryRecord* record);
static gint64 size (GeglCacheStore* self);
static GeglEntryRecord * pop (GeglCacheStore * self);
static GeglEntryRecord * peek (GeglCacheStore * self);

GType
gegl_null_cache_store_get_type(void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof(GeglNullCacheStoreClass),
	  NULL, /*base_init*/
	  NULL, /* base_finalize */

	  /* classed types, instantiated types */
	  class_init, /* class_init */
	  NULL, /* class_finalize */
	  NULL, /* class_data */

	  /* instantiated types */
	  sizeof(GeglNullCacheStore),
	  0, /* n_preallocs */
	  instance_init, /* instance_init */

	  /* value handling */
	  NULL /* value_table */
	};

      type = g_type_register_static (GEGL_TYPE_CACHE_STORE ,
				     "GeglNullCacheStore",
				     &typeInfo,
				     0);
    }
  return type;
}
GeglNullCacheStore *
gegl_null_cache_store_new (GeglCacheStatus status)
{
  GeglNullCacheStore * new_store = g_object_new (GEGL_TYPE_NULL_CACHE_STORE, NULL);
  new_store->status = status;
  return new_store;
}


static void
class_init(gpointer g_class,
	   gpointer class_data)
{
  GeglCacheStoreClass * cache_store_class = GEGL_CACHE_STORE_CLASS(g_class);

  cache_store_class->add=add;
  cache_store_class->remove = remove;
  cache_store_class->size = size;
  cache_store_class->pop = pop;
  cache_store_class->peek = peek;
  cache_store_class->zap = zap;
}

static void
instance_init(GTypeInstance *instance,
	      gpointer g_class)
{
  GeglNullCacheStore * self = GEGL_NULL_CACHE_STORE (instance);
  self->status = GEGL_UNDEFINED;
  self->record_head = NULL;
}

static void
add (GeglCacheStore * store, GeglEntryRecord * record)
{
  GeglNullCacheStore * self = GEGL_NULL_CACHE_STORE (store);
  gegl_entry_record_set_cache_store (record, store);
  GList* record_list = g_list_append (NULL, record);
  self->record_head = g_list_concat(self->record_head, record_list);
  gegl_entry_record_add_store_data (record, store, record_list);

  record->status = self->status;
  if (record->entry != NULL)
    {
      g_object_unref (G_OBJECT (record->entry));
      record->entry = NULL;
    }
}

static void
remove (GeglCacheStore* store, GeglEntryRecord* record)
{
  GeglNullCacheStore * self = GEGL_NULL_CACHE_STORE (store);
  GList * record_list = gegl_entry_record_get_store_data (record, store);
  self->record_head = g_list_delete_link (self->record_head, record_list);
  gegl_entry_record_remove_store_data (record, store, FALSE);
  gegl_entry_record_set_cache_store (record, NULL);
  record->status = GEGL_UNDEFINED;
}
static void
zap (GeglCacheStore* store, GeglEntryRecord* record)
{
  remove (store, record);
  gegl_entry_record_free (record);
}

static gint64
size (GeglCacheStore* self)
{
  return 0;
}

static GeglEntryRecord * pop (GeglCacheStore * store)
{
  GeglEntryRecord * record = peek (store);
  if (record == NULL)
    {
      return NULL;
    }
  remove(store, record);
  return record;
}
static GeglEntryRecord * peek (GeglCacheStore * store)
{
  GeglNullCacheStore * self = GEGL_NULL_CACHE_STORE (store);
  if (self->record_head == NULL)
    {
      return NULL;
    }
  GeglEntryRecord * record = self->record_head->data;
  g_return_val_if_fail (record != NULL, NULL);
  return record;
}
