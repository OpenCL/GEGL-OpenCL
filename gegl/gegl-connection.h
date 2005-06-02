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
#ifndef __GEGL_CONNECTION_H__
#define __GEGL_CONNECTION_H__

G_BEGIN_DECLS


GeglConnection * gegl_connection_new             (GeglNode       *sink,
                                                  GeglProperty   *sink_prop,
                                                  GeglNode       *source,
                                                  GeglProperty   *source_prop);
GeglNode       * gegl_connection_get_source_node (GeglConnection *connection);
GeglNode       * gegl_connection_get_sink_node   (GeglConnection *connection);
GeglProperty   * gegl_connection_get_source_prop (GeglConnection *connection);
GeglProperty   * gegl_connection_get_sink_prop   (GeglConnection *connection);
void             gegl_connection_set_sink_node   (GeglConnection *connection,
                                                  GeglNode       *sink);
void             gegl_connection_set_sink_prop   (GeglConnection *connection,
                                                  GeglProperty   *sink_prop);
void             gegl_connection_set_source_node (GeglConnection *connection,
                                                  GeglNode       *source);
void             gegl_connection_set_source_prop (GeglConnection *connection,
                                                  GeglProperty   *source_prop);


G_END_DECLS

#endif /* __GEGL_CONNECTION_H__ */
