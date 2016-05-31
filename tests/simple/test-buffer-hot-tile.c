/* This file is a test-case for GEGL
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
 * Copyright (C) 2016 Ell
 */

#include "config.h"

#include <stddef.h>
#include <stdio.h>

#include "gegl.h"

#define SUCCESS    0
#define FAILURE    -1

/* Test case for the issue fixed by commit
 * cd98e944f5c30a8549f9cc22d5eb7f50a9dca334
 */
static gint
test_set_clear_get (void)
{
  gint                 result = SUCCESS;
  gint                 width, height;
  const Babl          *format;
  GeglBuffer          *buffer;
  guchar               pixel;
  const GeglRectangle  pixel_rect = {0, 0, 1, 1};

  g_object_get (gegl_config(),
                "tile-width",  &width,
                "tile-height", &height,
                NULL);

  format = babl_format ("Y u8");
  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height), format);

  /* Write 1 to the top-left pixel of the buffer. */
  pixel = 1;
  gegl_buffer_set (buffer, &pixel_rect, 0, format, &pixel, GEGL_AUTO_ROWSTRIDE);

  /* Clear the buffer. */
  gegl_buffer_clear (buffer, NULL);

  /* Read back the pixel value; `pixel` should now be 0. */
  gegl_buffer_get (buffer, &pixel_rect, 1.0, format, &pixel,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  if (pixel != 0)
    result = FAILURE;

  g_object_unref (buffer);

  return result;
}

#define RUN_TEST(test) \
  do \
  { \
    printf (#test "..."); \
    fflush (stdout); \
    \
    if (test_##test () == SUCCESS) \
      printf (" passed\n"); \
    else \
      { \
        printf (" FAILED\n"); \
        result = FAILURE; \
      } \
  } while (FALSE)

int main (int argc, char *argv[])
{
  gint result = SUCCESS;

  gegl_init (&argc, &argv);

  RUN_TEST (set_clear_get);

  gegl_exit ();

  return result;
}
