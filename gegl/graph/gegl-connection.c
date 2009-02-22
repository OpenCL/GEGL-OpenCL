/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-connection.h"


struct _GeglConnection
{
  GeglNode *sink;
  GeglPad  *sink_pad;

  GeglNode *source;
  GeglPad  *source_pad;

  /*XXX: here is a nice place to store the negotiated Pixel Format representation */
};

GeglConnection *
gegl_connection_new (GeglNode *sink,
                     GeglPad  *sink_pad,
                     GeglNode *source,
                     GeglPad  *source_pad)
{
  GeglConnection *self = g_slice_new0 (GeglConnection);

  self->sink       = sink;
  self->sink_pad   = sink_pad;
  self->source     = source;
  self->source_pad = source_pad;

  return self;
}

void
gegl_connection_destroy (GeglConnection *self)
{
  g_slice_free (GeglConnection, self);
}

GeglNode *
gegl_connection_get_source_node (GeglConnection *self)
{
  return self->source;
}

void
gegl_connection_set_source_node (GeglConnection *self,
                                 GeglNode       *source)
{
  self->source = source;
}

GeglNode *
gegl_connection_get_sink_node (GeglConnection *self)
{
  return self->sink;
}

void
gegl_connection_set_sink_node (GeglConnection *self,
                               GeglNode       *sink)
{
  self->sink = sink;
}

GeglPad *
gegl_connection_get_sink_pad (GeglConnection *self)
{
  return self->sink_pad;
}

void
gegl_connection_set_sink_pad (GeglConnection *self,
                              GeglPad        *sink_pad)
{
  self->sink_pad = sink_pad;
}

GeglPad *
gegl_connection_get_source_pad (GeglConnection *self)
{
  return self->source_pad;
}

void
gegl_connection_set_source_pad (GeglConnection *self,
                                GeglPad        *source_pad)
{
  self->source_pad = source_pad;
}
