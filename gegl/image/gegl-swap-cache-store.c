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

#include <unistd.h>

#include <glib-object.h>

#include "gegl-image-types.h"

#include "gegl-cache-entry.h"
#include "gegl-entry-record.h"
#include "gegl-swap-cache-store.h"


typedef struct _SwapStoreData SwapStoreData;
struct _SwapStoreData
{
  gint64 offset;
  gint64 length;
  GList * record_list;
};

typedef struct _SwapGap SwapGap;
struct _SwapGap
{
  gint64 start;
  gint64 end;
};


static void  gegl_swap_cache_store_class_init (GeglSwapCacheStoreClass *klass);
static void  gegl_swap_cache_store_init       (GeglSwapCacheStore      *self);

static void             finalize         (GObject            *object);
static void             dispose          (GObject            *object);
static void             add              (GeglCacheStore     *self,
                                          GeglEntryRecord    *record);
static void             remove           (GeglCacheStore     *self,
                                          GeglEntryRecord    *record);
static void             zap              (GeglCacheStore     *store,
                                          GeglEntryRecord    *record);
static gint64           size             (GeglCacheStore     *self);
static GeglEntryRecord *pop              (GeglCacheStore     *self);
static GeglEntryRecord *peek             (GeglCacheStore     *self);
static void             swap_data_free   (GeglCacheStore     *store,
                                          GeglEntryRecord    *record,
                                          SwapStoreData      *store_data);
static void             swap_data_dirty  (GeglCacheStore     *store,
                                          GeglEntryRecord    *record,
                                          void               *data);
static SwapStoreData *  swap_data_new    (void);
static void             swap_in          (GeglSwapCacheStore *self,
                                          GeglCacheEntry     *entry,
                                          SwapStoreData      *store_data);
static void             swap_out         (GeglSwapCacheStore *self,
                                          GeglCacheEntry     *entry,
                                          SwapStoreData      *store_data);
static void             swap_zap         (GeglSwapCacheStore *self,
                                          GeglCacheEntry     *entry,
                                          SwapStoreData      *store_data);
static gint64           gap_list_get_gap (GeglSwapCacheStore *self,
                                          gsize               length);
static void             gap_list_add_gap (GeglSwapCacheStore *self,
                                          gint64              offset,
                                          gsize               length);
static SwapStoreData *  attach_record    (GeglSwapCacheStore *self,
                                          GeglEntryRecord    *record);
static void             detach_record    (GeglSwapCacheStore *self,
                                          GeglEntryRecord    *record);


/*
 * This global defines the amount a swap file should be extended by.
 * Currently 4MB
 */
static const gint64 SWAP_EXTEND = 4*1024*1024;


G_DEFINE_TYPE (GeglSwapCacheStore, gegl_swap_cache_store, GEGL_TYPE_CACHE_STORE)


GeglSwapCacheStore *
gegl_swap_cache_store_new (const gchar * template)
{
  GeglSwapCacheStore * new_store = g_object_new (GEGL_TYPE_SWAP_CACHE_STORE, NULL);
  gint fd;
  new_store->filename = g_strdup(template);
  fd = g_mkstemp(new_store->filename);
  if (fd == -1)
    {
      g_error("g_mkstemp failed to create the a temporary file using template \"%s\"", template);
      g_object_unref (new_store);
      new_store = NULL;
    }
  else
    {
      new_store->fd = fd;
      new_store->channel = g_io_channel_unix_new (fd);
      /*
       * g_io_channel_set_encoding can't fail here.
       */
      g_io_channel_set_encoding (new_store->channel, NULL, NULL);
    }
  return new_store;
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
  GeglSwapCacheStore * self = GEGL_SWAP_CACHE_STORE (object);
  if (! self->has_disposed )
    {
      self->has_disposed = TRUE;
      g_list_foreach (self->record_head, g_list_free_record, NULL);
      g_list_free (self->record_head);
    }

}

