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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gegl-options.h"
#include "gegl-dot.h"

#ifdef G_OS_WIN32
#include <direct.h>
#define getcwd(b,n) _getcwd(b,n)
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#define realpath(a,b) _fullpath(b,a,_MAX_PATH)
#endif

#if HAVE_GTK
#include <gtk/gtk.h>
#include "gegl-bin-gui-types.h"
#include "editor.h"
#endif

#define DEFAULT_COMPOSITION \
"<?xml version='1.0'?> <gegl> <node operation='gegl:crop'> <params> <param name='x'>0</param> <param name='y'>0</param> <param name='width'>512</param> <param name='height'>216</param> </params> </node> <node operation='gegl:over'> <node operation='gegl:shift'> <params> <param name='x'>15</param> <param name='y'>150</param> </params> </node> <gegl:opacity value='0.6'/> <node name='text' operation='gegl:text'> <params> <param name='string'>2000-2008 &#xA9; Calvin Williamson, Caroline Dahloff, Manish Singh, Jay Cox, Daniel Rogers, Sven Neumann, Michael Natterer,  &#xD8;yvind Kol&#xE5;s, Philip Lafleur, Dominik Ernst, Richard Kralovic, Kevin Cozens, Victor Bogado, Martin Nordholts, Geert Jordaens, Michael Schumacher, John Marshall, &#xC9;tienne Bersac, Mark Probst, H&#xE5;kon Hitland, Tor Lillqvist, Hans Breuer, Deji Akingunola, Bradley Broom, Hans Petter Jansson, Jan Heller, dmacks@netspace.org, Sven Anders, Hubert Figui&#xE8;re, Sam Hocevar, yahvuu at gmail.com, Nicolas Robidoux, Garry R. Osgood, Shlomi Fish and Jakub Steiner</param> <param name='font'>Sans</param> <param name='size'>8</param> <param name='color'>#000000</param> <param name='wrap'>485</param> <param name='alignment'>0</param> <param name='width'>355</param> <param name='height'>70</param> </params> </node> </node> <node operation='gegl:over'> <gegl:shift x='80' y='30'/> <gegl:over> <gegl:shift y='0' x='0'/> <gegl:dropshadow radius='8.0' x='0' y='0' opacity='1.2'/> <gegl:fill vector='M 0,50 C 0,78 24,100 50,100 C 77,100 100,78 100,50 C 100,45 99,40 98,35 C 82,35 66,35 50,35 C 42,35 35,42 35,50 C 35,58 42,65 50,65 C 56,65 61,61 64,56 C 67,51 75,55 73,60 C 69,69 60,75 50,75 C 36,75 25,64 25,50 C 25,36 36,25 50,25 L 93,25 C 83,9 67,0 49,0 C 25,0 0,20 0,50 z' color='#ffffffff'/> </gegl:over> <gegl:over> <gegl:shift x='88' y='0'/> <gegl:dropshadow radius='8.0' x='0' y='0' opacity='1.2'/> <gegl:fill vector='M 50,0 C 23,0 0,22 0,50 C 0,77 22,100 50,100 C 68,100 85,90 93,75 L 40,75 C 35,75 35,65 40,65 L 98,65 C 100,55 100,45 98,35 L 40,35 C 35,35 35,25 40,25 L 93,25 C 84,10 68,0 50,0 z' color='#ffffffff'/> </gegl:over> <gegl:over> <gegl:shift x='176' y='0'/> <gegl:dropshadow radius='8.0' x='0' y='0' opacity='1.2'/> <gegl:fill vector='M 0,50 C 0,78 24,100 50,100 C 77,100 100,78 100,50 C 100,45 99,40 98,35 C 82,35 66,35 50,35 C 42,35 35,42 35,50 C 35,58 42,65 50,65 C 56,65 61,61 64,56 C 67,51 75,55 73,60 C 69,69 60,75 50,75 C 36,75 25,64 25,50 C 25,36 36,25 50,25 L 93,25 C 83,9 67,0 49,0 C 25,0 0,20 0,50 z' color='#ffffffff'/> </gegl:over> <gegl:shift x='264' y='0'/> <gegl:dropshadow radius='8.0' x='0' y='0' opacity='1.2'/> <gegl:fill vector='M 30,4 C 12,13 0,30 0,50 C 0,78 23,100 50,100 C 71,100 88,88 96,71 L 56,71 C 42,71 30,59 30,45 L 30,4 z' color='#ffffffff'/> </node> <gegl:color value='white'/> </gegl>"
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

  o = gegl_options_parse (argc, argv);
  gegl_init (&argc, &argv);

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
          gchar real_path[PATH_MAX];
          gchar *temp1 = g_strdup (o->file);
          gchar *temp2 = g_path_get_dirname (temp1);
          path_root = g_strdup (realpath (temp2, real_path));
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
#         define  buf_size 128
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

             g_string_append (acc, "<gegl><gegl:load path='");
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
                               "operation", "gegl:png-save",
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
