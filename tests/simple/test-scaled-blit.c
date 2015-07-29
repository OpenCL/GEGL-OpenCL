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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static gboolean
test_scale (const gdouble scale, const gint x, const gint y, const Babl *format)
{
  GeglNode *checkerboard;
  GeglBuffer *tmp_buffer;
  gboolean result = FALSE;

  const gint bpp = babl_format_get_bytes_per_pixel (format);
  const gint scaled_width  = 32;
  const gint scaled_height = 32;
  gint pad = 32;

  guchar *output_buffer_scaled = gegl_malloc (scaled_width * scaled_height * bpp);
  guchar *output_node_scaled   = gegl_malloc (scaled_width * scaled_height * bpp);

  if (2 / scale > pad)
    pad = 2 / scale + 2;

  tmp_buffer = gegl_buffer_new (GEGL_RECTANGLE ((x / scale) - pad,
                                                (y / scale) - pad,
                                                (scaled_width / scale) + (2 * pad),
                                                (scaled_height / scale) + (2 * pad)),
                                babl_format ("RGBA float"));

  checkerboard = gegl_node_new_child(NULL,
                                     "operation", "gegl:checkerboard",
                                     "x", 16,
                                     "y", 16,
                                     NULL);

  gegl_node_blit_buffer (checkerboard,
                         tmp_buffer,
                         NULL,
                         0,
                         GEGL_ABYSS_NONE);

  gegl_buffer_get (tmp_buffer,
                   GEGL_RECTANGLE (x, y, scaled_width, scaled_height),
                   scale,
                   format,
                   output_buffer_scaled,
                   GEGL_AUTO_ROWSTRIDE,
                   GEGL_ABYSS_NONE);

  g_object_unref (checkerboard);
  g_object_unref (tmp_buffer);

  /* Re-create the node so we don't hit its cache */
  checkerboard = gegl_node_new_child(NULL,
                                     "operation", "gegl:checkerboard",
                                     "x", 16,
                                     "y", 16,
                                     NULL);

  gegl_node_blit (checkerboard,
                  scale,
                  GEGL_RECTANGLE (x, y, scaled_width, scaled_height),
                  format,
                  output_node_scaled,
                  GEGL_AUTO_ROWSTRIDE,
                  0);

  g_object_unref (checkerboard);

  if (0 == memcmp (output_buffer_scaled, output_node_scaled, scaled_width * scaled_height * bpp))
    {
      printf (".");
      fflush(stdout);
      result = TRUE;
    }
  else
    {
      printf ("\n scale=%.4f at %d, %d in \"%s\" ... FAIL\n", scale, x, y, babl_get_name (format));
      result = FALSE;
    }

  gegl_free (output_buffer_scaled);
  gegl_free (output_node_scaled);

  return result;
}

int main(int argc, char **argv)
{
  gint tests_run    = 0;
  gint tests_passed = 0;
  gint tests_failed = 0;

  gdouble scale_list[] = {5.0, 2.5, 2.0, 1.5, 1.0, 0.9, 0.5, 0.3, 0.1, 0.09, 0.05, 0.03};
  gint x_list[] = {-16, 0, 15};
  gint i, j, k;

  gegl_init(0, NULL);
  g_object_set(G_OBJECT(gegl_config()),
               "swap", "RAM",
               "use-opencl", FALSE,
               NULL);

  printf ("testing scaled blit\n");

  for (i = 0; i < sizeof(x_list) / sizeof(x_list[0]); ++i)
    {
      for (j = 0; j < sizeof(scale_list) / sizeof(scale_list[0]); ++j)
        {
          const Babl *format_list[] = {babl_format ("RGBA u8"), babl_format ("RGBA u16")};

          for (k = 0; k < sizeof(format_list) / sizeof(format_list[0]); ++k)
            {
              if (test_scale (scale_list[j], x_list[i], 0, format_list[k]))
                tests_passed++;
              else
                tests_failed++;
              tests_run++;
            }
        }
    }

  gegl_exit();

  printf ("\n");

  if (tests_passed == tests_run)
    return 0;
  return -1;

  return 0;
}
