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
 * Copyright 2003 Calvin Williamson
 *           2005-2008 Øyvind Kolås
 */

#ifndef __GEGL_OPERATIONS_H__
#define __GEGL_OPERATIONS_H__

#include <glib-object.h>

/* Used to look up the gtype when changing the type of operation associated
 * a GeglNode using just a string with the registered name.
 */
GType      gegl_operation_gtype_from_name   (const gchar *name);
gchar   ** gegl_list_operations             (guint *n_operations_p);
void       gegl_operation_gtype_cleanup     (void);

#endif
