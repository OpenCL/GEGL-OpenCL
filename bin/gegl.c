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

#include <glib.h>
#include <gegl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gegl-options.h"
#include "gegl-dot.h"

#ifdef G_OS_WIN32
#define realpath(a,b) _fullpath(b,a,_MAX_PATH)
#endif

#if HAVE_GTK
#include <gtk/gtk.h>
#include "editor.h"
#endif

#define DEFAULT_COMPOSITION \
"<?xml version='1.0' encoding='UTF-8'?> <gegl> <node operation='crop'> <params> <param name='x'>0.000000</param> <param name='y'>0.000000</param> <param name='width'>400.000000</param> <param name='height'>290.000000</param> </params> </node> <node operation='over'> <node operation='shift'> <params> <param name='x'>50.000000</param> <param name='y'>113.000000</param> </params> </node> <node operation='dropshadow'> <params> <param name='opacity'>2.000000</param> <param name='x'>2.000000</param> <param name='y'>2.000000</param> <param name='radius'>3.000000</param> </params> </node> <node operation='text'> <params> <param name='string'>A graph based image processing and\ncompositing engine.</param> <param name='font'>Sans</param> <param name='size'>10.000000</param> <param name='color'>rgb(1.0000, 1.0000, 1.0000)</param> <param name='wrap'>-1</param> <param name='alignment'>0</param> <param name='width'>178</param> <param name='height'>26</param> </params> </node> </node> <node operation='over'> <node operation='shift'> <params> <param name='x'>132.000000</param> <param name='y'>160.000000</param> </params> </node> <node operation='dropshadow'> <params> <param name='opacity'>2.000000</param> <param name='x'>2.000000</param> <param name='y'>2.000000</param> <param name='radius'>3.000000</param> </params> </node> <node operation='text'> <params> <param name='string'>2000-2007 © Calvin Williamson, Caroline Dahloff, Manish Singh, Jay Cox, Daniel Rogers, Sven Neumann, Michael Natterer, Øyvind Kolås, Philip Lafleur, Dominik Ernst, Richard Kralovic, Kevin Cozens, Victor Bogado, Martin Nordholts, Geert Jordaens, Michael Schumacher, John Marshall, Etienne Bersac, Mark Probst, Håkon Hitland, Tor Lillqvist, Garry R. Osgood and Jakub Steiner</param> <param name='font'>Sans</param> <param name='size'>10.458599</param> <param name='color'>rgb(1.0000, 1.0000, 1.0000)</param> <param name='wrap'>240</param> <param name='alignment'>0</param> <param name='width'>237</param> <param name='height'>130</param> </params> </node> </node> <node operation='over'> <node name='dropshadow' operation='dropshadow'> <params> <param name='opacity'>-4.487179</param> <param name='x'>3.750000</param> <param name='y'>2.500000</param> <param name='radius'>10.597015</param> </params> </node> <node operation='shift'> <params> <param name='x'>42.000000</param> <param name='y'>23.000000</param> </params> </node> <node operation='text'> <params> <param name='string'>GEGL</param> <param name='font'>Sans</param> <param name='size'>76.038217</param> <param name='color'>rgb(0.3093, 0.3910, 0.1680)</param> <param name='wrap'>-1</param> <param name='alignment'>0</param> <param name='width'>208</param> <param name='height'>89</param> </params> </node> </node> <node operation='whitebalance'> <params> <param name='high-a-delta'>0.365079</param> <param name='high-b-delta'>0.026511</param> <param name='low-a-delta'>0.000000</param> <param name='low-b-delta'>0.000000</param> <param name='saturation'>0.319548</param> </params> </node> <node operation='brightness-contrast'> <params> <param name='contrast'>0.229008</param> <param name='brightness'>0.000000</param> </params> </node> <node operation='FractalExplorer'> <params> <param name='width'>400</param> <param name='height'>290</param> <param name='fractaltype'>1</param> <param name='xmin'>-1.312500</param> <param name='xmax'>0.000000</param> <param name='ymin'>-0.609375</param> <param name='ymax'>0.562500</param> <param name='iter'>100</param> <param name='cx'>-0.820312</param> <param name='cy'>0.195312</param> <param name='redstretch'>1.000000</param> <param name='greenstretch'>1.000000</param> <param name='bluestretch'>1.000000</param> <param name='redmode'>1</param> <param name='greenmode'>1</param> <param name='bluemode'>0</param> <param name='redinvert'>false</param> <param name='greeninvert'>false</param> <param name='blueinvert'>false</param> <param name='ncolors'>256</param> <param name='useloglog'>false</param> </params> </node> </gegl>"

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
  gchar       *path_root = NULL;

  gegl_init (&argc, &argv);

  o = gegl_options_parse (argc, argv);


  if (o->xml)
    {
      gchar *tmp = g_malloc(PATH_MAX);
      tmp = getcwd (tmp, PATH_MAX);
      path_root = tmp;
    }
  else if (o->file)
    {
      if (!strcmp (o->file, "-"))  /* read XML from stdin */
        {
          gchar *tmp = g_malloc(PATH_MAX);
          tmp = getcwd (tmp, PATH_MAX);
          path_root = tmp;
        }
      else
        {
          gchar *temp1 = g_strdup (o->file);
          gchar *temp2;
          temp2 = g_strdup (g_path_get_dirname (temp1));
          path_root = g_strdup (realpath (temp2, NULL));
          g_free (temp1);
          g_free (temp2);
        }
    }

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

            {gchar *file_basename;
             gchar *tmp;
             tmp=g_strdup (o->file);
             file_basename = g_path_get_basename (tmp);

             g_string_append (acc, "<gegl><load path='");
             g_string_append (acc, file_basename);
             g_string_append (acc, "'/></gegl>");

             g_free (tmp);
            }

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

  gegl = gegl_node_new_from_xml (script, path_root);

  if (o->rest)
    {
      GeglNode *proxy;
      GeglNode *iter;

      gchar **operation = o->rest;
      proxy = gegl_node_get_output_proxy (gegl, "output");
      iter = gegl_node_get_producer (proxy, "input", NULL);

      while (*operation)
        {
          GeglNode *new = gegl_node_new_child (gegl, "operation", *operation, NULL);
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
          return 0;
        break;
      case GEGL_RUN_MODE_XML:
          g_print (gegl_node_to_xml (gegl, path_root));
          return 0;
        break;
      case GEGL_RUN_MODE_DOT:
          g_print (gegl_to_dot (gegl));
          return 0;
        break;
      case GEGL_RUN_MODE_PNG:
        {
          GeglNode *output = gegl_node_new_child (gegl,
                               "operation", "png-save",
                               "path", o->output,
                               NULL);
          gegl_node_connect_from (output, "input", gegl_node_get_output_proxy (gegl, "output"), "output");
          gegl_node_process (output);
          g_object_unref (output);
        }
        break;
      case GEGL_RUN_MODE_HELP:
        break;
      default:
        break;
    }

  g_free (o);
  g_object_unref (gegl);
  g_free (script);
  g_clear_error (&err);
  g_free (path_root);
  gegl_exit ();
  return 0;
}
