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

#else

#define GEGL_CHANT_META
#define GEGL_CHANT_NAME            layer
#define GEGL_CHANT_DESCRIPTION     "A layer in the traditional sense"
#define GEGL_CHANT_SELF            "layer.c"
#define GEGL_CHANT_CATEGORIES      "meta"
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

    gchar *cached_path;
    GeglBuffer *cached_buffer;

  gdouble p_opacity;
  gdouble p_x;
  gdouble p_y;
  gchar *p_composite_op;
};

static gboolean refresh_cache (GeglChantOperation *self);

static void
prepare (GeglOperation *operation,
         gpointer       dynamic_id)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv *priv;
  priv = (Priv*)self->priv;

  
  /* warning: this might trigger regeneration of the graph,
   *          for now this is evaded by just ignoring additional
   *          requests to be made into members of the graph
   */

  if (!priv->p_composite_op || strcmp (priv->p_composite_op, self->composite_op))
    {
      gegl_node_set (priv->composite_op,
                     "operation", self->composite_op,
                     NULL);
      if (priv->p_composite_op)
        g_free (priv->p_composite_op);
      priv->p_composite_op = g_strdup (self->composite_op);
    }
  if (self->src[0])
    {
      if (refresh_cache (self))
        {
          gegl_node_set (priv->load, 
                         "buffer", priv->cached_buffer,
                         NULL);
        }
    }
  else
    {
      gegl_node_connect_from (priv->opacity, "input", priv->aux, "output");
    }

  if (self->opacity != priv->p_opacity)
    {
      gegl_node_set (priv->opacity, 
                     "value",  self->opacity,
                     NULL);
      priv->p_opacity = self->opacity;
    }

  if (self->x != priv->p_x ||
      self->y != priv->p_y)
    {
      gegl_node_set (priv->shift, 
                     "x",  self->x,
                     "y",  self->y,
                     NULL);
      priv->p_x = self->x;
      priv->p_y = self->y;
    }
}

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
  priv->aux = gegl_graph_input (gegl, "aux");
  priv->output = gegl_graph_output (gegl, "output");

  priv->composite_op = gegl_graph_new_node (gegl,
                                         "operation", self->composite_op,
                                         NULL);

  priv->shift = gegl_graph_new_node (gegl, "operation", "shift", NULL);
  priv->opacity = gegl_graph_new_node (gegl, "operation", "opacity", NULL);
  
  priv->load = gegl_graph_new_node (gegl,
                                    "operation", "buffer",
                                    NULL);

  gegl_node_link_many (priv->load, priv->opacity, priv->shift, NULL);
  gegl_node_link_many (priv->input, priv->composite_op, priv->output, NULL);
  gegl_node_connect_from (priv->composite_op, "aux", priv->shift, "output");
}

static void
dispose (GObject *object)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (object);
  Priv *priv = (Priv*)self->priv;

  if (priv->cached_buffer)
    {
      g_object_unref (priv->cached_buffer);
      priv->cached_buffer = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->dispose (object);
}


static void
finalize (GObject *object)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (object);
  Priv *priv = (Priv*)self->priv;

  if (priv->cached_path)
      g_free (priv->cached_path);
  if (priv->p_composite_op)
      g_free (priv->p_composite_op);
  if (self->priv)
    g_free (self->priv);

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}

static void class_init (GeglOperationClass *klass)
{
  klass->prepare = prepare;
  klass->associate = associate;
    
  G_OBJECT_CLASS (klass)->dispose = dispose;
  G_OBJECT_CLASS (klass)->finalize = finalize;
}

static gboolean
refresh_cache (GeglChantOperation *self)
{
  Priv *priv = (Priv*)self->priv;

  if (!priv->cached_buffer ||
      ((priv->cached_path && self->src) &&
        strcmp (self->src, priv->cached_path)))
    {
      GeglNode *gegl;
      GeglNode *load;

      if (priv->cached_buffer)
        {
          g_object_unref (priv->cached_buffer);
          priv->cached_buffer = NULL;
          g_free (priv->cached_path);
        }

      gegl = g_object_new (GEGL_TYPE_NODE, NULL);
      load = gegl_graph_new_node (gegl, "operation", "load",
                                           "cache", FALSE,
                                           "path", self->src,
                                           NULL);
      gegl_node_apply (load, "output");
      gegl_node_get (load, "output", &(priv->cached_buffer), NULL);

      /* we unref the buffer since we effectifly need to steal the
       * contents XXX, only once here,. twice in node_blit.. since
       * we do not have any more use for it there.
       */
      g_object_unref (priv->cached_buffer);

      g_object_unref (gegl);
      priv->cached_path = g_strdup (self->src);
      return TRUE;
    }
  return FALSE;
}


#endif
