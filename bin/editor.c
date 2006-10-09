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

#include <gegl-plugin.h>
#include <gtk/gtk.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include "gegl-store.h"
#include "gegl-tree-editor.h"
#include "editor.h"
#include "gegl-view.h"

static gchar *blank_composition =
    "<gegl>"
        "<color value='white'/>"
    "</gegl>";

static gboolean
cb_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data);

Editor editor;

static void reset_gegl (GeglNode    *gegl,
                        const gchar *path);
static GtkWidget *create_menubar (Editor *editor);
static GtkWidget *
create_tree_window (Editor *editor)
{
  GtkWidget *self;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkWidget *hpaned_top_level;
  GtkWidget *hpaned_top;
  GtkWidget *vpaned;
  GtkWidget *drawing_area;
  GtkWidget *property_scroll;

  /* creation of ui components */
  self = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  vbox = gtk_vbox_new (FALSE, 1);
  hpaned_top = gtk_vpaned_new ();
  hpaned_top_level = gtk_hpaned_new ();
  drawing_area = g_object_new (GEGL_TYPE_VIEW,
                               "node", editor->gegl,
                               NULL);
  property_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (property_scroll), editor->property_editor);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (property_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  vpaned = gtk_vpaned_new ();

  menubar = create_menubar (editor); /*< depends on other widgets existing */

  /* packing */

  gtk_container_add (GTK_CONTAINER (self), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hpaned_top_level, TRUE, TRUE, 0);
  gtk_paned_pack1 (GTK_PANED (hpaned_top_level), hpaned_top, FALSE, TRUE);
  gtk_paned_pack2 (GTK_PANED (hpaned_top_level), drawing_area, TRUE, TRUE);

  gtk_paned_pack1 (GTK_PANED (hpaned_top), property_scroll, FALSE,
                   TRUE);
  gtk_paned_pack2 (GTK_PANED (hpaned_top), editor->tree_editor, FALSE, TRUE);

  /* setting properties for ui components */
  gtk_window_set_gravity (GTK_WINDOW (self), GDK_GRAVITY_STATIC);
  gtk_window_set_title (GTK_WINDOW (self), "GEGL - tree ui");
  gtk_widget_set_size_request (editor->tree_editor, -1, 200);
  gtk_widget_set_size_request (property_scroll, -1, 200);
  gtk_widget_set_size_request (drawing_area, 256, 256);

  g_signal_connect (G_OBJECT (self), "delete-event",
                    G_CALLBACK (cb_window_delete_event), NULL);

  gtk_window_set_geometry_hints (GTK_WINDOW (self), NULL, NULL,
                                 GDK_HINT_USER_POS);
  gtk_widget_show_all (vbox);

  editor->drawing_area = drawing_area;
  return self;
}


GeglNode *editor_output = NULL;

gint
editor_main (GeglNode    *gegl,
             const gchar *path)
{
  GtkWidget *treeview;

  editor.property_editor = gtk_vbox_new (FALSE, 0);
  editor.tree_editor = tree_editor_new (editor.property_editor);
  editor.graph_editor = NULL;/*gtk_label_new ("graph");*/
  editor.tree_window = create_tree_window (&editor);
  treeview = tree_editor_get_treeview (editor.tree_editor);
  gtk_container_add (GTK_CONTAINER (editor.property_editor), gtk_label_new (editor.composition_path));
  gtk_widget_show_all (editor.tree_window);

  reset_gegl (gegl, path);

  gtk_main ();
  return 0;
}

static void cb_about (GtkAction *action);
static void cb_quit_dialog (GtkAction *action);
static void cb_composition_new (GtkAction *action);
static void cb_composition_load (GtkAction *action);
static void cb_composition_save (GtkAction *action);

