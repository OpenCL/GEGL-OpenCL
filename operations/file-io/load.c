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

chant_string(path, "/tmp/lena.png", "Path of file to load.")

#else

#define CHANT_GRAPH
#define CHANT_NAME            load
#define CHANT_DESCRIPTION     "Multipurpose file loader, that uses other native handlers, and fallback conversion using image magick's convert."
#define CHANT_SELF            "load.c"
#define CHANT_CLASS_CONSTRUCT
#include "gegl-chant.h"
#include "gegl/gegl-extension-handler.h"

typedef struct _Priv Priv;
struct _Priv
{
  GeglNode *self;
  GeglNode *output;

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

  if (self->path[0])
    {
      const gchar *extension = strrchr (self->path, '.');
      const gchar *handler = NULL;
     
      if (extension)
        handler = gegl_extension_handler_get (extension);

      gegl_node_set (priv->load, 
                     "operation", handler,
                     "path",  self->path,
                     NULL);
    }
  else
    {
      gegl_node_set (priv->load,
                     "operation", "text",
                     "size", 20.0,
                     "string", "Eeeeek!",
                     NULL);
    }
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

  priv->output = gegl_graph_output (graph, "output");

  priv->load = gegl_graph_create_node (graph,
                                       "operation", "text",
                                       "string", "foo",
                                       NULL);

  gegl_node_connect (priv->output, "input", priv->load, "output");
}

static void class_init (GeglOperationClass *klass)
{
  klass->prepare = prepare;
  klass->associate = associate;
}

#endif
