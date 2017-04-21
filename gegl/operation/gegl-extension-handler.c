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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib.h>
#include "gegl-extension-handler.h"
#include "gegl-operation-handlers.h"

void
gegl_extension_handler_register (const gchar *extension,
                                 const gchar *handler)
{
  gegl_operation_handlers_register_loader (extension, handler);
}

void
gegl_extension_handler_register_loader (const gchar *extension,
                                        const gchar *handler)
{
  gegl_operation_handlers_register_loader (extension, handler);
}

void
gegl_extension_handler_register_saver (const gchar *extension,
                                       const gchar *handler)
{
  gegl_operation_handlers_register_saver (extension, handler);
}

const gchar *
gegl_extension_handler_get (const gchar *extension)
{
  return gegl_operation_handlers_get_loader (extension);
}

const gchar *
gegl_extension_handler_get_loader (const gchar *extension)
{
  return gegl_operation_handlers_get_loader (extension);
}

const gchar *
gegl_extension_handler_get_saver (const gchar *extension)
{
  return gegl_operation_handlers_get_saver (extension);
}
