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
 * Copyright (C) 2014 Daniel Sabo
 */
#include "gegl.h"
#include "gegl-buffer-backend.h"
#include "gegl-tile-backend-file.h"

#include <glib/gstdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gboolean
test_buffer_path (void)
{
  gboolean         result = TRUE;
  gchar           *tmpdir = NULL;
  gchar           *buf_a_path = NULL;
  gchar           *buf_a_real_path = NULL;
  GeglBuffer      *buf_a = NULL;
  GeglTileBackend *backend_a = NULL;
  GeglRectangle    roi = {0, 0, 128, 128};

  tmpdir = g_dir_make_tmp ("test-backend-file-XXXXXX", NULL);
  g_return_val_if_fail (tmpdir, FALSE);

  buf_a_path = g_build_filename (tmpdir, "buf_a.gegl", NULL);

  buf_a = g_object_new (GEGL_TYPE_BUFFER,
                        "format", babl_format ("R'G'B'A u8"),
                        "path", buf_a_path,
                        "x", roi.x,
                        "y", roi.y,
                        "width", roi.width,
                        "height", roi.height,
                        NULL);

  g_object_get (buf_a,
                "path", &buf_a_real_path,
                "backend", &backend_a,
                NULL);

  if (!GEGL_IS_TILE_BACKEND_FILE (backend_a))
    {
      printf ("Buffer did not use the file backend.\n");
      result = FALSE;
    }

  if (g_strcmp0 (buf_a_path, buf_a_real_path))
    {
      printf ("Paths do not match:\n%s\n%s\n",
              buf_a_path,
              buf_a_real_path);
      result = FALSE;
    }

  g_object_unref (buf_a);
  g_object_unref (backend_a);
  g_free (buf_a_real_path);

  g_unlink (buf_a_path);
  g_remove (tmpdir);

  g_free (tmpdir);
  g_free (buf_a_path);

  return result;
}

static gboolean
test_buffer_path_from_backend (void)
{
  gboolean         result = TRUE;
  gchar           *tmpdir = NULL;
  gchar           *buf_a_path = NULL;
  gchar           *buf_a_real_path = NULL;
  GeglBuffer      *buf_a = NULL;
  GeglTileBackend *backend_a = NULL;
  GeglRectangle    roi = {0, 0, 128, 128};

  tmpdir = g_dir_make_tmp ("test-backend-file-XXXXXX", NULL);
  g_return_val_if_fail (tmpdir, FALSE);

  buf_a_path = g_build_filename (tmpdir, "buf_a.gegl", NULL);

  backend_a = g_object_new (GEGL_TYPE_TILE_BACKEND_FILE,
                            "tile-width",  128,
                            "tile-height", 64,
                            "format",      babl_format ("R'G'B'A u8"),
                            "path",        buf_a_path,
                            NULL);

  buf_a = g_object_new (GEGL_TYPE_BUFFER,
                        "backend", backend_a,
                        "x", roi.x,
                        "y", roi.y,
                        "width", roi.width,
                        "height", roi.height,
                        NULL);

  g_object_unref (backend_a);
  backend_a = NULL;

  g_object_get (buf_a,
                "path", &buf_a_real_path,
                "backend", &backend_a,
                NULL);

  if (!GEGL_IS_TILE_BACKEND_FILE (backend_a))
    {
      printf ("Buffer did not use the file backend.\n");
      result = FALSE;
    }

  if (g_strcmp0 (buf_a_path, buf_a_real_path))
    {
      printf ("Paths do not match:\n%s\n%s\n",
              buf_a_path,
              buf_a_real_path);
      result = FALSE;
    }

  g_object_unref (buf_a);
  g_free (buf_a_real_path);

  g_unlink (buf_a_path);
  g_remove (tmpdir);

  g_free (tmpdir);
  g_free (buf_a_path);

  return result;
}

#define RUN_TEST(test_name) \
{ \
  if (test_name()) \
    { \
      printf ("" #test_name " ... PASS\n"); \
      tests_passed++; \
    } \
  else \
    { \
      printf ("" #test_name " ... FAIL\n"); \
      tests_failed++; \
    } \
  tests_run++; \
}

int main(int argc, char **argv)
{
  gint tests_run    = 0;
  gint tests_passed = 0;
  gint tests_failed = 0;

  gegl_init(0, NULL);
  g_object_set(G_OBJECT(gegl_config()),
               "swap", "RAM",
               "use-opencl", FALSE,
               NULL);

  RUN_TEST (test_buffer_path)
  RUN_TEST (test_buffer_path_from_backend)

  gegl_exit();

  if (tests_passed == tests_run)
    return 0;
  return -1;

  return 0;
}
