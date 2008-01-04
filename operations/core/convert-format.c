/* This file is an image processing operation for GEGL
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
 * Copyright 2007 Étienne Bersac <bersace03@laposte.net>
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 *
 * This operation is just a forked grey op with format parameters.
 */
#if GEGL_CHANT_PROPERTIES
gegl_chant_string(format, "RGBA float", "Babl ouput format string")
#else

#define GEGL_CHANT_NAME          convert_format
#define GEGL_CHANT_SELF          "convert-format.c"
#define GEGL_CHANT_DESCRIPTION   "Convert the data into the specified format"
#define GEGL_CHANT_CATEGORIES    "core:color"

#define GEGL_CHANT_POINT_FILTER
#define GEGL_CHANT_PREPARE

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  GeglChantOperation  *self = GEGL_CHANT_OPERATION (operation);
  g_assert (self->format);
  Babl *format = babl_format (self->format);
  /* check format ? */

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation *op,
         void          *in_buf,
         void          *out_buf,
         glong          samples) 
{
  return TRUE;
}

#endif
