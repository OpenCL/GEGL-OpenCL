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
#include <string.h>
#include "gegl.h"
#include "gegl-view.h"
#include "editor.h"
#include "gegl-tree-editor.h"


/* The reason for these surviving is that it was the API used to
 * map to oxide earlier, by expanding this, an XML DOM like API,
 * integrated with the custom tree model could probably be resurrected.
 */
GeglNode *gegl_parent           (GeglNode *item);
GeglNode *gegl_children         (GeglNode *item);
GeglNode *gegl_previous_sibling (GeglNode *item);
GeglNode *gegl_next_sibling     (GeglNode *item);

static void
remove_itm (GtkAction *action, gpointer userdata)
{
  /* we passed the view as userdata when we connected the signal */
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GeglNode *item;
  GeglNode *new_item = NULL;

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (!gtk_tree_selection_get_selected (tree_selection, &model, &iter))
    return;

  item = iter.user_data;

  if (item)
    {
      GeglNode *previous = gegl_previous_sibling (item);
      GeglNode *next     = gegl_next_sibling (item);

      gtk_tree_model_row_deleted (model, gtk_tree_model_get_path (model, &iter));

      if (previous)
        new_item = previous;
      else if (next)
        new_item = next;

      if (previous)
        {
          gegl_node_disconnect (previous, "input");
          if (next)
            {
              gegl_node_disconnect (item, "input");
              gegl_node_connect_from (previous, "input", next, "output");
            }
          g_object_unref (G_OBJECT (item));
        }
      else
        { /* we're unchaining from parent */
          GeglNode *parent = gegl_parent (item);
          g_assert (parent);
          gegl_node_disconnect (previous, "input");
          gegl_node_disconnect (parent, "aux");
          if (next)
            {
              gegl_node_disconnect (item, "input");
              gegl_node_connect_from (parent, "aux", next, "output");
            }
          g_object_unref (G_OBJECT (item));
        }
    }

  if (new_item)
    {
      GtkTreePath *path;
      iter.user_data = new_item;
      path = gtk_tree_model_get_path (model, &iter);
      gtk_tree_selection_select_path (tree_selection, path);
      gtk_tree_path_free (path);
    }
  gegl_gui_flush ();
}

static void
move_up (GtkAction *action, gpointer userdata)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GeglNode *parent=NULL,
           *prev=NULL,
           *item=NULL,
           *next=NULL,
           *next2=NULL;
  GtkTreeIter  parent_iter;
  GtkTreePath *parent_path = NULL;

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (!gtk_tree_selection_get_selected (tree_selection, &model, &iter))
    return;

  item = iter.user_data;
  item = gegl_previous_sibling (item);
  iter.user_data = item;
  if (!item)
    return;
  parent = gegl_parent (item);
  prev   = gegl_previous_sibling (item);
  next   = gegl_next_sibling (item);

  if (!next)
    return;
  next2  = gegl_next_sibling (next);
  if (!next2)
    return;

  if (parent)
    {
      memcpy (&parent_iter, &iter, sizeof (GtkTreeIter));
      parent_iter.user_data = parent;
      parent_path = gtk_tree_model_get_path (model, &parent_iter);
    }
  else
    {
      parent_path = gtk_tree_path_new ();
    }

  if (prev)
    {
       { /* compute the reordering that needs to be indicated to */
         gint *new_order;
         GeglNode *iter;
         gint before=0;
         gint after=0;
         gint i;

         for (iter = item; iter; iter = gegl_previous_sibling (iter), before++);
         for (iter = item; iter; iter = gegl_next_sibling (iter), after++);

         new_order = g_malloc (sizeof (gint) * (before + after - 1));
         for (i=0; i < before+after-1; i++)
           new_order[i]=i;
         new_order[before-1]=before;
         new_order[before]=before-1;

         if (parent)
           {
             GtkTreeIter parent_iter;
             memcpy (&parent_iter, &iter, sizeof (GtkTreeIter));
             parent_iter.user_data = parent;
             gtk_tree_model_rows_reordered (model, gtk_tree_model_get_path (model, &parent_iter), &parent_iter, new_order);
           }
         else
           {
             GtkTreePath *path = gtk_tree_path_new ();
             gtk_tree_model_rows_reordered (model, path, NULL, new_order);
             gtk_tree_path_free (path);
           }
         g_free (new_order);
       }

       gegl_node_disconnect (prev, "input");
       gegl_node_disconnect (item, "input");
       gegl_node_connect_from (prev, "input", next, "output");
       /* now it is kind of removed */

       gegl_node_disconnect (next, "input");
       gegl_node_connect_from    (next, "input", item, "output");
       gegl_node_connect_from    (item, "input", next2, "output");
       /* and hooked back in */
    }
  else /* need to rehook parent instead */
    {
       { /* compute the reordering that needs to be indicated to */
         gint *new_order;
         GeglNode *iter;
         gint before=0;
         gint after=0;
         gint i;

         for (iter = item; iter; iter = gegl_previous_sibling (iter), before++);
         for (iter = item; iter; iter = gegl_next_sibling (iter), after++);

         new_order = g_malloc (sizeof (gint) * (before + after - 1));
         for (i=0; i < before+after-1; i++)
           new_order[i]=i;
         new_order[before-1]=before;
         new_order[before]=before-1;

         if (parent)
           {
             GtkTreeIter parent_iter;
             memcpy (&parent_iter, &iter, sizeof (GtkTreeIter));
             parent_iter.user_data = parent;
             gtk_tree_model_rows_reordered (model, gtk_tree_model_get_path (model, &parent_iter), &parent_iter, new_order);
           }
         else
           {
             GtkTreePath *path = gtk_tree_path_new ();
             gtk_tree_model_rows_reordered (model, path, NULL, new_order);
             gtk_tree_path_free (path);
           }
         g_free (new_order);
       }
       gegl_node_disconnect (parent, "aux");
       gegl_node_disconnect (item, "input");
       gegl_node_connect_from (parent, "aux", next, "output");
       /* now it is kind of removed */

       gegl_node_disconnect (next, "input");
       gegl_node_connect_from    (next, "input", item, "output");
       gegl_node_connect_from    (item, "input", next2, "output");
       /* and hooked back in */
    }

  item = gegl_previous_sibling (item);
  iter.user_data = item;

  gtk_tree_selection_select_iter (tree_selection, &iter);
  gegl_gui_flush ();
}

