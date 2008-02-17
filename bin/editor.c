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

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "gegl-bin-gui-types.h"

#include "gegl.h"

#include "editor-optype.h"
#include "gegl-node-editor.h"
#include "gegl-options.h"
#include "gegl-store.h"
#include "gegl-tree-editor.h"
#include "gegl-view.h"
#include "gegl-view.h"

#include "editor.h"

#ifdef G_OS_WIN32
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#define realpath(a,b) _fullpath(b,a,_MAX_PATH)
#endif


#define  KEY_ZOOM_FACTOR  2.0


static gchar *blank_composition =
    "<gegl>"
        "<color value='white'/>"
    "</gegl>";

static void gegl_editor_update_title (void);
static gboolean
cb_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean
cb_window_keybinding (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
  if (event->keyval == GDK_l &&
      (event->state & (GDK_CONTROL_MASK & gtk_accelerator_get_default_mod_mask())))
    {
      if (editor.search_entry)
        gtk_widget_grab_focus (editor.search_entry);
      return TRUE;
    }
  else
    {
    }
  return FALSE;
}

Editor editor;

static void editor_set_gegl (GeglNode    *gegl);
static GtkWidget *create_menubar (Editor *editor);
static GtkWidget *
create_window (Editor *editor)
{
  GtkWidget *self;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *menubar;
  GtkWidget *hpaned_top_level;
  GtkWidget *hpaned_top;
  GtkWidget *vpaned;
  GtkWidget *view;
  GtkWidget *property_scroll;
  GtkWidget *add_box;
  GtkWidget *add_entry;

  /* creation of ui components */
  editor->window = self = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  vbox = gtk_vbox_new (FALSE, 1);
  hbox = gtk_hbox_new (FALSE, 1);
  vbox2 = gtk_vbox_new (FALSE, 1);
  hpaned_top = gtk_vpaned_new ();
  hpaned_top_level = gtk_hpaned_new ();
  view = g_object_new (GEGL_TYPE_VIEW, NULL);
  property_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (property_scroll), editor->property_editor);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (property_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  vpaned = gtk_vpaned_new ();

  menubar = create_menubar (editor); /*< depends on other widgets existing */

  add_box = gtk_hbox_new (FALSE, 1);
  add_entry = gtk_entry_new ();

  /* packing */

  gtk_container_add (GTK_CONTAINER (self), vbox);
  gtk_box_pack_start (GTK_BOX (hbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hpaned_top_level, TRUE, TRUE, 0);
  gtk_paned_pack2 (GTK_PANED (hpaned_top_level), hpaned_top, FALSE, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), gtk_label_new ("     "), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), add_box, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), view, TRUE, TRUE, 0);
  gtk_paned_pack1 (GTK_PANED (hpaned_top_level), vbox2, TRUE, TRUE);

  gtk_paned_pack1 (GTK_PANED (hpaned_top), property_scroll, FALSE,
                   TRUE);
  gtk_paned_pack2 (GTK_PANED (hpaned_top), editor->tree_editor, FALSE, TRUE);

    {
      GtkWidget *foo = gegl_typeeditor_optype (NULL, NULL, NULL);
      gtk_box_pack_start (GTK_BOX (add_box), foo, TRUE, TRUE, 0);
    }

  /* setting properties for ui components */
  gtk_window_set_gravity (GTK_WINDOW (self), GDK_GRAVITY_STATIC);
  gtk_window_set_title (GTK_WINDOW (self), "GEGL");
  gtk_widget_set_size_request (editor->tree_editor, -1, 100);
  gtk_widget_set_size_request (property_scroll, -1, 100);
  gtk_widget_set_size_request (view, 89, 55);

  g_signal_connect (self, "delete-event",
                    G_CALLBACK (cb_window_delete_event), NULL);

  g_signal_connect (self, "key-press-event",
                    G_CALLBACK (cb_window_keybinding), NULL);

  gtk_widget_show_all (vbox);

  g_signal_connect_swapped (view, "notify::scale",
                            G_CALLBACK (gegl_editor_update_title),
                            editor);

  editor->view = view;
  editor->structure = hpaned_top;
  editor->property_pane = property_scroll;
  editor->tree_pane = editor->tree_editor;
  gtk_widget_hide (editor->structure);
  return self;
}

GeglNode *editor_output = NULL;

