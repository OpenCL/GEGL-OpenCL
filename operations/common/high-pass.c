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
 * Copyright 2013 Ville Sokk <ville.sokk@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (std_dev, _("Std. Dev."), 4.0)
    description (_("Standard deviation (spatial scale factor)"))
    value_range (0.0, 10000.0)
    ui_range (0.0, 1000.0)
    ui_gamma (1.5)

property_double (contrast, _("Contrast"), 1.0)
    description(_("Contrast of high-pass"))
    value_range (0.0, 5.0)
    ui_range (0.0, 5.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     high_pass
#define GEGL_OP_C_SOURCE high-pass.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl, *input, *output, *invert, *blur, *opacity, *over, *sub, *mul, *add, *gam0, *gam1;

  gegl = operation->node;
  input = gegl_node_get_input_proxy (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");
  invert = gegl_node_new_child (gegl, "operation", "gegl:invert-gamma", NULL);
  blur = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);
  opacity = gegl_node_new_child (gegl, "operation", "gegl:opacity",
                                 "value", 0.5, NULL);
  over = gegl_node_new_child (gegl, "operation", "gegl:over", "srgb", TRUE, NULL);
  gam0 = gegl_node_new_child (gegl, "operation", "gegl:gamma", "value", 0.4545, NULL);
  sub = gegl_node_new_child (gegl, "operation", "gegl:add", "value", -0.5, NULL);
  mul = gegl_node_new_child (gegl, "operation", "gegl:multiply", "value", 1.0, NULL);
  add = gegl_node_new_child (gegl, "operation", "gegl:add", "value", 0.5, NULL);
  gam1 = gegl_node_new_child (gegl, "operation", "gegl:gamma", "value", 2.2, NULL);

  gegl_node_link_many (input, blur, invert, opacity, NULL);
  gegl_node_connect_to (opacity, "output", over, "aux");
  gegl_node_link_many (input, over, gam0, sub, mul, add, gam1, output, NULL);

  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "contrast", mul, "value");

  gegl_operation_meta_watch_nodes (operation, blur, mul, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gegl:high-pass",
                                 "title",       _("High Pass Filter"),
                                 "categories",  "frequency",
                                 "reference-hash", "5d10ee663c5ff908c3c081e516154873",
                                 "description",
                                 _("Enhances fine details."),
                                 NULL);
}

#endif
