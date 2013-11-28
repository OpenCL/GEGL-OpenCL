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
#include "opencl/gegl-cl.h"

#define SUCCESS  0
#define FAILURE -1
#define SKIP     77

static gboolean
test_opencl_conversion (const char *in_format_name,
                        const char *out_format_name,
                        GeglClColorOp expected_conversion)
{
  gboolean result = TRUE;
  const Babl *in_format  = babl_format (in_format_name);
  const Babl *out_format = babl_format (out_format_name);

  GeglClColorOp conversion_mode = gegl_cl_color_supported (in_format, out_format);

  if (conversion_mode != expected_conversion)
    {
      printf ("\n");
      printf ("Wrong conversion mode (%d) for \"%s\" -> \"%s\"\n", conversion_mode, in_format_name, out_format_name);
      result = FALSE;
    }

  return result;
}

#define RUN_TEST(in_format, out_format, expected) \
{ \
  if (test_opencl_conversion(in_format, out_format, expected)) \
    { \
      printf ("."); \
      fflush(stdout); \
      tests_passed++; \
    } \
  else \
    { \
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

  if (!gegl_cl_is_accelerated())
    {
      printf ("OpenCL disabled, skipping tests\n");
      gegl_exit ();
      return SKIP;
    }
  else
    {
      RUN_TEST ("RGBA float", "RGBA float", GEGL_CL_COLOR_EQUAL)
      RUN_TEST ("RGBA float", "RaGaBaA float", GEGL_CL_COLOR_CONVERT)
      /* RUN_TEST ("RGBA float", "RGB float", GEGL_CL_COLOR_CONVERT) */
      RUN_TEST ("RGBA float", "R'G'B'A float", GEGL_CL_COLOR_CONVERT)
      /* RUN_TEST ("RGBA float", "R'aG'aB'aA float", GEGL_CL_COLOR_CONVERT) */
      RUN_TEST ("RGBA float", "R'G'B'A u8", GEGL_CL_COLOR_CONVERT)
      RUN_TEST ("RGBA float", "R'G'B' u8", GEGL_CL_COLOR_CONVERT)

      RUN_TEST ("R'G'B'A float", "RGBA float", GEGL_CL_COLOR_CONVERT)
      RUN_TEST ("R'G'B'A float", "R'G'B'A float", GEGL_CL_COLOR_EQUAL)
      /* RUN_TEST ("R'G'B'A float", "R'G'B'A u8", GEGL_CL_COLOR_CONVERT) */
      /* RUN_TEST ("R'G'B'A float", "R'G'B' u8", GEGL_CL_COLOR_CONVERT) */

      RUN_TEST ("R'G'B'A u8", "RGBA float", GEGL_CL_COLOR_CONVERT)
      /* RUN_TEST ("R'G'B'A u8", "R'G'B'A float", GEGL_CL_COLOR_CONVERT) */
      RUN_TEST ("R'G'B'A u8", "R'G'B'A u8", GEGL_CL_COLOR_EQUAL)
    }

  printf ("\n");

  gegl_exit ();

  if (tests_passed == tests_run)
    return SUCCESS;
  return FAILURE;
}
