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
#ifndef _GEGL_HANDLER_EMPTY_H
#define _GEGL_HANDLER_EMPTY_H

#include <glib.h>
#include "gegl-buffer-types.h"
#include "gegl-tile.h"
#include "gegl-handler.h"

G_BEGIN_DECLS

#define GEGL_TYPE_HANDLER_EMPTY            (gegl_handler_empty_get_type ())
#define GEGL_HANDLER_EMPTY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_HANDLER_EMPTY, GeglHandlerEmpty))
#define GEGL_HANDLER_EMPTY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_HANDLER_EMPTY, GeglHandlerEmptyClass))
#define GEGL_IS_HANDLER_EMPTY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_HANDLER_EMPTY))
#define GEGL_IS_HANDLER_EMPTY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_HANDLER_EMPTY))
#define GEGL_HANDLER_EMPTY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_HANDLER_EMPTY, GeglHandlerEmptyClass))


typedef struct _GeglHandlerEmpty      GeglHandlerEmpty;
typedef struct _GeglHandlerEmptyClass GeglHandlerEmptyClass;

struct _GeglHandlerEmpty
{
  GeglHandler      parent_instance;
  GeglTile        *tile;
  GeglTileBackend *backend;
};

struct _GeglHandlerEmptyClass
{
  GeglHandlerClass parent_class;
};

GType gegl_handler_empty_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
