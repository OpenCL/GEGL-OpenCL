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

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "gegl-bin-gui-types.h"

#include "gegl-plugin.h"  /* FIXME: should just be gegl.h */

#include "editor.h"
#include "editor-optype.h"
#include "gegl-tree-editor.h"
#include "gegl-node-editor.h"
#include "gegl-tree-editor-action.h"

static void           chain_in_operation                     (const gchar        *op_type);
static void           menu_item_activate                     (GtkWidget          *widget,
                                                              gpointer            user_data);
static void           operation_class_iterate_for_completion (GType               type,
                                                              GtkListStore       *store);
static GtkTreeModel * create_completion_model                (GeglNode           *item);
static void           operation_class_iterate_for_menu       (GType               type,
                                                              GtkWidget          *menu,
                                                              GeglNode           *item);
static GtkWidget *    optype_menu                            (GeglNode           *item);
static void           gtk_option_menu_position               (GtkMenu            *menu,
                                                              gint               *x,
                                                              gint               *y,
                                                              gboolean           *push_in,
                                                              gpointer            user_data);
static void           button_clicked                         (GtkButton          *button,
                                                              gpointer            item);
static void           entry_activate                         (GtkEntry           *entry,
                                                              gpointer            user_data);
static gboolean       completion_match_selected              (GtkEntryCompletion *completion,
                                                              GtkTreeModel       *model,
                                                              GtkTreeIter        *iter,
                                                              gpointer            user_data);


static void
chain_in_operation (const gchar *op_type)
{
    GtkWidget *widget = editor.structure;
    gegl_add_sibling (op_type);
    gtk_widget_show (widget);
}

static void
menu_item_activate (GtkWidget *widget,
                    gpointer   user_data)
{
  GtkWidget *menu_item;
  GtkWidget *label;

  GeglNode    *item;
  const char *new_type;

  item = user_data;

  menu_item = widget;
  label = gtk_bin_get_child (GTK_BIN (menu_item));
  new_type = gtk_label_get_text (GTK_LABEL (label));

  if (!item)
    {
      chain_in_operation (new_type);
      return;
    }

  gegl_node_set (item, "operation", new_type, NULL);
  property_editor_rebuild (editor.property_editor, item);
    {
      GtkTreeIter iter;
      GtkTreeModel *model;
      GtkTreeView *treeview = GTK_TREE_VIEW (tree_editor_get_treeview (editor.tree_editor));
      GtkTreeSelection *tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

      if (gtk_tree_selection_get_selected (tree_selection, &model, &iter))
        {
          gtk_tree_model_row_changed (model, gtk_tree_model_get_path (model, &iter), &iter);
        }
      gegl_gui_flush ();
    }
#if 0

  item     = user_data;
  old_type = oxide_get_property (item, "type");
  old_name = oxide_get_property (item, "name");
  oxide_set_property (item, "type", new_type);

  if (!strcmp (old_name, "op") || !strcmp (old_name, old_type))
    {
      oxide_set_property (item, "name", new_type);
    }

  if (bauxite->property_editor)
    property_editor_rebuild (bauxite->property_editor, item);

  if (old_name)
   g_free (old_name);
  if (old_type)
    g_free (old_type);
#endif
}

static void
operation_class_iterate_for_completion (GType         type,
                                        GtkListStore *store)
{
  GType *ops;
  guint  children;
  gint   no;

  if (!type)
    return;
  ops = g_type_children (type, &children);

  for (no=0; no<children; no++)
    {
      GeglOperationClass *klass;

      klass = g_type_class_ref (ops[no]);
      if (klass->name != NULL &&
          klass->categories && strcmp (klass->categories, "hidden"))
        {
          GtkTreeIter iter;
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter, 0, klass->name, -1);
        }
      operation_class_iterate_for_completion (ops[no], store);
      g_type_class_unref (klass);
    }
  g_free (ops);
}

static GtkTreeModel *
create_completion_model (GeglNode *item)
{
  GtkListStore *store;

  store = gtk_list_store_new (1, G_TYPE_STRING);

  operation_class_iterate_for_completion (GEGL_TYPE_OPERATION, store);

  return GTK_TREE_MODEL (store);
}

