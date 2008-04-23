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
 * Copyright 2003 Calvin Williamson, Øyvind Kolås
 */

#ifndef __GEGL_INIT_H__
#define __GEGL_INIT_H__

G_BEGIN_DECLS


void           gegl_init              (gint    *argc,
                                       gchar ***argv);
GOptionGroup * gegl_get_option_group  (void);
void           gegl_exit              (void);

gboolean       gegl_get_debug_enabled (void); /* should be moved into config */

void           gegl_get_version          (int *major,
                                          int *minor,
                                          int *micro);


G_END_DECLS

#endif /* __GEGL_INIT_H__ */
