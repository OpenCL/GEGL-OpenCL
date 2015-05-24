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
 * Copyright (C) 2013 Daniel Sabo
 */
#include "gegl.h"
#include "gegl-buffer-backend.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gboolean
test_buffer_copy (void)
{
  gboolean result = TRUE;

  GeglBuffer *bufferA, *bufferB;
  const Babl *format = babl_format ("Y u8");
  const gint bpp = babl_format_get_bytes_per_pixel (format);
  GeglColor *white = gegl_color_new("white");
  GeglColor *black = gegl_color_new("black");
  GeglRectangle full_extent = {0, 0, 0, 0};
  GeglRectangle scaled_extent = {0, 0, 0, 0};

  int i, j;

  gint output_buffer_size;

  guchar *output_buffer_a, *output_buffer_b;

  bufferA = gegl_buffer_new (NULL, format);
  bufferB = gegl_buffer_new (NULL, format);

  g_object_get (bufferA,
                "tile-width", &full_extent.width,
                "tile-height", &full_extent.height,
                NULL);

  full_extent.width *= 2;
  full_extent.height *= 2;

  scaled_extent.width = full_extent.width / 4;
  scaled_extent.height = full_extent.height / 4;

  output_buffer_size = scaled_extent.width * scaled_extent.height * bpp;

  output_buffer_a = gegl_malloc (output_buffer_size);
  output_buffer_b   = gegl_malloc (output_buffer_size);

  gegl_buffer_set_extent (bufferA, &full_extent);
  gegl_buffer_set_extent (bufferB, &full_extent);

  gegl_buffer_set_color (bufferA, NULL, white);
  gegl_buffer_set_color (bufferB, NULL, black);

  /* Assert that the buffers differ before the copy, and cause the mipmaps to generate */
  gegl_buffer_get (bufferA,
                   &scaled_extent,
                   0.25,
                   format,
                   output_buffer_a,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  gegl_buffer_get (bufferB,
                   &scaled_extent,
                   0.25,
                   format,
                   output_buffer_b,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  if (0 == memcmp (output_buffer_a, output_buffer_b, output_buffer_size))
    {
      printf ("%s: buffers are already identical!\n", G_STRFUNC);
      result = FALSE;
    }

  /* Copy-on-write copy from B to A */
  gegl_buffer_copy (bufferA, NULL, GEGL_ABYSS_NONE,
                    bufferB, NULL);

  /* Assert that the copy-on-write actually happened */
  for (i = 0; i < 2; ++i)
    for (j = 0; j < 2; ++j)
      {
        GeglTile *tile_a = gegl_tile_source_get_tile ((GeglTileSource *) bufferA, i, j, 0);
        GeglTile *tile_b = gegl_tile_source_get_tile ((GeglTileSource *) bufferB, i, j, 0);

        if (gegl_tile_get_data (tile_a) != gegl_tile_get_data (tile_b))
          {
            printf ("%s: tile %d, %d not coppied!\n", G_STRFUNC, i, j);
            result = FALSE;
          }

        gegl_tile_unref (tile_a);
        gegl_tile_unref (tile_b);
      }

  /* Assert the buffers are the same after the copy */
  gegl_buffer_get (bufferA,
                   &scaled_extent,
                   0.25,
                   format,
                   output_buffer_a,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  gegl_buffer_get (bufferB,
                   &scaled_extent,
                   0.25,
                   format,
                   output_buffer_b,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  if (0 != memcmp (output_buffer_a, output_buffer_b, output_buffer_size))
    {
      printf ("%s: buffers don't match!\n", G_STRFUNC);
      result = FALSE;
    }

  gegl_free (output_buffer_a);
  gegl_free (output_buffer_b);

  g_object_unref (bufferA);
  g_object_unref (bufferB);
  g_object_unref (white);
  g_object_unref (black);

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

  RUN_TEST (test_buffer_copy)

  gegl_exit();

  if (tests_passed == tests_run)
    return 0;
  return -1;

  return 0;
}
