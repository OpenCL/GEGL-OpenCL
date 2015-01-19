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
 *           2013 Daniel Sabo
 */

#ifndef __GEGL_INIT_PRIVATE_H__
#define __GEGL_INIT_PRIVATE_H__

G_BEGIN_DECLS

/**
 * gegl_get_debug_enabled:
 *
 * Check if gegl has debugging turned on.
 *
 * Return value: TRUE if debugging is turned on, FALSE otherwise.
 */
gboolean      gegl_get_debug_enabled     (void);

/**
 * gegl_get_default_module_paths:
 *
 * Paths which modules should be loaded from
 *
 * Returns: ordered #GSList of #gchar* to load. Free using g_slist_free(l, g_free)
 */
GSList *
gegl_get_default_module_paths(void);

G_END_DECLS

#endif /* __GEGL_INIT_PRIVATE_H__ */