static void
operation_class_iterate_for_menu (GType      type,
                                  GtkWidget *menu,
                                  GeglNode  *item)
{
  GType *ops;
  guint  children;
  gint   no;

  if (!type)
    return;
  ops = g_type_children (type, &children);

  for (no = 0; no <children; no++)
    {
      GeglOperationClass *klass;

      klass = g_type_class_ref (ops[no]);
      if (klass->name != NULL)
        {
          if (klass->categories == NULL)
            {
              GtkWidget *menu_item = gtk_menu_item_new_with_label (klass->name);
              gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
              gtk_widget_show (menu_item);

              g_signal_connect (G_OBJECT (menu_item), "activate",
                                G_CALLBACK (menu_item_activate), item);
            }
          else if (!strcmp (klass->categories, "hidden"))
            {
              /* ignore it */
            }
          else
            {
              const gchar *ptr = klass->categories;
              while (*ptr)
                {
                  gchar category[64]="";
                  gint i=0;
                  while (*ptr && *ptr!=':' && i<63)
                    {
                      category[i++]=*(ptr++);
                      category[i]='\0';
                    }
                  if (*ptr==':')
                    ptr++;
                  {
                    GtkWidget *category_menu = NULL;

                    /* look for existing category_menu (store index of the one we should insert after, if none is found)*/
                    {
                      gint      position=0;
                      GList    *categories;
                      GList    *iter;

                      categories = gtk_container_get_children (GTK_CONTAINER (menu));
                      iter = categories;

                      while (iter)
                        {
                          GtkWidget *item;
                          GtkWidget *label;
                          const char *text;

                          item = iter->data;
                          label = gtk_bin_get_child (GTK_BIN (item));
                          text = gtk_label_get_text (GTK_LABEL (label));

                          if (!strcmp (text, category))
                            {
                              category_menu =
                                gtk_menu_item_get_submenu (GTK_MENU_ITEM (item));
                              g_assert (category_menu);
                              iter = NULL;
                            }
                          else if (strcmp (text, category)>0) /* we're further ahead in the
                                                                 alphabet than we should be */
                            {
                              iter = NULL;
                            }
                          else
                            {
                              iter = g_list_next (iter);
                              position ++;
                            }
                        }
                      g_list_free (categories);

                    /* add category_menu */
                    if (!category_menu)
                      {
                        GtkWidget *menu_item;
                        category_menu = gtk_menu_new ();
                        menu_item = gtk_menu_item_new_with_label (category);
                        /*gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);*/
                        gtk_menu_shell_insert (GTK_MENU_SHELL (menu), menu_item, position);
                        gtk_widget_show (menu_item);
                        gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                                                   category_menu);
                      }

                    }

                    /* add an item to the category */
                      {
                        gint position=0;
                        GtkWidget *menu_item = gtk_menu_item_new_with_label (klass->name);

                        {
                          GList    *items;
                          GList    *iter;

                          items = gtk_container_get_children (GTK_CONTAINER (category_menu));
                          iter = items;

                          while (iter)
                            {
                              GtkWidget *item;
                              GtkWidget *label;
                              const char *text;

                              item = iter->data;
                              label = gtk_bin_get_child (GTK_BIN (item));
                              text = gtk_label_get_text (GTK_LABEL (label));

                              if (!strcmp (text, klass->name))
                                {
                                  g_warning ("adding %s for the second time to category %s", klass->name, category);
                                  iter = NULL;
                                }
                              else if (strcmp (text, klass->name)>0) /* we're further ahead in the
                                                                     alphabet than we should be */
                                {
                                  iter = NULL;
                                }
                              else
                                {
                                  iter = g_list_next (iter);
                                  position ++;
                                }
                            }
                          g_list_free (items);
                        }
                        gtk_menu_shell_insert (GTK_MENU_SHELL (category_menu), menu_item, position);
                        gtk_widget_show (menu_item);

                        g_signal_connect (G_OBJECT (menu_item), "activate",
                                          G_CALLBACK (menu_item_activate), item);
                      }
                  }
                }
            }
        }
      operation_class_iterate_for_menu (ops[no], menu, item);
    }
  g_free (ops);
}

static GtkWidget *
optype_menu (GeglNode *item)
{
  GtkWidget *menu = gtk_menu_new ();
  /* perhaps pass a flag,. indicating whether we're a source or filter|composer */
  operation_class_iterate_for_menu (GEGL_TYPE_OPERATION, menu, item);

  return menu;
}

