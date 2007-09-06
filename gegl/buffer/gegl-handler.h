/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */
#ifndef _TILE_TRAIT_H
#define _TILE_TRAIT_H

#include <glib.h>
#include "gegl-tile.h"
#include "gegl-provider.h"
#include "gegl-handler.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_TRAIT            (gegl_handler_get_type ())
#define GEGL_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_TRAIT, GeglHandler))
#define GEGL_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_TRAIT, GeglHandlerClass))
#define GEGL_IS_TILE_TRAIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_TRAIT))
#define GEGL_IS_TILE_TRAIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_TRAIT))
#define GEGL_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_TRAIT, GeglHandlerClass))



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

#define gegl_handler_get_provider(handler)  ((handler)->provider)

G_END_DECLS

#endif
