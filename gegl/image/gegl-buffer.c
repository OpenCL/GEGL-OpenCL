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

#include <stdarg.h>
#include <string.h>

#include <glib-object.h>

#include "gegl-image-types.h"

#include "gegl-buffer.h"
#include "gegl-buffer-double.h"
#include "gegl-cache.h"
#include "gegl-cache-entry.h"

enum
{
  PROP_0,
  PROP_NUM_BANKS,
  PROP_ELEMENTS_PER_BANK,
  PROP_CACHE,
  PROP_LAST
};

#define GEGL_TYPE_BUFFER_CACHE_ENTRY               (cache_entry_get_type ())
#define GEGL_BUFFER_CACHE_ENTRY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER_CACHE_ENTRY, GeglBufferCacheEntry))
#define GEGL_BUFFER_CACHE_ENTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER_CACHE_ENTRY, GeglBufferCacheEntryClass))
#define GEGL_IS_BUFFER_CACHE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER_CACHE_ENTRY))
#define GEGL_IS_BUFFER_CACHE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER_CACHE_ENTRY))
#define GEGL_BUFFER_CACHE_ENTRY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER_CACHE_ENTRY, GeglBufferCacheEntryClass))

typedef struct _GeglBufferCacheEntry      GeglBufferCacheEntry;
typedef struct _GeglBufferCacheEntryClass GeglBufferCacheEntryClass;

struct _GeglBufferCacheEntry
{
  GeglCacheEntry  parent_instance;

  gpointer * banks;
  gsize num_banks;
  gsize bank_length;
};

struct _GeglBufferCacheEntryClass
{
  GeglCacheEntryClass  parent_class;
};

static gpointer cache_entry_parent_class;

static void                  cache_entry_class_init     (gpointer               g_class,
                                                         gpointer               class_data);
static void                  cache_entry_init           (GTypeInstance         *instance,
                                                         gpointer               g_class);
static void                  cache_entry_finalize       (GObject               *gobject);
static gsize                 cache_entry_flattened_size (const GeglCacheEntry  *self);
static void                  cache_entry_flatten        (GeglCacheEntry        *self,
                                                         gpointer               buffer,
                                                         gsize                  length);
static void                  cache_entry_unflatten      (GeglCacheEntry        *self,
                                                         gpointer               buffer,
                                                         gsize                  length);
static GType                 cache_entry_get_type       (void);
static void                  cache_entry_discard        (GeglCacheEntry        *self);
static GeglBufferCacheEntry *cache_entry_new            (GeglBuffer            *buffer);
static void                  cache_entry_set            (GeglBufferCacheEntry  *self,
                                                         GeglBuffer            *buffer);
static void                  class_init                 (GeglBufferClass       *klass);
static void                  init                       (GeglBuffer            *self,
                                                         GeglBufferClass       *klass);
static void                  finalize                   (GObject               *gobject);
static void                  dispose                    (GObject               *gobejct);
static GObject *             constructor                (GType                  type,
                                                         guint                  n_props,
                                                         GObjectConstructParam *props);
static void                  get_property               (GObject               *gobject,
                                                         guint                  prop_id,
                                                         GValue                *value,
                                                         GParamSpec            *pspec);
static void                  set_property               (GObject               *gobject,
                                                         guint                  prop_id,
                                                         const GValue          *value,
                                                         GParamSpec            *pspec);
static gpointer *            alloc_banks                (gsize                  num_banks,
                                                         gsize                  bank_length);
static void                  free_banks                 (gpointer              *banks,
                                                         gsize                  num_banks,
                                                         gsize                  bank_length);


static gpointer parent_class = NULL;

GType
gegl_buffer_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo = {
	sizeof (GeglBufferClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) class_init,
	(GClassFinalizeFunc) NULL,
	NULL,
	sizeof (GeglBuffer),
	0,
	(GInstanceInitFunc) init,
	NULL
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT,
				     "GeglBuffer",
				     &typeInfo, G_TYPE_FLAG_ABSTRACT);
    }
  return type;
}

static GType
cache_entry_get_type (void)
{
  static GType type = 0;
  if (!type)
    {
      static const GTypeInfo typeInfo = {
	/* interface types, classed types, instantiated types */
	sizeof (GeglBufferCacheEntryClass),
	NULL,			/* base_init */
	NULL,			/* base_finalize */

	/* classed types, instantiated types */
	cache_entry_class_init,		/* class_init */
	NULL,			/* class_finalize */
	NULL,			/* class_data */

	/* instantiated types */
	sizeof (GeglBufferCacheEntry),
	0,			/* n_preallocs */
	cache_entry_init,		/* instance_init */

	/* value handling */
	NULL			/* value_table */
      };

      type = g_type_register_static (GEGL_TYPE_CACHE_ENTRY,
				     "GeglBufferCacheEntry", &typeInfo, 0);
    }
  return type;
}

