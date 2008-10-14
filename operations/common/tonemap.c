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

gegl_chant_double(radius, _("Radius"),  0.0, 100.0, 20.0, _("radius"))
gegl_chant_double(blur,   _("Blur"),    0.0, 100.0, 15.0, _("blur"))
gegl_chant_double(amount, _("Amount"), -1.0,   2.0,  0.5, _("amount"))

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE       "tonemap.c"

#include "gegl-chant.h"

typedef struct _Priv Priv;
struct _Priv
{
  GeglNode *self;
  GeglNode *input;
  GeglNode *output;

  GeglNode *over;
  GeglNode *opacity;
  GeglNode *min;
  GeglNode *blur_min;
  GeglNode *max;
  GeglNode *blur_max;
  GeglNode *remap;
};

static void attach (GeglOperation *operation)
{
  GeglChantO *o;
  Priv       *priv;

  o    = GEGL_CHANT_PROPERTIES (operation);
  priv = g_malloc0 (sizeof (Priv));
  o->chant_data = (void*) priv;

  if (!priv->min)
    {
      GeglNode      *gegl;
      gegl = operation->node;

      priv->input    = gegl_node_get_input_proxy (gegl, "input");
      priv->output   = gegl_node_get_output_proxy (gegl, "output");

      priv->min = gegl_node_new_child (gegl,
                                       "operation", "gegl:box-min",
                                       NULL);

      priv->blur_min = gegl_node_new_child (gegl,
                                            "operation", "gegl:box-blur",
                                            NULL);

      priv->max = gegl_node_new_child (gegl,
                                       "operation", "gegl:box-max",
                                       NULL);

      priv->blur_max = gegl_node_new_child (gegl,
                                            "operation", "gegl:box-blur",
                                            NULL);

      priv->remap = gegl_node_new_child (gegl,
                                         "operation", "gegl:remap",
                                         NULL);

      priv->over = gegl_node_new_child (gegl,
                                         "operation", "gegl:over",
                                         NULL);

      priv->opacity = gegl_node_new_child (gegl,
                                         "operation", "gegl:opacity",
                                         NULL);

      gegl_node_link_many (priv->input, priv->min, priv->blur_min, NULL);
      gegl_node_link_many (priv->input, priv->max, priv->blur_max, NULL);

      gegl_node_connect_to (priv->input, "output", priv->remap, "input");
      gegl_node_connect_to (priv->blur_min, "output", priv->remap, "low");
      gegl_node_connect_to (priv->blur_max, "output", priv->remap, "high");

      gegl_node_connect_to (priv->input, "output", priv->over, "input");
      gegl_node_connect_to (priv->remap, "output", priv->opacity, "input");
      gegl_node_connect_to (priv->opacity, "output", priv->over, "aux");
      gegl_node_connect_to (priv->over, "output", priv->output, "input");




      gegl_operation_meta_redirect (operation, "blur", priv->blur_max, "radius");
      gegl_operation_meta_redirect (operation, "radius", priv->max, "radius");


      gegl_operation_meta_redirect (operation, "blur", priv->blur_min, "radius");
      gegl_operation_meta_redirect (operation, "radius", priv->min, "radius");
      gegl_operation_meta_redirect (operation, "amount", priv->opacity, "value");
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

  operation_class->name        = "gegl:tonemap";
  operation_class->categories  = "meta:enhance";
  operation_class->description = _("Local contrast enhancement");
}

#endif