static void cb_shrinkwrap (GtkAction *action);
static void cb_fit (GtkAction *action);
static void cb_fit_on_screen (GtkAction *action);
#if 0
static void cb_recompute (GtkAction *action);
#endif
static void cb_redraw (GtkAction *action);
static void cb_next_file (GtkAction *action);
static void cb_previous_file (GtkAction *action);


static GeglNode *input_stream(GeglNode *root)
{
  GeglNode *iter = gegl_node_get_output_proxy (root, "output");
  gegl_node_get_bounding_box (editor.gegl);  /* to trigger defined setting for all */
  while (iter &&
         gegl_node_get_producer (iter, "input", NULL)){
    iter = gegl_node_get_producer (iter, "input", NULL);
  }
  return iter;
}

/* avoided to make this a really public function, since the need
 * is a bit of a hack
 */
GeglProcessor *gegl_view_get_processor (GeglView *view);

static gboolean play(gpointer data)
{
  Editor *editor=data;
  GeglView *view=GEGL_VIEW (editor->view);
  gdouble   progress;
  g_object_get (gegl_view_get_processor (view), "progress", &progress, NULL);
  if (progress >= 1.0)
    {
      /* modify source to trigger consumption of a new element */
      GeglNode *source = input_stream (editor->gegl);
      gint frame;
      gegl_node_get (source, "frame", &frame, NULL);
      g_warning ("(%f) frame: %i->%i", progress, frame, frame +1);
      frame++;
      gegl_node_set (source, "frame", frame, NULL);
      gegl_gui_flush ();
    }
  else
    {
      g_warning ("(%f)", progress);
    }

  return TRUE;
}

static gboolean advance_slide (gpointer data)
{
  cb_next_file (NULL);
  return TRUE;
}

gint
editor_main (GeglNode    *gegl,
             GeglOptions *options)
{
  GtkWidget *treeview;

  editor.options = options;
  editor.property_editor = gtk_vbox_new (FALSE, 0);
  editor.tree_editor = tree_editor_new (editor.property_editor);
  editor.graph_editor = NULL;/*gtk_label_new ("graph");*/
  editor.window = create_window (&editor);
  treeview = tree_editor_get_treeview (editor.tree_editor);
  gtk_container_add (GTK_CONTAINER (editor.property_editor),
                     gtk_label_new (editor.options->file));
  gtk_widget_show (editor.window);
  gtk_container_set_border_width (GTK_CONTAINER (editor.property_editor), 6);

  editor_set_gegl (gegl);

  /*cb_shrinkwrap (NULL);*/
  cb_fit_on_screen (NULL);
  gegl_editor_update_title ();

  if (options->delay != 0.0)
    {
      g_timeout_add (1000 * options->delay, advance_slide, NULL);
    }
  if (options->play)
    {
      g_timeout_add (100, play, &editor);
    }

  /*gtk_window_fullscreen (GTK_WINDOW (editor.window));*/
  gtk_main ();
  return 0;
}

static void cb_about (GtkAction *action);
/*static void cb_introspect (GtkAction *action);*/
static void cb_export (GtkAction *action);
static void cb_quit_dialog (GtkAction *action);
static void cb_composition_new (GtkAction *action);
static void cb_composition_load (GtkAction *action);
static void cb_composition_save (GtkAction *action);

static void cb_zoom_in (GtkAction *action);
static void cb_zoom_out (GtkAction *action);
static void cb_zoom_50 (GtkAction *action);
static void cb_zoom_100 (GtkAction *action);
static void cb_zoom_200 (GtkAction *action);