static void
cache_entry_class_init (gpointer g_class,
			gpointer class_data)
{
  GeglCacheEntryClass *cache_entry_class= GEGL_CACHE_ENTRY_CLASS(g_class);
  GObjectClass * object_class = G_OBJECT_CLASS(g_class);
  object_class->finalize=cache_entry_finalize;
  cache_entry_class->flattened_size=cache_entry_flattened_size;
  cache_entry_class->flatten=cache_entry_flatten;
  cache_entry_class->unflatten=cache_entry_unflatten;
  cache_entry_class->discard = cache_entry_discard;

  cache_entry_parent_class = g_type_class_peek_parent (g_class);
}

static void
cache_entry_finalize (GObject * gobject)
{
  GeglBufferCacheEntry * self = GEGL_BUFFER_CACHE_ENTRY(gobject);
  if (self->banks != NULL )
    {
      free_banks (self->banks, self->num_banks, self->bank_length);
    }
  G_OBJECT_CLASS(cache_entry_parent_class)->finalize (gobject);
}

static void
cache_entry_init (GTypeInstance *instance,
		  gpointer g_class)
{
  GeglBufferCacheEntry * self = GEGL_BUFFER_CACHE_ENTRY(instance);
  self->banks=NULL;
  self->num_banks=0;
  self->bank_length=0;
}

static gsize
cache_entry_flattened_size (const GeglCacheEntry * entry) {
  GeglBufferCacheEntry * self = GEGL_BUFFER_CACHE_ENTRY(entry);
  return (self->num_banks) * (self->bank_length);
}
static void
cache_entry_flatten (GeglCacheEntry * entry, gpointer buffer, gsize length) {
  GeglBufferCacheEntry * self = GEGL_BUFFER_CACHE_ENTRY(entry);
  g_return_if_fail(length >= cache_entry_flattened_size(entry));
  g_return_if_fail(self->banks == NULL);
  gsize i;
  for (i=0;i<(self->num_banks);i++) {
    memcpy(buffer,self->banks[i],self->bank_length);
    buffer+=self->bank_length;
  }
  cache_entry_discard (entry);
}
static void
cache_entry_unflatten (GeglCacheEntry * entry, gpointer buffer, gsize length) {
  g_return_if_fail(length >= cache_entry_flattened_size(entry));
  GeglBufferCacheEntry * self = GEGL_BUFFER_CACHE_ENTRY(entry);
  gsize i;
  if (self->banks != NULL) {
    free_banks(self->banks,self->num_banks,self->bank_length);
  }
  self->banks=alloc_banks(self->num_banks,self->bank_length);
  for (i=0;i<(self->num_banks);i++) {
    memcpy(self->banks[i],buffer,self->bank_length);
    buffer+=self->bank_length;
  }
}
static void
cache_entry_discard (GeglCacheEntry * entry)
{
  GeglBufferCacheEntry * self = GEGL_BUFFER_CACHE_ENTRY(entry);
  free_banks(self->banks,self->num_banks,self->bank_length);
  self->banks=NULL;
}
static GeglBufferCacheEntry *
cache_entry_new (GeglBuffer* buffer) {
  GeglBufferCacheEntry * entry = g_object_new(GEGL_TYPE_BUFFER_CACHE_ENTRY,NULL);
  g_return_val_if_fail (buffer->banks != NULL, NULL);
  cache_entry_set (entry, buffer);
  return entry;
}

static void
cache_entry_set (GeglBufferCacheEntry * self, GeglBuffer * buffer)
{
  self->banks=buffer->banks;
  self->num_banks=buffer->num_banks;
  self->bank_length=(buffer->elements_per_bank)*(buffer->bytes_per_element);
}

