/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    Foobar is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Foobar; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Daniel S. Rogers
 *
 */

#ifndef GEGL_BUFFER_DOUBLE_H
#define GEGL_BUFFER_DOUBLE_H

#include "gegl-buffer.h"

#define GEGL_TYPE_BUFFER_DOUBLE               (gegl_buffer_double_get_type ())
#define GEGL_BUFFER_DOUBLE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER_DOUBLE, GeglBufferDouble))
#define GEGL_BUFFER_DOUBLE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER_DOUBLE, GeglBufferDoubleClass))
#define GEGL_IS_BUFFER_DOUBLE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_BUFFER_DOUBLE))
#define GEGL_IS_BUFFER_DOUBLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER_DOUBLE))
#define GEGL_BUFFER_DOUBLE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER_DOUBLE, GeglBufferDoubleClass))

GType gegl_buffer_double_get_type(void);

typedef struct _GeglBufferDouble GeglBufferDouble;
struct _GeglBufferDouble
{
	GeglBuffer parent;
};

typedef struct _GeglBufferDoubleClass GeglBufferDoubleClass;
struct _GeglBufferDoubleClass
{
	GeglBufferClass parent_class;
};


#endif
