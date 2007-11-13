/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
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
#include "gegl-handlers.h"

#define GEGL_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER, GeglBufferClass))
#define GEGL_IS_BUFFER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER))
#define GEGL_IS_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER))
#define GEGL_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER, GeglBufferClass))

struct _GeglBuffer
{
  GeglHandlers      parent_object; /* which is a GeglHandler which has a
                                      provider field which is used for chaining
                                      sub buffers with their anchestors */

  GeglRectangle     extent;        /* the dimensions of the buffer */

  Babl             *format;  /* the pixel format used for pixels in this
                                buffer */
 
  gint              shift_x; /* The relative offset of origins compared with */
  gint              shift_y; /* anchestral storage buffer, during            */
                             /* construction relative to immediate provider  */

  GeglRectangle     abyss;

  GeglTile         *hot_tile; /* cached tile for speeding up pget/pset (1x1
                                 sized gets/sets)*/

  GeglInterpolator *interpolator; /* cached interpolator for speeding up random
                                     access interpolated fetches from the
                                     buffer */

  GeglStorage      *storage;

  gint              min_x; /* the extent of tile indices that has been */
  gint              min_y; /* produced by _get_tile for this buffer */
  gint              max_x; /* this is used in gegl_buffer_void to narrow */
  gint              max_y; /* down the tiles kill messages are sent for */
  gint              max_z;
};

struct _GeglBufferClass
{
  GeglHandlersClass parent_class;
};


const GeglRectangle* gegl_buffer_get_abyss  (GeglBuffer    *buffer);

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
