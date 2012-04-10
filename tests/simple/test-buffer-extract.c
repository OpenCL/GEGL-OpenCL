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
 * Copyright (C) 2010 Martin Nordholts
 */

#include "config.h"

#include "gegl.h"
#include "gegl-plugin.h"

#define SUCCESS  0
#define FAILURE -1

static int
test_buffer_extract (void)
{
  gint result = SUCCESS;
  GeglBuffer *buffer = gegl_buffer_new (GEGL_RECTANGLE (0,0,1,1),
                                        babl_format ("R'G'B'A u8"));
  guchar srcpix[4] = {1,2,3,4};
  guchar dstpix[4] = {0};

  gegl_buffer_set (buffer, NULL, 0, NULL, srcpix, GEGL_AUTO_ROWSTRIDE);
  gegl_buffer_get (buffer, NULL, 1.0, 
       babl_format_new ("name", "B' u8",
                          babl_model ("R'G'B'A"),
                          babl_type ("u8"),
                          babl_component ("B'"),
                          NULL),
      dstpix, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  g_object_unref (buffer);

  if (dstpix[0] != 3)
    result = FAILURE;
  
  return result;
}


int main(int argc, char *argv[])
{
  gint result = SUCCESS;

  gegl_init (&argc, &argv);

  if (result == SUCCESS)
    result = test_buffer_extract ();

  gegl_exit ();

  return result;
}