static GtkActionEntry action_entries[] = {
  {"CompositionMenu", NULL, "_Composition", NULL, NULL, NULL},
  {"ViewMenu", NULL, "_View", NULL, NULL, NULL},
  {"HelpMenu", NULL, "_Help", NULL, NULL, NULL},

  {"New", GTK_STOCK_NEW,
   "_New", "<control>N",
   "Create a new composition",
   G_CALLBACK (cb_composition_new)},

  {"Open", GTK_STOCK_OPEN,
   "_Open", "<control>O",
   "Open a composition",
   G_CALLBACK (cb_composition_load)},

  {"Save", GTK_STOCK_SAVE,
   "_Save", "<control>S",
   "Save current composition",
   G_CALLBACK (cb_composition_save)},

  {"Quit", GTK_STOCK_QUIT,
   "_Quit", "<control>Q",
   "Quit",
   G_CALLBACK (cb_quit_dialog)},

  {"About", NULL,
   "_About", "<control>A",
   "About",
   G_CALLBACK (cb_about)},
};
static guint n_action_entries = G_N_ELEMENTS (action_entries);

static const gchar *ui_info =
  "<ui>"
  "  <menubar name='MenuBar'>"
  "    <menu action='CompositionMenu'>"
  "      <separator/>"
  "      <menuitem action='New'/>"
  "      <menuitem action='Open'/>"
  "      <menuitem action='Save'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
  "      <separator/>"
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='About'/>"
  "    </menu>"
  "  </menubar>"
  "</ui>";

static GtkWidget *
create_menubar (Editor *editor)
{
  GtkActionGroup *actions;
  GtkUIManager *ui;
  GError   *error = NULL;


  ui = gtk_ui_manager_new ();
  actions = gtk_action_group_new ("Actions");
  gtk_action_group_add_actions (actions, action_entries, n_action_entries,
                                NULL);

  gtk_ui_manager_set_add_tearoffs (ui, TRUE);
  gtk_ui_manager_insert_action_group (ui, actions, 0);

#if 0
  gtk_window_add_accel_group (GTK_WINDOW (editor->tree_window),
                              gtk_ui_manager_get_accel_group (ui));
#endif

  if (!gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error))
    {
      g_message ("building menus failed: %s", error->message);
      g_error_free (error);
    }

  return gtk_ui_manager_get_widget (ui, "/MenuBar");
}

static void
cb_composition_new (GtkAction *action)
{
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *alert;
  GtkWidget *label;
  gint      result;

  dialog = gtk_dialog_new_with_buttons ("GEGL - new composition",
                                        GTK_WINDOW (editor.tree_window),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        NULL);

  alert =
    StockIcon (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG,
               editor.tree_window);

  label =
    gtk_label_new
    ("Discard current composition?\nAll unsaved data will be lost.");

  gtk_misc_set_padding (GTK_MISC (label), 10, 10);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (hbox), alert);
  gtk_container_add (GTK_CONTAINER (hbox), label);

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  gtk_widget_show_all (dialog);
  result = gtk_dialog_run (GTK_DIALOG (dialog));

  switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
      {
        reset_gegl (gegl_xml_parse (blank_composition), "untitled.xml");

      }
      break;
    default:
      break;
    }
  gtk_widget_destroy (dialog);
}

static gboolean
cb_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  cb_quit_dialog (NULL);
  return TRUE;
}

static void
cb_composition_load (GtkAction *action)
{
  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new ("Load GEGL composition",
                                        GTK_WINDOW (editor.tree_window),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                        NULL);
  filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "text/xml");
  gtk_file_filter_set_name (filter, "GEGL composition");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                        "/home/pippin/src/editor/", NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      const gchar *filename;
      gchar *xml;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      g_file_get_contents (filename, &xml, NULL, NULL);
      reset_gegl (gegl_xml_parse (xml), filename);
      g_free (xml);
    }
  gtk_widget_destroy (dialog);
}

static void
cb_composition_save (GtkAction *action)
{

  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new ("Save GEGL composition",
                                        GTK_WINDOW (editor.tree_window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                        NULL);

  filter = gtk_file_filter_new ();
  gtk_file_filter_add_mime_type (filter, "text/xml");
  gtk_file_filter_set_name (filter, "GEGL composition");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                        "/home/pippin/media/video/", NULL);

  {
    gchar absolute_path[PATH_MAX];
    realpath (editor.composition_path, absolute_path);
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), absolute_path);
  }

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      const gchar *filename =
        gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (filename)
        {
          gchar     *full_filename;
          gchar      abs_filepath[PATH_MAX];
          const gchar *abs_path;
          gchar     *xml;

          if (strstr (filename, "xml"))
            {
              full_filename = strdup (filename);
            }
          else
            {
              full_filename = g_malloc (strlen (filename) + 4);
              strcpy (full_filename, filename);
              full_filename = strcat (full_filename, ".xml");
            }
          if (editor.composition_path)
            g_free (editor.composition_path);
          editor.composition_path = g_strdup (full_filename);

          realpath (full_filename, abs_filepath);
          abs_path = dirname (abs_filepath);
          xml = gegl_to_xml (editor.gegl); /*oxide_xml (editor->oxide, abs_path);*/

          g_file_set_contents (full_filename, xml, -1, NULL);

          g_free (xml);
          g_free (full_filename);
        }
    }
  gtk_widget_destroy (dialog);
}

