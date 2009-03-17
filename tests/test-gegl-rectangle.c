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
 * Copyright (C) 2009 Martin Nordholts
 */

#include <string.h>

#include "gegl.h"
#include "gegl-utils.h"


#define SUCCESS  0
#define FAILURE -1

#define INFINITE_PLANE G_MININT / 2, G_MININT / 2, G_MAXINT, G_MAXINT

#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))

typedef struct
{
  GeglRectangle rect1;
  GeglRectangle rect2;

  GeglRectangle bounding_box_result;

  GeglRectangle intersect_result;
  gboolean      intersect_return_value;

  gboolean      contains_return_value;
} GeglRectangleTestCase;

static GeglRectangleTestCase tests[] =
{
  /* 1 */
  { { INFINITE_PLANE },
    { INFINITE_PLANE },

    { INFINITE_PLANE },

    { INFINITE_PLANE },
    TRUE,

    TRUE },

  /* 2 */
  { { INFINITE_PLANE   },
    { -10, -10, 20, 20 },

    { INFINITE_PLANE   },

    { -10, -10, 20, 20 },
    TRUE,

    TRUE },


  /* 3 */
  { { -10, -10, 20, 20 },
    { INFINITE_PLANE   },

    { INFINITE_PLANE   },

    { -10, -10, 20, 20 },
    TRUE,

    FALSE },


  /* 4 */
  { { -10, -10, 10, 10 },
    {  0,   0,  10, 10 },

    { -10, -10, 20, 20 },

    {  0,   0,  0,  0  },
    FALSE,

    FALSE },


  /* 5 */
  { {  0,   0,  10, 10 },
    { -10, -10, 10, 10 },

    { -10, -10, 20, 20 },

    {  0,   0,  0,  0 },
    FALSE,

    FALSE },


  /* 6 */
  { { -10, -10, 0, 0 },
    {  1,   2,  3, 4 },

    {  1,   2,  3, 4 },

    {  0,   0,  0, 0 },
    FALSE,

    FALSE },


  /* 7 */
  { {  1,   2,  3, 4 },
    { -10, -10, 0, 0 },

    {  1,   2,  3, 4 },

    {  0,   0,  0, 0 },
    FALSE,

    FALSE },


  /* 8 */
  { { G_MININT / 2, G_MININT / 2, 1, 1 },
    { INFINITE_PLANE                   },

    { INFINITE_PLANE                   },

    { G_MININT / 2, G_MININT / 2, 1, 1 },
    TRUE,

    FALSE },


  /* 9 */
  { { G_MAXINT / 2 - 1, G_MAXINT / 2- 1, 1, 1  },
    { INFINITE_PLANE                           },

    { INFINITE_PLANE                           },

    { G_MAXINT / 2 - 1, G_MAXINT / 2 - 1, 1, 1 },
    TRUE,

    FALSE },


  /* 10 */
  { { -5,  -5,  10, 10 },
    {  0,   0,  5,  5  },

    { -5,  -5,  10, 10 },

    {  0,   0,  5,  5  },
    TRUE,

    TRUE },
};

int main(int argc, char *argv[])
{
  GeglRectangle expected_infinite_plane = gegl_rectangle_infinite_plane ();
  GeglRectangle infinite_plane          = { INFINITE_PLANE };
  int           result = SUCCESS;
  int           i      = 0;

  /* Make sure our representation of an infinite plane GeglRectangle
   * is correct
   */
  if (! gegl_rectangle_equal (&infinite_plane, &expected_infinite_plane))
    {
      result = FAILURE;
      g_printerr("This test case and GEGL does not represent an infinite plane\n"
                 "GeglRectangle in the same way, update this test case. Aborting.\n");
      goto abort;
    }

  for (i = 0; i < ARRAY_SIZE (tests); i++)
    {
      GeglRectangle result_rect;
      gboolean      return_value;

      /* gegl_rectangle_bounding_box() */
      gegl_rectangle_bounding_box (&result_rect,
                                   &tests[i].rect1,
                                   &tests[i].rect2);
      if (! gegl_rectangle_equal (&result_rect, &tests[i].bounding_box_result))
        {
          result = FAILURE;
          g_printerr("The gegl_rectangle_bounding_box() test #%d failed. Aborting.\n", i + 1);
          goto abort;
        }

      /* gegl_rectangle_intersect() */
      return_value = gegl_rectangle_intersect (&result_rect,
                                               &tests[i].rect1,
                                               &tests[i].rect2);
      if (! gegl_rectangle_equal (&result_rect, &tests[i].intersect_result) ||
          return_value != tests[i].intersect_return_value)
        {
          result = FAILURE;
          g_printerr("The gegl_rectangle_intersect() test #%d failed. Aborting.\n", i + 1);
          goto abort;
        }

      /* gegl_rectangle_contains() */
      return_value = gegl_rectangle_contains (&tests[i].rect1,
                                              &tests[i].rect2);
      if (return_value != tests[i].contains_return_value)
        {
          result = FAILURE;
          g_printerr("The gegl_rectangle_contains() test #%d failed. Aborting.\n", i + 1);
          goto abort;
        }
    }

abort:
  return result;
}
