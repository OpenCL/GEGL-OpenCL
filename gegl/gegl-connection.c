/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"
#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-connection.h"


struct _GeglConnection
{
  GeglNode *sink;
  GeglPad  *sink_prop;

  GeglNode *source;
  GeglPad  *source_prop;

  /*XXX: here is a nice place to store the negotiated Pixel Format representation */
};

GeglConnection *
gegl_connection_new (GeglNode *sink,
                     GeglPad  *sink_prop,
                     GeglNode *source,
                     GeglPad  *source_prop)
{
  GeglConnection *self = g_new0 (GeglConnection, 1);

  self->sink        = sink;
  self->sink_prop   = sink_prop;
  self->source      = source;
  self->source_prop = source_prop;

  return self;
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
gegl_connection_get_sink_prop (GeglConnection *self)
{
  return self->sink_prop;
}

void
gegl_connection_set_sink_prop (GeglConnection *self,
                               GeglPad        *sink_prop)
{
  self->sink_prop = sink_prop;
}

GeglPad *
gegl_connection_get_source_prop (GeglConnection *self)
{
  return self->source_prop;
}

void
gegl_connection_set_source_prop (GeglConnection *self,
                                 GeglPad        *source_prop)
{
  self->source_prop = source_prop;
}
