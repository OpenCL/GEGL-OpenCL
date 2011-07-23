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
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_HANDLER_H__
#define __GEGL_TILE_HANDLER_H__

#include "gegl-tile-source.h"

/***
 * GeglTileHandler define the chain pattern that allow to stack GeglTileSource classes on
 * top of each others, each doing different task in response to command given (see GeglTileSource).
 *
 * A classical GeglBuffer is a stack of subclasses of GeglTileHandler with a GeglTileBackend on
 * the bottom. This architecture is designed to be modular and flexible on purpose, even allowing
 * to use a GeglBuffer as a source for another GeglBuffer.
 */

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_HANDLER            (gegl_tile_handler_get_type ())
#define GEGL_TILE_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_HANDLER, GeglTileHandler))
#define GEGL_TILE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_HANDLER, GeglTileHandlerClass))
#define GEGL_IS_TILE_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_HANDLER))
#define GEGL_IS_TILE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_HANDLER))
#define GEGL_TILE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_HANDLER, GeglTileHandlerClass))

struct _GeglTileHandler
{
  GeglTileSource  parent_instance;
  GeglTileSource *source; /* The source of the data, which we can rely on if
                             our command handler doesn't handle a command, this
                             is typically done with gegl_tile_handler_source_command
                             passing ourself as the first parameter. */
};

struct _GeglTileHandlerClass
{
  GeglTileSourceClass parent_class;
};

GType gegl_tile_handler_get_type (void) G_GNUC_CONST;

void
gegl_tile_handler_set_source (GeglTileHandler *handler,
                              GeglTileSource  *source);


#define gegl_tile_handler_get_source(handler)  (((GeglTileHandler*)handler)->source)

#define gegl_tile_handler_source_command(handler,command,x,y,z,data) (gegl_tile_handler_get_source(handler)?gegl_tile_source_command(gegl_tile_handler_get_source(handler), command, x, y, z, data):NULL)


G_END_DECLS

#endif
