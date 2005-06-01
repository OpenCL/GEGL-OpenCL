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

#include "gegl-cache-entry.h"
#include "gegl-heap-cache-store.h"
#include "gegl-entry-record.h"


static void             class_init    (gpointer         g_class,
                                       gpointer         class_data);
static void             dispose       (GObject         *object);
static void             finalize      (GObject         *object);
static void             instance_init (GTypeInstance   *instance,
                                       gpointer         g_class);
static void             add           (GeglCacheStore  *self,
                                       GeglEntryRecord *record);
static void             remove        (GeglCacheStore  *self,
                                       GeglEntryRecord *record);
static void             zap           (GeglCacheStore  *self,
                                       GeglEntryRecord *record);
static gint64           size          (GeglCacheStore  *self);
static GeglEntryRecord *pop           (GeglCacheStore  *self);
static GeglEntryRecord *peek          (GeglCacheStore  *self);


static gpointer parent_class;


GType
gegl_heap_cache_store_get_type(void)
{
  static GType type=0;
  if (!type)
    {
      static const GTypeInfo typeInfo =
	{
	  /* interface types, classed types, instantiated types */
	  sizeof(GeglHeapCacheStoreClass),
	  NULL, /*base_init*/
	  NULL, /* base_finalize */

	  /* classed types, instantiated types */
	  class_init, /* class_init */
	  NULL, /* class_finalize */
	  NULL, /* class_data */

	  /* instantiated types */
	  sizeof(GeglHeapCacheStore),
	  0, /* n_preallocs */
	  instance_init, /* instance_init */

	  /* value handling */
	  NULL /* value_table */
	};

      type = g_type_register_static (GEGL_TYPE_CACHE_STORE ,
				     "GeglHeapCacheStore",
				     &typeInfo,
				     0);
    }
  return type;
}

GeglHeapCacheStore *
gegl_heap_cache_store_new (void)
{
  return g_object_new (GEGL_TYPE_HEAP_CACHE_STORE, NULL);
}


static void
class_init(gpointer g_class,
	   gpointer class_data)
{
  GeglCacheStoreClass * cache_store_class = GEGL_CACHE_STORE_CLASS(g_class);
  GObjectClass * object_class = G_OBJECT_CLASS (g_class);

  cache_store_class->add = add;
  cache_store_class->remove = remove;
  cache_store_class->zap = zap;
  cache_store_class->size = size;
  cache_store_class->pop = pop;
  cache_store_class->peek = peek;

  object_class->dispose = dispose;
  object_class->finalize = finalize;

  parent_class = g_type_class_peek_parent (g_class);
}

static void
g_list_free_record (gpointer data, gpointer user_data)
{
  GeglEntryRecord * record = (GeglEntryRecord *)data;
  gegl_entry_record_free (record);
}

static void
dispose (GObject * object)
{
  GeglHeapCacheStore * self = GEGL_HEAP_CACHE_STORE (object);
  if (! self->has_disposed )
    {
      self->has_disposed = TRUE;
      g_list_foreach (self->record_head, g_list_free_record, NULL);
      g_list_free (self->record_head);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
finalize (GObject * object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
instance_init(GTypeInstance *instance,
	      gpointer g_class)
{
  GeglHeapCacheStore * self = GEGL_HEAP_CACHE_STORE (instance);
  self->size = 0;
  self->record_head = NULL;
  self->has_disposed = FALSE;
}

static void
add (GeglCacheStore * store, GeglEntryRecord * record)
{
  GeglHeapCacheStore * self = GEGL_HEAP_CACHE_STORE (store);
  gegl_entry_record_set_cache_store (record, store);
  GList* record_list = g_list_append (NULL, record);
  self->record_head = g_list_concat(self->record_head, record_list);
  gegl_entry_record_add_store_data (record, store, record_list);

  self->size += gegl_cache_entry_flattened_size (record->entry);
  record->status = GEGL_STORED;
}

static void remove (GeglCacheStore* store, GeglEntryRecord* record)
{
  GeglHeapCacheStore * self = GEGL_HEAP_CACHE_STORE (store);
  GList * record_list = gegl_entry_record_get_store_data (record, store);
  self->record_head = g_list_delete_link (self->record_head, record_list);
  gegl_entry_record_remove_store_data (record, store, FALSE);
  gegl_entry_record_set_cache_store (record, NULL);
  self->size -= gegl_cache_entry_flattened_size (record->entry);
  record->status = GEGL_UNDEFINED;
}

static void zap (GeglCacheStore* store, GeglEntryRecord* record)
{
  remove (store, record);
  gegl_entry_record_free (record);
}

static gint64
size (GeglCacheStore* store)
{
  GeglHeapCacheStore * self = GEGL_HEAP_CACHE_STORE (store);
  return self->size;
}

static GeglEntryRecord * pop (GeglCacheStore * store)
{
  GeglEntryRecord * record = peek (store);
  if (record != NULL)
    {
      remove (store, record);
    }
  return record;
}

static GeglEntryRecord * peek (GeglCacheStore * store)
{
  GeglHeapCacheStore * self = GEGL_HEAP_CACHE_STORE (store);
  if (self->record_head == NULL)
    {
      return NULL;
    }
  GeglEntryRecord * record = self->record_head->data;
  g_return_val_if_fail (record != NULL, NULL);
  return record;
}
