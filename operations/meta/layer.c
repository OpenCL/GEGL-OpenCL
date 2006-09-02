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
#ifdef CHANT_SELF

chant_string(composite_op, "over", "Composite operation to use")
chant_double(opacity, 0.0, 1.0, 1.0, "Opacity")
chant_double(x, -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, "horizontal position")
chant_double(y, -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, "vertical position")
chant_string(src, "", "source datafile (png, jpg, raw, svg, bmp, tif, ..)")
chant_pointer(private, "private stuff do not touch")

#else

#define CHANT_GRAPH
#define CHANT_NAME            layer
#define CHANT_DESCRIPTION     "A layer in the traditional sense"
#define CHANT_SELF            "layer.c"
#define CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"

typedef struct _Priv Priv;
struct _Priv
{
  GeglNode *self;
  GeglNode *input;
  GeglNode *aux;
  GeglNode *output;

  GeglNode *composite_op;
    GeglNode *shift;
    GeglNode *opacity;
    GeglNode *load;
};

static void
prepare (GeglOperation *operation)
{
  ChantInstance *self = CHANT_INSTANCE (operation);
  Priv *priv;
  priv = (Priv*)self->priv;

  /* warning: this might trigger regeneration of the graph,
   *          for now this is evaded by just ignoring additional
   *          requests to be made into members of the graph
   */
  gegl_node_set (priv->composite_op,
                 "operation", self->composite_op,
                 NULL);
  if (self->src[0])
    {
      gegl_node_set (priv->load, 
                     "path",  self->src,
                     NULL);
    }
  else
    {
      gegl_node_connect (priv->opacity, "input", priv->aux, "output");
    }

  gegl_node_set (priv->opacity, 
                 "value",  self->opacity,
                 NULL);
  gegl_node_set (priv->shift, 
                 "x",  self->x,
                 "y",  self->y,
                 NULL);
}

static void associate (GeglOperation *operation)
{
  ChantInstance *self = CHANT_INSTANCE (operation);
  Priv *priv = (Priv*)self->priv;
  GeglGraph *graph;
  g_assert (priv == NULL);

  priv = g_malloc0 (sizeof (Priv));
  self->priv = (void*) priv;

  priv->self = GEGL_OPERATION (self)->node;
  graph = GEGL_GRAPH (priv->self);

  priv->input = gegl_graph_input (graph, "input");
  priv->aux = gegl_graph_input (graph, "aux");
  priv->output = gegl_graph_output (graph, "output");

  priv->composite_op = gegl_graph_create_node (graph,
                                         "operation", self->composite_op,
                                         NULL);

  priv->shift = gegl_graph_create_node (graph, "operation", "shift", NULL);
  priv->opacity = gegl_graph_create_node (graph, "operation", "opacity", NULL);
  
  priv->load = gegl_graph_create_node (graph,
                                       "operation", "load",
                                       NULL);

  gegl_node_connect (priv->opacity, "input", priv->load, "output");
  gegl_node_connect (priv->shift, "input", priv->opacity, "output");
  gegl_node_connect (priv->composite_op, "aux", priv->shift, "output");
  gegl_node_connect (priv->composite_op, "input", priv->input, "output");
  gegl_node_connect (priv->output, "input", priv->composite_op, "output");
}

static void class_init (GeglOperationClass *klass)
{
  klass->prepare = prepare;
  klass->associate = associate;
}

#endif
