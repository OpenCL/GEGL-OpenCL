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

#ifndef __GEGL_BUFFER_H__
#define __GEGL_BUFFER_H__

#include "gegl-object.h"
#include "gegl-cache.h"

#ifdef __cplusplus
extern "C"
{
#endif				/* __cplusplus */

#define GEGL_TYPE_BUFFER               (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))
#define GEGL_BUFFER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER, GeglBufferClass))
#define GEGL_IS_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER))
#define GEGL_IS_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER))
#define GEGL_BUFFER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER, GeglBufferClass))

  GType gegl_buffer_get_type (void);

/* I hate type enums.  This is what JAI does, and I just do what
 * they do for now, and try to figure out something better later.
 */

  typedef enum
  {
    TYPE_DOUBLE,
    TYPE_UNKNOWN
  } TransferType;

  typedef struct _GeglBuffer GeglBuffer;
  struct _GeglBuffer
  {
    GeglObject object;

    gint elements_per_bank;
    gint num_banks;
    gint bytes_per_element;
    TransferType transfer_type;
    GeglCache* cache;
    /* <private> */
    gpointer *banks;
    /*
     * Share count is basically a reference count.
     * However it really is only keeping track of the 
     * number of refrences that care about accessing
     * the data.  Updates to share_count should
     * update refcount atomicly.
     */
    gshort share_count;
    gshort lock_count;
    gsize entry_id;
    GeglCacheEntry * entry;
    gboolean has_expired;
    gboolean has_disposed;
  };

  typedef struct _GeglBufferClass GeglBufferClass;
  struct _GeglBufferClass
  {
    GeglObjectClass object_class;
    gdouble (*get_element_double) (const GeglBuffer * self, gint bank,
				     gint index);
    void (*set_element_double) (GeglBuffer * self, gint bank, gint index,
				gdouble elem);
    
  };

  TransferType gegl_buffer_get_transfer_type (const GeglBuffer * self);
  gint gegl_buffer_get_num_banks (const GeglBuffer * self);
  gint gegl_buffer_get_elements_per_bank (const GeglBuffer * self);
  gpointer *gegl_buffer_get_banks (const GeglBuffer * self);

  gdouble gegl_buffer_get_element_double (const GeglBuffer * self, gint bank,
					  gint index);
  void gegl_buffer_set_element_double (GeglBuffer * self, gint bank,
				       gint index, gdouble elem);

  GeglBuffer *gegl_buffer_create (TransferType type,
				  const gchar * first_property_name, ...);
  /* Acquire a reference and increment the share count.
   * Use this when you wish to indicate you want a reference and
   * are interested in the buffer data.
  */
  void gegl_buffer_acquire (GeglBuffer * self);
  /* Release a reference and decrement the share count.
   * Use this to indicate you are finished with the buffer if and only
   * if you acquired your reference with gegl_buffer_acquire
   */
  void gegl_buffer_release (GeglBuffer * self);

  /* This gives you an unshared copy of the buffer source.  This copy
   * is acquired and locked. This may be source, if source has a
   * share_count of 1.  If a new buffer is allocated, then source is
   * unlocked and unacquired exactly once.  Always make a copy before
   * writing to the banks, unless you want to write to all shared
   * copies of this buffer.  Always make sure source is acquired and
   * locked when calling this.
   */
  GeglBuffer *gegl_buffer_unshare (GeglBuffer * source);

  /* a buffer is finalized if the share_count is zero, but the ref_count
   * is non-zero.  This might happen if some g_object_ref references were
   * still around after all gegl_buffer_acquire references had been released.
   *
   * One should only acquire references that were acquired when handed to you.
   * In otherwords if someone hands you a reference that they acquired with
   * g_object_ref, don't gegl_buffer_acquire it.  That would be bad.
   */
  gboolean gegl_buffer_is_finalized(const GeglBuffer * source);

  /* This locks the buffer into memory. As long as the buffer has at least one
   * lock held on it, the banks are gaurenteed to be on the heap.  Locks
   * can be nested.  Always unlock as many times as you locked.
   */
  void gegl_buffer_lock(GeglBuffer* self);

  /*
   * Once a buffer is completely unlocked, it is pushed into the
   * cache, if it exists.  Otherwise it says on the heap and is
   * released when the share_count goes to zero.  is_dirty indicates
   * whether to destroy any disk-resident copies of this tile.  If you
   * are unsure, TRUE is the safe answer.  If is_dirty is false the
   * disk-resident copy (if it exists) of the tile may be reused.
   * is_dirty is immediatly propagated to the cache, not stored in the
   * buffer.
   */
  void gegl_buffer_unlock(GeglBuffer* self, gboolean is_dirty);

  /*
   * Indicate that this buffer should use the GeglCache cache for any
   * of its cache operations.  The persistance of this buffer directly
   * reflects the persistence settings of the cache.
   */
  void gegl_buffer_attach(GeglBuffer * self, GeglCache * cache);
  /*
   * Detach this buffer from it's cache.  This indicates that this
   * buffer is no longer cached.  Doing this will mean that the buffer
   * is always on the heap.
   */
  void gegl_buffer_detach(GeglBuffer * self);

    
  
#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
