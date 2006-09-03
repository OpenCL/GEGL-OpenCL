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
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double(radius, 0.0, 30.0, 1.4, "Radius")
gegl_chant_double(scale,  0.0, 100.0, 10.0, "Scale")

#else

#define GEGL_CHANT_GRAPH
#define GEGL_CHANT_NAME            unsharp_mask
#define GEGL_CHANT_DESCRIPTION     "Performs an unsharp mask"
#define GEGL_CHANT_SELF            "unsharp-mask.c"
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
};

static void prepare (GeglOperation *operation);
static void associate (GeglOperation *operation)
{
  GeglChantOperation *self;
  Priv          *priv;
  GeglGraph     *graph;

  self       = GEGL_CHANT_OPERATION (operation);
  priv       = g_malloc0 (sizeof (Priv));
  self->priv = (void*) priv;

  graph = GEGL_GRAPH (operation->node);

  /* aquire interior nodes representing the graphs pads */
  priv->input  = gegl_graph_input (graph, "input");
  priv->output = gegl_graph_output (graph, "output");

  if (!priv->add)
    {
      priv->add      = gegl_graph_create_node (graph,
                                               "operation", "add",
                                               NULL);

      priv->multiply = gegl_graph_create_node (graph,
                                               "operation", "multiply",
                                               NULL);

      priv->subtract = gegl_graph_create_node (graph,
                                               "operation", "subtract",
                                               NULL);

      priv->blur     = gegl_graph_create_node (graph,
                                               "operation", "gaussian-blur",
                                               NULL);

      gegl_node_connect (priv->subtract, "input", priv->input,    "output");
      gegl_node_connect (priv->blur,     "input", priv->input,    "output");

      gegl_node_connect (priv->subtract, "aux",   priv->blur,     "output");

      gegl_node_connect (priv->multiply, "input", priv->subtract, "output");

      gegl_node_connect (priv->add,      "input", priv->input,    "output");
      gegl_node_connect (priv->add,      "aux",   priv->multiply, "output");

      gegl_node_connect (priv->output,   "input", priv->add,      "output");
    }
}

static void prepare (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv          *priv = (Priv*)self->priv;

  gegl_node_set (priv->multiply, "value",    self->scale,
                                  NULL);

  gegl_node_set (priv->blur,     "radius-x", self->radius,
                                 "radius-y", self->radius,
                                 NULL);
}

static void class_init (GeglOperationClass *klass)
{
  klass->prepare = prepare;
  klass->associate = associate;
}

#endif
