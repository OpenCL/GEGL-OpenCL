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

#ifndef GEGL_ENTRY_RECORD_H
#define GEGL_ENTRY_RECORD_H

#include <glib-object.h>

struct _GeglCache;
struct _GeglCacheStore;
struct _GeglCacheEntry;

typedef enum _GeglCacheStatus GeglCacheStatus;
enum _GeglCacheStatus
  {
    GEGL_STORED,
    GEGL_FETCHED,
    GEGL_DISCARDED,
    GEGL_UNDEFINED
  };

typedef struct _GeglEntryRecord GeglEntryRecord;
struct _GeglEntryRecord
{
  struct _GeglCache * cache;
  struct _GeglCacheStore * store;
  GeglCacheStatus status;
  struct _GeglCacheEntry * entry;
  GHashTable * store_data;
};

typedef void (*GeglStoreDataFunc) (struct _GeglCacheStore *, gpointer data);
GeglEntryRecord * gegl_entry_record_new (struct _GeglCache * cache, struct _GeglCacheEntry * entry);
void gegl_entry_record_free (GeglEntryRecord * record);
void gegl_entry_record_set_cache (GeglEntryRecord * record,
				  struct _GeglCache * cache);
void gegl_entry_record_set_cache_store (GeglEntryRecord * record,
					struct _GeglCacheStore * store);
void gegl_entry_record_add_store_data (GeglEntryRecord * record,
				       struct _GeglCacheStore * store,
				       gpointer data);
void gegl_entry_record_add_store_data_full (GeglEntryRecord * record,
					    struct _GeglCacheStore * store,
					    gpointer data,
					    GeglStoreDataFunc free_data,
					    GeglStoreDataFunc dirty);
void gegl_entry_record_remove_store_data (GeglEntryRecord * record,
					  struct _GeglCacheStore * store,
					  gboolean free_data);
gpointer gegl_entry_record_steal_store_data (GeglEntryRecord * record,
					     struct _GeglCacheStore * store);
gpointer gegl_entry_record_get_store_data (GeglEntryRecord * record,
					   struct _GeglCacheStore * store);
void gegl_entry_record_dirty (GeglEntryRecord * record);
#endif
