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


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double(x, _("X"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                  _("Horizontal position"))
gegl_chant_double(y, _("Y"), -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                  _("Vertical position"))
gegl_chant_double(width, _("Width"), 0, G_MAXDOUBLE, 0.0,
                  _("Horizontal extent"))
gegl_chant_double(height, _("Height"), 0, G_MAXDOUBLE, 0.0,
                  _("Vertical extent"))
gegl_chant_color(color, _("Color"), "white",
                  _("Color to render"))

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE "rectangle.c"
#include "gegl-chant.h"

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
gegl_chant_class_init (GeglChantClass *klass)
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
