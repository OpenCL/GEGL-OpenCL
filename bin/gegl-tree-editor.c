/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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

#include <gtk/gtk.h>
#include "gegl-plugin.h"
#include "gegl-tree-editor.h"
#include "gegl-node-editor.h"
#include "editor.h"

extern GeglNode *editor_output;

void property_editor_rebuild (GtkWidget *container,
                              GeglNode  *node)
{
  GtkWidget *editor;

  gtk_container_foreach (GTK_CONTAINER (container),
                         (GtkCallback) gtk_widget_destroy, NULL);

  /*editor = property_editor_general (col1, col2, node);*/
  editor = gegl_node_editor_new (node, TRUE);
  if (editor)
    gtk_box_pack_start (GTK_BOX (container), editor, TRUE, TRUE, 0);
  gtk_widget_show_all (container);
}

/*
static void
action_remove (GtkAction *action,
               gpointer   userdata);

static void
action_move_down (GtkAction *action,
                  gpointer   userdata);
static void
action_move_up (GtkAction *action,
                gpointer   userdata);
                */

gboolean
 view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event,
                       gpointer userdata);

void
cell_edited_callback (GtkCellRendererText * cell,
                      gchar *path_string, gchar *new_text, gpointer user_data)
{
  GtkTreeModel *gegl_model;
  GtkTreePath *path;
  GtkTreeIter iter;
  GeglNode    *item;

  gegl_model = gtk_tree_view_get_model (GTK_TREE_VIEW (user_data));

  path = gtk_tree_path_new_from_string (path_string);
  if (!path)
    {
      gtk_tree_path_free (path);
      return;
    }
  gtk_tree_model_get_iter (GTK_TREE_MODEL (gegl_model), &iter, path);

  item = iter.user_data;
  if (item)
    gegl_node_set (item, "name", new_text, NULL);

  gtk_tree_path_free (path);
}

static void
selection_changed (GtkTreeSelection * selection, gpointer user_data)
{
  GtkWidget *top_vbox;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GeglNode    *item;

  top_vbox = user_data;
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    {
#ifdef HAVE_GRAPH_EDITOR
      graph_editor_set_root (editor->graph_editor, NULL);
#endif
      return;
    }

  item = iter.user_data;

  if (item)
    {
      property_editor_rebuild (top_vbox, item);

#ifdef HAVE_CURVE_EDITOR
      curve_editor_set_gegl (editor->curve_editor, item);
#endif
#ifdef HAVE_GRAPH_EDITOR
      if (gegl_type (item) == GeglGraph)
        graph_editor_set_root (editor->graph_editor, item);
      else
        graph_editor_set_root (editor->graph_editor, NULL);
#endif
    }
}

GtkWidget *
tree_editor_get_treeview (GtkWidget *tree_editor)
{
  return g_object_get_data (G_OBJECT (tree_editor), "treeview");
}


GeglNode    *
tree_editor_get_active (GtkWidget *tree_editor)
{
  GtkTreeView *treeview =
    GTK_TREE_VIEW (tree_editor_get_treeview (tree_editor));
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GeglNode    *item;

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (!gtk_tree_selection_get_selected (tree_selection, &model, &iter))
    {
      GeglNode *proxy = gegl_node_get_output_proxy (editor.gegl, "output");
      return gegl_node_get_producer (proxy, "input", NULL); 
    }

  item = iter.user_data;
  return item;
}

void
tree_editor_set_active (GtkWidget *tree_editor, GeglNode *item)
{
  GtkTreeView *treeview =
    GTK_TREE_VIEW (tree_editor_get_treeview (tree_editor));
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreePath *path;

  if (!item)
    return;

  tree_selection = gtk_tree_view_get_selection (treeview);
  model = gtk_tree_view_get_model (treeview);

  gtk_tree_model_get_iter_first (model, &iter);
  iter.user_data = item;

  path = gtk_tree_model_get_path (model, &iter);

  // gtk_tree_view_collapse_all (treeview);
  gtk_tree_view_expand_to_path (treeview, path);
  gtk_tree_selection_select_iter (tree_selection, &iter);
  gtk_tree_view_scroll_to_cell (treeview, path, NULL, TRUE, 0.5, 0);

  gtk_tree_path_free (path);
}

