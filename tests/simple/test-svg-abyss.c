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

static void
dump_to_png (const char *filename, GeglBuffer *buffer)
{
  GeglNode *ptn, *src, *dst;

  ptn = gegl_node_new();
  src = gegl_node_new_child(ptn, "operation", "gegl:buffer-source",
                            "buffer", buffer,
                            NULL);

  dst = gegl_node_new_child(ptn, "operation", "gegl:png-save",
                            "path", filename,
                            "compression", 6,
                            NULL);

  gegl_node_connect_to (src, "output",  dst, "input");

  gegl_node_process(dst);

  g_object_unref(ptn);
}

static gboolean
test_operation (const char *operation_name)
{
  gboolean result = FALSE;

  const Babl *format = babl_format ("RGBA u8");
  const gint bpp = babl_format_get_bytes_per_pixel (format);
  const gint out_width = 5 + 10;
  const gint out_height = 5 + 10;

  guchar *output_with_abyss = gegl_malloc (out_width * out_height * bpp);
  guchar *output_no_abyss   = gegl_malloc (out_width * out_height * bpp);

  GeglColor *upper_color = gegl_color_new ("rgb(0.2, 0.8, 0.2)");
  GeglColor *lower_color = gegl_color_new ("rgb(0.0, 0.0, 0.0)");
  GeglColor *transparent = gegl_color_new ("rgba(0.0, 0.0, 0.0, 0.0)");

  {
    /* Using abyss */
    GeglNode *ptn = gegl_node_new();

    GeglNode *test_op, *upper_rect, *lower_rect;

    test_op = gegl_node_new_child (ptn,
                                   "operation", operation_name,
                                   NULL);

    upper_rect = gegl_node_new_child (ptn,
                                   "operation", "gegl:rectangle",
                                   "color", upper_color,
                                   "x", 5.0,
                                   "y", 5.0,
                                   "width", 10.0,
                                   "height", 10.0,
                                   NULL);

    lower_rect = gegl_node_new_child (ptn,
                                   "operation", "gegl:rectangle",
                                   "color", lower_color,
                                   "x", 0.0,
                                   "y", 0.0,
                                   "width", 10.0,
                                   "height", 10.0,
                                   NULL);

    gegl_node_connect_to (lower_rect, "output", test_op, "input");
    gegl_node_connect_to (upper_rect, "output", test_op, "aux");

    {
      int i;
      guchar *out = output_with_abyss;
      for (i = 0; i < out_height; i++)
        {
          gegl_node_blit (test_op,
                          1.0,
                          GEGL_RECTANGLE (0, i, out_width, 1),
                          format,
                          out,
                          GEGL_AUTO_ROWSTRIDE,
                          0);
          out += out_width * bpp;
        }
    }

    g_object_unref (ptn);
  }

  {
    /* Explicit transparency */
    GeglNode *ptn = gegl_node_new();

    GeglNode *test_op, *upper_rect, *lower_rect;
    GeglNode *upper_over, *lower_over;
    GeglNode *background;

    test_op = gegl_node_new_child (ptn,
                                   "operation", operation_name,
                                   NULL);

    background = gegl_node_new_child (ptn,
                                   "operation", "gegl:rectangle",
                                   "color", transparent,
                                   "x", 0.0,
                                   "y", 0.0,
                                   "width", 10.0 + 5.0,
                                   "height", 10.0 + 5.0,
                                   NULL);

    upper_rect = gegl_node_new_child (ptn,
                                   "operation", "gegl:rectangle",
                                   "color", upper_color,
                                   "x", 5.0,
                                   "y", 5.0,
                                   "width", 10.0,
                                   "height", 10.0,
                                   NULL);

    upper_over = gegl_node_new_child (ptn,
                                   "operation", "gegl:over",
                                   NULL);

    lower_rect = gegl_node_new_child (ptn,
                                   "operation", "gegl:rectangle",
                                   "color", lower_color,
                                   "x", 0.0,
                                   "y", 0.0,
                                   "width", 10.0,
                                   "height", 10.0,
                                   NULL);

    lower_over = gegl_node_new_child (ptn,
                                   "operation", "gegl:over",
                                   NULL);

    gegl_node_connect_to (lower_rect, "output", lower_over, "aux");
    gegl_node_connect_to (upper_rect, "output", upper_over, "aux");

    gegl_node_connect_to (background, "output", lower_over, "input");
    gegl_node_connect_to (background, "output", upper_over, "input");

    gegl_node_connect_to (lower_over, "output", test_op, "input");
    gegl_node_connect_to (upper_over, "output", test_op, "aux");

    {
      int i;
      guchar *out = output_no_abyss;
      for (i = 0; i < out_height; i++)
        {
          gegl_node_blit (test_op,
                          1.0,
                          GEGL_RECTANGLE (0, i, out_width, 1),
                          format,
                          out,
                          GEGL_AUTO_ROWSTRIDE,
                          0);
          out += out_width * bpp;
        }
    }

    g_object_unref (ptn);
  }

  if (0 == memcmp (output_with_abyss, output_no_abyss, out_width * out_height * bpp))
    {
      printf (".");
      fflush(stdout);
      result = TRUE;
    }
  else
    {
      printf ("\n %s ... FAIL\n", operation_name);
      result = FALSE;
    }

  if (!result)
    {
      GeglBuffer *linear;
      gchar *filename;

      filename = g_strconcat (operation_name, " - with abyss.png", NULL);
      linear = gegl_buffer_linear_new_from_data (output_with_abyss,
                                                format,
                                                GEGL_RECTANGLE (0, 0, out_width, out_height),
                                                GEGL_AUTO_ROWSTRIDE,
                                                NULL,
                                                NULL);
      dump_to_png (filename, linear);
      g_free (filename);
      g_object_unref (linear);

      filename = g_strconcat (operation_name, " - no abyss.png", NULL);
      linear = gegl_buffer_linear_new_from_data (output_no_abyss,
                                                 format,
                                                 GEGL_RECTANGLE (0, 0, out_width, out_height),
                                                 GEGL_AUTO_ROWSTRIDE,
                                                 NULL,
                                                 NULL);
      dump_to_png (filename, linear);
      g_free (filename);
      g_object_unref (linear);
    }

  gegl_free (output_with_abyss);
  gegl_free (output_no_abyss);

  return result;
}

int main(int argc, char **argv)
{
  gint tests_run    = 0;
  gint tests_passed = 0;
  gint tests_failed = 0;

  gint i;

  const char *operation_names[] = {
    "svg:src",
    "svg:dst",

    "svg:src-over",
    "svg:dst-over",

    "svg:src-in",
    "svg:dst-in",

    "svg:src-out",
    "svg:dst-out",

    "svg:src-atop",
    "svg:dst-atop",

    "svg:clear",
    "svg:xor",
  };

  gegl_init(0, NULL);
  g_object_set(G_OBJECT(gegl_config()),
               "swap", "RAM",
               "use-opencl", FALSE,
               NULL);

  printf ("testing abyss handling of svg blends\n");

  for (i = 0; i < G_N_ELEMENTS(operation_names); ++i)
    {
      if (test_operation (operation_names[i]))
        tests_passed++;
      else
        tests_failed++;
      tests_run++;
    }

  gegl_exit();

  printf ("\n");

  if (tests_passed == tests_run)
    return 0;
  return -1;

  return 0;
}