static void
finalize (GObject *object)
{
  GeglSwapCacheStore *store = GEGL_SWAP_CACHE_STORE (object);
  gchar              *locale_filename;

  g_io_channel_unref (store->channel);

  locale_filename = g_filename_from_utf8(store->filename, -1, NULL, NULL, NULL);
  unlink(locale_filename);

  g_free (locale_filename);
  g_free (store->filename);

  G_OBJECT_CLASS (gegl_swap_cache_store_parent_class)->finalize (object);
}

static void
gegl_swap_cache_store_class_init (GeglSwapCacheStoreClass *klass)
{
  GeglCacheStoreClass *cache_store_class = GEGL_CACHE_STORE_CLASS (klass);
  GObjectClass        *gobject_class     = G_OBJECT_CLASS (klass);

  cache_store_class->add    = add;
  cache_store_class->remove = remove;
  cache_store_class->zap    = zap;
  cache_store_class->size   = size;
  cache_store_class->pop    = pop;
  cache_store_class->peek   = peek;

  gobject_class->finalize   = finalize;
  gobject_class->dispose    = dispose;
}

static void
gegl_swap_cache_store_init (GeglSwapCacheStore *self)
{
  self->size         = 0;
  self->record_head  = NULL;
  self->length       = 0;
  self->gaps         = NULL;
  self->filename     = NULL;
  self->has_disposed = FALSE;
}

static void
add (GeglCacheStore * store, GeglEntryRecord * record)
{
  GeglSwapCacheStore * self = GEGL_SWAP_CACHE_STORE (store);
  SwapStoreData * store_data = NULL;

  store_data = attach_record (self, record);
  g_return_if_fail (store_data != NULL);
  swap_in (self, record->entry, store_data);

}

static void remove (GeglCacheStore* store, GeglEntryRecord* record)
{
  GeglSwapCacheStore * self = GEGL_SWAP_CACHE_STORE (store);
  SwapStoreData * store_data = gegl_entry_record_get_store_data (record, store);
  swap_out (self, record->entry, store_data);
  detach_record (self, record);
}

static void zap (GeglCacheStore * store, GeglEntryRecord * record)
{
  GeglSwapCacheStore * self = GEGL_SWAP_CACHE_STORE (store);
  SwapStoreData * store_data = gegl_entry_record_get_store_data (record, store);
  swap_zap (self, record->entry, store_data);
  detach_record (self, record);
  gegl_entry_record_free (record);
}

static gint64
size (GeglCacheStore* store)
{
  GeglSwapCacheStore * self = GEGL_SWAP_CACHE_STORE (store);
  return self->size;
}

static GeglEntryRecord *
pop (GeglCacheStore * store)
{
  GeglEntryRecord * record = peek (store);
  if (record == NULL)
    {
      return NULL;
    }
  remove (store, record);
  return record;
}

static GeglEntryRecord *
peek (GeglCacheStore * store)
{
  /*
   * Peek here doesn't return a valid entry.  Since it is still in
   * the store, it is still swapped out.
   */
  GeglSwapCacheStore * self = GEGL_SWAP_CACHE_STORE (store);
  if (self->record_head == NULL)
    {
      return NULL;
    }
  GeglEntryRecord * record = self->record_head->data;
  g_return_val_if_fail (record != NULL, NULL);
  return record;
}

static void
swap_data_free (GeglCacheStore * store, GeglEntryRecord * record, SwapStoreData * store_data)
{
  g_free (store_data);
}

static void
swap_data_dirty (GeglCacheStore * store, GeglEntryRecord * record, void * data)
{
  GeglSwapCacheStore * self = GEGL_SWAP_CACHE_STORE(store);
  SwapStoreData * store_data = (SwapStoreData *)data;
  /*
   * If we are here, and record->cache_store is not equal to the store
   * that added this SwapStoreData, it means that this swap store data
   * points to data that is swapped out, but not yet dirty.  Now we
   * make it dirty, by adding it back to the gap_list.
   */
  if (record->store != store)
    {
      swap_zap(self, record->entry, store_data);
    }
}

static SwapStoreData *
swap_data_new ()
{
  SwapStoreData * store_data = g_new(SwapStoreData, 1);
  store_data->offset = 0;
  store_data->length = 0;
  store_data->record_list = NULL;
  return store_data;
}

