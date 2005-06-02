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
 *  Copyright 2003 Calvin Williamson
 *
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-connection.h"


struct _GeglConnection
{
  GeglNode * sink;
  GeglProperty *sink_prop;

  GeglNode * source;
  GeglProperty *source_prop;
};

GeglConnection *
gegl_connection_new(GeglNode *sink,
                    GeglProperty *sink_prop,
                    GeglNode *source,
                    GeglProperty *source_prop)
{
  GeglConnection *connection = g_new(GeglConnection, 1);
  connection->sink = sink;
  connection->sink_prop = sink_prop;
  connection->source = source;
  connection->source_prop = source_prop;

  return connection;
}

GeglNode *
gegl_connection_get_source_node (GeglConnection *connection)
{
  return connection->source;
}

void
gegl_connection_set_source_node (GeglConnection *connection,
                                 GeglNode *source)
{
  connection->source = source;
}

GeglNode *
gegl_connection_get_sink_node (GeglConnection *connection)
{
  return connection->sink;
}

void
gegl_connection_set_sink_node (GeglConnection *connection,
                               GeglNode *sink)
{
  connection->sink = sink;
}

GeglProperty *
gegl_connection_get_sink_prop (GeglConnection *connection)
{
  return connection->sink_prop;
}

void
gegl_connection_set_sink_prop (GeglConnection *connection,
                               GeglProperty *sink_prop)
{
  connection->sink_prop = sink_prop;
}

GeglProperty *
gegl_connection_get_source_prop (GeglConnection *connection)
{
  return connection->source_prop;
}

void
gegl_connection_set_source_prop (GeglConnection *connection,
                                 GeglProperty *source_prop)
{
  connection->source_prop = source_prop;
}