static void
class_init (GeglBufferClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = finalize;
  gobject_class->dispose = dispose;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->constructor = constructor;

  g_object_class_install_property (gobject_class, PROP_ELEMENTS_PER_BANK,
				   g_param_spec_int ("elements_per_bank",
						     "Elements Per Bank",
						     "GeglBuffer elements in each bank",
						     0,
						     G_MAXINT,
						     0,
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_NUM_BANKS,
				   g_param_spec_int ("num_banks",
						     "Number of Banks ",
						     "GeglBuffer number of banks",
						     0,
						     G_MAXINT,
						     0,
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_CACHE,
				   g_param_spec_pointer ("cache",
							 "The Cache",
							 "The cache that backs this GeglBuffer",
							 G_PARAM_CONSTRUCT_ONLY |
							 G_PARAM_READWRITE));

}

static void
init (GeglBuffer * self, GeglBufferClass * klass)
{
  self->banks = NULL;
  self->elements_per_bank = 0;
  self->num_banks = 0;
  self->entry_id=0;
  self->lock_count=0;
  self->share_count=0;
  self->cache=NULL;
  self->entry = NULL;
  self->has_expired = FALSE;
  self->has_disposed = FALSE;
}

static void
finalize (GObject * gobject)
{
  GeglBuffer *self = GEGL_BUFFER (gobject);

  if (self->banks != NULL)
    {
      free_banks (self->banks,self->num_banks,(self->bytes_per_element)*(self->elements_per_bank));
    }
  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
dispose (GObject * gobject)
{
  GeglBuffer * self = GEGL_BUFFER (gobject);
  if (!self->has_disposed)
    {
      GeglBuffer * buffer = GEGL_BUFFER (gobject);
      if (buffer->cache != NULL)
	{
	  g_object_unref (buffer->cache);
	  buffer->cache = NULL;
	}
      if (buffer->entry != NULL)
	{
	  g_object_unref (G_OBJECT(buffer->entry));
	  buffer->entry = NULL;
	}
      self->has_disposed = TRUE;
    }
}

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam * props)
{
  GObject *gobject =
    G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglBuffer *self = GEGL_BUFFER (gobject);

  self->banks=alloc_banks (self->num_banks,(self->bytes_per_element)*(self->elements_per_bank));
  ++(self->share_count);
  return gobject;
}

static void
set_property (GObject * gobject,
	      guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GeglBuffer *self = GEGL_BUFFER (gobject);

  switch (prop_id)
    {
    case PROP_ELEMENTS_PER_BANK:
      self->elements_per_bank = g_value_get_int (value);
      break;
    case PROP_NUM_BANKS:
      self->num_banks = g_value_get_int (value);
      break;
    case PROP_CACHE:
      self->cache = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
get_property (GObject * gobject,
	      guint prop_id, GValue * value, GParamSpec * pspec)
{
  GeglBuffer *self = GEGL_BUFFER (gobject);

  switch (prop_id)
    {
    case PROP_ELEMENTS_PER_BANK:
      g_value_set_int (value, gegl_buffer_get_elements_per_bank (self));
      break;
    case PROP_NUM_BANKS:
      g_value_set_int (value, gegl_buffer_get_num_banks (self));
      break;
    case PROP_CACHE:
      g_value_set_pointer (value, self->cache);
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static gpointer *
alloc_banks (gsize num_banks, gsize bank_length)
{
  gpointer *banks = NULL;
  gint i;

  g_return_val_if_fail (num_banks > 0, NULL);
  g_return_val_if_fail (bank_length > 0, NULL);

  banks = g_new (gpointer, num_banks);

  for (i = 0; i < num_banks; i++)
    {
      banks[i] =
	g_malloc (bank_length);
    }
  return banks;
}

static void
free_banks (gpointer* banks, gsize num_banks, gsize bank_length)
{
  gint i;
  for (i = 0; i < num_banks; i++)
    g_free (banks[i]);

  g_free (banks);
}


/**
 * gegl_buffer_elements_per_bank:
 * @self: a #GeglBuffer
 *
 * Gets the elements per bank.
 *
 * Returns: number of elements per bank.
 **/
gint
gegl_buffer_get_elements_per_bank (const GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), 0);

  return self->elements_per_bank;
}

/**
 * gegl_buffer_num_banks:
 * @self: a #GeglBuffer
 *
 * Gets the number of banks.
 *
 * Returns: number of banks.
 **/
gint
gegl_buffer_get_num_banks (const GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, 0);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), 0);

  return self->num_banks;
}

/**
 * gegl_buffer_get_banks:
 * @self: a #GeglBuffer
 *
 * Gets the data pointers.
 *
 * Returns: pointers to the buffers.
 **/
gpointer *
gegl_buffer_get_banks (const GeglBuffer * self)
{
  g_return_val_if_fail (self != NULL, NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (self), NULL);

  return self->banks;
}

gdouble
gegl_buffer_get_element_double (const GeglBuffer * self, gint bank,
				gint index)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (self), 0);
  GeglBufferClass *klass = GEGL_BUFFER_GET_CLASS (self);
  g_return_val_if_fail (klass->get_element_double != NULL, 0.0);

  return klass->get_element_double (self, bank, index);
}

