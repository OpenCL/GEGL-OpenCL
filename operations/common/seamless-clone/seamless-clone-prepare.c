/* This file is an image processing operation for GEGL
 *
 * seamless-clone-prepare.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * This is part 1 of 2 in the process of real-time seamless cloning.
 * This operation takes a paste to be seamlessly cloned, and a property
 * which is a pointer to a pointer. Through that pointer, the operation
 * should return a data structure that is to be passed to the next
 * operations.
 */

#ifdef GEGL_CHANT_PROPERTIES
gegl_chant_pointer (result, _("result"),
  _("A pointer to a pointer (gpointer*) to store the result in"))
gegl_chant_int (max_refine_steps, _("Refinement Steps"), 0, 100000.0, 2000,
                _("Maximal amount of refinement points to be used for the interpolation mesh"))
#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "seamless-clone-prepare.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"
#include "poly2tri-c/render/mesh-render.h"
#include "seamless-clone-common.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("R'G'B'A float");
  gpointer   *dest = GEGL_CHANT_PROPERTIES (operation) -> result;

  gegl_operation_set_format (operation, "input",  format);

  if (dest == NULL)
    {
      g_warning ("sc-prepare: No place to store the preprocessing result given!");
    }
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *roi)
{
  gpointer *dest = GEGL_CHANT_PROPERTIES (operation) -> result;

  if (dest == NULL)
    {
      return FALSE;
    }

  *dest = sc_generate_cache (input, roi, GEGL_CHANT_PROPERTIES (operation) -> max_refine_steps);

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSinkClass     *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  operation_class->prepare     = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:seamless-clone-prepare",
    "categories",  "programming",
    "description", _("Seamless cloning preprocessing operation"),
    NULL);

  sink_class->process          = process;
  sink_class->needs_full       = TRUE;
}

#endif
