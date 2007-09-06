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
#include <glib.h>
#include <glib-object.h>
#include "gegl-provider.h"

G_DEFINE_TYPE (GeglProvider, gegl_provider, G_TYPE_OBJECT)

static GeglTile *
get_tile (GeglProvider *gegl_provider,
          gint           x,
          gint           y,
          gint           z)
{
  g_warning ("implementationless %s called", __FUNCTION__);
  return NULL;
}

static gboolean
message (GeglProvider  *gegl_provider,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  g_warning ("Unhandled message: %i, %i, %i, %p", message, x, y, data);
  return FALSE;
}

static void
gegl_provider_class_init (GeglProviderClass *klass)
{
  /*GObjectClass *gobject_class = G_OBJECT_CLASS (klass);*/
  klass->get_tile = get_tile;
  klass->message  = message;
}

static void
gegl_provider_init (GeglProvider *self)
{
}

/**
 *  GeglTile
 *  @gegl_provider: a GeglProvider
 *  @x: horizontal index of requested tile
 *  @y: vertical index of requested tile
 *
 *  Return value: the tile requested (or NULL for sparse backends)
 */
GeglTile *
gegl_provider_get_tile (GeglProvider *gegl_provider,
                          gint           x,
                          gint           y,
                          gint           z)
{
  GeglProviderClass *klass;

  g_return_val_if_fail (GEGL_IS_TILE_STORE (gegl_provider), NULL);

  klass = GEGL_PROVIDER_GET_CLASS (gegl_provider);

  return klass->get_tile (gegl_provider, x, y, z);
}

gboolean
gegl_provider_message (GeglProvider  *gegl_provider,
                         GeglTileMessage message,
                         gint            x,
                         gint            y,
                         gint            z,
                         gpointer        data)
{
  GeglProviderClass *klass;

  g_return_val_if_fail (GEGL_IS_TILE_STORE (gegl_provider), -1);

  klass = GEGL_PROVIDER_GET_CLASS (gegl_provider);

  return klass->message (gegl_provider, message, x, y, z, data);
}

