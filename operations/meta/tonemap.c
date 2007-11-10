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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#if GEGL_CHANT_PROPERTIES

gegl_chant_double(radius, 0.0, 100.0, 20.0, "radius")
gegl_chant_double(blur, 0.0, 100.0,   15.0,   "blur")
gegl_chant_double(amount, -1.0, 2.0, 0.5, "amount")

#else

#define GEGL_CHANT_META
#define GEGL_CHANT_NAME            tonemap
#define GEGL_CHANT_DESCRIPTION     "local contrast enhancement"
#define GEGL_CHANT_SELF            "tonemap.c"
#define GEGL_CHANT_CATEGORIES      "meta:enhance"
#define GEGL_CHANT_CLASS_INIT
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
  GeglChantOperation *self;
  Priv          *priv;

  self       = GEGL_CHANT_OPERATION (operation);
  priv       = g_malloc0 (sizeof (Priv));
  self->priv = (void*) priv;


  if (!priv->min)
    {
      GeglNode      *gegl;
      gegl = operation->node;

      priv->input    = gegl_node_get_input_proxy (gegl, "input");
      priv->output   = gegl_node_get_output_proxy (gegl, "output");

      priv->min = gegl_node_new_child (gegl,
                                       "operation", "box-min",
                                       NULL);

      priv->blur_min = gegl_node_new_child (gegl,
                                            "operation", "box-blur",
                                            NULL);

      priv->max = gegl_node_new_child (gegl,
                                       "operation", "box-max",
                                       NULL);

      priv->blur_max = gegl_node_new_child (gegl,
                                            "operation", "box-blur",
                                            NULL);

      priv->remap = gegl_node_new_child (gegl,
                                         "operation", "remap",
                                         NULL);

      priv->over = gegl_node_new_child (gegl,
                                         "operation", "over",
                                         NULL);

      priv->opacity = gegl_node_new_child (gegl,
                                         "operation", "opacity",
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

static void class_init (GeglOperationClass *klass)
{
  klass->attach = attach;
}

#endif
