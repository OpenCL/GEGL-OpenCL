/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2009 Øyvind Kolås.
 */

#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-lookup.h"

GeglLookup *
gegl_lookup_new (GeglLookupFunction *function,
                 gpointer            data)
{
  GeglLookup *lookup = g_slice_new0 (GeglLookup);
  lookup->function = function;
  lookup->data = data;
  return lookup;
}

void
gegl_lookup_free (GeglLookup *lookup)
{
  g_slice_free (GeglLookup, lookup);
}
