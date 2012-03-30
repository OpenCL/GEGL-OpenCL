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
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double_ui (radius1, _("Radius 1"), 0.0, 1000.0, 1.0, 0.0, 10.0, 1.5,
                     _("Radius"))
gegl_chant_double_ui (radius2, _("Radius 2"), 0.0, 1000.0, 2.0, 0.0, 10.0, 1.5,
                     _("Radius"))

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE       "difference-of-gaussians.c"

#include "gegl-chant.h"

typedef struct _Priv Priv;
struct _Priv
{
  GeglNode *self;
  GeglNode *input;
  GeglNode *output;

  GeglNode *multiply;
  GeglNode *subtract;
  GeglNode *blur1;
  GeglNode *blur2;
};

static void attach (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *priv = g_new0 (Priv, 1);

  o->chant_data = (void*) priv;

  if (!priv->blur1)
    {
      GeglNode      *gegl;
      gegl = operation->node;

      priv->input    = gegl_node_get_input_proxy (gegl, "input");
      priv->output   = gegl_node_get_output_proxy (gegl, "output");

      priv->subtract = gegl_node_new_child (gegl,
                                            "operation", "gegl:subtract",
                                            NULL);

      priv->blur1    = gegl_node_new_child (gegl,
                                            "operation", "gegl:gaussian-blur",
                                            NULL);

      priv->blur2    = gegl_node_new_child (gegl,
                                            "operation", "gegl:gaussian-blur",
                                            NULL);

      gegl_node_link_many (priv->input, priv->blur1, priv->subtract, priv->output, NULL);
      gegl_node_link (priv->input, priv->blur2);

      gegl_node_connect_from (priv->subtract, "aux",   priv->blur2,     "output");

      gegl_operation_meta_redirect (operation, "radius1", priv->blur1, "std-dev-x");
      gegl_operation_meta_redirect (operation, "radius1", priv->blur1, "std-dev-y");
      gegl_operation_meta_redirect (operation, "radius2", priv->blur2, "std-dev-x");
      gegl_operation_meta_redirect (operation, "radius2", priv->blur2, "std-dev-y");
    }
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->attach = attach;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:difference-of-gaussians",
    "categories" , "meta:edge",
    "description",
        _("Does an edge detection based on the difference of two gaussian blurs."),
        NULL);
}

#endif
