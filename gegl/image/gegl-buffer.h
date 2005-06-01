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

G_BEGIN_DECLS


#define GEGL_TYPE_BUFFER            (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))
#define GEGL_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER, GeglBufferClass))
#define GEGL_IS_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER))
#define GEGL_IS_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER))
#define GEGL_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER, GeglBufferClass))


typedef struct _GeglBufferClass GeglBufferClass;

struct _GeglBuffer
{
  GeglObject      parent_instance;

  gint            elements_per_bank;
  gint            num_banks;
  gint            bytes_per_element;
  TransferType    transfer_type;
  GeglCache      *cache;

  /* <private> */
  gpointer       *banks;
  /*
   * Share count is basically a reference count.
   * However it really is only keeping track of the
   * number of refrences that care about accessing
   * the data.  Updates to share_count should
   * update refcount atomicly.
   */
  gshort          share_count;
  gshort          lock_count;
  gsize           entry_id;
  GeglCacheEntry *entry;
  gboolean        has_expired;
  gboolean        has_disposed;
};

struct _GeglBufferClass
{
  GeglObjectClass object_class;

  gdouble (* get_element_double) (const GeglBuffer *self,
                                  gint              bank,
                                  gint              index);
  void    (* set_element_double) (GeglBuffer       *self,
                                  gint              bank,
                                  gint              index,
                                  gdouble           elem);
};


GType        gegl_buffer_get_type              (void) G_GNUC_CONST;

TransferType gegl_buffer_get_transfer_type     (const GeglBuffer *self);

gint         gegl_buffer_get_num_banks         (const GeglBuffer *self);
gint         gegl_buffer_get_elements_per_bank (const GeglBuffer *self);
gpointer   * gegl_buffer_get_banks             (const GeglBuffer *self);

gdouble      gegl_buffer_get_element_double    (const GeglBuffer *self,
                                                gint              bank,
                                                gint              index);
void         gegl_buffer_set_element_double    (GeglBuffer       *self,
                                                gint              bank,
                                                gint              index,
                                                gdouble           elem);

GeglBuffer * gegl_buffer_create                (TransferType      type,
                                                const gchar      *first_property_name,
                                                ...);

void         gegl_buffer_acquire               (GeglBuffer       *self);
void         gegl_buffer_release               (GeglBuffer       *self);

GeglBuffer * gegl_buffer_unshare               (GeglBuffer       *source);

gboolean     gegl_buffer_is_finalized          (const GeglBuffer *source);

void         gegl_buffer_lock                  (GeglBuffer       *self);
void         gegl_buffer_unlock                (GeglBuffer       *self,
                                                gboolean          is_dirty);

void         gegl_buffer_attach                (GeglBuffer       *self,
                                                GeglCache        *cache);
void         gegl_buffer_detach                (GeglBuffer       *self);


G_END_DECLS

#endif /* __GEGL_BUFFER_H__ */
