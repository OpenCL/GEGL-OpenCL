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
 * Copyright 2009 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int_ui (levels, _("Levels"), 1, 64, 8, 1, 64, 2,
                   _("number of levels per component"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE       "posterize.c"
#define GEGLV4

#include "gegl-chant.h"

#ifndef RINT
#define RINT(a) ((gint)(a+0.5))
#endif


static gboolean process (GeglOperation       *operation,
                         void                *in_buf,
                         void                *out_buf,
                         glong                samples,
                         const GeglRectangle *roi,
                         gint                 level)
{
  GeglChantO *o      = GEGL_CHANT_PROPERTIES (operation);
  gfloat     *src    = in_buf;
  gfloat     *dest   = out_buf;
  gfloat      levels = o->levels;

  while (samples--)
    {
      gint i;
      for (i=0; i < 3; i++)
        dest[i] = RINT (src[i]   * levels) / levels;
      dest[3] = src[3];

      src  += 4;
      dest += 4;
    }

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:posterize",
    "categories" , "color",
    "description",
       _("Reduces the number of levels in each color component of the image."),
       NULL);

}

#endif