GtkWidget *
tree_editor_new (GtkWidget *property_editor)
{
  GtkWidget *self;
  GtkWidget *tree_scroll;
  GtkWidget *treeview;
  GtkWidget *bot_vbox;

  {
    treeview = gtk_tree_view_new ();
    bot_vbox = gtk_vbox_new (FALSE, 0);

    {                           /* build the actual treeview */
      GtkTreeViewColumn *col;
      GtkCellRenderer *renderer;

      col = gtk_tree_view_column_new ();
      renderer = gtk_cell_renderer_pixbuf_new ();

      gtk_tree_view_column_pack_start (col, renderer, FALSE);
      gtk_tree_view_column_add_attribute (col, renderer, "stock-id",
                                          GEGL_STORE_COL_ICON);
      renderer = gtk_cell_renderer_text_new ();

      g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);
      g_signal_connect (renderer, "edited", (GCallback) cell_edited_callback,
                        treeview);

      gtk_tree_view_column_pack_start (col, renderer, TRUE);
      gtk_tree_view_column_add_attribute (col, renderer, "text",
                                          GEGL_STORE_COL_NAME);

      gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), col);

      g_object_set (G_OBJECT (treeview), "headers-visible", FALSE,
                    "search-column", GEGL_STORE_COL_NAME,
                    "rules-hint", FALSE, NULL);
    }
    {
      GtkTreeSelection *tree_selection;

      tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
      gtk_tree_selection_set_mode (tree_selection, GTK_SELECTION_SINGLE);
      if (property_editor)
        g_signal_connect (tree_selection, "changed",
                          (GCallback) selection_changed, property_editor);
      else
        fprintf (stderr, "property_editor expected to be inited\n");
    }
  }

  g_signal_connect (G_OBJECT (treeview), "button-press-event",
                    (GCallback) view_onButtonPressed, NULL);

  tree_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (tree_scroll),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request (tree_scroll, 256, 256);
  gtk_container_add (GTK_CONTAINER (tree_scroll), treeview);

  gtk_box_pack_start (GTK_BOX (bot_vbox), tree_scroll, TRUE, TRUE, 0);

  if(0){
    GtkWidget *butbox = gtk_hbox_new (FALSE, 1);
    GtkWidget *button;
    GtkWidget *image;

    button = gtk_button_new ();
    image = StockIcon (GTK_STOCK_NEW, GTK_ICON_SIZE_BUTTON, tree_scroll);
    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_box_pack_start (GTK_BOX (butbox), button, TRUE, TRUE, 0);

    button = gtk_button_new ();
    image = StockIcon (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON, tree_scroll);
    /*g_signal_connect (G_OBJECT (button), "clicked",
                      (GCallback) action_move_up, treeview);*/
    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_box_pack_start (GTK_BOX (butbox), button, TRUE, TRUE, 0);

    button = gtk_button_new ();
    image = StockIcon (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON, tree_scroll);
    /*g_signal_connect (G_OBJECT (button), "clicked",
                      (GCallback) action_move_down, treeview);*/
    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_box_pack_start (GTK_BOX (butbox), button, TRUE, TRUE, 0);

    button = gtk_button_new ();
    image = StockIcon (GTK_STOCK_COPY, GTK_ICON_SIZE_BUTTON, tree_scroll);
    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_box_pack_start (GTK_BOX (butbox), button, TRUE, TRUE, 0);

    button = gtk_button_new ();
    image = StockIcon (GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON, tree_scroll);
    /*g_signal_connect (G_OBJECT (button), "clicked", (GCallback) action_remove,
                      treeview);*/
    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_box_pack_start (GTK_BOX (butbox), button, TRUE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (bot_vbox), butbox, FALSE, FALSE, 0);
  }

  self = bot_vbox;

  g_object_set_data (G_OBJECT (self), "treeview", treeview);
  return self;
}

static const gchar *ui_info =
  "<ui>"
  "  <popup name='PopupOp'>"
  "    <menuitem action='AddSiblingOp'/>"
  "    <menuitem action='AddChildOp'/>"
  "    <menuitem action='Remove'/>"
  "    <separator/>"
  "    <menuitem action='MoveUp'/>"
  "    <menuitem action='MoveDown'/>"
  "    <separator/>"
  "    <menuitem action='ExpandAll'/>"
  "    <menuitem action='CollapseAll'/>"
  "  </popup>"
  "</ui>";

void
view_popup_menu (GtkWidget *treeview, GdkEventButton *event,
                 gpointer userdata)
{
  GtkTreeSelection *tree_selection;
  GtkTreeIter iter;
  GeglNode    *item = NULL;

  GtkUIManager *ui;
  GError   *error = NULL;
  GtkWidget *menu;

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  if (!gtk_tree_selection_get_selected (tree_selection, NULL, &iter))
    return;
  item = GEGL_NODE (iter.user_data);
  if (!item)
    return;

  ui = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui, tree_editor_action_group (treeview),
                                      0);

  //gtk_window_add_accel_group (GTK_WINDOW (mainwindow)
  //
  if (!gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error))
    {
      g_message ("building menus failed :%s", error->message);
      g_error_free (error);
      return;
    }
/*
  switch (gegl_type (item))
    {
    case GeglGraph:
      menu = gtk_ui_manager_get_widget (ui, "/PopupGraph");
      break;
    case GeglOp:*/
      menu = gtk_ui_manager_get_widget (ui, "/PopupOp");
/*    break;
    case GeglClip:
      menu = gtk_ui_manager_get_widget (ui, "/PopupClip");
      break;
    case GeglSequence:
      menu = gtk_ui_manager_get_widget (ui, "/PopupSequence");
      break;
    default:
      menu = NULL;
      break;
    }
*/
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                  (event != NULL) ? event->button : 0,
                  gdk_event_get_time ((GdkEvent *) event));

  return;
}

gboolean
view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event,
                      gpointer userdata)
{
  /* single click with the right mouse button? */
  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      /* optional: select row if no row is selected or only
       *  one other row is selected (will only do something
       *  if you set a tree selection mode as described later
       *  in the tutorial) */
      if (1)
        {
          GtkTreeSelection *selection;

          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

          if (gtk_tree_selection_count_selected_rows (selection) <= 1)
            {
              GtkTreePath *path;

              /* Get tree path for row that was clicked */
              if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
                                                 event->x, event->y,
                                                 &path, NULL, NULL, NULL))
                {
                  gtk_tree_selection_unselect_all (selection);
                  gtk_tree_selection_select_path (selection, path);
                  gtk_tree_path_free (path);
                }
            }
        }                       /* end of optional bit */

      view_popup_menu (treeview, event, userdata);

      return TRUE;              /* we handled this */
    }

  return FALSE;                 /* we did not handle this */
}
