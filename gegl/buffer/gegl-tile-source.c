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
#include "config.h"
#include <glib.h>
#include <glib-object.h>
#include "gegl-tile-source.h"

G_DEFINE_TYPE (GeglTileSource, gegl_tile_source, G_TYPE_OBJECT)

static gpointer
gegl_tile_source_command_eek (GeglTileSource  *gegl_tile_source,
         GeglTileCommand  command,
         gint             x,
         gint             y,
         gint             z,
         gpointer         data)
{
  g_warning ("Unimplemented %s %i, %i, %i, %p", G_STRFUNC, command, x, y, data);
  return NULL;
}

static void
gegl_tile_source_class_init (GeglTileSourceClass *klass)
{  
  klass->command = gegl_tile_source_command_eek;
}

static void
gegl_tile_source_init (GeglTileSource *self)
{
}
