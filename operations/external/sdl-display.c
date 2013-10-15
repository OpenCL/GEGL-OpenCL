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
#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "sdl-display.c"

#include "gegl-chant.h"
#include <SDL.h>

typedef struct {
  SDL_Surface *screen;
  gint         width;
  gint         height;
} SDLState;

static void
init_sdl (void)
{
  static int inited = 0;

  if (!inited)
    {
      inited = 1;

      if (SDL_Init (SDL_INIT_VIDEO) < 0)
        {
          fprintf (stderr, "Unable to init SDL: %s\n", SDL_GetError ());
          return;
        }
      atexit (SDL_Quit);
      SDL_EnableUNICODE (1);
    }
}

static gboolean idle (gpointer data)
{
  SDL_Event event;
  while (SDL_PollEvent  (&event))
    {
      switch (event.type)
        {
          case SDL_QUIT:
            exit (0);
        }
    }
  return TRUE;
}

static guint handle = 0;

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  SDLState     *state = NULL;

  if(!o->chant_data)
      o->chant_data = g_new0 (SDLState, 1);
  state = o->chant_data;

  init_sdl ();

  if (!handle)
    handle = g_timeout_add (500, idle, NULL);

  if (!state->screen ||
       state->width  != result->width ||
       state->height != result->height)
    {
      state->screen = SDL_SetVideoMode (result->width, result->height, 32, SDL_SWSURFACE);
      if (!state->screen)
        {
          fprintf (stderr, "Unable to set SDL mode: %s\n",
                   SDL_GetError ());
          return -1;
        }

      state->width  = result->width ;
      state->height = result->height;
    }

  /*
   * There seems to be a valid faster path to the SDL desired display format
   * in B'G'R'A, perhaps babl should have been able to figure this out ito?
   *
   */
  gegl_buffer_get (input,
       NULL,
       1.0,
       babl_format_new (babl_model ("R'G'B'A"),
                        babl_type ("u8"),
                        babl_component ("B'"),
                        babl_component ("G'"),
                        babl_component ("R'"),
                        babl_component ("A"),
                        NULL),
       state->screen->pixels, GEGL_AUTO_ROWSTRIDE,
       GEGL_ABYSS_NONE);

  SDL_UpdateRect (state->screen, 0, 0, 0, 0);
  SDL_WM_SetCaption (o->window_title, o->icon_title);

  state->width = result->width ;
  state->height = result->height;

  return  TRUE;
}

static void
finalize (GObject *object)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (object);

  if (o->chant_data)
    {
      g_free (o->chant_data);
      o->chant_data = NULL;
    }

  G_OBJECT_CLASS (gegl_chant_parent_class)->finalize (object);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass           *object_class;
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  object_class->finalize = finalize;

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:sdl-display",
    "categories"  , "display",
    "description" ,
        _("Displays the input buffer in an SDL window (restricted to one"
          " display op/process, due to SDL implementation issues)."),
        NULL);
}
#endif
