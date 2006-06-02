/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"
#include <babl.h>
#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-init.h"


static gboolean gegl_initialized = FALSE;


void
gegl_init (int *argc,
           char ***argv)
{
  if (gegl_initialized)
    return;
  g_type_init ();
  babl_init ();
  gegl_initialized = TRUE;
}

void
gegl_exit (void)
{
  babl_destroy ();
}
