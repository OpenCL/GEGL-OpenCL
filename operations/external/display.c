/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string  (window_title, _(""), "window_title",
                    _("Title to be given to output window"))
gegl_chant_string  (icon_title, _(""), "icon_title",
                    _("Icon to be used for output window"))

gegl_chant_pointer (screen, "", "private")
gegl_chant_int(w, "", 0, 1000, 0, "private")
gegl_chant_int(h, "", 0, 1000, 0, "private")
gegl_chant_int(width, "", 0, 1000, 0, "private")
gegl_chant_int(height, "", 0, 1000, 0, "private")

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "display.c"

#include "gegl-chant.h"
#include <SDL.h>
#include <signal.h>

#ifdef G_OS_WIN32
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#endif

static void
sighandler (int signal)
{
  switch (signal)
    {                           /* I want my ctrl+C back! :) */
    case SIGINT:
    case SIGTERM:
    case SIGQUIT:
      exit (0);
      break;
    }
}

static void
init_sdl (void)
{
  static int inited = 0;

  if (!inited)
    {
      inited = 1;

      /*signal (SIGINT, sighandler);
      signal (SIGQUIT, sighandler);*/ 

      if (SDL_Init (SDL_INIT_VIDEO) < 0)
        {
          fprintf (stderr, "Unable to init SDL: %s\n", SDL_GetError ());
          return;
        }
      atexit (SDL_Quit);
      SDL_EnableUNICODE (1);
    }
}

/*static int instances = 0;*/

static gboolean idle (gpointer data)
{
  SDL_Event event;
  while (SDL_PollEvent  (&event))
    {
      switch (event.type)
        {
          case SDL_QUIT:
            sighandler (SIGQUIT);
        }
    }
  return TRUE;
}

static guint handle = 0;

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer   *source;
  SDL_Surface **sdl_outwin = NULL;      /*op_sym (op, "sdl_outwin");*/

  init_sdl ();
  if (!handle)
    handle = g_idle_add (idle, NULL);

  if (!o->screen ||
       o->width  != result->width ||
       o->height != result->height)
    {
      if (sdl_outwin)
        {
          if (o->screen)
            {
              SDL_FreeSurface (o->screen);
              o->screen = NULL;
            }

          o->screen = SDL_CreateRGBSurface (SDL_SWSURFACE,
                                            result->width, result->height, 32, 0xff0000,
                                            0x00ff00, 0x0000ff, 0x000000);

          *sdl_outwin = o->screen;
          if (!o->screen)
            {
              fprintf (stderr, "CreateRGBSurface failed: %s\n",
                       SDL_GetError ());
              return -1;
            }
        }
      else
        {
          o->screen = SDL_SetVideoMode (result->width, result->height, 32, SDL_SWSURFACE);
          if (!o->screen)
            {
              fprintf (stderr, "Unable to set SDL mode: %s\n",
                       SDL_GetError ());
              return -1;
            }
        }
      o->width  = result->width ;
      o->height = result->height;
    }

  /*
   * There seems to be a valid faster path to the SDL desired display format
   * in B'G'R'A, perhaps babl should have been able to figure this out ito?
   *
   */
  source = gegl_buffer_create_sub_buffer (input, result);
  gegl_buffer_get (source,
       1.0,
       NULL,
       babl_format_new (babl_model ("R'G'B'A"),
                        babl_type ("u8"),
                        babl_component ("B'"),
                        babl_component ("G'"),
                        babl_component ("R'"),
                        babl_component ("A"),
                        NULL),
       ((SDL_Surface*)o->screen)->pixels, GEGL_AUTO_ROWSTRIDE);
  g_object_unref (source);

  if (!sdl_outwin)
    {
      SDL_UpdateRect (o->screen, 0, 0, 0, 0);
      SDL_WM_SetCaption (o->window_title, o->icon_title);
    }

  o->width = result->width ;
  o->height = result->height;

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  operation_class->name        = "gegl:display";
  operation_class->categories  = "output";
  operation_class->description =
        _("Displays the input buffer in an SDL window (restricted to one"
          " display op/process, due to SDL implementation issues, a gtk+"
          " based replacement would be nice.");
}
#endif