static void
cb_quit_dialog (GtkAction *action)
{
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *alert;
  gint      result;

  dialog = gtk_dialog_new_with_buttons ("GEGL - quit confirmation",
                                        GTK_WINDOW (editor.tree_window),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                        GTK_STOCK_SAVE, 4,
                                        GTK_STOCK_QUIT, GTK_RESPONSE_ACCEPT,
                                        NULL);


  alert =
    StockIcon (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG,
               editor.tree_window);

  label = gtk_label_new ("Really quit?\nAll unsaved data will be lost.");
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);

  hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_add (GTK_CONTAINER (hbox), alert);
  gtk_container_add (GTK_CONTAINER (hbox), label);

  gtk_widget_show_all (hbox);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  result = gtk_dialog_run (GTK_DIALOG (dialog));
  switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
      gtk_main_quit ();
      break;
    case 4:
      cb_composition_save (action);
    default:
      break;
    }
  gtk_widget_destroy (dialog);
}

static void
cb_about (GtkAction *action)
{
  GtkWidget *window;
  GtkWidget *about;
  GeglNode  *gegl;

  gegl = gegl_xml_parse (
   "<gegl> <over> <invert/> <shift x='20.0' y='140.0'/> <text string=\"GEGL is a image processing and compositing framework.\n\nGUI editor Copyright © 2006 Øyvind Kolås\nGEGL and it's editor comes with ABSOLUTELY NO WARRANTY. This is free software, and you are welcome to redistribute it under certain conditions. The processing and compositing library GEGL is licensed under LGPLv2 and the editor itself is licensed as GPLv2.\" font='Sans' size='10.0' wrap='300' alignment='0' width='224' height='52'/> </over> <over> <shift x='20.0' y='10.0'/> <dropshadow opacity='1.0' x='10.0' y='10.0' radius='5.0'/> <text string='GEGL' font='Sans' size='100.0' wrap='-1' alignment='0'/> </over> <perlin-noise alpha='12.30' beta='0.10' zoff='-1.0' seed='20.0' n='6.0'/> </gegl>"
   );
   
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "About gugl");
  about = g_object_new (GEGL_TYPE_VIEW,
                        "node", gegl,
                        NULL);
  g_object_unref (gegl);
  gtk_container_add (GTK_CONTAINER (window), about);
  gtk_widget_set_size_request (about, 320, 260);

  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (gtk_widget_destroy), window);
  gtk_widget_show_all (window);
}

GtkWidget *
StockIcon (const gchar *id, GtkIconSize size, GtkWidget *widget)
{
  GdkPixbuf *icon;
  GtkWidget *image;

  icon = gtk_widget_render_icon (widget, id, size, "");
  image = gtk_image_new_from_pixbuf (icon);

  g_object_unref (icon);
  return image;
}



static void reset_gegl (GeglNode    *gegl,
                        const gchar *path)
{
  GeglStore *store = gegl_store_new ();
  GtkWidget *treeview;

  if (editor.composition_path)
    g_free (editor.composition_path);
  editor.composition_path = g_strdup (path);


  if (editor.gegl)
    g_object_unref (editor.gegl);
  editor.gegl = gegl;

  g_object_set (editor.drawing_area, "node", editor.gegl, NULL);
  gegl_store_set_root (store, gegl_graph_output (editor.gegl, "output"));
  gegl_store_set_gegl (store, editor.gegl);
  treeview = tree_editor_get_treeview (editor.tree_editor);
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), NULL);
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                               GTK_TREE_MODEL (store));
}