static GtkActionEntry action_entries[] = {
  {"CompositionMenu", NULL, "_Composition", NULL, NULL, NULL},
  {"ViewMenu", NULL, "_View", NULL, NULL, NULL},
  {"HelpMenu", NULL, "_Help", NULL, NULL, NULL},

  {"New", GTK_STOCK_NEW,
   "_New", "<control>N",
   "Create a new composition",
   G_CALLBACK (cb_composition_new)},

  {"Next", GTK_STOCK_GO_FORWARD,
   "_Next", "<control>a",
   "Go to next file in list",
   G_CALLBACK (cb_next_file)},

  {"Previous", GTK_STOCK_GO_BACK,
   "_Previous", "<control>z",
   "Go to previous file in list",
   G_CALLBACK (cb_previous_file)},

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

  {"About", GTK_STOCK_ABOUT,
   "_About", "",
   "About",
   G_CALLBACK (cb_about)},
/*
  {"Introspect", NULL,
   "_Introspect", "<control>I",
   "Introspect",
   G_CALLBACK (cb_introspect)},*/

  {"Export", GTK_STOCK_SAVE,
   "_Export", "<control><shift>E",
   "Export to PNG",
   G_CALLBACK (cb_export)},

  {"ShrinkWrap", NULL,
   "_Shrink Wrap", "<control>E",
   "Size the window to the image, if feasible",
   G_CALLBACK (cb_shrinkwrap)},

  {"Fit", GTK_STOCK_ZOOM_FIT,
   "_Fit", "<control>F",
   "Fit the image in window",
   G_CALLBACK (cb_fit)},

  {"FitOnScreen", NULL,
   "_Fit On Screen", "",
   "Fit the image on screen",
   G_CALLBACK (cb_fit_on_screen)},

  {"ZoomIn", GTK_STOCK_ZOOM_IN,
   "Zoom In", "<control>plus",
   "",
   G_CALLBACK (cb_zoom_in)},

  {"ZoomOut", GTK_STOCK_ZOOM_OUT,
   "Zoom Out", "<control>minus",
   "",
   G_CALLBACK (cb_zoom_out)},

  {"Zoom50", NULL,
   "50%", "<control>2",
   "",
   G_CALLBACK (cb_zoom_50)},

  {"Zoom100", GTK_STOCK_ZOOM_100,
   "100%", "<control>1",
   "",
   G_CALLBACK (cb_zoom_100)},

  {"Zoom200", NULL,
   "200%", "",
   "",
   G_CALLBACK (cb_zoom_200)},

#if 0
  {"Recompute", NULL,
   "_Recompute View", "<shift><control>R",
   "Recalculate all image data (for working around dirt bugs)",
   G_CALLBACK (cb_recompute)},
#endif

  {"Redraw", NULL,
   "_Redraw View", "<control>R",
   "Repaints all image data (works around display glitches)",
   G_CALLBACK (cb_redraw)},
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
  "      <menuitem action='Next'/>"
  "      <menuitem action='Previous'/>"
  "      <separator/>"
  "      <menuitem action='Export'/>"
  "      <separator/>"
  "      <menuitem action='Quit'/>"
  "      <separator/>"
  "    </menu>"
  "    <menu action='ViewMenu'>"
  "      <menuitem action='FitOnScreen'/>"
  "      <menuitem action='Fit'/>"
  "      <menuitem action='ShrinkWrap'/>"
  "      <separator/>"
  "      <menuitem action='ZoomIn'/>"
  "      <menuitem action='ZoomOut'/>"
  "      <menuitem action='Zoom50'/>"
  "      <menuitem action='Zoom100'/>"
  "      <menuitem action='Zoom200'/>"
  "      <separator/>"
  "      <menuitem action='Redraw'/>"
  /*"      <menuitem action='Recompute'/>"*/
  "      <separator/>"
  "      <menuitem action='Structure'/>"
  "      <menuitem action='Tree'/>"
  "      <menuitem action='Properties'/>"
  /*"      <menuitem action='Introspect'/>"*/
  "    </menu>"
  "    <menu action='HelpMenu'>"
  "      <menuitem action='About'/>"
  "    </menu>"
  "  </menubar>"
  "</ui>";

static void cb_tree_visible (GtkAction *action, gpointer userdata);
static void cb_properties_visible (GtkAction *action, gpointer userdata);
static void cb_structure_visible (GtkAction *action, gpointer userdata);

static GtkToggleActionEntry toggle_entries[]={
    {"Tree", NULL,
     "TreeView", NULL,
     "Toggle visibility of tree structure of composition",
     G_CALLBACK (cb_tree_visible),
     TRUE},
    {"Properties", NULL,
     "PropertiesView", NULL,
     "Toggle visibility of property editor",
     G_CALLBACK (cb_properties_visible),
     TRUE},
    {"Structure", NULL,
     "StructureView", "F5",
     "Toggle visibility of sidebar",
     G_CALLBACK (cb_structure_visible),
     FALSE},
};

static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);

