/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES

gegl_chant_string  (window_title, "window_title",
                    "Title to be given output window")
gegl_chant_string  (icon_title, "icon_title",
                    "Icon to be used for output window")
gegl_chant_pointer (screen, "private")
gegl_chant_int(w, 0, 1000, 0, "private")
gegl_chant_int(h, 0, 1000, 0, "private")
gegl_chant_int(width, 0, 1000, 0, "private")
gegl_chant_int(height, 0, 1000, 0, "private")

#else

#define GEGL_CHANT_SINK
#define GEGL_CHANT_NAME        display
#define GEGL_CHANT_DESCRIPTION "Displays the input buffer in an SDL window (restricted to one display op/process, due to SDL implementation issues, a gtk+ based replacement would be nice."
#define GEGL_CHANT_SELF        "display.c"
#define GEGL_CHANT_CATEGORIES  "output"
#include "gegl-chant.h"

#include <SDL.h>
#include <signal.h>

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

      signal (SIGINT, sighandler);
      signal (SIGQUIT, sighandler);

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


static gboolean
process (GeglOperation *operation,
         gpointer       dynamic_id)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer         *input;
  GeglBuffer         *source;
  GeglRectangle      *result  = gegl_operation_result_rect (operation, dynamic_id);
  SDL_Surface       **sdl_outwin = NULL;      //op_sym (op, "sdl_outwin");

  input = GEGL_BUFFER (gegl_operation_get_data (operation, dynamic_id, "input"));

  g_assert (input);

  init_sdl ();

  if (!self->screen ||
       self->w != result->w||
       self->h != result->h)
    {
      if (sdl_outwin)
        {
          if (self->screen)
            {
              SDL_FreeSurface (self->screen);
              self->screen = NULL;
            }

          self->screen = SDL_CreateRGBSurface (SDL_SWSURFACE,
                                            result->w, result->h, 32, 0xff0000,
                                            0x00ff00, 0x0000ff, 0x000000);

          *sdl_outwin = self->screen;
          if (!self->screen)
            {
              fprintf (stderr, "CreateRGBSurface failed: %s\n",
                       SDL_GetError ());
              return -1;
            }
        }
      else
        {
          self->screen = SDL_SetVideoMode (result->w, result->h, 32, SDL_SWSURFACE);
          if (!self->screen)
            {
              fprintf (stderr, "Unable to set SDL mode: %s\n",
                       SDL_GetError ());
              return -1;
            }
        }
      self->w = result->w;
      self->h = result->h;
    }

  /*
   * There seems to be a valid faster path to the SDL desired display format
   * in B'G'R'A, perhaps babl should have been able to figure this out itself?
   *
   */
  source = g_object_new (GEGL_TYPE_BUFFER,
                         "source", input,
                         "x",      result->x,
                         "y",      result->y,
                         "width",  result->w,
                         "height", result->h,
                         NULL);
  gegl_buffer_get (source, NULL, ((SDL_Surface*)self->screen)->pixels,
       babl_format_new (babl_model ("R'G'B'A"),
                        babl_type ("u8"),
                        babl_component ("B'"),
                        babl_component ("G'"),
                        babl_component ("R'"),
                        babl_component ("A"),
                        NULL),
       1.0);
  g_object_unref (source);

  if (!sdl_outwin)
    {
      SDL_UpdateRect (self->screen, 0, 0, 0, 0);
      SDL_WM_SetCaption (self->window_title, self->icon_title);
    }

  self->width = result->w;
  self->height = result->h;

  return  TRUE;
}

#endif
