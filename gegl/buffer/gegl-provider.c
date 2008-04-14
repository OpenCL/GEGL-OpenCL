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
  g_warning ("implementationless %s called", G_STRFUNC);
  return NULL;
}

static gpointer
message (GeglProvider  *gegl_provider,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  g_warning ("Unhandled message: %s %i, %i, %i, %p", G_STRFUNC, message, x, y, data);
  return NULL;
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

  klass = GEGL_PROVIDER_GET_CLASS (gegl_provider);

  return klass->get_tile (gegl_provider, x, y, z);
}

gpointer
gegl_provider_message (GeglProvider    *gegl_provider,
                       GeglTileMessage  message,
                       gint             x,
                       gint             y,
                       gint             z,
                       gpointer         data)
{
  GeglProviderClass *klass;

  klass = GEGL_PROVIDER_GET_CLASS (gegl_provider);

  return klass->message (gegl_provider, message, x, y, z, data);
}

