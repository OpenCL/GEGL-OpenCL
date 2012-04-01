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


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (opacity, _("Opacity"), -2.0, 2.0, 0.5, _("Opacity"))
gegl_chant_double_ui (x, _("X"), -G_MAXDOUBLE, G_MAXDOUBLE, 20.0, -20.0, 20.0, 1.0,
                   _("Horizontal shadow offset"))
gegl_chant_double_ui (y, _("Y"), -G_MAXDOUBLE, G_MAXDOUBLE, 20.0, -20.0, 20.0, 1.0,
                   _("Vertical shadow offset"))
gegl_chant_double_ui (radius, _("Radius"), 0.0, G_MAXDOUBLE, 10.0, 0.0, 300.0, 1.5,
                   _("Blur radius"))

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE       "dropshadow.c"

#include "gegl-chant.h"

/* in attach we hook into graph adding the needed nodes */
static void attach (GeglOperation *operation)
{
  GeglNode *gegl  = operation->node;
  GeglNode *input, *output, *over, *translate, *opacity, *blur, *darken, *black;
  GeglColor *black_color = gegl_color_new ("rgb(0.0,0.0,0.0)");

  input     = gegl_node_get_input_proxy (gegl, "input");
  output    = gegl_node_get_output_proxy (gegl, "output");
  over      = gegl_node_new_child (gegl, "operation", "gegl:over", NULL);
  translate = gegl_node_new_child (gegl, "operation", "gegl:translate", NULL);
  opacity   = gegl_node_new_child (gegl, "operation", "gegl:opacity", NULL);
  blur      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);
  darken    = gegl_node_new_child (gegl, "operation", "gegl:src-in", NULL);
  black     = gegl_node_new_child (gegl, "operation", "gegl:color",
                                     "value", black_color,
                                     NULL);
  g_object_unref (black_color);

  gegl_node_link_many (input, darken, blur, opacity, translate, over, output, NULL);
  gegl_node_connect_from (over, "aux", input, "output");
  gegl_node_connect_from (darken, "aux", black, "output");

  gegl_operation_meta_redirect (operation, "opacity", opacity, "value");
  gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "x", translate, "x");
  gegl_operation_meta_redirect (operation, "y", translate, "y");
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:dropshadow",
    "categories" , "meta:effects",
    "description",
    _("Creates a dropshadow effect on the input buffer"),
    NULL);
}

#endif
