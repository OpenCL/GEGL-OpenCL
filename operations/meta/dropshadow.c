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
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (opacity, "Opacity", -10.0, 10.0, 0.5, "Opacity")
gegl_chant_double (x, "X", -G_MAXDOUBLE, G_MAXDOUBLE, 20.0,
                   "Horizontal shadow offset.")
gegl_chant_double (y, "Y", -G_MAXDOUBLE, G_MAXDOUBLE, 20.0,
                   "Vertical shadow offset.")
gegl_chant_double (radius, "Radius", -G_MAXDOUBLE, G_MAXDOUBLE, 10.0,
                   "Blur radius.")

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE       "dropshadow.c"

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

/* in attach we hook into graph adding the needed nodes */
static void attach (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv *priv = (Priv*)o->chant_data;
  g_assert (priv == NULL);

  priv = g_malloc0 (sizeof (Priv));
  o->chant_data = (void*) priv;

  priv->self = operation->node;

  if (!priv->over)
    {
      GeglNode *gegl  = priv->self;
      priv->input     = gegl_node_get_input_proxy (gegl, "input");
      priv->output    = gegl_node_get_output_proxy (gegl, "output");
      priv->over      = gegl_node_new_child (gegl, "operation", "over", NULL);
      priv->translate = gegl_node_new_child (gegl, "operation", "translate", NULL);
      priv->opacity   = gegl_node_new_child (gegl, "operation", "opacity", NULL);
      priv->blur      = gegl_node_new_child (gegl, "operation", "gaussian-blur", NULL);
      priv->darken    = gegl_node_new_child (gegl, "operation", "src-in", NULL);
      priv->black     = gegl_node_new_child (gegl, "operation", "color",
                                         "value", gegl_color_new ("rgb(0.0,0.0,0.0)"),
                                         NULL);

      gegl_node_link_many (priv->input, priv->darken, priv->blur, priv->opacity, priv->translate, priv->over, priv->output, NULL);
      gegl_node_connect_from (priv->over, "aux", priv->input, "output");
      gegl_node_connect_from (priv->darken, "aux", priv->black, "output");

      gegl_operation_meta_redirect (operation, "opacity", priv->opacity, "value");
      gegl_operation_meta_redirect (operation, "radius", priv->blur, "std-dev-x");
      gegl_operation_meta_redirect (operation, "radius", priv->blur, "std-dev-y");
      gegl_operation_meta_redirect (operation, "x", priv->translate, "x");
      gegl_operation_meta_redirect (operation, "y", priv->translate, "y");
    }
}


static void
operation_class_init (GeglChantClass *klass)
{
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->attach = attach;

  operation_class->name        = "dropshadow";
  operation_class->categories  = "meta:effects";
  operation_class->description =
        "Creates a dropshadow effect on the input buffer.";
}

#endif