static GtkActionGroup *get_actions(void)
{
  static GtkActionGroup *actions = NULL;
  if (!actions)
    {
      actions = gtk_action_group_new ("Actions");
      gtk_action_group_add_actions (actions, action_entries, n_action_entries,
                                    NULL);
      gtk_action_group_add_toggle_actions (actions, toggle_entries, n_toggle_entries,
                                           (void*)0x6367); /* <- probably userdata */
    }
  return actions;
}

static GtkWidget *
create_menubar (Editor *editor)
{
  GtkUIManager *ui;
  GError   *error = NULL;

  ui = gtk_ui_manager_new ();
  gtk_ui_manager_set_add_tearoffs (ui, TRUE);
  gtk_ui_manager_insert_action_group (ui, get_actions (), 0);

  gtk_window_add_accel_group (GTK_WINDOW (editor->window),
                              gtk_ui_manager_get_accel_group (ui));

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

  dialog = gtk_dialog_new_with_buttons ("GEGL - New Composition",
                                        GTK_WINDOW (editor.window),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  alert = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
                                    GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (hbox), alert);

  label = gtk_label_new ("Discard current composition?\n"
                         "All unsaved data will be lost.");
  gtk_container_add (GTK_CONTAINER (hbox), label);

  gtk_widget_show_all (dialog);
  result = gtk_dialog_run (GTK_DIALOG (dialog));

  switch (result)
    {
    case GTK_RESPONSE_ACCEPT:
      {
      /* FIXME: should append to list of files, and set as current
        editor_set_gegl (gegl_parse_xml (blank_composition), "untitled.xml");
        */
        editor_set_gegl (gegl_node_new_from_xml (blank_composition, "/"));
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

  dialog = gtk_file_chooser_dialog_new ("Load GEGL Composition",
                                        GTK_WINDOW (editor.window),
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
      /* FIXME: append name to list of files in options */
      /* FIXME: handle non XML files */
        {
          gchar *temp = g_strdup (filename);
          gchar *path = g_strdup (g_path_get_dirname (temp));
          editor_set_gegl (gegl_node_new_from_xml (xml, path));
          g_free (temp);
          g_free (path);
        }
      g_free (xml);
    }
  gtk_widget_destroy (dialog);
}

static void
cb_composition_save (GtkAction *action)
{

  GtkWidget *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new ("Save GEGL Composition",
                                        GTK_WINDOW (editor.window),
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
    realpath (editor.options->file, absolute_path);
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), absolute_path);
  }

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
      const gchar *filename =
        gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
      if (filename)
        {
          gchar *full_filename;
          gchar  abs_filepath[PATH_MAX];
          gchar *abs_path;
          gchar *xml;

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
          /*
           * FIXME: append new path to end of list or something and set as current,..
          if (editor.options->file)
            g_free (editor.options->file);
          editor.options->file = g_strdup (full_filename);
          */

          realpath (full_filename, abs_filepath);
          abs_path = g_strdup (g_path_get_dirname (abs_filepath));
          xml = gegl_node_to_xml (editor.gegl, abs_path); /*oxide_xml (editor->oxide, abs_path);*/

          g_file_set_contents (full_filename, xml, -1, NULL);

          g_free (abs_path);
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

  dialog = gtk_dialog_new_with_buttons ("GEGL - Confirm Quit",
                                        GTK_WINDOW (editor.window),
                                        GTK_DIALOG_MODAL,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                                        GTK_STOCK_SAVE, 4,
                                        GTK_STOCK_QUIT, GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  hbox = gtk_hbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), hbox);

  alert = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING,
                                    GTK_ICON_SIZE_DIALOG);
  gtk_container_add (GTK_CONTAINER (hbox), alert);

  label = gtk_label_new ("Really quit?\nAll unsaved data will be lost.");
  gtk_container_add (GTK_CONTAINER (hbox), label);

  gtk_widget_show_all (hbox);

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

static gboolean file_is_gegl_xml (const gchar *path)
{
  gchar *extension;

  extension = strrchr (path, '.');
  if (!extension)
    return FALSE;
  extension++;
  if (extension[0]=='\0')
    return FALSE;
  if (!strcmp (extension, "xml")||
      !strcmp (extension, "XML"))
    return TRUE;
  return FALSE;
}