static void
move_down (GtkAction *action, gpointer userdata)
{
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GeglNode *parent=NULL,
           *prev=NULL,
           *item=NULL,
           *next=NULL,
           *next2=NULL;

  GtkTreeIter  parent_iter;
  GtkTreePath *parent_path = NULL;

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (!gtk_tree_selection_get_selected (tree_selection, &model, &iter))
    return;

  item = iter.user_data;
  if (!item)
    return;
  parent = gegl_parent (item);
  prev   = gegl_previous_sibling (item);
  next   = gegl_next_sibling (item);

  if (!next)
    return;
  next2  = gegl_next_sibling (next);
  if (!next2)
    return;

  if (parent)
    {
      memcpy (&parent_iter, &iter, sizeof (GtkTreeIter));
      parent_iter.user_data = parent;
      parent_path = gtk_tree_model_get_path (model, &parent_iter);
    }
  else
    {
      parent_path = gtk_tree_path_new ();
    }


  if (prev)
    {
       gegl_node_disconnect (prev, "input");
       gegl_node_disconnect (item, "input");
       gegl_node_connect_from    (prev, "input", next, "output");

       gegl_node_disconnect (next, "input");
       gegl_node_connect_from    (next, "input", item,  "output");
       gegl_node_connect_from    (item, "input", next2, "output");

       { /* compute the reordering that needs to be indicated to */
         gint *new_order;
         GeglNode *iter;
         gint before=0;
         gint after=0;
         gint i;

         for (iter = item; iter; iter = gegl_previous_sibling (iter), before++);
         for (iter = item; iter; iter = gegl_next_sibling (iter), after++);

         new_order = g_malloc (sizeof (gint) * (before + after - 1));
         for (i=0; i < before+after-1; i++)
           new_order[i]=i;
         new_order[before-1]=before-2;
         new_order[before-2]=before-1;

         gtk_tree_model_rows_reordered (model, parent_path, parent?&parent_iter:NULL, new_order);
         g_free (new_order);
       }
    }
  else /* need to rehook parent instead */
    {

       gegl_node_disconnect (parent, "aux");
       gegl_node_disconnect (item, "input");
       gegl_node_connect_from (parent, "aux", next, "output");

       gegl_node_disconnect (next, "input");
       gegl_node_connect_from (next, "input", item, "output");
       gegl_node_connect_from (item, "input", next2, "output");


       { /* compute the reordering that needs to be indicated to */
         gint *new_order;
         GeglNode *itr;
         gint after=0;
         gint i;

         for (itr = next; itr; itr = gegl_next_sibling (itr), after++);

         new_order = g_malloc (sizeof (gint) * (after));
         for (i=0; i < after; i++)
           new_order[i]=i;
         new_order[0]=1;
         new_order[1]=0;

         gtk_tree_model_rows_reordered (model, parent_path, parent?&parent_iter:NULL, new_order);
         g_free (new_order);
       }
    }
  gtk_tree_path_free (parent_path);
  gegl_gui_flush ();
}

