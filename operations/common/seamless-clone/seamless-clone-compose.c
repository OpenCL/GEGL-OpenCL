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

#ifdef GEGL_CHANT_PROPERTIES
gegl_chant_int (max_refine_steps, _("Refinement Steps"), 0, 100000.0, 2000,
                _("Maximal amount of refinement points to be used for the interpolation mesh"))

gegl_chant_int (xoff, _("X offset"), -100000, +100000, 0,
                _("How much horizontal offset should applied to the paste"))

gegl_chant_int (yoff, _("Y offset"), -100000, +100000, 0,
                _("How much vertical offset should applied to the paste"))

gegl_chant_string (error_msg, _("Error message"), NULL, _("An error message in case of a failure"))
#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE       "seamless-clone-compose.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

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
  
  gegl_node_connect_from (input, "output", seamless, "input");
  gegl_node_connect_from (aux, "output", seamless, "aux");
  gegl_node_connect_from (input, "output", overlay, "input");
  gegl_node_connect_from (seamless, "output", overlay, "aux");
  gegl_node_connect_from (overlay, "output", output, "input");

  gegl_operation_meta_redirect (operation, "max-refine-steps", seamless, "max-refine-steps");
  gegl_operation_meta_redirect (operation, "xoff", seamless, "xoff");
  gegl_operation_meta_redirect (operation, "yoff", seamless, "yoff");
  gegl_operation_meta_redirect (operation, "error-msg", seamless, "error-msg");
}

static void
gegl_chant_class_init (GeglChantClass *klass)
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
