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
 *           2013 Daniel Sabo
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

gegl_property_double (x, "nick", _("X"),
    "blurb", _("Horizontal position"),
    "unit", "pixel-coordinate",
    "axis", "x",
    NULL)

gegl_property_double (y, "nick", _("Y"),
    "blurb", _("Vertical position"),
    "unit", "pixel-coordinate",
    "axis", "y",
    NULL)

gegl_property_double (width, "nick", _("Width"),
    "blurb", _("Horizontal extent"),
    "min", 0.0, "max", G_MAXDOUBLE,
    "unit", "pixel-distance",
    "axis", "x",
    NULL)

gegl_property_double (height, "nick", _("Height"),
    "blurb", _("Vertical extent"),
    "min", 0.0, "max", G_MAXDOUBLE,
    "unit", "pixel-distance",
    "axis", "y",
    NULL)

gegl_property_color(color, "nick", _("Color"),
    "blurb", _("Color to render"),
    "default", "white",
    NULL)

#else

#define GEGL_OP_META
#define GEGL_OP_C_FILE "rectangle.c"
#include "gegl-op.h"

static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglNode *output, *color, *crop;

  output = gegl_node_get_output_proxy (gegl, "output");

  color = gegl_node_new_child (gegl, "operation", "gegl:color", NULL);
  crop  = gegl_node_new_child (gegl, "operation", "gegl:crop", NULL);

  gegl_operation_meta_watch_node (operation, color);
  gegl_operation_meta_watch_node (operation, crop);

  gegl_node_link_many (color, crop, output, NULL);

  gegl_operation_meta_redirect (operation, "color", color, "value");
  gegl_operation_meta_redirect (operation, "x", crop, "x");
  gegl_operation_meta_redirect (operation, "y", crop, "y");
  gegl_operation_meta_redirect (operation, "width", crop, "width");
  gegl_operation_meta_redirect (operation, "height", crop, "height");
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
  "name"       , "gegl:rectangle",
  "categories" , "input",
  "description",
        _("A rectangular source of a fixed size with a solid color"),
        NULL);
}

#endif
