/* This file is an image processing operation for GEGL
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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (s_amount, _("shadows amount"), 0.2)
    value_range (0.0, 1.0)

property_double (s_tonalwidth, _("shadows tonal width"), 0.1)
    value_range (0.001, 1.0)

property_double (s_radius, _("shadows radius"), 5.0)
    value_range (0.0, 100.0)

property_double (h_amount, _("highlights amount"), 0.2)
    value_range (0.0, 1.0)

property_double (h_tonalwidth, _("highlights tonal width"), 0.1)
    value_range (0.001, 1.0)

property_double (h_radius, _("highlights radius"), 5.0)
    value_range (0.0, 100.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     shadows_highlights
#define GEGL_OP_C_SOURCE shadows-highlights.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl;
  GeglNode *input;
  GeglNode *output;
  GeglNode *s_blur;
  GeglNode *h_blur;
  GeglNode *shprocess;

  gegl   = operation->node;
  input  = gegl_node_get_input_proxy (gegl, "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  s_blur = gegl_node_new_child (gegl,
                                "operation",    "gegl:gaussian-blur",
                                "abyss-policy", 1,
                                NULL);

  h_blur = gegl_node_new_child (gegl,
                                "operation",    "gegl:gaussian-blur",
                                "abyss-policy", 1,
                                NULL);

  shprocess = gegl_node_new_child (gegl,
                                   "operation", "gegl:shadows-highlights-correction",
                                   NULL);

  gegl_node_link (input, s_blur);
  gegl_node_link (input, h_blur);
  gegl_node_link_many (input, shprocess, output, NULL);

  gegl_node_connect_to (s_blur, "output", shprocess, "aux");
  gegl_node_connect_to (h_blur, "output", shprocess, "aux2");

  gegl_operation_meta_redirect (operation, "s-amount", shprocess, "s-amount");
  gegl_operation_meta_redirect (operation, "s-tonalwidth", shprocess, "s-tonalwidth");
  gegl_operation_meta_redirect (operation, "s-radius", s_blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "s-radius", s_blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "h-amount", shprocess, "h-amount");
  gegl_operation_meta_redirect (operation, "h-tonalwidth", shprocess, "h-tonalwidth");
  gegl_operation_meta_redirect (operation, "h-radius", h_blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "h-radius", h_blur, "std-dev-y");

  gegl_operation_meta_watch_nodes (operation, s_blur, h_blur, shprocess, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shadows-highlights",
    "title",       _("Shadows Highlights"),
    "categories",  "light",
    "description", _("Performs shadows and highlights correction"),
    NULL);
}

#endif
