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
 * Copyright 2006, 2010 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (std_dev, _("Standard Deviation"), 0.55)
    description(_("Standard deviation (spatial scale factor)"))
    value_range (0.2, 300)
    ui_range    (0.2, 40.0)
    ui_gamma    (3.0)
    ui_meta     ("unit", "pixel-distance")

property_double (scale, _("Scale"), 4.0)
    description(_("Scale, strength of effect"))
    value_range (0.0, 300.0)
    ui_range    (0.0, 10.0)
    ui_gamma    (3.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     unsharp_mask
#define GEGL_OP_C_SOURCE unsharp-mask.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl, *input, *output, *add, *multiply, *subtract, *blur;

  gegl = operation->node;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");
  add      = gegl_node_new_child (gegl, "operation", "gegl:add", NULL);
  multiply = gegl_node_new_child (gegl, "operation", "gegl:multiply", NULL);
  subtract = gegl_node_new_child (gegl, "operation", "gegl:subtract", NULL);
  blur     = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);

  gegl_node_link_many (input, subtract, multiply, NULL);
  gegl_node_link (input, blur);
  gegl_node_link_many (multiply, add, output, NULL);

  gegl_node_connect_from (subtract, "aux",   blur,     "output");
  gegl_node_connect_from (add,      "aux",   input, "output");

  gegl_operation_meta_redirect (operation, "scale", multiply, "value");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-y");

  gegl_operation_meta_watch_nodes (operation, add, multiply, subtract, blur, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:unsharp-mask",
    "title",       _("Unsharp Mask"),
    "categories",  "enhance:sharpen",
    "description", _("Sharpen image, by adding difference to blurred image, a technique for sharpening originally used in darkrooms."),
    NULL);
}

#endif
