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

gegl_chant_string  (path, "/tmp/lena.png",
                    "Path of file to load.")
gegl_chant_boolean (cache, TRUE,
                    "Whether to cache the entire buffer loaded from the file.")

#else

#define GEGL_CHANT_GRAPH
#define GEGL_CHANT_NAME            load
#define GEGL_CHANT_DESCRIPTION     "Multipurpose file loader, that uses other "\
                                   "native handlers, and fallback conversion using "\
                                   "image magick's convert."
#define GEGL_CHANT_SELF            "load.c"
#define GEGL_CHANT_CATEGORIES      "sources:meta"
#define GEGL_CHANT_CLASS_INIT
#include "gegl-chant.h"
#include "gegl/gegl-extension-handler.h"
#include <stdio.h>

typedef struct _Priv Priv;
struct _Priv
{
  GeglNode   *self;
  GeglNode   *output;

  GeglNode   *load;

  gchar      *cached_path;
  GeglBuffer *cached_buffer;
};

static void refresh_cache (GeglChantOperation *self);

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
  if (priv->cached_path)
    {
      g_free (priv->cached_path);
      priv->cached_path = NULL;
    }

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->dispose (object);
}

static void
finalize (GObject *object)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (object);

  if (self->priv)
    g_free (self->priv);

  G_OBJECT_CLASS (g_type_class_peek_parent (G_OBJECT_GET_CLASS (object)))->finalize (object);
}

static void
prepare (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  Priv *priv = (Priv*)self->priv;

  /* warning: this might trigger regeneration of the graph,
   *          for now this is evaded by just ignoring additional
   *          requests to be made into members of the graph
   */

  if (self->cache)
    {
      refresh_cache (self);
      gegl_node_set (priv->load,
                     "operation", "buffer",
                     "buffer", priv->cached_buffer,
                     NULL);
    }
  else
    {
      if (self->path[0])
        {
          const gchar *extension = strrchr (self->path, '.');
          const gchar *handler = NULL;

          if (!g_file_test (self->path, G_FILE_TEST_EXISTS))
            {
              gchar *tmp = g_malloc(strlen (self->path) + 100);
              sprintf (tmp, "File '%s' does not exist", self->path);
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
                             "path",  self->path,
                             NULL);
            }
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
  
  G_OBJECT_CLASS (klass)->dispose = dispose;
  G_OBJECT_CLASS (klass)->finalize = finalize;
}


static void
refresh_cache (GeglChantOperation *self)
{
  Priv *priv = (Priv*)self->priv;

  if (!priv->cached_buffer ||
      ((priv->cached_path && self->path) &&
        strcmp (self->path, priv->cached_path)))
    {
        GeglGraph *gegl;
        GeglNode  *load;

        if (priv->cached_buffer)
          {
            g_object_unref (priv->cached_buffer);
            priv->cached_buffer = NULL;
            g_free (priv->cached_path);
          }

        gegl = g_object_new (GEGL_TYPE_GRAPH, NULL);
        load = gegl_graph_create_node (gegl, "operation", "load",
                                             "cache", FALSE,
                                             "path", self->path,
                                             NULL);
        gegl_node_apply (load, "output");
        gegl_node_get (load, "output", &(priv->cached_buffer), NULL);

        /* we unref the buffer since we effectifly need to steal the
         * contents XXX, only once here,. twice in node_blit..
         */
        g_object_unref (priv->cached_buffer);


        g_object_unref (gegl);
        priv->cached_path = g_strdup (self->path);
  }
}

#endif
