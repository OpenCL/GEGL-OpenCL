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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES
   /* no properties */
#else

#define GEGL_CHANT_NAME          grey
#define GEGL_CHANT_SELF          "grey.c"
#define GEGL_CHANT_DESCRIPTION   "turns the image greyscale"
#define GEGL_CHANT_CATEGORIES    "color"

#define GEGL_CHANT_POINT_FILTER
#define GEGL_CHANT_PREPARE

#include "gegl-chant.h"

static void prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("YA float");

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
