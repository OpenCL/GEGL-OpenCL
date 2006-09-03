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

gegl_chant_double(opacity, 0.0, 1.0, 0.5, "Opacity")
gegl_chant_double(x, -G_MAXDOUBLE, G_MAXDOUBLE, 20.0, "horizontal position")
gegl_chant_double(y, -G_MAXDOUBLE, G_MAXDOUBLE, 20.0, "vertical position")
gegl_chant_double(radius, -G_MAXDOUBLE, G_MAXDOUBLE, 10.0, "blur radius")

#else

#define GEGL_CHANT_GRAPH
#define GEGL_CHANT_NAME            dropshadow
#define GEGL_CHANT_DESCRIPTION     "Creates a dropshadow"
#define GEGL_CHANT_SELF            "dropshadow.c"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"

typedef struct _Priv Priv;
struct _Priv
{
  GeglNode *self;
  GeglNode *input;
  GeglNode *output;

  GeglNode *over;
  GeglNode *translate;
  GeglNode *opacity;
  GeglNode *blur;
  GeglNode *darken;
};

static void
prepare (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv *priv;
  priv = (Priv*)self->priv;

  /* parameters might have changed, so set the properties again */

  gegl_node_set (priv->translate, "x", self->x,
                                  "y", self->y,
                                  NULL);
  gegl_node_set (priv->opacity,   "value", self->opacity,
                                  NULL);
  gegl_node_set (priv->blur,     "radius-x", self->radius,
                                  "radius-y", self->radius,
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
  priv->output = gegl_graph_output (graph, "output");

  if (!priv->over)
    {
      GeglGraph *graph = GEGL_GRAPH (priv->self);
      priv->over = gegl_graph_create_node (graph, "operation", "over", NULL);
      priv->translate = gegl_graph_create_node (graph, "operation", "translate", NULL);
      priv->opacity = gegl_graph_create_node (graph, "operation", "opacity", NULL);
      priv->blur = gegl_graph_create_node (graph, "operation", "gaussian-blur", NULL);
      priv->darken = gegl_graph_create_node (graph, "operation", "brightness-contrast", "brightness", -0.9, NULL);

      gegl_node_connect (priv->darken, "input", priv->input, "output");
      gegl_node_connect (priv->blur,   "input", priv->darken, "output");
      gegl_node_connect (priv->opacity, "input", priv->blur, "output");
      gegl_node_connect (priv->translate, "input", priv->opacity, "output");

      gegl_node_connect (priv->over, "input", priv->translate, "output");
      gegl_node_connect (priv->over, "aux", priv->input, "output");
      gegl_node_connect (priv->output, "input", priv->over, "output");
    }
}

static void class_init (GeglOperationClass *klass)
{
  klass->prepare = prepare;
  klass->associate = associate;
}

#endif