static void do_load (void)
{
  GeglOptions *o = editor.options;
  gchar *xml = NULL;

  gchar *temp1 = g_strdup (o->file);
  gchar *temp2;
  gchar *path_root;
  temp2 = g_strdup (g_path_get_dirname (temp1));
  path_root = g_strdup (realpath (temp2, NULL));
  g_free (temp1);
  g_free (temp2);

  if (file_is_gegl_xml (o->file))
    {
      GError      *err      = NULL;
      g_file_get_contents (o->file, &xml, NULL, &err);
      if (err != NULL)
        {
          g_warning ("Unable to read file: %s", err->message);
        }
    }
  else
    {
      GString *acc = g_string_new ("");

            {gchar *file_basename;
             gchar *tmp;
             tmp=g_strdup (o->file);
             file_basename = g_path_get_basename (tmp);

             g_string_append (acc, "<gegl><load path='");
             g_string_append (acc, file_basename);
             g_string_append (acc, "'/></gegl>");

             g_free (tmp);
            }

      xml = g_string_free (acc, FALSE);
    }

  editor_set_gegl (gegl_node_new_from_xml (xml, path_root));
  g_free (path_root);
  g_free (xml);
}

static void cb_next_file (GtkAction *action)
{
  gegl_options_next_file (editor.options);
  do_load ();
  cb_fit (NULL);
}


static void cb_previous_file (GtkAction *action)
{
  gegl_options_previous_file (editor.options);
  do_load ();
  cb_fit (NULL);
}


