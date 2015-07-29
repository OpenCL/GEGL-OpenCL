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

#include <string.h>
#include <stdio.h>

#include "gegl.h"

#define SUCCESS  0
#define FAILURE -1

static gboolean
test_convert_common (const Babl *in_format,
                     const Babl *out_format)
{
  /* Validate that gegl:convert-format produces out_format when given in_format */
  gboolean result = TRUE;

  GeglNode *ptn, *src, *convert, *sink;
  GeglBuffer *src_buffer;
  GeglBuffer *sink_buffer = NULL;

  src_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 10, 10), in_format);

  ptn  = gegl_node_new ();

  src  = gegl_node_new_child (ptn,
                              "operation", "gegl:buffer-source",
                              "buffer", src_buffer,
                              NULL);

  convert = gegl_node_new_child (ptn,
                                 "operation", "gegl:convert-format",
                                 "format", out_format,
                                 NULL);

  sink = gegl_node_new_child (ptn,
                              "operation", "gegl:buffer-sink",
                              "buffer", &sink_buffer,
                              "format", NULL,
                              NULL);

  gegl_node_link_many (src, convert, sink, NULL);

  gegl_node_blit_buffer (sink, NULL, NULL, 0, GEGL_ABYSS_NONE);

  if (out_format != gegl_buffer_get_format (sink_buffer))
    {
      printf ("Got %s expected %s\n", babl_get_name (gegl_buffer_get_format (sink_buffer)),
                                      babl_get_name (out_format));
      result = FALSE;
    }

  if (!gegl_rectangle_equal (gegl_buffer_get_extent (src_buffer),
                             gegl_buffer_get_extent (sink_buffer)))
    result = FALSE;

  g_object_unref (ptn);
  g_object_unref (src_buffer);
  g_object_unref (sink_buffer);

  return result;
}

static gboolean
test_convert_001 (void)
{
  return test_convert_common (babl_format ("RGB float"),
                              babl_format ("Y float"));
}

static gboolean
test_convert_002 (void)
{
  return test_convert_common (babl_format ("RGB float"),
                              babl_format ("RGB u8"));
}

static gboolean
test_same_001 (void)
{
  return test_convert_common (babl_format ("RGB float"),
                              babl_format ("RGB float"));
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

int main(int argc, char *argv[])
{
  gint tests_run    = 0;
  gint tests_passed = 0;
  gint tests_failed = 0;

  gegl_init (&argc, &argv);

  RUN_TEST (test_convert_001)
  RUN_TEST (test_convert_002)
  RUN_TEST (test_same_001)

  gegl_exit ();

  if (tests_passed == tests_run)
    return SUCCESS;
  return FAILURE;
}
