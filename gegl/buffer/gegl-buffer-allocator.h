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
#include <glib.h>
#include <glib-object.h>

#include "gegl-buffer-types.h"

#ifndef _GEGL_BUFFER_ALLOCATOR_H
#define _GEGL_BUFFER_ALLOCATOR_H

#define GEGL_TYPE_BUFFER_ALLOCATOR            (gegl_buffer_allocator_get_type ())
#define GEGL_BUFFER_ALLOCATOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER_ALLOCATOR, GeglBufferAllocator))
#define GEGL_BUFFER_ALLOCATOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER_ALLOCATOR, GeglBufferAllocatorClass))
#define GEGL_IS_BUFFER_ALLOCATOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER_ALLOCATOR))
#define GEGL_IS_BUFFER_ALLOCATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER_ALLOCATOR))
#define GEGL_BUFFER_ALLOCATOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER_ALLOCATOR, GeglBufferAllocatorClass))

#include "gegl-buffer.h"
#include "gegl-buffer-private.h"

struct _GeglBufferAllocator
{
  GeglBuffer parent_object;
  gint       x_used;      /* currently used in x direction */
  gint       max_height;  /* height of the talles allocation in this run */
  gint       y_used;      /* the current y pos used for returns */
};

struct _GeglBufferAllocatorClass
{
  GeglBufferClass parent_class;
};

GType         gegl_buffer_allocator_get_type           (void) G_GNUC_CONST;

void gegl_buffer_allocators_free (void);

GeglBuffer *
gegl_buffer_new_from_format (const void *babl_format,
                             gint        x,
                             gint        y,
                             gint        width,
                             gint        height);

#endif
