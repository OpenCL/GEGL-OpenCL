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

#include "gegl-buffer.h"
#include "gegl-cache-entry.h"

#include <stdarg.h>
#include <string.h>

/* these are needed for the factory method*/
#include "gegl-buffer-double.h"


#define GEGL_TYPE_BUFFER_CACHE_ENTRY               (cache_entry_get_type ())
#define GEGL_BUFFER_CACHE_ENTRY(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER_CACHE_ENTRY, GeglBufferCacheEntry))
#define GEGL_BUFFER_CACHE_ENTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER_CACHE_ENTRY, GeglBufferCacheEntryClass))
#define GEGL_IS_BUFFER_CACHE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER_CACHE_ENTRY))
#define GEGL_IS_BUFFER_CACHE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER_CACHE_ENTRY))
#define GEGL_BUFFER_CACHE_ENTRY_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER_CACHE_ENTRY, GeglBufferCacheEntryClass))

enum
{
  PROP_0,
  PROP_NUM_BANKS,
  PROP_ELEMENTS_PER_BANK,
  PROP_LAST
};

typedef struct _GeglBufferCacheEntry GeglBufferCacheEntry;
struct _GeglBufferCacheEntry {
  GeglCacheEntry parent;
  gpointer * banks;
  gsize num_banks;
  gsize bank_length;
};
typedef struct _GeglBufferCacheEntryClass GeglBufferCacheEntryClass;
struct _GeglBufferCacheEntryClass {
  GeglCacheEntryClass parent;
};

static void cache_entry_class_init (gpointer g_class,
				    gpointer class_data);
static void cache_entry_init (GTypeInstance *instance,
			      gpointer g_class);
static gsize cache_entry_flattened_size (const GeglCacheEntry * self);
static void cache_entry_flatten (GeglCacheEntry * self, gpointer buffer, gsize length);
static void cache_entry_unflatten (GeglCacheEntry* self, gpointer buffer, gsize length);
static GType cache_entry_get_type (void);
static GeglBufferCacheEntry * cache_entry_new (GeglBuffer* buffer);

static void finalize (GObject * object);
static void class_init (GeglBufferClass * klass);
static void init (GeglBuffer * self, GeglBufferClass * klass);
static void finalize (GObject * gobject);
static void dispose (GObject * gobject);
static GObject *constructor (GType type, guint n_props,
			     GObjectConstructParam * props);
static void get_property (GObject * gobject, guint prop_id, GValue * value,
			  GParamSpec * pspec);
static void set_property (GObject * gobject, guint prop_id,
			  const GValue * value, GParamSpec * pspec);

static gpointer *alloc_banks (gsize num_banks, gsize bank_length);
static void free_banks (gpointer* banks, gsize num_banks, gsize bank_length);

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
  
  cache_entry_class->flattened_size=cache_entry_flattened_size;
  cache_entry_class->flatten=cache_entry_flatten;
  cache_entry_class->unflatten=cache_entry_unflatten;
  
}

static void
cache_entry_init (GTypeInstance *instance,
		  gpointer g_class)
{
  GeglBufferCacheEntry * self = GEGL_BUFFER_CACHE_ENTRY(instance);
  GeglCacheEntry * entry = GEGL_CACHE_ENTRY(instance);
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
  free_banks(self->banks,self->num_banks,self->bank_length);
  self->banks=NULL;
  
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

static GeglBufferCacheEntry *
cache_entry_new (GeglBuffer* buffer) {
  GeglBufferCacheEntry * entry = g_object_new(GEGL_TYPE_BUFFER_CACHE_ENTRY,NULL);
  GeglCacheEntry * cache_entry = GEGL_CACHE_ENTRY(cache_entry);
  entry->banks=buffer->banks;
  entry->num_banks=buffer->num_banks;
  entry->bank_length=(buffer->elements_per_bank)*(buffer->bytes_per_element);
  return entry;
}

static void
class_init (GeglBufferClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = finalize;
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

}

static void
init (GeglBuffer * self, GeglBufferClass * klass)
{
  self->banks = NULL;
  self->elements_per_bank = 0;
  self->num_banks = 0;
  self->unique_id=0;
  self->lock_count=0;
  self->share_count=0;
  self->cache=NULL;
}

static void
finalize (GObject * gobject)
{
  GeglBuffer *self = GEGL_BUFFER (gobject);

  free_banks (self->banks,self->num_banks,(self->bytes_per_element)*(self->elements_per_bank));

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static GObject *
constructor (GType type, guint n_props, GObjectConstructParam * props)
{
  GObject *gobject =
    G_OBJECT_CLASS (parent_class)->constructor (type, n_props, props);
  GeglBuffer *self = GEGL_BUFFER (gobject);

  self->banks=alloc_banks (self->num_banks,(self->bytes_per_element)*(self->elements_per_bank));

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


void
gegl_buffer_acquire (GeglBuffer * self)
{
  /* FIXME: Implement this */
}

void
gegl_buffer_release (GeglBuffer * self)
{
  /* FIXME: Implement this */
}

GeglBuffer *
gegl_buffer_unshare (const GeglBuffer * source)
{
  return NULL;
}

gboolean
gegl_buffer_is_finalized(const GeglBuffer * source)
{
  /* FIXME: Implement this */
  return FALSE;
}

void
gegl_buffer_lock(GeglBuffer* self)
{
  /* FIXME: Implement this */
}

void
gegl_buffer_unlock(GeglBuffer* self, gboolean is_dirty)
{
}

void
gegl_buffer_attach(GeglBuffer * self, GeglCache * cache)
{
  /* FIXME: Implement this */
}

void
gegl_buffer_detach(GeglBuffer * self)
{
}

gsize
gegl_buffer_get_unique_id (const GeglBuffer* self)
{
  /* FIXME: Implement this */
  return 0;
}
