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

#ifndef __GEGL_EXTENSION_HANDLER_H__
#define __GEGL_EXTENSION_HANDLER_H__

#include <glib.h>

void          gegl_extension_handler_register (const gchar *extension,
                                               const gchar *handler);
const gchar * gegl_extension_handler_get      (const gchar *extension);
void          gegl_extension_handler_cleanup  (void);

#endif