void
gegl_buffer_set_element_double (GeglBuffer * self,
				gint bank, gint index, gdouble elem)
{
  g_return_if_fail (GEGL_IS_BUFFER (self));
  GeglBufferClass *klass = GEGL_BUFFER_GET_CLASS (self);
  g_return_if_fail (klass->set_element_double != NULL);

  return klass->set_element_double (self, bank, index, elem);
}

TransferType
gegl_buffer_get_transfer_type (const GeglBuffer * self)
{
  g_return_val_if_fail (GEGL_IS_BUFFER (self), TYPE_UNKNOWN);

  return self->transfer_type;
}

GeglBuffer *
gegl_buffer_create (TransferType type, const gchar * first_property_name, ...)
{
  GeglBuffer *buff = NULL;
  va_list args;

  va_start (args, first_property_name);
  switch (type)
    {
    case TYPE_DOUBLE:
      buff =
	(GeglBuffer *) g_object_new_valist (GEGL_TYPE_BUFFER_DOUBLE,
					    first_property_name, args);
      break;
    default:
      g_error ("Can not create GeglBuffer subclass of type %d", type);
    }
  va_end (args);
  return buff;
}


/**
 * gegl_buffer_acquire:
 * @self:
 *
 * Acquire a reference and increment the share count.  Use this when
 * you wish to indicate you want a reference and are interested in the
 * buffer data.
 **/
void
gegl_buffer_acquire (GeglBuffer * self)
{
  g_return_if_fail (!gegl_buffer_is_finalized(self));
  GObject * object = G_OBJECT(self);
  g_object_ref(object);
  ++(self->share_count);
}

/**
 * gegl_buffer_release:
 * @self:
 *
 * Release a reference and decrement the share count.  Use this to
 * indicate you are finished with the buffer if and only if you
 * acquired your reference with gegl_buffer_acquire
 **/
void
gegl_buffer_release (GeglBuffer * self)
{
  g_return_if_fail (self->share_count > 0);
  GObject * object = G_OBJECT(self);
  --(self->share_count);
  if (self->share_count == 0)
    {
      if (self->cache != NULL)
	{
	  if (self->entry_id != 0)
	    {
	      gegl_cache_flush (self->cache, self->entry_id);
	      self->entry_id=0;
	    }
	}
    }
  g_object_unref(object);
}

/**
 * gegl_buffer_unshare:
 * @source:
 *
 * This gives you an unshared copy of the buffer source.  This copy is
 * acquired and locked. This may be source, if source has a
 * share_count of 1.  If a new buffer is allocated, then source is
 * unlocked and unacquired exactly once.  Always make a copy before
 * writing to the banks, unless you want to write to all shared copies
 * of this buffer.  Always make sure source is acquired and locked
 * when calling this.
 *
 * Return value:
 **/
GeglBuffer *
gegl_buffer_unshare (GeglBuffer * source)
{
  GeglBuffer * new_buffer;
  int i=0;
  g_return_val_if_fail (source->share_count >0,NULL);
  if (source->share_count == 1)
    {
      return source;
    }
  new_buffer = gegl_buffer_create (source->transfer_type,
				   "elements_per_bank", source->elements_per_bank,
				   "num_banks", source->num_banks,
				   "cache", source->cache, NULL);
  gegl_buffer_lock (new_buffer);
  for (i=0;i<(source->num_banks);i++)
    {
      memcpy(new_buffer->banks[i], source->banks[i],
	     (source->elements_per_bank) * (source->bytes_per_element));
    }
  gegl_buffer_unlock (source, FALSE);
  gegl_buffer_release (source);
  return new_buffer;
}

/**
 * gegl_buffer_is_finalized:
 * @source: the buffer
 *
 * a buffer is finalized if the share_count is zero, but the ref_count
 * is non-zero.  This might happen if some g_object_ref references were
 * still around after all gegl_buffer_acquire references had been released.
 *
 * One should only acquire references that were acquired when handed to you.
 * In otherwords if someone hands you a reference that they acquired with
 * g_object_ref, don't gegl_buffer_acquire it.  That would be bad.

 * Return value: %TRUE if the buffer is finalized.
 **/
gboolean
gegl_buffer_is_finalized(const GeglBuffer * source)
{
  return (source->share_count <= 0);
}

/**
 * gegl_buffer_lock:
 * @self:
 *
 * This locks the buffer into memory. As long as the buffer has at
 * least one lock held on it, the banks are gaurenteed to be on the
 * heap.  Locks can be nested.  Always unlock as many times as you
 * locked.
 **/
