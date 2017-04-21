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

/* similar to gegl_extension_handler_register_loader(), kept for
 * compatibility reasons, do NOT use in newly written code.
 * TODO: remove this function in future versions!
 */
void          gegl_extension_handler_register        (const gchar *extension,
                                                      const gchar *handler) G_GNUC_DEPRECATED;

/* deprecated: use gegl_operation_handlers_register_loader() instead.
 * TODO: remove this function in future versions!
 */
void          gegl_extension_handler_register_loader (const gchar *extension,
                                                      const gchar *handler) G_GNUC_DEPRECATED;

/* deprecated: use gegl_operation_handlers_register_saver() instead.
 * TODO: remove this function in future versions!
 */
void          gegl_extension_handler_register_saver  (const gchar *extension,
                                                      const gchar *handler) G_GNUC_DEPRECATED;

/* similar to gegl_extension_handler_get_loader(), kept for
 * compatibility reasons, do NOT use in newly written code.
 * TODO: remove this function in future versions!
 */
const gchar * gegl_extension_handler_get             (const gchar *extension) G_GNUC_DEPRECATED;

/* deprecated: use gegl_operation_handlers_get_loader() instead.
 * TODO: remove this function in future versions!
 */
const gchar * gegl_extension_handler_get_loader      (const gchar *extension) G_GNUC_DEPRECATED;

/* deprecated: use gegl_operation_handlers_get_saver() instead.
 * TODO: remove this function in future versions!
 */
const gchar * gegl_extension_handler_get_saver       (const gchar *extension) G_GNUC_DEPRECATED;

#endif
