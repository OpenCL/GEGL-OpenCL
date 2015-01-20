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

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

   /* no properties */

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_C_SOURCE invert-linear.c

#include "gegl-op.h"

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat *in  = in_buf;
  gfloat *out = out_buf;

  while (samples--)
    {
      out[0] = 1.0 - in[0];
      out[1] = 1.0 - in[1];
      out[2] = 1.0 - in[2];
      out[3] = in[3];

      in += 4;
      out+= 4;
    }
  return TRUE;
}

#include "opencl/invert-linear.cl.h"

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process  = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:invert-linear",
    "title",       _("Invert"),
    "compat-name", "gegl:invert",
    "categories" , "color",
    "description",
       _("Inverts the components (except alpha), the result is the "
         "corresponding \"negative\" image."),
    "cl-source"  , invert_linear_cl_source,
    NULL);
}

#endif
