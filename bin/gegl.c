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

#if HAVE_GTK
#include <gtk/gtk.h>
#include "editor.h"

#endif


#define DEFAULT_COMPOSITION "<?xml version='1.0' encoding='UTF-8'?> <gegl> <crop x='0.000000' y='-70.000000' width='232.000000' height='377.000000'/> <over> <shift x='24.000000' y='145.000000'/> <dropshadow opacity='2.400000' x='2.000000' y='2.000000' radius='6.000000'/> <text string='This user interface of this application is a work in progress, and it might be the case that quie a few of the buttons and options should have do not push me signs, or WARNINGS about proper use, this is not the case, so expect loss of experimental data.' font='Sans' size='10.000000' color='rgb(1.0000, 1.0000, 1.0000)' wrap='190' alignment='0' width='190' height='91'/> </over> <over> <gaussian-blur radius-x='1.200000' radius-y='1.200000' filter=''/> <shift x='20.000000' y='60.000000'/> <text string='GEGL' font='Sans' size='50.000000' color='rgb(0.7857, 0.0451, 0.0000)' wrap='-1' alignment='0' width='138' height='59'/> </over> <gaussian-blur radius-x='10.0' radius-y='0.0' filter=''/> <perlin-noise/> </gegl>"


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
          o->file = leaked_string;
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
          editor_main (gegl, o->file);
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
      case GEGL_RUN_MODE_PNG:
        {
          GeglNode *output = gegl_graph_new_node (gegl,
                               "operation", "png-save",
                               "path", o->output,
                               NULL);
          gegl_node_connect (output, "input", gegl_graph_output (gegl, "output"), "output");
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
