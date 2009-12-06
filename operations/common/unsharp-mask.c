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

gegl_chant_double(std_dev, _("Std. Dev."), 0.0, 100.0, 1.0,
                  _("Standard deviation (spatial scale factor)"))
gegl_chant_double(scale,  _("Scale"), 0.0, 100.0, 1.0, _("Scale, strength of effect."))

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE       "unsharp-mask.c"

#include "gegl-chant.h"

typedef struct _Priv Priv;
struct _Priv
{
  GeglNode *self;
  GeglNode *input;
  GeglNode *output;

  GeglNode *add;
  GeglNode *multiply;
  GeglNode *subtract;
  GeglNode *blur;
};

static void attach (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *priv = g_malloc0 (sizeof (Priv));

  o->chant_data = (void*) priv;

  if (!priv->add)
    {
      GeglNode *gegl;
      gegl = operation->node;

      priv->input    = gegl_node_get_input_proxy (gegl, "input");
      priv->output   = gegl_node_get_output_proxy (gegl, "output");
      priv->add      = gegl_node_new_child (gegl,
                                            "operation", "gegl:add",
                                            NULL);

      priv->multiply = gegl_node_new_child (gegl,
                                            "operation", "gegl:multiply",
                                            NULL);

      priv->subtract = gegl_node_new_child (gegl,
                                            "operation", "gegl:subtract",
                                            NULL);

      priv->blur     = gegl_node_new_child (gegl,
                                            "operation", "gegl:gaussian-blur",
                                            NULL);

      gegl_node_link_many (priv->input, priv->subtract, priv->multiply, NULL);
      gegl_node_link (priv->input, priv->blur);
      gegl_node_link_many (priv->multiply, priv->add, priv->output, NULL);

      gegl_node_connect_from (priv->subtract, "aux",   priv->blur,     "output");
      gegl_node_connect_from (priv->add,      "aux",   priv->input, "output");

      gegl_operation_meta_redirect (operation, "scale", priv->multiply, "value");
      gegl_operation_meta_redirect (operation, "std-dev", priv->blur, "std-dev-x");
      gegl_operation_meta_redirect (operation, "std-dev", priv->blur, "std-dev-y");
    }
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->attach = attach;

  operation_class->name        = "gegl:unsharp-mask";
  operation_class->categories  = "meta:enhance";
  operation_class->description =
        _("Performs an unsharp mask on the input buffer (sharpens an image by "
          "adding false mach-bands around edges).");
}

#endif
