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
 * Copyright (C) 2003, 2004, 2006, 2007, 2008 Øyvind Kolås
 */

#include "config.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <gegl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gegl-options.h"
#ifdef HAVE_SPIRO
#include "gegl-path-spiro.h"
#endif
#include "gegl-path-smooth.h"

#ifdef G_OS_WIN32
#include <direct.h>
#define getcwd(b,n) _getcwd(b,n)
#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif
#define realpath(a,b) _fullpath(b,a,_MAX_PATH)
#endif

#define DEFAULT_COMPOSITION \
"<?xml version='1.0' encoding='UTF-8'?> <gegl> <node operation='gegl:crop'> <params> <param name='x'>0</param> <param name='y'>0</param> <param name='width'>690</param> <param name='height'>670</param> </params> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>14</param> <param name='y'>613</param> </params> </node> <node operation='gegl:opacity'> <params> <param name='value'>0.59999999999999998</param> </params> </node> <node name='text' operation='gegl:text'> <params> <param name='string'>2000-2010 © Calvin Williamson, Caroline Dahloff, Manish Singh, Jay Cox Daniel Rogers, Sven Neumann, Michael Natterer, Øyvind Kolås, Philip Lafleur, Dominik Ernst, Richard Kralovic, Kevin Cozens, Victor Bogado, Martin Nordholts, Geert Jordaens, Michael Schumacher, John Marshall, Étienne Bersac, Mark Probst, Håkon Hitland, Tor Lillqvist, Hans Breuer, Deji Akingunola and Bradley Broom, Hans Petter Jansson, Jan Heller, dmacks@netscpace.org, Sven Anders, Hubert Figuière, Sam Hocevar, yahvuu at gmail.com, Nicolas Robidoux, Ruben Vermeersch, Gary V. Vaughan, James Legg, Henrik Åkesson, Fryderyk Dziarmagowski, Ozan Caglayan, Tobias Mueller, Nils Philippsen, Adam Turcotte, Danny Robson, Javier Jardón and Yakkov Selkowitz, Kaja Liiv, Eric Doust, Garry R. Osgood, Øyvind Kolås, Kevin Cozens and Shlomi Fish, Jakub Steiner, and Tonda Tavalec </param> <param name='font'>Sans</param> <param name='size'>8</param> <param name='color'>rgb(0.0000, 0.0000, 0.0000)</param> <param name='wrap'>628</param> <param name='alignment'>0</param> <param name='width'>622</param> <param name='height'>40</param> </params> </node> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>300</param> <param name='y'>500</param> </params> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>0</param> <param name='y'>0</param> </params> </node> <node operation='gegl:dropshadow'> <params> <param name='opacity'>1.2</param> <param name='x'>0</param> <param name='y'>0</param> <param name='radius'>8</param> </params> </node> <gegl:path d='M0,50 C0,78 24,100 50,100 C77,100 100,78 100,50 C100,45 99,40 98,35 C82,35 66,35 50,35 C42,35 35,42 35,50 C35,58 42,65 50,65 C56,65 61,61 64,56 C67,51 75,55 73,60 C69,69 60,75 50,75 C36,75 25,64 25,50 C25,36 36,25 50,25 L93,25 C83,9 67,0 49,0 C25,0 0,20 0,50 z' fill='white'/> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>88</param> <param name='y'>0</param> </params> </node> <node operation='gegl:dropshadow'> <params> <param name='opacity'>1.2</param> <param name='x'>0</param> <param name='y'>0</param> <param name='radius'>8</param> </params> </node> <node operation='gegl:path'> <params> <param name='d'>M50,0 C23,0 0,22 0,50 C0,77 22,100 50,100 C68,100 85,90 93,75 L40,75 C35,75 35,65 40,65 L98,65 C100,55 100,45 98,35 L40,35 C35,35 35,25 40,25 L93,25 C84,10 68,0 50,0 z</param> <param name='fill'>rgb(1.0000, 1.0000, 1.0000)</param> </params> </node> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>176</param> <param name='y'>0</param> </params> </node> <node operation='gegl:dropshadow'> <params> <param name='opacity'>1.2</param> <param name='x'>0</param> <param name='y'>0</param> <param name='radius'>8</param> </params> </node> <node operation='gegl:path'> <params> <param name='d'>M0,50 C0,78 24,100 50,100 C77,100 100,78 100,50 C100,45 99,40 98,35 C82,35 66,35 50,35 C42,35 35,42 35,50 C35,58 42,65 50,65 C56,65 61,61 64,56 C67,51 75,55 73,60 C69,69 60,75 50,75 C36,75 25,64 25,50 C25,36 36,25 50,25 L93,25 C83,9 67,0 49,0 C25,0 0,20 0,50 z</param> <param name='fill'>rgb(1.0000, 1.0000, 1.0000)</param> </params> </node> </node> <node operation='gegl:translate'> <params> <param name='x'>264</param> <param name='y'>0</param> </params> </node> <node operation='gegl:dropshadow'> <params> <param name='opacity'>1.2</param> <param name='x'>0</param> <param name='y'>0</param> <param name='radius'>8</param> </params> </node> <node operation='gegl:path'> <params> <param name='d'>M30,4 C12,13 0,30 0,50 C0,78 23,100 50,100 C71,100 88,88 96,71 L56,71 C42,71 30,59 30,45 L30,4 z</param> <param name='fill'>rgb(1.0000, 1.0000, 1.0000)</param> </params> </node> </node> <node operation='gegl:rotate'> <params> <param name='origin-x'>0</param> <param name='origin-y'>0</param> <param name='filter'>linear</param> <param name='hard-edges'>false</param> <param name='lanczos-width'>3</param> <param name='degrees'>42</param> </params> </node> <node operation='gegl:checkerboard'> <params> <param name='x'>43</param> <param name='y'>44</param> <param name='x-offset'>0</param> <param name='y-offset'>0</param> <param name='color1'>rgb(0.7097, 0.7097, 0.7097)</param> <param name='color2'>rgb(0.7661, 0.7661, 0.7661)</param> </params> </node> </gegl>"

