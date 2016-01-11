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

#ifndef __GEGL_OPERATION_HANDLERS_H__
#define __GEGL_OPERATION_HANDLERS_H__

gboolean      gegl_operation_handlers_register_loader (const gchar *content_type,
                                                       const gchar *handler);

gboolean      gegl_operation_handlers_register_saver  (const gchar *content_type,
                                                       const gchar *handler);

const gchar * gegl_operation_handlers_get_loader      (const gchar *content_type);

const gchar * gegl_operation_handlers_get_saver       (const gchar *content_type);

#endif
