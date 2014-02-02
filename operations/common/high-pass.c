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

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double_ui (std_dev, _("Std. Dev."), 0.0, 10000.0, 4.0, 0.0, 1000.0,
                      1.5, _("Standard deviation (spatial scale factor)"))
gegl_chant_double_ui (contrast, _("Contrast"), 0.0, 5.0, 1.0,
                      0.0, 5.0, 1.0, _("Contrast of high-pass"))

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE "high-pass.c"

#include "gegl-chant.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl, *input, *output, *invert, *blur, *opacity, *over, *contrast;

  gegl = operation->node;
  input = gegl_node_get_input_proxy (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");
  invert = gegl_node_new_child (gegl, "operation", "gegl:invert-linear", NULL);
  blur = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);
  opacity = gegl_node_new_child (gegl, "operation", "gegl:opacity",
                                 "value", 0.5, NULL);
  over = gegl_node_new_child (gegl, "operation", "gegl:over", NULL);
  contrast = gegl_node_new_child (gegl, "operation",
                                  "gegl:brightness-contrast", NULL);

  gegl_node_link_many (input, invert, blur, opacity, NULL);
  gegl_node_connect_to (opacity, "output", over, "aux");
  gegl_node_link_many (input, over, contrast, output, NULL);

  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "contrast", contrast, "contrast");

  gegl_operation_meta_watch_nodes (operation, invert, blur, opacity, over, contrast, NULL);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gegl:high-pass",
                                 "categories",  "meta:enhance",
                                 "description",
                                 _("Enhances fine details."),
                                 NULL);
}

#endif