static void
add_sibling_op (GtkAction *action, gpointer userdata)
{
  /* we passed the view as userdata when we connected the signal */
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GeglNode    *item, *new_item = NULL;

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (!gtk_tree_selection_get_selected (tree_selection, &model, &iter))
    return;

  item = iter.user_data;
  g_assert (item);
  if (item)
    {
      GeglNode *previous = gegl_previous_sibling (item);

      new_item = gegl_graph_new_node (GEGL_STORE (model)->gegl,
                                         "operation", "nop",
                                         NULL);

      if (previous)  /* we're just chaining in */
        {
          gegl_node_disconnect (previous, "input");
          gegl_node_connect_from (previous, "input", new_item, "output");
          gegl_node_connect_from (new_item, "input", item, "output");
        }
      else /* we're chaining in as the new aux of the parent level */
        {
          GeglNode *parent = gegl_parent (item);
          gegl_node_disconnect (parent, "aux");
          gegl_node_connect_from (parent, "aux", new_item, "output");
          gegl_node_connect_from (new_item, "input", item, "output");
        }

      iter.user_data = new_item;
      gtk_tree_model_row_inserted (model, gtk_tree_model_get_path (model, &iter), &iter);
    }
  gtk_tree_selection_select_iter (tree_selection, &iter);
}

GeglNode *gegl_add_sibling (const gchar *type)
{
  GeglNode *node;

  /* hack hack */ 
  tree_editor_set_active (editor.tree_editor, 
    tree_editor_get_active (editor.tree_editor));
  add_sibling_op (NULL, tree_editor_get_treeview (editor.tree_editor));
  node = tree_editor_get_active (editor.tree_editor);
  gegl_node_set (node, "operation", type, NULL);
  tree_editor_set_active (editor.tree_editor, node);
  property_editor_rebuild (editor.property_editor, node);
  return node;
}

static void
add_child_op (GtkAction *action, gpointer userdata)
{
  /* we passed the view as userdata when we connected the signal */
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  GtkTreeSelection *tree_selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GeglNode    *item, *new_item = NULL;

  tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  if (!gtk_tree_selection_get_selected (tree_selection, &model, &iter))
    return;

  item = iter.user_data;
  g_assert (item);
  if (item)
    {
      if (gegl_children (item))
        {
        }
      else
        {
          new_item = gegl_graph_new_node (GEGL_STORE (model)->gegl,
                                             "operation", "blank",
                                             NULL);
          gegl_node_connect_from (item, "aux", new_item, "output");
        }

      iter.user_data = new_item;
      gtk_tree_model_row_inserted (model, gtk_tree_model_get_path (model, &iter), &iter);
    }
  gtk_tree_selection_select_iter (tree_selection, &iter);
}

static void
expand_all (GtkAction *action, gpointer userdata)
{
  /* we passed the view as userdata when we connected the signal */
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  gtk_tree_view_expand_all (treeview);
}

static void
collapse_all (GtkAction *action, gpointer userdata)
{
  /* we passed the view as userdata when we connected the signal */
  GtkTreeView *treeview = GTK_TREE_VIEW (userdata);
  gtk_tree_view_collapse_all (treeview);
}

static GtkActionEntry entries[] = {
  {"TreeEditor", NULL, "_TreeEditor", NULL, NULL, NULL},

  {"Remove", NULL,
   "Remove", NULL,
   "Remove item",
   G_CALLBACK (remove_itm)},

  {"MoveUp", NULL,
   "MoveUp", NULL,
   "Move item up",
   G_CALLBACK (move_up)},

  {"MoveDown", NULL,
   "MoveDown", NULL,
   "Move item down",
   G_CALLBACK (move_down)},

  {"AddSiblingOp", NULL,
   "AddSiblingOp", NULL,
   "add sibling op",
   G_CALLBACK (add_sibling_op)},

  {"AddChildOp", NULL,
   "AddChildOp", NULL,
   "add child op",
   G_CALLBACK (add_child_op)},

  {"ExpandAll", NULL,
   "ExpandAll", NULL,
   "ExpandAll",
   G_CALLBACK (expand_all)},

  {"CollapseAll", NULL,
   "CollapseAll", NULL,
   "CollapseAll",
   G_CALLBACK (collapse_all)},
};
static guint n_entries = G_N_ELEMENTS (entries);

GtkActionGroup *
tree_editor_action_group (GtkWidget *treeview)
{
  static GtkActionGroup *action_group = NULL;

  if (action_group)
    return action_group;

  action_group = gtk_action_group_new ("TreeEditor");
  gtk_action_group_add_actions (action_group, entries, n_entries, treeview);

  return action_group;
}
