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
    /* <private> */
    gpointer *banks;
    
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
#ifdef __cplusplus
}
#endif				/* __cplusplus */

#endif
