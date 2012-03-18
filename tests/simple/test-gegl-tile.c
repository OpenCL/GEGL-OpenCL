/*
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
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
 */


#include <gegl.h>
#include <gegl-buffer-backend.h>


#define ADD_TEST(function) g_test_add_func ("/gegl-tile/" #function, function);


static void
unlock_callback (GeglTile *tile,
                 gpointer user_data)
{
  gboolean *callback_called = user_data;
  *callback_called = TRUE;
}

static void
free_callback (gpointer user_data)
{
  gboolean *callback_called = user_data;
  *callback_called = TRUE;
}

/**
 * Tests that gegl_tile_set_unlock_notify() can be used to set a
 * callback that is called in gegl_tile_unlock().
 **/
static void
set_unlock_notify (void)
{
  GeglTile *tile = gegl_tile_new (1);
  gboolean callback_called = FALSE;

  gegl_tile_set_unlock_notify (tile, unlock_callback, &callback_called);
  g_assert (! callback_called);

  gegl_tile_lock (tile);
  g_assert (! callback_called);

  gegl_tile_unlock(tile);
  g_assert (callback_called);
}

/**
 * Tests that gegl_tile_set_data_full() results in a callback when the
 * tile is freed.
 **/
static void
set_data_full (void)
{
  GeglTile *tile = gegl_tile_new (1);
  gboolean callback_called = FALSE;
  guchar data = 42;

  gegl_tile_set_data_full (tile, &data, 1, free_callback, &callback_called);
  g_assert (! callback_called);

  gegl_tile_unref (tile);
  g_assert (callback_called);
}

int
main (int    argc,
      char **argv)
{
  g_type_init ();
  gegl_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (set_unlock_notify);
  ADD_TEST (set_data_full);

  return g_test_run ();
}
