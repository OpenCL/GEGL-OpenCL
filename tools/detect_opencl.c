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

#include <stdio.h>

#include "gegl.h"
#include "opencl/gegl-cl.h"

int main(int argc, char **argv)
{
  int ret;

  gegl_init(&argc, &argv);

  if (gegl_cl_is_accelerated())
    {
      printf ("yes");
      ret = 0;
    }
  else
    {
      printf ("no");
      ret = 1;
    }

  gegl_exit ();

  return ret;
}
