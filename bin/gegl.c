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
 * Copyright (C) 2003, 2004, 2006, 2007, 2008, 2016 Øyvind Kolås
 */

#include "config.h"

#include <glib.h>
#include <glib/gprintf.h>
#include <glib/gi18n-lib.h>
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
#include "operation/gegl-extension-handler.h"

#ifdef G_OS_WIN32
#include <direct.h>
#define getcwd(b,n) _getcwd(b,n)
#define realpath(a,b) _fullpath(b,a,_MAX_PATH)
#endif

#define DEFAULT_COMPOSITION \
"<?xml version='1.0' encoding='UTF-8'?> <gegl> <node operation='gegl:crop'> <params> <param name='x'>0</param> <param name='y'>0</param> <param name='width'>395</param> <param name='height'>200</param> </params> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>80</param> <param name='y'>162</param> </params> </node> <node operation='gegl:opacity'> <params> <param name='value'>0.5</param> </params> </node> <node name='text' operation='gegl:text'> <params> <param name='string'>2000-2011 © Various contributors</param> <param name='font'>Sans</param> <param name='size'>12</param> <param name='color'>rgb(0.0000, 0.0000, 0.0000)</param> <param name='wrap'>628</param> <param name='alignment'>0</param> <param name='width'>622</param> <param name='height'>40</param> </params> </node> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>20</param> <param name='y'>50</param> </params> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>0</param> <param name='y'>0</param> </params> </node> <node operation='gegl:dropshadow'> <params> <param name='opacity'>1.2</param> <param name='x'>0</param> <param name='y'>0</param> <param name='radius'>8</param> </params> </node> <gegl:fill-path d='M0,50 C0,78 24,100 50,100 C77,100 100,78 100,50 C100,45 99,40 98,35 C82,35 66,35 50,35 C42,35 35,42 35,50 C35,58 42,65 50,65 C56,65 61,61 64,56 C67,51 75,55 73,60 C69,69 60,75 50,75 C36,75 25,64 25,50 C25,36 36,25 50,25 L93,25 C83,9 67,0 49,0 C25,0 0,20 0,50 z' color='white'/> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>88</param> <param name='y'>0</param> </params> </node> <node operation='gegl:dropshadow'> <params> <param name='opacity'>1.2</param> <param name='x'>0</param> <param name='y'>0</param> <param name='radius'>8</param> </params> </node> <node operation='gegl:fill-path'> <params> <param name='d'>M50,0 C23,0 0,22 0,50 C0,77 22,100 50,100 C68,100 85,90 93,75 L40,75 C35,75 35,65 40,65 L98,65 C100,55 100,45 98,35 L40,35 C35,35 35,25 40,25 L93,25 C84,10 68,0 50,0 z</param> <param name='color'>rgb(1.0000, 1.0000, 1.0000)</param> </params> </node> </node> <node operation='gegl:over'> <node operation='gegl:translate'> <params> <param name='x'>176</param> <param name='y'>0</param> </params> </node> <node operation='gegl:dropshadow'> <params> <param name='opacity'>1.2</param> <param name='x'>0</param> <param name='y'>0</param> <param name='radius'>8</param> </params> </node> <node operation='gegl:fill-path'> <params> <param name='d'>M0,50 C0,78 24,100 50,100 C77,100 100,78 100,50 C100,45 99,40 98,35 C82,35 66,35 50,35 C42,35 35,42 35,50 C35,58 42,65 50,65 C56,65 61,61 64,56 C67,51 75,55 73,60 C69,69 60,75 50,75 C36,75 25,64 25,50 C25,36 36,25 50,25 L93,25 C83,9 67,0 49,0 C25,0 0,20 0,50 z</param> <param name='color'>rgb(1.0000, 1.0000, 1.0000)</param> </params> </node> </node> <node operation='gegl:translate'> <params> <param name='x'>264</param> <param name='y'>0</param> </params> </node> <node operation='gegl:dropshadow'> <params> <param name='opacity'>1.2</param> <param name='x'>0</param> <param name='y'>0</param> <param name='radius'>8</param> </params> </node> <node operation='gegl:fill-path'> <params> <param name='d'>M30,4 C12,13 0,30 0,50 C0,78 23,100 50,100 C71,100 88,88 96,71 L56,71 C42,71 30,59 30,45 L30,4 z</param> <param name='color'>rgb(1.0000, 1.0000, 1.0000)</param> </params> </node> </node> <node operation='gegl:rotate'> <params> <param name='origin-x'>0</param> <param name='origin-y'>0</param> <param name='sampler'>linear</param>  <param name='degrees'>42</param> </params> </node> <node operation='gegl:checkerboard'> <params> <param name='x'>43</param> <param name='y'>44</param> <param name='x-offset'>0</param> <param name='y-offset'>0</param> <param name='color1'>rgb(0.7097, 0.7097, 0.7097)</param> <param name='color2'>rgb(0.7661, 0.7661, 0.7661)</param> </params> </node> </gegl>"

