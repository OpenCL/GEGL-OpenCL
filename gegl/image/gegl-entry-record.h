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

#ifndef __GEGL_ENTRY_RECORD_H__
#define __GEGL_ENTRY_RECORD_H__

G_BEGIN_DECLS


typedef void (* GeglStoreDataFunc) (GeglCacheStore *store,
                                    gpointer entry_record,
                                    gpointer data);


struct _GeglEntryRecord
{
  GeglCache       *cache;
  GeglCacheStore  *store;
  GeglCacheStatus  status;
  GeglCacheEntry  *entry;
  GHashTable      *store_data;
};


GeglEntryRecord *gegl_entry_record_new                 (GeglCache         *cache,
                                                        GeglCacheEntry    *entry);
void             gegl_entry_record_free                (GeglEntryRecord   *record);
void             gegl_entry_record_set_cache           (GeglEntryRecord   *record,
                                                        GeglCache         *cache);
void             gegl_entry_record_set_cache_store     (GeglEntryRecord   *record,
                                                        GeglCacheStore    *store);
void             gegl_entry_record_add_store_data      (GeglEntryRecord   *record,
                                                        GeglCacheStore    *store,
                                                        gpointer           data);
void             gegl_entry_record_add_store_data_full (GeglEntryRecord   *record,
                                                        GeglCacheStore    *store,
                                                        gpointer           data,
                                                        GeglStoreDataFunc  free_data,
                                                        GeglStoreDataFunc  dirty);
void             gegl_entry_record_remove_store_data   (GeglEntryRecord   *record,
                                                        GeglCacheStore    *store,
                                                        gboolean           free_data);
gpointer         gegl_entry_record_steal_store_data    (GeglEntryRecord   *record,
                                                        GeglCacheStore    *store);
gpointer         gegl_entry_record_get_store_data      (GeglEntryRecord   *record,
                                                        GeglCacheStore    *store);
void             gegl_entry_record_dirty               (GeglEntryRecord   *record);


G_END_DECLS

#endif /* __GEGL_ENTRY_RECORD_H__ */
