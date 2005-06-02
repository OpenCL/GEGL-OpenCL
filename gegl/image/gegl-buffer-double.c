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
 *  Copyright 2003-2004 Daniel S. Rogers
 *
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-image-types.h"

#include "gegl-buffer-double.h"


static void     gegl_buffer_double_class_init (GeglBufferDoubleClass *klass);
static void     gegl_buffer_double_init       (GeglBufferDouble      *self);
static gdouble  get_element_double            (const GeglBuffer      *self,
                                               gint                   bank,
                                               gint                   index);
static void     set_element_double            (GeglBuffer            *self,
                                               gint                   bank,
                                               gint                   index,
                                               gdouble                elem);


G_DEFINE_TYPE(GeglBufferDouble, gegl_buffer_double, GEGL_TYPE_BUFFER)


static void
gegl_buffer_double_class_init (GeglBufferDoubleClass *klass)
{
  GeglBufferClass *buffer_class = GEGL_BUFFER_CLASS (klass);

  buffer_class->get_element_double = get_element_double;
  buffer_class->set_element_double = set_element_double;
}

static void
gegl_buffer_double_init (GeglBufferDouble *self)
{
  GeglBuffer *buffer = GEGL_BUFFER (self);

  buffer->bytes_per_element = sizeof (gdouble);
}

static void
set_element_double (GeglBuffer * self, gint bank, gint index, gdouble elem)
{

  g_return_if_fail (GEGL_IS_BUFFER_DOUBLE (self));
  g_return_if_fail (self->num_banks > bank);
  g_return_if_fail (self->elements_per_bank > index);

  ((gdouble **) self->banks)[bank][index] = elem;
}

static gdouble
get_element_double (const GeglBuffer * self, gint bank, gint index)
{

  g_return_val_if_fail (GEGL_IS_BUFFER_DOUBLE (self), 0.0);
  g_return_val_if_fail (self->num_banks > bank, 0.0);
  g_return_val_if_fail (self->elements_per_bank > index, 0.0);

  return ((gdouble **) self->banks)[bank][index];
}
