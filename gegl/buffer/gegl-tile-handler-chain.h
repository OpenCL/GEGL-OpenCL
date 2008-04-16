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

#ifndef __GEGL_TILE_HANDLER_CHAIN_H__
#define __GEGL_TILE_HANDLER_CHAIN_H__

#include "gegl-tile-handler.h"

#define GEGL_TYPE_TILE_HANDLER_CHAIN            (gegl_tile_handler_chain_get_type ())
#define GEGL_TILE_HANDLER_CHAIN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_HANDLER_CHAIN, GeglTileHandlerChain))
#define GEGL_TILE_HANDLER_CHAIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_HANDLER_CHAIN, GeglTileHandlerChainClass))
#define GEGL_IS_TILE_HANDLER_CHAIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_HANDLER_CHAIN))
#define GEGL_IS_TILE_HANDLER_CHAIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_HANDLER_CHAIN))
#define GEGL_TILE_HANDLER_CHAIN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_HANDLER_CHAIN, GeglTileHandlerChainClass))

struct _GeglTileHandlerChain
{
  GeglTileHandler  parent_instance;

  GSList      *chain;
};

struct _GeglTileHandlerChainClass
{
  GeglTileHandlerClass parent_class;
};

GType         gegl_tile_handler_chain_get_type   (void) G_GNUC_CONST;

/**
 * gegl_tile_handler_chain_add:
 * @tile_handler_chain: a #GeglTileHandlerChain
 * @handler: a #GeglTileHandler.
 *
 * Adds @handler to the list of tile_handler_chain to be processed, the order tile_handler_chain
 * are added in is from original source to last processing element, commands
 * are passed from the last added to the first one in the chain, tiles
 * and other results are passed back up
 *
 * Returns: the added handler.
 */
GeglTileHandler * gegl_tile_handler_chain_add
                                      (GeglTileHandlerChain *tile_handler_chain,
                                       GeglTileHandler      *handler);

/* returns the first matching handler of a specified type (or NULL) */
GeglTileHandler * gegl_tile_handler_chain_get_first
                                      (GeglTileHandlerChain *tile_handler_chain,
                                       GType                 type);

#endif
