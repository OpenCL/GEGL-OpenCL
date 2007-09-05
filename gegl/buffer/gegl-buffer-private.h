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

#ifndef _GEGL_BUFFER_PRIVATE_H
#define _GEGL_BUFFER_PRIVATE_H

#include <glib.h>
#include <glib-object.h>

#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include <babl/babl.h>
#include "gegl-tile-traits.h"

#define GEGL_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER, GeglBufferClass))
#define GEGL_IS_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER))
#define GEGL_IS_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER))
#define GEGL_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER, GeglBufferClass))

struct _GeglBuffer
{
  GeglHandlers    parent_object;

  gint              x;      /* exported through gegl_buffer_extents()    */
  gint              y;      /*  -"-  */
  gint              width;  /*  -"-  */
  gint              height; /*  -"-  */

  Babl             *format;

  gint              shift_x;
  gint              shift_y;
  gint              total_shift_x;
  gint              total_shift_y;

  gint              abyss_x;
  gint              abyss_y;
  gint              abyss_width;
  gint              abyss_height;

  GeglTile         *hot_tile;
  GeglInterpolator *interpolator;
};

struct _GeglBufferClass
{
  GeglHandlersClass parent_class;
};

GeglStorage  * gegl_buffer_storage    (GeglBuffer    *buffer);

GeglRectangle  gegl_buffer_get_abyss  (GeglBuffer    *buffer);

gint           gegl_buffer_leaks      (void);

void           gegl_buffer_stats      (void);

void           gegl_buffer_save       (GeglBuffer    *buffer,
                                       const gchar   *path,
                                       GeglRectangle *roi);

/* flush any unwritten data (flushes the hot-cache of a single
 * tile used by gegl_buffer_set for 1x1 pixel sized rectangles
 */
void          gegl_buffer_flush      (GeglBuffer *buffer);


#endif