static void
swap_in (GeglSwapCacheStore * self, GeglCacheEntry * entry, SwapStoreData * store_data)
{
  gsize gap_length;
  gint64 offset;
  gchar * buffer;
  gssize bytes_written;
  GError * err = NULL;

  if (store_data->offset == -1)
    {
      /*
       * This means that there is some old, clean data on the disk
       * already.  No swap_in necessary.  Just discard the old
       * gegl_cache_entry data;
       */
      gegl_cache_entry_discard (entry);
      return;
    }

  gap_length = gegl_cache_entry_flattened_size(entry);
  offset = gap_list_get_gap(self, gap_length);

  /*
   * There are plenty of things that can go wrong with mmap.  It
   * should really be error checked here.
   */

  buffer = g_malloc (gap_length);
  gegl_cache_entry_flatten(entry, buffer, gap_length);
  g_io_channel_seek_position (self->channel, offset, G_SEEK_SET, &err);
  if (err != NULL)
    {
      g_error (err->message);
    }
  g_io_channel_write_chars ( self->channel, buffer, gap_length, &bytes_written, &err);
  /*g_io_channel_write_chars ( self->channel, buffer, gap_length, &bytes_written, &err);*/
  if (err != NULL)
    {
      g_error (err->message);
    }
  g_free(buffer);
  store_data->offset = offset;
  store_data->length = gap_length;
}

static void
swap_zap (GeglSwapCacheStore * self, GeglCacheEntry * entry, SwapStoreData * store_data)
{
  if (store_data->length != -1)
    {
      gap_list_add_gap(self, store_data->offset, store_data->length);
    }

  store_data->offset = -1;
  store_data->length = -1;
}

static void
swap_out (GeglSwapCacheStore * self, GeglCacheEntry * entry, SwapStoreData * store_data)
{

  gchar * buffer;
  gsize bytes_read;
  GError * err = NULL;
  /*
   * stupid mmap needing offset to be a multiple of page size.
   * I wonder if this is efficient. I don't know the overhead of mmap.
   */
  /*
   * There are plenty of things that can go wrong with mmap.  It
   * should really be error checked here.
   */

  buffer = g_new (gchar, store_data->length);
  g_io_channel_seek_position (self->channel, store_data->offset, G_SEEK_SET, &err);
  if (err != NULL)
    {
      g_error (err->message);
    }
  g_io_channel_read_chars (self->channel, buffer, store_data->length, &bytes_read, &err);
  if (err != NULL)
    {
      g_error (err->message);
    }
  gegl_cache_entry_unflatten(entry, buffer, store_data->length);

  g_free(buffer);
  /* We don't affect the gap list here.  That is done with the tile is
   *  considered dirty, after calling swap_data_dirty
   */
}

static SwapStoreData *
attach_record(GeglSwapCacheStore * self, GeglEntryRecord * record)
{
  GeglCacheStore * store = GEGL_CACHE_STORE (self);
  GList * record_list;
  SwapStoreData * store_data;
  gegl_entry_record_set_cache_store (record, store);
  store_data = gegl_entry_record_get_store_data (record, store);
  if (store_data != NULL)
    {
      /* This means we are adding a record which has been in this
       * cache before and possibly also has old non-dirty data around.
       * We should reuse it.
       */
      g_return_val_if_fail (store_data->record_list == NULL, NULL);
    }
  else
    {
      store_data = swap_data_new();
    }
  record_list = g_list_append (NULL, record);
  self->record_head = g_list_concat(self->record_head, record_list);
  self->size += gegl_cache_entry_flattened_size (record->entry);
  store_data->record_list = record_list;
  gegl_entry_record_add_store_data_full (record, store, store_data, (GeglStoreDataFunc)swap_data_free, (GeglStoreDataFunc)swap_data_dirty);
  record->status = GEGL_STORED;
  return store_data;
}