static void
gegl_enable_fatal_warnings (void)
{
  GLogLevelFlags fatal_mask;

  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;

  g_log_set_always_fatal (fatal_mask);
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
      !strcmp (extension, "XML")||
      !strcmp (extension, "svg")
      )
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

  if (o->fatal_warnings)
    {
      gegl_enable_fatal_warnings ();
    }

  g_thread_init (NULL);
  gegl_init (&argc, &argv);
#ifdef HAVE_SPIRO
  gegl_path_spiro_init ();
#endif
  gegl_path_smooth_init ();

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

	  {
	    gchar *file_basename;
	    gchar *tmp;
	    tmp = g_strdup (o->file);
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
          GeglNode *new;

	  new = gegl_node_new_child (gegl, "operation", *operation, NULL);
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
        {
          GeglNode *output = gegl_node_new_child (gegl,
						  "operation", "gegl:display",
						  "window-title", o->file,
						  NULL);
          gegl_node_connect_from (output, "input", gegl_node_get_output_proxy (gegl, "output"), "output");
          gegl_node_process (output);
          g_object_unref (output);
          g_main_loop_run (g_main_loop_new (NULL, TRUE));
        }
        break;
      case GEGL_RUN_MODE_XML:
	g_printf ("%s\n", gegl_node_to_xml (gegl, path_root));
	return 0;
        break;
#if 0
      case GEGL_RUN_MODE_DOT:
	g_print ("%s\n", gegl_to_dot (gegl));
	return 0;
        break;
#endif
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
      case GEGL_RUN_MODE_PPM:
        {
          GeglNode *output = gegl_node_new_child (gegl,
						  "operation", "gegl:ppm-save",
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
