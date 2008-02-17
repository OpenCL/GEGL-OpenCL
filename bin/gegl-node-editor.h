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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#ifndef GEGL_NODE_EDITOR_H
#define GEGL_NODE_EDITOR_H

#include <gtk/gtkvbox.h>

#define GEGL_TYPE_NODE_EDITOR            (gegl_node_editor_get_type ())
#define GEGL_NODE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_NODE_EDITOR, GeglNodeEditor))
#define GEGL_NODE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_NODE_EDITOR, GeglNodeEditorClass))
#define GEGL_IS_NODE_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_NODE_EDITOR))
#define GEGL_IS_NODE_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_NODE_EDITOR))
#define GEGL_NODE_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_NODE_EDITOR, GeglNodeEditorClass))

typedef struct _GeglNodeEditorClass GeglNodeEditorClass;

struct _GeglNodeEditor
{
  GtkVBox       parent_instance;
  gboolean      operation_switcher;
  GtkSizeGroup *col1;
  GtkSizeGroup *col2;
  GeglNode     *node;
};

struct _GeglNodeEditorClass
{
  GtkVBoxClass parent_class;
  gchar        *pattern;

  void (*construct) (GeglNodeEditor *self);  /* virtual method used to add to the container */
};

GType       gegl_node_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gegl_node_editor_new      (GeglNode *node,
                                       gboolean  operation_switcher);
/* utility method */
cairo_t *                              gegl_widget_get_cr (GtkWidget *widget);

/* class functions */

/* set the pattern that this matches on */
void     gegl_node_editor_class_set_pattern (GeglNodeEditorClass *klass,
                                             const gchar         *pattern);
gboolean gegl_node_editor_class_matches     (GeglNodeEditorClass *klass,
                                             const gchar         *operation_name);

G_END_DECLS


#endif /* NODE_EDITOR_H */
