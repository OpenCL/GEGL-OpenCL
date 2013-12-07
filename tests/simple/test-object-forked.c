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
#include "operation/gegl-operation.h"

#define SUCCESS  0
#define FAILURE -1

int main(int argc, char *argv[])
{
  GeglBuffer *buf_a;
  GeglBuffer *buf_b;
  gboolean    result = TRUE;

  gegl_init (&argc, &argv);

  buf_a = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 10, 10), babl_format ("RGB float"));
  buf_b = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 10, 10), babl_format ("RGB float"));

  if (gegl_object_get_has_forked (G_OBJECT (buf_a)))
    result = FALSE;
  if (gegl_object_get_has_forked (G_OBJECT (buf_b)))
    result = FALSE;

  gegl_object_set_has_forked (G_OBJECT (buf_a));

  if (!gegl_object_get_has_forked (G_OBJECT (buf_a)))
    result = FALSE;
  if (gegl_object_get_has_forked (G_OBJECT (buf_b)))
    result = FALSE;

  gegl_object_set_has_forked (G_OBJECT (buf_a));

  if (!gegl_object_get_has_forked (G_OBJECT (buf_a)))
    result = FALSE;
  if (gegl_object_get_has_forked (G_OBJECT (buf_b)))
    result = FALSE;

  gegl_object_set_has_forked (G_OBJECT (buf_b));

  if (!gegl_object_get_has_forked (G_OBJECT (buf_a)))
    result = FALSE;
  if (!gegl_object_get_has_forked (G_OBJECT (buf_b)))
    result = FALSE;

  g_object_unref (buf_a);
  g_object_unref (buf_b);

  gegl_exit ();

  if (result)
    return SUCCESS;
  return FAILURE;
}