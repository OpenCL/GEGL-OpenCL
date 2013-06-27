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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Ville Sokk <ville.sokk@gmail.com>
 */

#ifndef __GEGL_TILE_BACKEND_SWAP_H__
#define __GEGL_TILE_BACKEND_SWAP_H__

#include <glib.h>
#include "gegl-tile-backend.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_BACKEND_SWAP            (gegl_tile_backend_swap_get_type ())
#define GEGL_TILE_BACKEND_SWAP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_BACKEND_SWAP, GeglTileBackendSwap))
#define GEGL_TILE_BACKEND_SWAP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_BACKEND_SWAP, GeglTileBackendSwapClass))
#define GEGL_IS_TILE_BACKEND_SWAP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_BACKEND_SWAP))
#define GEGL_IS_TILE_BACKEND_SWAP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_BACKEND_SWAP))
#define GEGL_TILE_BACKEND_SWAP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_BACKEND_SWAP, GeglTileBackendSwapClass))


typedef struct _GeglTileBackendSwap      GeglTileBackendSwap;
typedef struct _GeglTileBackendSwapClass GeglTileBackendSwapClass;

struct _GeglTileBackendSwapClass
{
  GeglTileBackendClass parent_class;
};

struct _GeglTileBackendSwap
{
  GeglTileBackend  parent_instance;
  GHashTable      *index;
};

GType gegl_tile_backend_swap_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