void
gegl_buffer_lock(GeglBuffer* self)
{
  ++(self->lock_count);
  if ((self->banks == NULL) && (self->entry_id != 0))
    {
      g_return_if_fail (self->cache != NULL);
      gint fetch_results = gegl_cache_fetch (self->cache, self->entry_id, &(self->entry));
      if (fetch_results == GEGL_FETCH_EXPIRED)
	{
	  gegl_cache_flush (self->cache, self->entry_id);
	  self->entry_id = 0;
	  self->has_expired = TRUE;
	  return;
	}
      g_return_if_fail (fetch_results == GEGL_FETCH_SUCCEEDED);
      GeglBufferCacheEntry * bcache_entry;
      bcache_entry = GEGL_BUFFER_CACHE_ENTRY (self->entry);
      self->banks = bcache_entry->banks;
      bcache_entry->banks = NULL;
    }
}

/**
 * gegl_buffer_unlock:
 * @self:
 * @is_dirty:
 *
 * Once a buffer is completely unlocked, it is pushed into the cache,
 * if it exists.  Otherwise it stays on the heap and is released when
 * the share_count goes to zero.  is_dirty indicates whether to
 * destroy any disk-resident copies of this tile.  If you are unsure,
 * TRUE is the safe answer.  If is_dirty is false the disk-resident
 * copy (if it exists) of the tile may be reused.  is_dirty is
 * immediatly propagated to the cache, not stored in the buffer.
 **/
void
gegl_buffer_unlock(GeglBuffer* self, gboolean is_dirty)
{
  g_return_if_fail (self->lock_count > 0);
  --(self->lock_count);
  if (self->share_count <= 0)
    {
      g_warning ("GeglBuffer unlocked after final release");
      return;
    }
  if (self->cache == NULL || self->has_expired)
    {
      return;
    }
  if (is_dirty && (self->entry_id != 0))
    {
      gegl_cache_mark_as_dirty (self->cache, self->entry_id);
    }
  if (self->lock_count == 0)
    {
      if (self->entry == NULL)
	{
	  self->entry = GEGL_CACHE_ENTRY (cache_entry_new (self));
	}
      else
	{
	  GeglBufferCacheEntry * buffer_entry;
	  buffer_entry = GEGL_BUFFER_CACHE_ENTRY(self->entry);
	  cache_entry_set (buffer_entry, self);
	}
      self->banks = NULL;
      if (self->entry_id == 0)
	{
	  gint put_results = gegl_cache_put (self->cache,
					     self->entry,
					     &(self->entry_id));
	  g_return_if_fail (put_results == GEGL_PUT_SUCCEEDED);
	}
      else
	{
	  GeglFetchResults fetch_results;
	  fetch_results = gegl_cache_unfetch (self->cache,
					      self->entry_id,
					      self->entry);
	  g_return_if_fail (fetch_results == GEGL_FETCH_SUCCEEDED);
	}
      g_object_unref (self->entry);
      self->entry = NULL;
    }
}

/**
 * gegl_buffer_attach:
 * @self:  the buffer
 * @cache: the cache to attach the buffer to
 *
 * Indicate that this buffer should use the GeglCache cache for any of
 * its cache operations.  The persistance of this buffer directly
 * reflects the persistence settings of the cache.
 **/
void
gegl_buffer_attach(GeglBuffer * self, GeglCache * cache)
{
  gegl_buffer_lock (self);
  if (self->cache != NULL )
    {
      g_object_unref (self->cache);
    }
  g_object_ref (cache);
  self->cache=cache;
  self->entry_id = 0;
  gegl_buffer_unlock (self, TRUE);
}

/**
 * gegl_buffer_detach:
 * @self: the buffer
 *
 * Detach this buffer from its cache.  This indicates that this buffer
 * is no longer cached.  Doing this will mean that the buffer is
 * always on the heap.
 **/
void
gegl_buffer_detach(GeglBuffer * self)
{
  gegl_buffer_lock (self);
  gegl_cache_flush (self->cache, self->entry_id);
  if ( self->entry != NULL )
    {
      g_object_unref (self->entry);
      self->entry = NULL;
    }
  if (self->cache != NULL )
    {
      g_object_unref (self->cache);
      self->cache=NULL;
    }
  if ( self->entry != NULL )
    {
      g_object_unref (self->entry);
      self->entry = NULL;
    }
  self->entry_id=0;
  gegl_buffer_unlock (self, FALSE);
}
