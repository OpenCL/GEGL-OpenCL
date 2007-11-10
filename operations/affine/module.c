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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Philip Lafleur
 */

#include <glib-object.h>
#include <gegl-module.h>

#include "module.h"
#include "affine.h"

static GTypeModule          *affine_module;
static const GeglModuleInfo  modinfo =
{
  GEGL_MODULE_ABI_VERSION,
  "affine",
  "v0.0",
  "(c) 2006, released under the LGPL",
  "July 2006"
};

GTypeModule *
affine_module_get_module (void)
{
  return affine_module;
}

const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

GType rotate_get_type    (void);
GType scale_get_type     (void);
GType shear_get_type     (void);
GType translate_get_type (void);
GType reflect_get_type   (void);

gboolean
gegl_module_register (GTypeModule *module)
{
  GType dummy;
  affine_module = module;

  dummy = op_affine_get_type ();
  dummy = rotate_get_type ();
  dummy = scale_get_type ();
  dummy = shear_get_type ();
  dummy = translate_get_type ();
  dummy = reflect_get_type ();

  return TRUE;
}