#define STDIN_BUF_SIZE 128

static void
gegl_enable_fatal_warnings (void)
{
  GLogLevelFlags fatal_mask;

  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;

  g_log_set_always_fatal (fatal_mask);
}

int gegl_str_has_image_suffix (char *path);
int gegl_str_has_video_suffix (char *path);

static gboolean file_is_gegl_composition (const gchar *path)
{
  gchar *extension;

  extension = strrchr (path, '.');
  if (!extension)
    return FALSE;
  extension++;
  if (extension[0]=='\0')
    return FALSE;
  if (!strcmp (extension, "xml")||
      !strcmp (extension, "gegl")||
      !strcmp (extension, "XML")||
      !strcmp (extension, "svg")
      )
    return TRUE;
  return FALSE;
}

static gboolean is_xml_fragment (const char *data)
{
  int i;
  for (i = 0; data && data[i]; i++)
    switch (data[i])
    {
      case ' ':case '\t':case '\n':case '\r': break;
      case '<': return TRUE;
      default: return FALSE;
    }
  return FALSE;
}

int mrg_ui_main (int argc, char **argv, char **ops);

gint
main (gint    argc,
      gchar **argv)
{
  GeglOptions *o         = NULL;
  GeglNode    *gegl      = NULL;
  gchar       *script    = NULL;
  GError      *err       = NULL;
  gchar       *path_root = NULL;

#if HAVE_MRG
  g_setenv ("GEGL_MIPMAP_RENDERING", "1", TRUE);
#endif

  g_object_set (gegl_config (),
                "application-license", "GPL3",
#if HAVE_MRG
                "use-opencl", FALSE,
#endif
                NULL);

  o = gegl_options_parse (argc, argv);
  gegl_init (NULL, NULL);
#ifdef HAVE_SPIRO
  gegl_path_spiro_init ();
#endif
  gegl_path_smooth_init ();


  if (o->fatal_warnings)
    {
      gegl_enable_fatal_warnings ();
    }

  if (o->xml)
    {
      path_root = g_get_current_dir ();
    }
  else if (o->file)
    {
      if (!strcmp (o->file, "-"))  /* read XML from stdin */
        {
          path_root = g_get_current_dir ();
        }
      else
        {
          gchar *tmp = g_path_get_dirname (o->file);
          gchar *tmp2 = realpath (tmp, NULL);
          path_root = g_strdup (tmp2);
          g_free (tmp);
          free (tmp2); /* don't use g_free - realpath isn't glib */
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
          gchar buf[STDIN_BUF_SIZE];
          GString *acc = g_string_new ("");

          while (fgets (buf, STDIN_BUF_SIZE, stdin))
            {
              g_string_append (acc, buf);
            }
          script = g_string_free (acc, FALSE);
        }
      else if (file_is_gegl_composition (o->file))
        {
          g_file_get_contents (o->file, &script, NULL, &err);
          if (err != NULL)
            {
              g_warning (_("Unable to read file: %s"), err->message);
            }
        }
      else
        {
          gchar *file_basename = g_path_get_basename (o->file);

          if (gegl_str_has_video_suffix (file_basename))
            script = g_strconcat ("<gegl><gegl:ff-load path='",
                                  file_basename,
                                  "'/></gegl>",
                                  NULL);
          else
          script = g_strconcat ("<gegl><gegl:load path='",
                                file_basename,
                                "'/></gegl>",
                                NULL);

          g_free (file_basename);
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

  if (o->mode == GEGL_RUN_MODE_DISPLAY)
    {
#if HAVE_MRG
      mrg_ui_main (argc, argv, o->rest);
      return 0;
#endif
    }

  if (is_xml_fragment (script))
    gegl = gegl_node_new_from_xml (script, path_root);
  else
    gegl = gegl_node_new_from_serialized (script, path_root);

  if (!gegl)
    {
      g_print (_("Invalid graph, abort.\n"));
      return 1;
    }

  {
  GeglNode *proxy = gegl_node_get_output_proxy (gegl, "output");
  GeglNode *iter = gegl_node_get_producer (proxy, "input", NULL);
  if (o->rest)
    {
      GError *error = NULL;
      gegl_create_chain_argv (o->rest, iter, proxy, 0, gegl_node_get_bounding_box (gegl).height, path_root, &error);
      if (error)
      {
        fprintf (stderr, "Error: %s\n", error->message);
      }
      if (o->serialize)
      {
        fprintf (stderr, "%s\n", gegl_serialize (iter,
            gegl_node_get_producer (proxy, "input", NULL), "/",
            GEGL_SERIALIZE_VERSION|GEGL_SERIALIZE_INDENT));
      }
    }
  }

  switch (o->mode)
    {
      case GEGL_RUN_MODE_DISPLAY:
        {
          GeglNode *output = gegl_node_new_child (gegl,
                                                  "operation", "gegl:display",
                                                  o->file ? "window-title" : NULL, o->file,
                                                  NULL);
          gegl_node_connect_from (output, "input", gegl_node_get_output_proxy (gegl, "output"), "output");
          gegl_node_process (output);
          g_main_loop_run (g_main_loop_new (NULL, TRUE));
          g_object_unref (output);
        }
        break;
      case GEGL_RUN_MODE_XML:
        g_printf ("%s\n", gegl_node_to_xml (gegl, path_root));
        return 0;
        break;

      case GEGL_RUN_MODE_OUTPUT:
      if (gegl_str_has_video_suffix ((void*)o->output))
        {
          GeglNode *output = gegl_node_new_child (gegl,
                                                  "operation", "gegl:ff-save",
                                                  "path", o->output,
                                                  "video-bit-rate", 4000,
                                                  NULL);
          {
            GeglRectangle bounds = gegl_node_get_bounding_box (gegl);
            GeglBuffer *tempb;
            GeglNode *n0;
            GeglNode *iter;
            GeglAudioFragment *audio = NULL;
            int frame_no = 0;
            guchar *temp;

            bounds.x *= o->scale;
            bounds.y *= o->scale;
            bounds.width *= o->scale;
            bounds.height *= o->scale;
            temp = gegl_malloc (bounds.width * bounds.height * 4);
            tempb = gegl_buffer_new (&bounds, babl_format("R'G'B'A u8"));

            n0 = gegl_node_new_child (gegl, "operation", "gegl:buffer-source",
                                            "buffer", tempb,
                                            NULL);
            gegl_node_connect_from (output, "input", n0, "output");

            iter = gegl_node_get_output_proxy (gegl, "output");

            while (gegl_node_get_producer (iter, "input", NULL))
              iter = (gegl_node_get_producer (iter, "input", NULL));
            {
              int duration = 0;
              gegl_node_get (iter, "frames", &duration, NULL);

              while (frame_no < duration)
              {
                gegl_node_blit (gegl, o->scale, &bounds,
                                babl_format("R'G'B'A u8"), temp,
                                GEGL_AUTO_ROWSTRIDE,
                                GEGL_BLIT_DEFAULT);

                gegl_buffer_set (tempb, &bounds, 0.0, babl_format ("R'G'B'A u8"),
                                 temp, GEGL_AUTO_ROWSTRIDE);

                gegl_node_get (iter, "audio", &audio, NULL);
                if (audio)
                  gegl_node_set (output, "audio", audio, NULL);
                fprintf (stderr, "\r%i/%i %p", frame_no, duration-1, audio);

                gegl_node_process (output);

                frame_no ++;
                gegl_node_set (iter, "frame", frame_no, NULL);
              }
              fprintf (stderr, "\n");
            }
            gegl_free (temp);
            g_object_unref (tempb);
            g_object_unref (output);
          }

        }
      else
        {
          GeglNode *output = gegl_node_new_child (gegl,
                                                  "operation", "gegl:save",
                                                  "path", o->output,
                                                  NULL);

          if (o->scale != 1.0){
            GeglRectangle bounds = gegl_node_get_bounding_box (gegl);
            GeglBuffer *tempb;
            GeglNode *n0;

            guchar *temp;

            bounds.x *= o->scale;
            bounds.y *= o->scale;
            bounds.width *= o->scale;
            bounds.height *= o->scale;
            temp = gegl_malloc (bounds.width * bounds.height * 4);
            tempb = gegl_buffer_new (&bounds, babl_format("R'G'B'A u8"));
            gegl_node_blit (gegl, o->scale, &bounds, babl_format("R'G'B'A u8"), temp, GEGL_AUTO_ROWSTRIDE,
                            GEGL_BLIT_DEFAULT);

            gegl_buffer_set (tempb, &bounds, 0.0, babl_format ("R'G'B'A u8"),
                             temp, GEGL_AUTO_ROWSTRIDE);

            n0 = gegl_node_new_child (gegl, "operation", "gegl:buffer-source",
                                            "buffer", tempb,
                                            NULL);
            gegl_node_connect_from (output, "input", n0, "output");
            gegl_node_process (output);
            gegl_free (temp);
            g_object_unref (tempb);
          }
          else
          {
            gegl_node_connect_from (output, "input", gegl, "output");
            gegl_node_process (output);
          }

          g_object_unref (output);
        }
        break;

      case GEGL_RUN_MODE_HELP:
        break;

      default:
        g_warning (_("Unknown GeglOption mode: %d"), o->mode);
        break;
    }

  g_list_free_full (o->files, g_free);
  g_free (o);
  g_object_unref (gegl);
  g_free (script);
  g_clear_error (&err);
  g_free (path_root);
  gegl_exit ();
  return 0;
}

int gegl_str_has_image_suffix (char *path)
{
  return g_str_has_suffix (path, ".jpg") ||
         g_str_has_suffix (path, ".png") ||
         g_str_has_suffix (path, ".JPG") ||
         g_str_has_suffix (path, ".PNG") ||
         g_str_has_suffix (path, ".tif") ||
         g_str_has_suffix (path, ".tiff") ||
         g_str_has_suffix (path, ".TIF") ||
         g_str_has_suffix (path, ".TIFF") ||
         g_str_has_suffix (path, ".jpeg") ||
         g_str_has_suffix (path, ".JPEG") ||
         g_str_has_suffix (path, ".CR2") ||
         g_str_has_suffix (path, ".cr2") ||
         g_str_has_suffix (path, ".exr");
}

int gegl_str_has_video_suffix (char *path)
{
  return g_str_has_suffix (path, ".avi") ||
         g_str_has_suffix (path, ".AVI") ||
         g_str_has_suffix (path, ".mp4") ||
         g_str_has_suffix (path, ".dv") ||
         g_str_has_suffix (path, ".DV") ||
         g_str_has_suffix (path, ".mp3") ||
         g_str_has_suffix (path, ".MP3") ||
         g_str_has_suffix (path, ".mpg") ||
         g_str_has_suffix (path, ".ogv") ||
         g_str_has_suffix (path, ".MPG") ||
         g_str_has_suffix (path, ".webm") ||
         g_str_has_suffix (path, ".MP4") ||
         g_str_has_suffix (path, ".mkv") ||
         g_str_has_suffix (path, ".gif") ||
         g_str_has_suffix (path, ".GIF") ||
         g_str_has_suffix (path, ".MKV") ||
         g_str_has_suffix (path, ".mov") ||
         g_str_has_suffix (path, ".ogg");
}

