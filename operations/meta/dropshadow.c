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

gegl_chant_double (opacity, 0.0, 100.0, 0.5, "Opacity")
gegl_chant_double (x, -G_MAXDOUBLE, G_MAXDOUBLE, 20.0,
                   "Horizontal shadow offset.")
gegl_chant_double (y, -G_MAXDOUBLE, G_MAXDOUBLE, 20.0,
                   "Vertical shadow offset.")
gegl_chant_double (radius, -G_MAXDOUBLE, G_MAXDOUBLE, 10.0,
                   "Blur radius.")

#else

#define GEGL_CHANT_GRAPH
#define GEGL_CHANT_NAME            dropshadow
#define GEGL_CHANT_DESCRIPTION     "Creates a dropshadow effect on the input buffer."
#define GEGL_CHANT_SELF            "dropshadow.c"
#define GEGL_CHANT_CATEGORIES      "meta:effects"
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
  GeglNode *black;
};


/* prepare for an evaluation */
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

/* in associate we hook into graph adding the needed nodes */
static void associate (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv *priv = (Priv*)self->priv;
  GeglNode *gegl;
  g_assert (priv == NULL);

  priv = g_malloc0 (sizeof (Priv));
  self->priv = (void*) priv;

  priv->self = GEGL_OPERATION (self)->node;
  gegl = priv->self;
  priv->input = gegl_graph_input (gegl, "input");
  priv->output = gegl_graph_output (gegl, "output");

  if (!priv->over)
    {
      priv->over = gegl_graph_new_node (gegl, "operation", "over", NULL);
      priv->translate = gegl_graph_new_node (gegl, "operation", "translate", NULL);
      priv->opacity = gegl_graph_new_node (gegl, "operation", "opacity", NULL);
      priv->blur = gegl_graph_new_node (gegl, "operation", "gaussian-blur", NULL);
      priv->darken = gegl_graph_new_node (gegl, "operation", "in", NULL);
      priv->black = gegl_graph_new_node (gegl, "operation", "color",
                                         "value", gegl_color_from_string ("rgb(0.0,0.0,0.0)"),
                                         NULL);

      gegl_node_link_many (priv->input, priv->darken, priv->blur, priv->opacity, priv->translate, priv->over, priv->output, NULL);
      gegl_node_connect (priv->over, "aux", priv->input, "output");
      gegl_node_connect (priv->darken, "aux", priv->black, "output");
    }
}

static void class_init (GeglOperationClass *klass)
{
  klass->prepare = prepare;
  klass->associate = associate;
}

#endif
