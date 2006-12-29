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

#include "config.h"

#include <glib.h>
#include <gegl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gegl-options.h"
#include "gegl-dot.h"

#if HAVE_GTK
#include <gtk/gtk.h>
#include "editor.h"
#endif

#define DEFAULT_COMPOSITION \
"<?xml version='1.0' encoding='UTF-8'?> <gegl> <over> <shift x='32.500000' y='157.500000'/> <dropshadow opacity='5.000000' x='2.000000' y='2.000000' radius='3.000000'/> <text string='GEGL is a graph based image processing and compositing framework. 2000-2006 © Calvin Williamson, Caroline Dahloff, Manish Singh, Jay Cox, Daniel Rogers, Sven Neumann, Michael Natterer, Øyvind Kolås, Philip Lafleur, Dominik Ernst, Richard Kralovic, Kevin Cozens, Victor Bogado, Martin Nordholts, Geert Jordaens, Garry R. Osgood and Jakub Steiner' font='Sans' size='10.458599' color='rgb(1.0000, 1.0000, 1.0000)' wrap='240' alignment='0' width='240' height='104'/> </over> <over> <dropshadow opacity='-4.375000' x='3.750000' y='2.500000' radius='15.000000'/> <shift x='42.500000' y='43.750000'/> <text string='GEGL' font='Sans' size='76.038217' color='rgb(0.1172, 0.1057, 0.0283)' wrap='-1' alignment='0' width='208' height='89'/> </over> <whitebalance high-a-delta='-0.110463' high-b-delta='0.026511' low-a-delta='0.000000' low-b-delta='0.000000' saturation='0.319548'/> <brightness-contrast contrast='0.558148' brightness='-0.150230'/> <FractalExplorer width='300' height='400' fractaltype='0' xmin='0.049180' xmax='0.688525' ymin='0.000000' ymax='0.590164' iter='50' cx='-0.750000' cy='0.200000' redstretch='1.000000' greenstretch='1.000000' bluestretch='1.000000' redmode='1' greenmode='1' bluemode='0' redinvert='false' greeninvert='false' blueinvert='false' ncolors='256' useloglog='false'/> </gegl>"




/*FIXME: this should be in gegl.h*/

GeglNode * gegl_graph_output          (GeglNode     *graph,
                                       const gchar  *name);


GeglNode * gegl_node_get_connected_to (GeglNode     *self,
                                       gchar        *pad_name);

/******************/

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

gint
main (gint    argc,
      gchar **argv)
{
  GeglOptions *o        = NULL;
  GeglNode    *gegl     = NULL;
  gchar       *script   = NULL;
  GError      *err      = NULL;

  gegl_init (&argc, &argv);

  o = gegl_options_parse (argc, argv);

  if (o->xml)
    {
      script = g_strdup (o->xml);
    }
  else if (o->file)
    {
      if (!strcmp (o->file, "-"))  /* read XML from stdin */
        {
          gint  buf_size = 128;
          gchar buf[buf_size];
          GString *acc = g_string_new ("");

          while (fgets (buf, buf_size, stdin))
            {
              g_string_append (acc, buf);
            }
          script = g_string_free (acc, FALSE);
        }
      else if (file_is_gegl_xml (o->file))
        {
          g_file_get_contents (o->file, &script, NULL, &err);
          if (err != NULL)
            {
              g_warning ("Unable to read file: %s", err->message);
            }
        }
      else
        {
          gchar *leaked_string = g_malloc (strlen (o->file) + 5);
          GString *acc = g_string_new ("");

          g_string_append (acc, "<gegl><load path='");
          g_string_append (acc, o->file);
          g_string_append (acc, "'/></gegl>");

          script = g_string_free (acc, FALSE);

          leaked_string[0]='\0';
          strcat (leaked_string, o->file);
          strcat (leaked_string, ".xml");
          /*o->file = leaked_string;*/
        }
    }
  else
    {
      if (o->rest)
        {
          script = g_strdup ("<gegl></gegl>");
        }
      else
        {
          script = g_strdup (DEFAULT_COMPOSITION);
        }
    }

  gegl = gegl_xml_parse (script);

  if (o->rest)
    {
      GeglNode *proxy;
      GeglNode *iter;

      gchar **operation = o->rest;
      proxy = gegl_graph_output (gegl, "output");
      iter = gegl_node_get_connected_to (proxy, "input");

      while (*operation)
        {
          GeglNode *new = gegl_graph_new_node (gegl, "operation", *operation, NULL);
          if (iter)
            {
              gegl_node_link_many (iter, new, proxy, NULL);
            }
          else
            {
              gegl_node_link_many (new, proxy, NULL);
            }
          iter = new;
          operation++;
        }
    }

  switch (o->mode)
    {
      case GEGL_RUN_MODE_EDITOR:
#if HAVE_GTK
          gtk_init (&argc, &argv);
          editor_main (gegl, o);
#endif
          g_object_unref (gegl);
          g_free (o);
          gegl_exit ();
          return 0;
        break;
      case GEGL_RUN_MODE_XML:
          g_print (gegl_to_xml (gegl));
          gegl_exit ();
          return 0;
        break;
      case GEGL_RUN_MODE_DOT:
          g_print (gegl_to_dot (gegl));
          gegl_exit ();
          return 0;
        break;
      case GEGL_RUN_MODE_PNG:
        {
          GeglNode *output = gegl_graph_new_node (gegl,
                               "operation", "png-save",
                               "path", o->output,
                               NULL);
          gegl_node_connect_from (output, "input", gegl_graph_output (gegl, "output"), "output");
          gegl_node_process (output);

          g_object_unref (gegl);
          g_free (o);
          gegl_exit ();

        }
        break;
      case GEGL_RUN_MODE_HELP:
        break;
      default:
        break;
    }
  return 0;
}
