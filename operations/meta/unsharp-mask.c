/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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

gegl_chant_double(radius, 0.0, 30.0, 1.4, "Radius")
gegl_chant_double(scale,  0.0, 100.0, 10.0, "Scale")

#else

#define GEGL_CHANT_GRAPH
#define GEGL_CHANT_NAME            unsharp_mask
#define GEGL_CHANT_DESCRIPTION     "Performs an unsharp mask on the input buffer (sharpens an image by adding false mach-bands around edges)."
#define GEGL_CHANT_SELF            "unsharp-mask.c"
#define GEGL_CHANT_CATEGORIES      "meta:enhance"
#define GEGL_CHANT_CLASS_INIT
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

  double p_radius;
  double p_scale;
};

static void prepare (GeglOperation *operation);
static void associate (GeglOperation *operation)
{
  GeglChantOperation *self;
  Priv          *priv;
  GeglNode      *gegl;

  self       = GEGL_CHANT_OPERATION (operation);
  priv       = g_malloc0 (sizeof (Priv));
  self->priv = (void*) priv;

  gegl = operation->node;

  /* aquire interior nodes representing the graphs pads */
  priv->input  = gegl_graph_input (gegl, "input");
  priv->output = gegl_graph_output (gegl, "output");

  if (!priv->add)
    {
      priv->add      = gegl_graph_new_node (gegl,
                                            "operation", "add",
                                            NULL);

      priv->multiply = gegl_graph_new_node (gegl,
                                            "operation", "multiply",
                                            NULL);

      priv->subtract = gegl_graph_new_node (gegl,
                                            "operation", "subtract",
                                            NULL);

      priv->blur     = gegl_graph_new_node (gegl,
                                            "operation", "gaussian-blur",
                                            NULL);

      gegl_node_link_many (priv->input, priv->subtract, priv->multiply, NULL);
      gegl_node_link (priv->input, priv->blur);
      gegl_node_link_many (priv->input, priv->add, priv->output, NULL);


      gegl_node_connect (priv->subtract, "aux",   priv->blur,     "output");
      gegl_node_connect (priv->add,      "aux",   priv->multiply, "output");

    }
}

static void prepare (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv          *priv = (Priv*)self->priv;

  if (self->scale != priv->p_scale)
    {
      gegl_node_set (priv->multiply, "value",    self->scale,
                                      NULL);
      priv->p_scale = self->scale;
    }

  if (self->radius != priv->p_radius)
    {
      gegl_node_set (priv->blur,     "radius-x", self->radius,
                                     "radius-y", self->radius,
                                     NULL);
      priv->p_radius = self->radius;
    }
}

static void class_init (GeglOperationClass *klass)
{
  klass->prepare = prepare;
  klass->associate = associate;
}

#endif