static void
cb_about (GtkAction *action)
{
  GtkWidget *window;
  GtkWidget *about;
  GeglNode  *gegl;

  gegl = gegl_node_new_from_xml (
   "<gegl> <over> <invert/> <shift x='20.0' y='140.0'/> <text string=\"GEGL is a image processing and compositing framework.\n\nGUI editor Copyright © 2006, 2007 Øyvind Kolås, Kevin Cozens, Sven Neumann and Michael Schumacher\nGEGL and its editor come with ABSOLUTELY NO WARRANTY. This is free software, and you are welcome to redistribute it under certain conditions. The processing and compositing library GEGL is licensed under LGPLv3+ and the editor itself is licensed as GPLv3+.\" font='Sans' size='10.0' wrap='300' alignment='0' width='224' height='52'/> </over> <over> <shift x='20.0' y='10.0'/> <dropshadow opacity='1.0' x='10.0' y='10.0' radius='5.0'/> <text string='GEGL' font='Sans' size='100.0' wrap='-1' alignment='0'/> </over> <perlin-noise alpha='12.30' scale='0.10' zoff='-1.0' seed='20.0' n='6.0'/> </gegl>"
  ,NULL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "About GEGL");
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

/*
static void
cb_introspect (GtkAction *action)
{
  GtkWidget *window;
  GtkWidget *introspect;
  GeglNode  *gegl;
  GeglNode  *dot;
  GeglRectangle bounding_box;

  gegl = gegl_node_new ();
  dot = gegl_node_new_child (gegl,
                             "operation", "introspect",
                             "node", editor.gegl,
                             NULL);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GEGL Introspection");
  introspect = g_object_new (GEGL_TYPE_VIEW,
                             "node", dot,
                             NULL);
  bounding_box = gegl_node_get_bounding_box (dot);
  bounding_box = gegl_node_get_bounding_box (dot);
  g_object_unref (gegl);

  gtk_container_add (GTK_CONTAINER (window), introspect);

  gtk_widget_set_size_request (introspect, bounding_box.width  * 0.70, bounding_box.height * 0.70);
  g_object_set (introspect, "scale", 0.70, NULL);

  g_signal_connect (G_OBJECT (window), "delete-event",
                    G_CALLBACK (gtk_widget_destroy), window);
  gtk_widget_show_all (window);

}*/

static void cb_structure_visible (GtkAction *action, gpointer userdata)
{
  GtkWidget *widget = editor.structure;

  if (GTK_WIDGET_VISIBLE (widget))
    {
      gtk_widget_hide (widget);
    }
  else
    {
      gtk_widget_show (widget);
    }
}

static void cb_properties_visible (GtkAction *action, gpointer userdata)
{
  GtkWidget *widget = editor.property_pane;
  if (GTK_WIDGET_VISIBLE (widget))
    {
      gtk_widget_hide (widget);
    }
  else
    {
      gtk_widget_show (widget);
    }
}

static void cb_tree_visible (GtkAction *action, gpointer userdata)
{
  GtkWidget *widget = editor.tree_pane;
  if (GTK_WIDGET_VISIBLE (widget))
    {
      gtk_widget_hide (widget);
    }
  else
    {
      gtk_widget_show (widget);
    }
}

static void cb_fit (GtkAction *action)
{
  GeglRectangle defined = gegl_node_get_bounding_box (editor.gegl);
  gint          x, y;
  gdouble       hscale;
  gdouble       vscale;

  hscale = (gdouble) editor.view->allocation.width / defined.width ;
  vscale = (gdouble) editor.view->allocation.height / defined.height ;

  if (hscale > vscale)
    {
      hscale = vscale;
      y=0;
      x= (editor.view->allocation.width - defined.width  * hscale) / 2 / hscale;
    }
  else
    {
      vscale = hscale;
      x=0;
      y= (editor.view->allocation.height - defined.height  * vscale) / 2 / vscale;
    }

  g_object_set (editor.view,
                "x",     defined.x - x,
                "y",     defined.y - y,
                "scale", hscale,
                NULL);
}


static void cb_fit_on_screen (GtkAction *action)
{
  GeglRectangle defined = gegl_node_get_bounding_box (editor.gegl);
  gint ow = editor.view->allocation.width;

  g_object_set (editor.view, "x", defined.x, "y", defined.y, NULL);
  {
    GdkScreen *screen= gtk_window_get_screen (GTK_WINDOW (editor.window));

    gint    screen_width, screen_height;

    screen_width = gdk_screen_get_width (screen);
    screen_height = gdk_screen_get_height (screen);

    gtk_window_resize (GTK_WINDOW (editor.window), screen_width * 0.75,
                                                   screen_height * 0.75);
    while (ow == editor.view->allocation.width)
      gtk_main_iteration();
  }
  cb_fit (action);
  cb_shrinkwrap (action);
}

static void cb_shrinkwrap (GtkAction *action)
{
  GeglRectangle defined = gegl_node_get_bounding_box (editor.gegl);
  /*g_warning ("shrink wrap %i,%i %ix%i", defined.x, defined.y, defined.width , defined.h);*/

  g_object_set (editor.view, "x", defined.x, "y", defined.y, NULL);
  {
    GdkScreen *screen= gtk_window_get_screen (GTK_WINDOW (editor.window));

    gint    screen_width, screen_height;
    gint    width, height;
    gdouble scale;
    g_object_get (editor.view, "scale", &scale, NULL);

    /* compute a base width/height for the other contents of the window */
    screen_width = gdk_screen_get_width (screen);
    screen_height = gdk_screen_get_height (screen);
    gtk_window_get_size (GTK_WINDOW (editor.window), &width, &height);
    width -= editor.view->allocation.width;
    height -= editor.view->allocation.height;

    /* add the area consumed by the canvas content */
    width += defined.width  * scale;
    height += defined.height  * scale;

    if (width > screen_width)
      width = screen_width;
    if (height > screen_height)
      height = screen_height;

    gtk_window_resize (GTK_WINDOW (editor.window), width, height);
  }
/*
  for (i=0;i<40;i++){
    gtk_main_iteration ();
  }*/
}

#if 0
static void cb_recompute (GtkAction *action)
{
  GeglNode *node;

  g_object_get (GEGL_VIEW(editor.view), "node", &node, NULL);
  /*gegl_node_disable_cache (node);*/
  this used to just forcibly remove the existing cache object for the
    toplevel cache.
  */
  gegl_gui_flush ();
}
#endif

static void cb_redraw (GtkAction *action)
{
  gtk_widget_queue_draw (editor.view);
}

static void gegl_editor_update_title (void)
{
  gdouble zoom;
  gchar buf[512];
  g_object_get (editor.view, "scale", &zoom, NULL);
  sprintf (buf, "GEGL %2.0f%%", zoom * 100);

  gtk_window_set_title (GTK_WINDOW (editor.window), buf);
}

static void cb_zoom_100 (GtkAction *action)
{
  gint width, height;
  gint x,y;
  gdouble scale;

  width = editor.view->allocation.width;
  height = editor.view->allocation.height;

  g_object_get (editor.view,
                "x", &x,
                "y", &y,
                "scale", &scale,
                NULL);

  x += (width/2) / scale;
  y += (height/2) / scale;

  scale = 1.0;

  x -= (width/2) / scale;
  y -= (height/2) / scale;

  g_object_set (editor.view,
                "x", x,
                "y", y,
                "scale", scale,
                NULL);
}

static void cb_zoom_200 (GtkAction *action)
{
  gint width, height;
  gint x,y;
  gdouble scale;

  width = editor.view->allocation.width;
  height = editor.view->allocation.height;

  g_object_get (editor.view,
                "x", &x,
                "y", &y,
                "scale", &scale,
                NULL);

  x += (width/2) / scale;
  y += (height/2) / scale;

  scale = 2.0;

  x -= (width/2) / scale;
  y -= (height/2) / scale;

  g_object_set (editor.view,
                "x", x,
                "y", y,
                "scale", scale,
                NULL);
}

static void cb_zoom_50 (GtkAction *action)
{
  gint width, height;
  gint x,y;
  gdouble scale;

  width = editor.view->allocation.width;
  height = editor.view->allocation.height;

  g_object_get (editor.view,
                "x", &x,
                "y", &y,
                "scale", &scale,
                NULL);

  x += (width/2) / scale;
  y += (height/2) / scale;

  scale = 0.5;

  x -= (width/2) / scale;
  y -= (height/2) / scale;

  g_object_set (editor.view,
                "x", x,
                "y", y,
                "scale", scale,
                NULL);
}


static void cb_zoom_in (GtkAction *action)
{
  gint    width, height;
  gint    x, y;
  gdouble scale;
  gdouble focus_x, focus_y;

  width  = editor.view->allocation.width;
  height = editor.view->allocation.height;

  g_object_get (editor.view,
                "x",     &x,
                "y",     &y,
                "scale", &scale,
                NULL);

  focus_x = (x + width  / 2) / scale;
  focus_y = (y + height / 2) / scale;

  scale *= KEY_ZOOM_FACTOR;

  x = focus_x * scale - width  / 2;
  y = focus_y * scale - height / 2;

  g_object_set (editor.view,
                "x",     x,
                "y",     y,
                "scale", scale,
                NULL);
}

static void cb_zoom_out (GtkAction *action)
{
  gint    width, height;
  gint    x, y;
  gdouble scale;
  gdouble focus_x, focus_y;

  width  = editor.view->allocation.width;
  height = editor.view->allocation.height;

  g_object_get (editor.view,
                "x",     &x,
                "y",     &y,
                "scale", &scale,
                NULL);

  focus_x = (x + width  / 2) / scale;
  focus_y = (y + height / 2) / scale;

  scale /= KEY_ZOOM_FACTOR;

  x = focus_x * scale - width  / 2;
  y = focus_y * scale - height / 2;

  g_object_set (editor.view,
                "x",     x,
                "y",     y,
                "scale", scale,
                NULL);

  gegl_gui_flush ();
}


#include "export.h"

static void cb_export (GtkAction *action)
{
  export_window ();
}

void editor_refresh_structure (void)
{
  GeglStore *store = gegl_store_new ();
  GtkWidget *treeview;

  gegl_store_set_gegl (store, editor.gegl);
  treeview = tree_editor_get_treeview (editor.tree_editor);
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), NULL);
  gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                           GTK_TREE_MODEL (store));
}

typedef struct _GeglPad GeglPad;
void gegl_pad_set_format (gpointer,gpointer);
gpointer gegl_node_get_pad (gpointer, const gchar *name);

static void editor_set_gegl (GeglNode    *gegl)
{
/*
  if (editor.options->file)
    g_free (editor.options->file);
  editor.options->file = g_strdup (path);*/

  if (editor.gegl)
    g_object_unref (editor.gegl);
  editor.gegl = gegl;

    {
      GeglPad *pad;
      pad = gegl_node_get_pad (gegl, "output");
      g_assert (pad);
      gegl_pad_set_format (pad, babl_format ("R'G'B' u8"));
    }

  g_object_set (editor.view, "node", editor.gegl, NULL);
  editor_refresh_structure ();
}

void
gegl_gui_flush (void)
{
  gegl_view_repaint (GEGL_VIEW (editor.view));
}
