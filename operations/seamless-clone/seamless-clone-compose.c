/* This file is an image processing operation for GEGL
 *
 * seamless-clone-compose.c
 * Copyright (C) 2012 Barak Itkin <lightningismyname@gmail.com>
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

#ifdef GEGL_PROPERTIES
property_int (max_refine_scale, _("Refinement steps"), 2000)
  description(_("Maximal amount of refinement points to be used for the interpolation mesh"))
  value_range (0, 100000)

property_int (xoff, _("Offset X"), 0)
  description (_("How much horizontal offset should applied to the paste"))
  value_range (0, 100000)
  ui_meta     ("axis", "x")
  ui_meta     ("unit", "pixel-coordinate")

property_int (yoff, _("Offset Y"), 0)
  description(_("How much vertical offset should applied to the paste"))
  value_range (0, 100000)
  ui_meta     ("axis", "y")
  ui_meta     ("unit", "pixel-coordinate")

property_string (error_msg, _("Error message"), "")
  description (_("An error message in case of a failure"))

#else

#define GEGL_OP_META
#define GEGL_OP_NAME seamless_clone_compose
#define GEGL_OP_C_FILE       "seamless-clone-compose.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  /**
   * <input (BG)>                <aux (FG)>
   *    |"output"             "output"|
   *    |                             |
   *    | "input"             "aux"   |
   *    +-----> <seamless-clone> <----+
   *    |           "output"|
   *    |                   |
   *    | "input"    "aux"  |
   *    +---> <overlay> <---+
   *            |"output"
   *            v
   *         <output>
   */
  GeglNode *gegl = operation->node;
  GeglNode *input, *aux, *seamless, *overlay, *output;

  input = gegl_node_get_input_proxy (gegl, "input");
  aux = gegl_node_get_input_proxy (gegl, "aux");
  seamless = gegl_node_new_child (gegl, "operation", "gegl:seamless-clone", NULL);
  /* Don't use a regular gegl:over, since we want the alpha to be set
   * by the background buffer - we need this for area which had opacity
   * which was more than half but not fulll. */
  overlay = gegl_node_new_child (gegl, "operation", "svg:src-atop", NULL);
  output = gegl_node_get_output_proxy (gegl, "output");
 
  gegl_node_connect_to (input, "output", seamless, "input");
  gegl_node_connect_to (aux, "output", seamless, "aux");
  gegl_node_connect_to (input, "output", overlay, "input");
  gegl_node_connect_to (seamless, "output", overlay, "aux");
  gegl_node_connect_to (overlay, "output", output, "input");

  gegl_operation_meta_redirect (operation, "max-refine-scale", seamless, "max-refine-scale");
  gegl_operation_meta_redirect (operation, "xoff", seamless, "xoff");
  gegl_operation_meta_redirect (operation, "yoff", seamless, "yoff");
  gegl_operation_meta_redirect (operation, "error-msg", seamless, "error-msg");

  gegl_operation_meta_watch_nodes (operation, seamless, overlay, NULL);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass  *operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:seamless-clone-compose",
    "categories",  "compositors:meta:blend",
    "description", "Seamless cloning composite operation",
    NULL);
}

#endif