static void
detach_record(GeglSwapCacheStore * self, GeglEntryRecord * record)
{
  GeglCacheStore * store = GEGL_CACHE_STORE (self);
  SwapStoreData * store_data;
  store_data = gegl_entry_record_get_store_data (record, store);
  self->record_head = g_list_delete_link (self->record_head, store_data->record_list);
  store_data->record_list = NULL;
  self->size -= gegl_cache_entry_flattened_size (record->entry);
  gegl_entry_record_set_cache_store (record, NULL);
  record->status = GEGL_UNDEFINED;

}

static gint
find_sized_gap (gconstpointer data, gconstpointer userdata)
{
  SwapGap  *gap = (SwapGap *)data;
  gsize  *length = (gsize *)userdata;
  if ((gap->end - gap->start+1) >= *length)
    {
      return 0;
    }
  else
    {
      return -1;
    }

}

static gint
find_after_gap (gconstpointer data, gconstpointer userdata)
{
  /* this returns true when the gap is after the gap_to_match */
  SwapGap * gap = (SwapGap *) data;
  SwapGap * gap_to_match = (SwapGap *) userdata;
  if (gap->start > gap_to_match->end)
    {
      return 0;
    }
  else
    {
      return -1;
    }
}

static gint64
gap_list_get_gap(GeglSwapCacheStore * self, gsize length)
{
  GList * found_link;
  gint64 offset=0;
  found_link = g_list_find_custom(self->gaps, &length, find_sized_gap);
  if (found_link == NULL)
    {
      GError * err = NULL;
      gint64 extend_amount;
      extend_amount = length > SWAP_EXTEND ? length : SWAP_EXTEND;
      g_io_channel_seek_position (self->channel, extend_amount, G_SEEK_END, &err);
      if (err !=NULL)
	{
	  g_error (err->message);
	}
      offset = self->length;
      self->length += extend_amount;
      gap_list_add_gap(self, offset+length, self->length);
    }
  else
    {
      SwapGap * gap = (SwapGap *)found_link->data;
      offset = gap->start;
      if ((gap->end - gap->start) == length)
	{
	  self->gaps = g_list_remove_link (self->gaps, found_link);
	}
      else
	{
	  gap->start += (length + 1);
	}
    }
  return offset;
}

static void
gap_list_add_gap(GeglSwapCacheStore * self, gint64 offset, gsize length)
{
  GList * found_link;
  SwapGap * new_gap = g_new(SwapGap, 1);
  new_gap->start = offset;
  new_gap->end = offset + length-1;
  found_link = g_list_find_custom (self->gaps, new_gap, find_after_gap);
  if (found_link == NULL)
    {
      self->gaps = g_list_append(self->gaps, new_gap);
      return;
    }
  else
    {
      /*
       * found_link should be the first gap after the requested gap.
       */
      GList * after = found_link;
      GList * before = NULL;
      GList * new = NULL;
      before = g_list_previous(after);
      if (before != NULL)
	{
	  SwapGap * before_gap = (SwapGap *)before->data;
	  /*check for curruption*/
	  if (before_gap->end >= new_gap->start)
	    {
	      g_error("Gap List corruption");
	    }
	  /*merge with the previous gap, if they touch*/
	  if (before_gap->end + 1 == new_gap->start)
	    {
	      before_gap->end = new_gap->end;
	      g_free (new_gap);
	      new_gap = before_gap;
	      new = before;
	    }
	}

      if (after != NULL)
	{
	  SwapGap * after_gap = (SwapGap *)after->data;
	  /*check for corruption*/
	  if (after_gap->start <= new_gap->end)
	    {
	      g_error("Gap list corruption");
	    }
	  /*merge with next gap if they touch*/
	  if ((new_gap->end+1) == after_gap->start)
	    {
	      /*merge gaps*/
	      after_gap->start = new_gap->start;
	      /*free new if it was merged from a previous gap*/
	      if (new != NULL)
		{
		  self->gaps = g_list_remove_link(self->gaps, new);
		  g_free (new->data);
		  g_list_free(new);
		}
	      new = after;
	      new_gap = after_gap;
	    }
	}
      if (new == NULL)
	{
	  self->gaps = g_list_insert_before (self->gaps, after, new_gap);
	}
    }
}
