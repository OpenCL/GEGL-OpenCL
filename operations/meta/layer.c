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

/* FIXME: need to make this OpenRaster inspired layer integrate better
 * with the newer caching system
 */

#if GEGL_CHANT_PROPERTIES

gegl_chant_string(composite_op, "over", "Composite operation to use")
gegl_chant_double(opacity, 0.0, 1.0, 1.0, "Opacity")
gegl_chant_double(x, -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, "horizontal position")
gegl_chant_double(y, -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, "vertical position")
gegl_chant_path(src, "", "source datafile (png, jpg, raw, svg, bmp, tif, ..)")

#else

#define GEGL_CHANT_META
#define GEGL_CHANT_NAME            layer
#define GEGL_CHANT_DESCRIPTION     "A layer in the traditional sense"
#define GEGL_CHANT_SELF            "layer.c"
#define GEGL_CHANT_CATEGORIES      "meta"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-old-chant.h"
#include <glib/gprintf.h>

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

  gdouble p_opacity;
  gdouble p_x;
  gdouble p_y;
  gchar *p_composite_op;
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

  if (!priv->p_composite_op || strcmp (priv->p_composite_op, self->composite_op))
    {
      gegl_node_set (priv->composite_op,
                     "operation", self->composite_op,
                     NULL);
      if (priv->p_composite_op)
        g_free (priv->p_composite_op);
      priv->p_composite_op = g_strdup (self->composite_op);
    }

  if (self->src[0]==0 && priv->cached_path == NULL)
    {
      gegl_node_connect_from (priv->opacity, "input", priv->aux, "output");
    }
  else
    { /* FIXME:
       * this reimplements the "load" op, which shouldn't be neccesary, but
       * currently seems to be neccesary since GEGL doesn't like a meta-op
       * to be implemented using another meta-op.
       */
      if (self->src[0] &&
          (priv->cached_path == NULL || strcmp (self->src, priv->cached_path)))
        {
          const gchar *extension = strrchr (self->src, '.');
          const gchar *handler = NULL;

          if (!g_file_test (self->src, G_FILE_TEST_EXISTS))
            {
              gchar *tmp = g_malloc(strlen (self->src) + 100);
              g_sprintf (tmp, "File '%s' does not exist", self->src);
              g_warning ("load: %s", tmp);
              gegl_node_set (priv->load,
                             "operation", "text",
                             "size", 12.0,
                             "string", tmp,
                             NULL);
              g_free (tmp);
            }
          else
            {
              if (extension)
                handler = gegl_extension_handler_get (extension);
              gegl_node_set (priv->load, 
                             "operation", handler,
                             NULL);
              gegl_node_set (priv->load, 
                             "path",  self->src,
                             NULL);
            }
          if (priv->cached_path)
            g_free (priv->cached_path);
          priv->cached_path = g_strdup (self->src);
        }
      else
        {
        }
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

static void attach (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv *priv = (Priv*)self->priv;
  GeglNode *gegl;
  g_assert (priv == NULL);

  priv = g_malloc0 (sizeof (Priv));
  self->priv = (void*) priv;

  priv->self = GEGL_OPERATION (self)->node;
  gegl = priv->self;

  priv->input = gegl_node_get_input_proxy (gegl, "input");
  priv->aux = gegl_node_get_input_proxy (gegl, "aux");
  priv->output = gegl_node_get_output_proxy (gegl, "output");

  priv->composite_op = gegl_node_new_child (gegl,
                                         "operation", self->composite_op,
                                         NULL);

  priv->shift = gegl_node_new_child (gegl, "operation", "shift", NULL);
  priv->opacity = gegl_node_new_child (gegl, "operation", "opacity", NULL);

  priv->load = gegl_node_new_child (gegl,
                                    "operation", "text",
                                    "string", "foo",
                                    NULL);

  gegl_node_link_many (priv->load, priv->opacity, priv->shift, NULL);
  gegl_node_link_many (priv->input, priv->composite_op, priv->output, NULL);
  gegl_node_connect_from (priv->composite_op, "aux", priv->shift, "output");
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
  klass->attach = attach;

  G_OBJECT_CLASS (klass)->finalize = finalize;
}

#endif
