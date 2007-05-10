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

#define GEGL_TYPE_BUFFER (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))

typedef enum {
  GEGL_INTERPOLATION_NEAREST
} GeglInterpolation;


GType   gegl_buffer_get_type   (void) G_GNUC_CONST;

void    gegl_buffer_get        (GeglBuffer    *buffer,
                                GeglRectangle *rect,
                                gdouble        scale,
                                void          *format,
                                void          *dest);

void    gegl_buffer_set        (GeglBuffer       *buffer,
                                GeglRectangle    *rect,
                                void             *format,
                                void             *src);

void    gegl_buffer_sample     (GeglBuffer       *buffer,
                                gdouble           x,
                                gdouble           y,
                                gdouble           scale,
                                gpointer          dest,
                                Babl             *format,
                                GeglInterpolation interpolation);

#endif
