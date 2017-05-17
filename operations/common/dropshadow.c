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

property_double (x, _("X"), 20.0)
  description   (_("Horizontal shadow offset"))
  ui_range      (-40.0, 40.0)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "x")

property_double (y, _("Y"), 20.0)
  description   (_("Vertical shadow offset"))
  ui_range      (-40.0, 40.0)
  ui_meta       ("unit", "pixel-distance")
  ui_meta       ("axis", "y")

property_double (radius, _("Blur radius"), 10.0)
  value_range   (0.0, G_MAXDOUBLE)
  ui_range      (0.0, 300.0)
  ui_gamma      (1.5)
  ui_meta       ("unit", "pixel-distance")

property_color  (color, _("Color"), "black")
    /* TRANSLATORS: the string 'black' should not be translated */
  description   (_("The shadow's color (defaults to 'black')"))

/* It does make sense to sometimes have opacities > 1 (see GEGL logo
 * for example)
 */
property_double (opacity, _("Opacity"), 0.5)
  value_range   (0.0, 2.0)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     dropshadow
#define GEGL_OP_C_SOURCE dropshadow.c

#include "gegl-op.h"

/* in attach we hook into graph adding the needed nodes */
static void
attach (GeglOperation *operation)
{
  GeglNode  *gegl = operation->node;
  GeglNode  *input, *output, *over, *translate, *opacity, *blur, *darken, *color;
  GeglColor *black_color = gegl_color_new ("rgb(0.0,0.0,0.0)");

  input     = gegl_node_get_input_proxy (gegl, "input");
  output    = gegl_node_get_output_proxy (gegl, "output");
  over      = gegl_node_new_child (gegl, "operation", "gegl:over", NULL);
  translate = gegl_node_new_child (gegl, "operation", "gegl:translate", NULL);
  opacity   = gegl_node_new_child (gegl, "operation", "gegl:opacity", NULL);
  blur      = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", 
                                         "clip-extent", FALSE, 
                                         "abyss-policy", 0,
                                         NULL);
  darken    = gegl_node_new_child (gegl, "operation", "gegl:src-in", NULL);
  color     = gegl_node_new_child (gegl, "operation", "gegl:color",
                                   "value", black_color,
                                   NULL);

  g_object_unref (black_color);

  gegl_node_link_many (input, darken, blur, opacity, translate, over, output,
                       NULL);
  gegl_node_connect_from (over, "aux", input, "output");
  gegl_node_connect_from (darken, "aux", color, "output");

  gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "radius", blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "x", translate, "x");
  gegl_operation_meta_redirect (operation, "y", translate, "y");
  gegl_operation_meta_redirect (operation, "color", color, "value");
  gegl_operation_meta_redirect (operation, "opacity", opacity, "value");

  gegl_operation_meta_watch_nodes (operation,
                                   over, translate, opacity,
                                   blur, darken, color,
                                   NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:dropshadow",
    "categories",  "light",
    "title",       _("Dropshadow"),
    "description",
    _("Creates a dropshadow effect on the input buffer"),
    NULL);
}

#endif
