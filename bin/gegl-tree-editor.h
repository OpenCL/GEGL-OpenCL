/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#ifndef GEGL_TREE_EDITOR_H
#define GEGL_TREE_EDITOR_H

#include "gegl-tree-editor-action.h"
#include <gegl.h>
#include "gegl-store.h"

GtkWidget * tree_editor_new          (GtkWidget *property_editor);

GeglNode  * tree_editor_get_active   (GtkWidget *tree_editor);

void        tree_editor_set_active   (GtkWidget *tree_editor,
                                      GeglNode  *item);

GtkWidget * tree_editor_get_treeview (GtkWidget *tree_editor);

void        property_editor_rebuild  (GtkWidget *container,
                                      GeglNode  *node);

#endif /* TREE_EDITOR_H */
