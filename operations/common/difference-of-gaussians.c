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

property_double (radius1, _("Radius 1"), 1.0)
  value_range (0.0, 1000.0)
  ui_range (0.0, 10.0)
  ui_gamma (1.5)

property_double (radius2, _("Radius 2"), 2.0)
  value_range (0.0, 1000.0)
  ui_range (0.0, 20.0)
  ui_gamma (1.5)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     difference_of_gaussians
#define GEGL_OP_C_SOURCE difference-of-gaussians.c

#include "gegl-op.h"

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglNode *input, *output, *subtract, *blur1, *blur2;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");

  subtract = gegl_node_new_child (gegl,
                                  "operation", "gegl:subtract",
                                  NULL);

  blur1    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur",
                                  NULL);

  blur2    = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur",
                                  NULL);

  gegl_node_link_many (input, blur1, subtract, output, NULL);
  gegl_node_link (input, blur2);

  gegl_node_connect_from (subtract, "aux", blur2, "output");

  gegl_operation_meta_redirect (operation, "radius1", blur1, "std-dev-x");
  gegl_operation_meta_redirect (operation, "radius1", blur1, "std-dev-y");
  gegl_operation_meta_redirect (operation, "radius2", blur2, "std-dev-x");
  gegl_operation_meta_redirect (operation, "radius2", blur2, "std-dev-y");

  gegl_operation_meta_watch_nodes (operation, subtract, blur1, blur2, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:difference-of-gaussians",
    "title",       _("Difference of Gaussians"),
    "categories",  "edge-detect",
    "reference-hash", "cbb8f17cb0eda77182b676b8ab76714c",
    "description", _("Edge detection with control of edge thickness, based "
                     "on the difference of two gaussian blurs"),
    NULL);
}

#endif
