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

#ifndef __GEGL_HANDLERS_H__
#define __GEGL_HANDLERS_H__

#include "gegl-handler.h"

#define GEGL_TYPE_HANDLERS            (gegl_handlers_get_type ())
#define GEGL_HANDLERS(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_HANDLERS, GeglHandlers))
#define GEGL_HANDLERS_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_HANDLERS, GeglHandlersClass))
#define GEGL_IS_HANDLERS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_HANDLERS))
#define GEGL_IS_HANDLERS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_HANDLERS))
#define GEGL_HANDLERS_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_HANDLERS, GeglHandlersClass))

struct _GeglHandlers
{
  GeglHandler  parent_instance;

  GSList      *chain;
};

struct _GeglHandlersClass
{
  GeglHandlerClass parent_class;
};

GType         gegl_handlers_get_type   (void) G_GNUC_CONST;

/**
 * gegl_handlers_add:
 * @handlers: a #GeglHandlers
 * @handler: a #GeglHandler.
 *
 * Adds @handler to the list of handlers to be processed, the order handlers
 * are added in is from original provider to last processing element, messages
 * are passed from the last added to the first one in the chain.
 *
 * Returns: the added handler.
 */
GeglHandler * gegl_handlers_add        (GeglHandlers *handlers,
                                        GeglHandler  *handler);

/* returns the first matching handler of a specified type (or NULL) */
GeglHandler * gegl_handlers_get_first  (GeglHandlers *handlers,
                                        GType         type);

#endif
