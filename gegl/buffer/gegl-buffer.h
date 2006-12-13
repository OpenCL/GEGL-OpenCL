/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>

#include "gegl-buffer-types.h"
#include <babl/babl.h>

#ifndef _GEGL_BUFFER_H
#define _GEGL_BUFFER_H

#define GEGL_TYPE_BUFFER            (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))
#define GEGL_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER, GeglBufferClass))
#define GEGL_IS_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER))
#define GEGL_IS_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER))
#define GEGL_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER, GeglBufferClass))

#include "gegl-tile-traits.h"

struct _GeglBuffer
{
  GeglTileTraits parent_object;
  gpointer       format;
  gint           x;   /* the relative position in relation to parent buffer */
  gint           y;   /* the relative position in relation to parent buffer */
  gint           width;
  gint           height;

  gint           shift_x;
  gint           shift_y;
  gint           total_shift_x;
  gint           total_shift_y;

  gint           abyss_x;
  gint           abyss_y;
  gint           abyss_width;
  gint           abyss_height;
};

struct _GeglBufferClass
{
  GeglTileTraitsClass parent_class;
};

GType         gegl_buffer_get_type   (void) G_GNUC_CONST;


void        * gegl_buffer_get_format (GeglBuffer    *buffer);
gint          gegl_buffer_pixels     (GeglBuffer    *buffer);
gint          gegl_buffer_px_size    (GeglBuffer    *buffer);

void          gegl_buffer_get        (GeglBuffer    *buffer,
                                      GeglRectangle *rect,
                                      void          *dest,
                                      void          *format,
                                      gdouble        scale);

void          gegl_buffer_set        (GeglBuffer    *buffer,
                                      GeglRectangle *rect,
                                      void          *src,
                                      void          *format);

GeglStorage * gegl_buffer_storage    (GeglBuffer    *buffer);

gint          gegl_buffer_leaks      (void);

void          gegl_buffer_stats      (void);

/* the following are remnants of how horizon used the precursor of the
 * tile manager for it's purposes. For now it is not used

gboolean      gegl_buffer_idle       (GeglBuffer *gegl_buffer);


void          gegl_buffer_add_dirty  (GeglBuffer *gegl_buffer,
                                      gint        x,
                                      gint        y);

void          gegl_buffer_flush_dirty(GeglBuffer *buffer);

gboolean      gegl_buffer_is_dirty   (GeglBuffer *buffer,
                                      gint        x,
                                      gint        y);
*/

#endif
