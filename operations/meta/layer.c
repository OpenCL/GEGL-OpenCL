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

gegl_chant_string(composite_op, "over", "Composite operation to use")
gegl_chant_double(opacity, 0.0, 1.0, 1.0, "Opacity")
gegl_chant_double(x, -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, "horizontal position")
gegl_chant_double(y, -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, "vertical position")
gegl_chant_string(src, "", "source datafile (png, jpg, raw, svg, bmp, tif, ..)")
gegl_chant_pointer(private, "private stuff do not touch")

#else

#define GEGL_CHANT_GRAPH
#define GEGL_CHANT_NAME            layer
#define GEGL_CHANT_DESCRIPTION     "A layer in the traditional sense"
#define GEGL_CHANT_SELF            "layer.c"
#define GEGL_CHANT_CLASS_INIT
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
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
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
      gegl_operation_prepare (priv->load->operation);
      g_warning ("(src should be used)");
    }
  else
    {
      g_warning ("(aux should be used)");
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
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
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
  gegl_operation_prepare (priv->load->operation);

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
