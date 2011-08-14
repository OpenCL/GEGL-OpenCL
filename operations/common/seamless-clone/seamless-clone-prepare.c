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
#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE       "seamless-clone-prepare.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"
#include "poly2tri-c/render/mesh-render.h"
#include "seamless-clone.h"

static void
prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("R'G'B'A float");

  gegl_operation_set_format (operation, "input",  format);
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *roi)
{
  ScPreprocessResult *result = sc_preprocess_new ();
  
  /* First, find the paste outline */
  result->outline = sc_outline_find_ccw (roi, input);

  /* Then, Generate the mesh */
  result->mesh = sc_make_fine_mesh (result->outline, &result->mesh_bounds);

  /* Finally, Generate the mesh sample list for each point */
  result->sampling = sc_mesh_sampling_compute (result->outline, result->mesh);

  /* If caching of UV is desired, it shold be done here, and possibly
   * by using a regular operation rather than a sink one, so that we can
   * output UV coords */

  GEGL_CHANT_PROPERTIES (operation) -> result = result;

  /*
  sc_mesh_sampling_free (mesh_sampling);
  p2tr_triangulation_free (mesh);
  sc_outline_free (outline);
  */
  
  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationSinkClass     *sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  operation_class->prepare     = prepare;
  operation_class->name        = "gegl:seamless-clone-prepare";
  operation_class->categories  = "programming";
  operation_class->description = _("Seamless cloning preprocessing operation");

  sink_class->process          = process;
  sink_class->needs_full       = TRUE;
}

#endif
