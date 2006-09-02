/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#ifdef GEGL_CHANT_SELF

#else

#define GEGL_CHANT_FILTER
#define GEGL_CHANT_NAME            nop
#define GEGL_CHANT_DESCRIPTION     "Passthrough operation"
#define GEGL_CHANT_SELF            "nop.c"
#define GEGL_CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  /* not used */
  return  TRUE;
}

static gboolean
op_evaluate (GeglOperation *operation,
             const gchar   *output_prop)
{
  GeglOperationFilter      *op_filter = GEGL_OPERATION_FILTER (operation);
  gboolean success = FALSE;

  if (op_filter->output != NULL)
    {
      g_object_unref (op_filter->output);
      op_filter->output = NULL;
    }

  if (op_filter->input != NULL)
    {
      op_filter->output=g_object_ref (op_filter->input);
      g_object_unref (op_filter->input);
      op_filter->input=NULL;
    }
  /* the NOP op does not complain about NULL inputs */
  return success;
}
static void class_init (GeglOperationClass *klass)
{
  klass->evaluate = op_evaluate;
}


#endif
