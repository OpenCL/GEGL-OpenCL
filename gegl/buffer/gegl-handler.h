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

#ifndef __GEGL_HANDLER_H__
#define __GEGL_HANDLER_H__

#include "gegl-provider.h"

G_BEGIN_DECLS

#define GEGL_TYPE_HANDLER            (gegl_handler_get_type ())
#define GEGL_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_HANDLER, GeglHandler))
#define GEGL_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_HANDLER, GeglHandlerClass))
#define GEGL_IS_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_HANDLER))
#define GEGL_IS_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_HANDLER))
#define GEGL_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_HANDLER, GeglHandlerClass))


struct _GeglHandler
{
  GeglProvider  parent_instance;

  GeglProvider *provider;
};

struct _GeglHandlerClass
{
  GeglProviderClass parent_class;
};

GType gegl_handler_get_type (void) G_GNUC_CONST;

#define gegl_handler_get_provider(handler)  (((GeglHandler*)handler)->provider)


gpointer   gegl_handler_chain_up (GeglHandler     *handler,
                                  GeglTileCommand  command,
                                  gint             x,
                                  gint             y,
                                  gint             z,
                                  gpointer         data);

G_END_DECLS

#endif