static void
gtk_option_menu_position (GtkMenu  *menu,
                          gint     *x,
                          gint     *y,
                          gboolean *push_in,
                          gpointer  user_data)
{
  gint      popup_x;
  gint      popup_y;
  gint      window_x;
  gint      window_y;

  GtkWidget *window;
  GtkWidget *button;

  button = user_data;
  window = gtk_widget_get_toplevel (button);

  gtk_widget_translate_coordinates (button, window, 0, 0, &popup_x, &popup_y);
  gtk_window_get_position (GTK_WINDOW (window), &window_x, &window_y);
  popup_x += window_x;
  popup_y += window_y;

  if (x)
    *x = popup_x;
  if (y)
    *y = popup_y;
}

static void
button_clicked (GtkButton * button, gpointer item)
{
  GtkMenu  *menu;

  menu = GTK_MENU (optype_menu (item));
  gtk_menu_popup (menu, NULL, NULL, gtk_option_menu_position, button, 0, 0);
}



GtkWidget *
gegl_typeeditor_optype (GtkSizeGroup   *col1,
                        GtkSizeGroup   *col2,
                        GeglNodeEditor *node_editor)
{
  GtkWidget *hbox;
  GtkWidget *label = NULL;
  GtkWidget *hbox2;
  GtkWidget *entry;
  GtkEntryCompletion *completion;
  GtkTreeModel *completion_model;
  GtkWidget *button;
  GtkWidget *button_arrow;
  char     *current_type;
  GeglNode  *item = NULL;

  if (node_editor)
    item =node_editor->node;

  hbox = gtk_hbox_new (FALSE, 5);
  if (col1) label = gtk_label_new ("operation");
    else
  label = gtk_label_new ("add:");
  hbox2 = gtk_hbox_new (FALSE, 0);
  entry = gtk_entry_new ();
  if (!col1)
    editor.search_entry = entry;
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 6);
  completion = gtk_entry_completion_new ();
  completion_model = create_completion_model (item);
  //button           = gtk_button_new_with_label ("...");
  button = gtk_button_new ();
  button_arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);

  if (item)
    {
  current_type = g_strdup (gegl_node_get_operation (item));
    }
  else
    {
      current_type = g_strdup ("Type a GEGL operation, or examine to dropdown menu to the right");
    }

  if (label) gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (button), button_arrow);
  if (label && col1)
    {
      gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
      gtk_size_group_add_widget (col1, label);
    }
  if (col2) gtk_size_group_add_widget (col2, hbox2);

  if (current_type)
    {
      gtk_entry_set_text (GTK_ENTRY (entry), current_type);
      g_free (current_type);
    }
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  gtk_entry_set_completion (GTK_ENTRY (entry), completion);
  gtk_entry_completion_set_model (completion, completion_model);
  gtk_entry_completion_set_text_column (completion, 0);

  g_signal_connect (G_OBJECT (entry), "activate",
                    G_CALLBACK (entry_activate), item);
  g_signal_connect (G_OBJECT (completion), "match-selected",
                    G_CALLBACK (completion_match_selected), item);

  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (button_clicked), item);

  g_object_unref (completion);
  g_object_unref (completion_model);

  return hbox;
}

static void
entry_activate (GtkEntry *entry,
                gpointer  user_data)
{
  GeglNode   *item = user_data;
  const char *new_type = gtk_entry_get_text (entry);

  if (!item)
    {
      chain_in_operation (new_type);
      return;
    }
  gegl_node_set (item, "operation", new_type, NULL);
  property_editor_rebuild (editor.property_editor, item);

    {
      GtkTreeIter iter;
      GtkTreeModel *model;
      GtkTreeView *treeview = GTK_TREE_VIEW (tree_editor_get_treeview (editor.tree_editor));
      GtkTreeSelection *tree_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

      if (gtk_tree_selection_get_selected (tree_selection, &model, &iter))
        {
          gtk_tree_model_row_changed (model, gtk_tree_model_get_path (model, &iter), &iter);
          gegl_gui_flush ();
        }
    }
}

static gboolean
completion_match_selected (GtkEntryCompletion *completion,
                           GtkTreeModel       *model,
                           GtkTreeIter        *iter,
                           gpointer            user_data)
{
  GtkWidget *entry     = gtk_entry_completion_get_entry (completion);
  gchar     *new_value = NULL;

  gtk_tree_model_get (model, iter, 0, &new_value, -1);
  gtk_entry_set_text (GTK_ENTRY (entry), new_value);
  entry_activate (GTK_ENTRY (entry), user_data);

  g_free (new_value);
  return TRUE;
}
